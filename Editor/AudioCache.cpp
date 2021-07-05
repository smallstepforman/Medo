/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Audio manager (accurate seeking + cache frames)
 */

#include <cassert>
#include <deque>

#include <MediaKit.h>
#include <interface/Bitmap.h>
#include <kernel/OS.h>

#include "Project.h"
#include "MediaSource.h"
#include "AudioCache.h"
#include "VideoManager.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

static const size_t		kMaxReadAttempts	= 5;
static const int64		kMaxAudioBufferSize = 60*48000*4*2;		//	60 second block @ avg_sample_rate * avg_sample_size

/**********************************
	Audio Cache
***********************************/

AudioCache :: AudioCache()
{
	system_info si;
	get_system_info(&si);
	fAudioCacheMaxSize = (si.free_memory/10);	//	10%
	fCacheMaxBitmaps = (si.free_memory*0.04f) / (4*3840*64);	//	4% @ 3840x64 pixels, 32 bits
	fAudioCacheCurrentSize = 0;

	printf("[AudioCache]  Max buffers=%ld, Max Bitmaps = %ld\n", fAudioCacheMaxSize/kMaxAudioBufferSize, fCacheMaxBitmaps);
}

AudioCache :: ~AudioCache()
{
	DEBUG("~AudioCache() AudioItem::size=%ld, AudioBitmap::size=%ld\n", fAudioCache.size(), fBitmapCache.size());

	for (auto i : fAudioCache)
		delete [] i.buffer;

	for (auto i : fBitmapCache)
		delete i.bitmap;
}

/*	FUNCTION:		AudioCache::AUDIO_ITEM::PrintToStream
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Debug cache item
*/
void AudioCache :: AudioCache::AUDIO_ITEM::PrintToStream()
{
	DEBUG("[AudioCache] source(%s), audio_start(%ld), audio_end(%ld, buffer(%p), buffer_size(%ld)\n", source->GetFilename().String(), audio_start, audio_end, buffer, buffer_size);
}

/*	FUNCTION:		AudioCache :: ReadFile
	ARGS:			destination
					source
					start, end
					buffer_size (note - we've allocated + source->GetAudioBufferSize() to cater for extra read)
	RETURN:			status
	DESCRIPTION:	Read media file into audio buffer
*/
status_t AudioCache :: ReadFile(uint8 *destination, const MediaSource *source, int64 start, int64 end, const size_t buffer_size)
{
	assert(source != nullptr);
	BMediaTrack *audio_track = source->GetAudioTrack();
	assert(audio_track != nullptr);
	assert(start < end);
	assert(start >= 0);
	assert(end <= audio_track->CountFrames());

	int64 requested_start = start;
	if (start < 0)
		start = 0;
	const int64 count_frames = audio_track->CountFrames();
	int64 requested_end = end;
	if (requested_end > count_frames)
		requested_end = count_frames;
	start = requested_start;
	end = requested_end;

	const size_t kSampleSize = source->GetAudioSampleSize();

	//	Seek to frame
	size_t attempt = 0;
	status_t st;
	int64 num_read;

	if (!gVideoManager->LockMediaKit())
		return B_ERROR;

	do
	{
		st = audio_track->SeekToFrame(&start, B_MEDIA_SEEK_CLOSEST_BACKWARD);
	}
	while ((st != B_OK) && (++attempt <= kMaxReadAttempts));
	if (st != B_OK)
	{
		DEBUG("AudioCache::ReadFile() Cannot seek to frame %ld, file=%s\n", requested_start, source->GetFilename().String());
		gVideoManager->UnlockMediaKit();
		return st;
	}
	DEBUG("AudioCache::ReadFile() Seek request(%ld), start(%ld). end(%ld)  status_t(%d)\n", requested_start, start, end, st);

	uint8 *p = destination;

	//	Seek to actual frame (after keyframe)
	while (start < requested_start)
	{
		attempt = 0;
		do
		{
			st = audio_track->ReadFrames(source->GetAudioBuffer(), &num_read);
		} while ((st != B_OK) && (++attempt <= kMaxReadAttempts) && (audio_track->CurrentFrame() < audio_track->CountFrames()));
		if (st != B_OK)
		{
			DEBUG("AudioCache::ReadFile() ReadFrames()#1 returned %d, start(%ld), end(%ld), num_read(%ld)\n", st, start, end, num_read);
			break;
		}
		
		start += num_read;
		//	Cater for read of requested data
		if (start >= requested_start)
		{
			size_t amount = start - requested_start;
			memcpy(p, source->GetAudioBuffer() + (num_read - amount)*kSampleSize, amount*kSampleSize);
			p += amount*kSampleSize;
			DEBUG("AudioCache::ReadFile() ReadFrames()#2 start(%ld), amount(%ld), size(%ld), num_read(%ld)\n", start, amount, kSampleSize, num_read);
		}
	}

	//	Read frames into buffer
	while ((start <= end) && (p <= destination + buffer_size))
	{
		attempt = 0;
		do
		{
			st = audio_track->ReadFrames(p, &num_read);
		} while ((st != B_OK) && (++attempt <= kMaxReadAttempts) && (audio_track->CurrentFrame() < audio_track->CountFrames()));
		if (st != B_OK)
		{
			DEBUG("AudioCache::ReadFile() ReadFrames()#3 returned %d, start(%ld), end(%ld), num_read(%ld)\n", st, start, end, num_read);
			break;
		}
		start += num_read;
		p += num_read * kSampleSize;
	}
	gVideoManager->UnlockMediaKit();

	DEBUG("AudioCache::ReadFile() complete.  req_start(%ld), start(%ld), end(%ld), num_read(%ld), total_read(%ld)\n", requested_start, start, end, num_read, p-destination);
	const size_t remaining = p - destination;
	if (remaining < buffer_size)
		memset(p, source->GetAudioDataFormat() == media_raw_audio_format::B_AUDIO_UCHAR ? 0x80 : 0x00, buffer_size - remaining);
	return st;
}

/*	FUNCTION:		AudioCache :: GetAudioBufferUnlocked
	ARGS:			manager_semaphore
					source
					audio_start, audio_end
					audio_buffer_size
	RETURN:			audio buffer, size
	DESCRIPTION:	Retrive audio buffer from cache (or load)
*/
uint8 * AudioCache :: GetAudioBufferUnlocked(sem_id manager_semaphore, const MediaSource *source, const int64 audio_start, int64 &audio_end, size_t &audio_buffer_size)
{
	status_t err;
	while ((err = acquire_sem(manager_semaphore)) == B_INTERRUPTED) ;
	if (err != B_OK)
		return nullptr;

	unsigned char *audio_buffer = GetAudioBufferLocked(source, audio_start, audio_end, audio_buffer_size);
	release_sem_etc(manager_semaphore, 1, B_DO_NOT_RESCHEDULE);
	return audio_buffer;
}

/*	FUNCTION:		AudioCache :: GetAudioBuffer
	ARGS:			source
					start_frame, end_frame
					audio_buffer_size
	RETURN:			audio buffer, size
	DESCRIPTION:	Retrive audio buffer from cache (or load)
*/
uint8 * AudioCache :: GetAudioBufferLocked(const MediaSource *source, const int64 audio_start, int64 &audio_end, size_t &audio_buffer_size)
{
	assert(source);
	assert(audio_start >=0);
	assert(audio_end >= 0);
	assert(audio_start < audio_end);

	const size_t kSampleSize = source->GetAudioSampleSize();

	DEBUG("AudioCache::GetAudioBufferLocked() source=%s, start=%ld, end=%ld\n", source->GetFilename().String(), audio_start, audio_end);
	for (std::deque<AUDIO_ITEM>::iterator i = fAudioCache.begin(); i != fAudioCache.end(); i++)
	{
		if ((*i).source == source)
		{
			if (((*i).audio_start <= audio_start) && ((*i).audio_end >= audio_end))
			{
				//	Match, determine start pointer
				audio_buffer_size = (audio_end - audio_start)*kSampleSize;
				//	Push to front (take care not to thrash deque)
				if (i - fAudioCache.begin() > 16)
				{
					//	Push item to top of cache
					AUDIO_ITEM item = *i;
					fAudioCache.erase(i);
					fAudioCache.push_front(item);
					i = fAudioCache.begin();
				}
				return (*i).buffer + (audio_start - (*i).audio_start)*kSampleSize;
			}
		}
	}

	//	audio buffer not in cache, need to load it

	AUDIO_ITEM item = CreateAudioItem(source, audio_start, source->GetAudioNumberSamples());
	audio_end = item.audio_end;
	audio_buffer_size = (audio_end - audio_start)*kSampleSize;

	if (fAudioCacheCurrentSize >= fAudioCacheMaxSize)
	{
		AUDIO_ITEM back = fAudioCache.back();
		delete back.buffer;
		fAudioCacheCurrentSize -= back.buffer_size;
		fAudioCache.pop_back();
	}

	fAudioCache.push_front(item);
	fAudioCacheCurrentSize += item.buffer_size;
	return item.buffer;
}

/*	FUNCTION:		AudioCache :: CreateAudioItem
	ARGS:			source
					audio_start, audio_end
	RETURN:			AUDIO_CACHE item
	DESCRIPTION:	Alloc/Read up to kMaxAudioBufferSize buffer
*/
const AudioCache::AUDIO_ITEM AudioCache :: CreateAudioItem(const MediaSource *source, const int64 audio_start, const int64 audio_end)
{
	const size_t kSampleSize = source->GetAudioSampleSize();

	AUDIO_ITEM item;
	item.source = source;
	item.audio_start = audio_start;
	item.audio_end = audio_end;
	item.buffer_size = (audio_end - audio_start)*kSampleSize;
	//	Restrict item to kMaxAudioBufferSize
	if (item.buffer_size > kMaxAudioBufferSize)
	{
		int64 num_samples = kMaxAudioBufferSize / kSampleSize;	//	eliminate remainder
		item.buffer_size = num_samples*kSampleSize;
		item.audio_end = audio_start + num_samples;
	}
	item.buffer = new uint8 [item.buffer_size + source->GetAudioBufferSize()];	//	overcommit since ReadFrames() returns media_format.u.raw_audio.buffer_size
	DEBUG("CreateAudioItem(%p, size=%ld, alloc=%ld) high=%p\n", item.buffer, item.buffer_size, item.buffer_size + source->GetAudioBufferSize(), item.buffer + item.buffer_size + source->GetAudioBufferSize());
	DEBUG("AudioCache::CreateAudioItem(start=%ld, req_end=%ld, actual_end=%ld)\n", item.audio_start, audio_end, item.audio_end);
	DEBUG("mem = %p, buffer_size=%ld, total_size=%ld\n", item.buffer, item.buffer_size, item.buffer_size + source->GetAudioBufferSize());
	if (item.buffer)
	{
		ReadFile(item.buffer, source, item.audio_start, item.audio_end, item.buffer_size + source->GetAudioBufferSize());
	}
	else
		printf("AudioCache::CreateAudioItem() out of memory (requested=%ld, count_samples=%ld)\n", item.buffer_size, item.audio_end - item.audio_start);

	return item;
}

/*********************************************
	Audio bitmap
**********************************************/


/*	FUNCTION:		AudioCache :: CreateBitmap
	ARGS:			manager_semaphore
					source
					audio_start, audio_end
					samples_pixel
					width, height
	RETURN:			BBitmap
	DESCRIPTION:	Create bitmap representing audio track
*/
BBitmap * AudioCache :: CreateBitmap(sem_id manager_semaphore, const MediaSource *source, const int64 audio_start, const int64 audio_end, const int64 samples_pixel, const int32 width, const int32 height)
{
	const int kNumberChannels = source->GetAudioNumberChannels();
	const size_t kSampleSize = source->GetAudioSampleSize();

	//	preprocess audio buffer
	float min = 1000, max = -1000;
	struct RANGE
	{
		float	max;
		float	min;
	};
	std::vector<RANGE> samples;
	samples.resize(width * kNumberChannels);

	int64 current_start = audio_start;
	int64 current_end = audio_end;
	size_t audio_buffer_size;
	unsigned char *audio_buffer = GetAudioBufferUnlocked(manager_semaphore, source, audio_start, current_end, audio_buffer_size);
	if (!audio_buffer)
		return nullptr;

	RANGE *r = samples.data();
	int64 col = 0;
	unsigned char *a = audio_buffer;

	while (col < width)
	{
		if (a + samples_pixel*kSampleSize > audio_buffer + audio_buffer_size)
		{
			current_start = current_end;
			current_end = audio_end;
			if (current_start >= current_end)
				break;

			audio_buffer = GetAudioBufferUnlocked(manager_semaphore, source, current_start, current_end, audio_buffer_size);
			if (!audio_buffer)
				return nullptr;
			a = audio_buffer;
			if (a + samples_pixel*kSampleSize > audio_buffer + audio_buffer_size)
			{
				printf("AudioCache::CreateBitmap() - warning, buffer overflow (%ld*%ld > %ld)\n", samples_pixel, kSampleSize, audio_buffer_size);
				break;
			}
		}
		for (int channel = 0; channel < kNumberChannels; channel++)
		{
			const float *fp = (float *)a;
			fp += channel;
			//	find range_min/range_max in [samples_pixel] interval
			float range_max = -1.0f;
			float range_min = 1.0f;
			const int kStep = (audio_end - audio_start > 60*kFramesSecond) ? 16 :			//	evert 16th sample used
								(audio_end - audio_start > 10*kFramesSecond) ? 8 : 1;		//	every 8th sample used
			for (int s=0; s < samples_pixel; s+=kStep)
			{
				float f = *fp;
				if (f > range_max)
					range_max = f;
				if (f < range_min)
					range_min = f;
				fp += kNumberChannels;
			}
			r->max = range_max;
			r->min = range_min;
			if (range_min < min)	min = range_min;
			if (range_max > max)	max = range_max;
			r++;
		}
		a += samples_pixel*kSampleSize;
		col++;
	}
	//printf("col=%ld, width=%d, current_end=%ld, actual_end=%ld\n", col, width, current_end, audio_end);

	float scale = 2.0f/(max - min);

	//	Create bitmap
	BBitmap *image = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32);
	image->Lock();
	const int kDrawChannels = (height/kNumberChannels > 24 ? kNumberChannels : 1);
	for (int channel = 0; channel < kDrawChannels; channel++)
	{
		int r0 = height * (float(channel)/float(kDrawChannels));
		int r1 = height * (float(channel+1)/float(kDrawChannels));

		//	Draw sample
		for (int32 row=r0; row < r1; row++)
		{
			r = &samples[channel];
			float vpos = 1.0f - 2.0*(row-r0)/((r1-r0) - 1.0f);
			uint8 *d = (uint8 *)image->Bits() + row*width*4;
			for (int32 col=0; col < width; col++)
			{
				if ((vpos <= r->max*scale) && (vpos >= r->min*scale))
					*((uint32 *)d) = 0xff000000;	//	ARGB
				else
					*((uint32 *)d) = 0xffffc000;	//	ARGB

				d += 4;
				r += kNumberChannels;
			}
		}
	}
	image->Unlock();
	DEBUG("AudioCache::CreateBitmap() min=%f, max=%f\n", min, max);
	return image;
}

/*	FUNCTION:		AudioCache :: FindBitmapLocked
	ARGS:			source
					audio_start, audio_end
					width, height
	RETURN:			BBitmap or nullptr
	DESCRIPTION:	Retrive bitmap from cache (if it exists)
*/
BBitmap	* AudioCache :: FindBitmapLocked(const MediaSource *source, const int64 audio_start, const int64 audio_end, const int32 width, const int32 height)
{
	for (std::deque<BITMAP_ITEM>::iterator i = fBitmapCache.begin(); i != fBitmapCache.end(); i++)
	{
		if (((*i).source == source) && ((*i).audio_start == audio_start) && ((*i).audio_end == audio_end) && ((*i).width == width) && ((*i).height == height))
		{
			BITMAP_ITEM anItem = *i;
			fBitmapCache.erase(i);
			fBitmapCache.push_front(anItem);
			return anItem.bitmap;
		}
	}
	return nullptr;
}

/*	FUNCTION:		AudioCache :: FindSimilarBitmapLocked
	ARGS:			source
					audio_start, audio_end
					width, height
	RETURN:			BBitmap or nullptr
	DESCRIPTION:	Retrive "similar" bitmap from cache
					This is used to temporarily display a bitmap during a flood of FrameResized() messages
*/
BBitmap	* AudioCache :: FindSimilarBitmapLocked(const MediaSource *source, const int64 audio_start, const int64 audio_end, const int32 width, const int32 height)
{
	std::deque<BITMAP_ITEM>::iterator mi = fBitmapCache.end();
	int32 dist2 = 4*3840*3840 + 4*2160*2160;	//	max distance (8K resolution)

	for (std::deque<BITMAP_ITEM>::iterator i = fBitmapCache.begin(); i != fBitmapCache.end(); i++)
	{
		if (((*i).source == source) && ((*i).audio_start == audio_start))
		{
			float d2 = ((*i).width - width)*((*i).width - width) +
					((*i).height - height)*((*i).height - height);
			if (d2 < dist2)
			{
				dist2 = d2;
				mi = i;
			}
		}
	}

	if (mi != fBitmapCache.end())
	{
		BITMAP_ITEM anItem = *mi;
		fBitmapCache.erase(mi);
		fBitmapCache.push_front(anItem);
		return anItem.bitmap;
	}
	else
		return nullptr;
}

/*	FUNCTION:		AudioCache :: CreateBitmapUnlocked
	ARGS:			manager_semaphore
					source
					audio_start, audio_end
					width, height
	RETURN:			BBitmap
	DESCRIPTION:	Retrive bitmap from cache, otherwise create new bitmap
					Note - since AUDIO_CACHE is private to AudioCache, we need to pass "manager_semaphore" argument
					since it allows generating large bitmaps without blocking
*/
BBitmap * AudioCache :: CreateBitmapUnlocked(sem_id manager_semaphore, const MediaSource *source, const int64 audio_start, const int64 audio_end, const int32 width, const int32 height)
{
	//	Create bitmap (unlocked)
	BITMAP_ITEM item;
	item.source = source;
	item.audio_start = audio_start;
	item.audio_end = audio_end;
	item.width = width;
	item.height = height;
	item.samples_pixel = (audio_end - audio_start)/width;
	item.bitmap = CreateBitmap(manager_semaphore, source, audio_start, audio_end, item.samples_pixel, width, height);

	status_t err;
	while ((err = acquire_sem(manager_semaphore)) == B_INTERRUPTED) ;
	if (err != B_OK)
	{
		delete [] item.bitmap;
		return nullptr;
	}

	//	Add item to cache
	if (fBitmapCache.size() < fCacheMaxBitmaps)
	{
		fBitmapCache.push_front(item);
	}
	else
	{
		delete fBitmapCache.back().bitmap;
		fBitmapCache.pop_back();
		fBitmapCache.push_front(item);
	}
	release_sem_etc(manager_semaphore, 1, B_DO_NOT_RESCHEDULE);
	
	return item.bitmap;
}

