/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Video manager (accurate seeking + cache frames)
 */
 
/*	TODO:
		BitmapCache needs to work with different size source media
*/

#include <cassert>
#include <cstdio>
#include <vector>

#include <MediaKit.h>
#include <interface/Bitmap.h>
#include <kernel/OS.h>
#include "Actor/Actor.h"
#include "Actor/Platform.h"

#include "MediaSource.h"
#include "MedoWindow.h"
#include "Project.h"
#include "ImageUtility.h"
#include "VideoManager.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

static const int		kMaxReadAttempts	= 5;
static const size_t		kThumbnailWidth		= 16*6;
static const size_t		kThumbnailHeight	= 9*6;

/**********************************
	VideoBitmapCache
***********************************/
class VideoBitmapCache
{
private:
	struct FRAME
	{
		BBitmap			*bitmap;
		MediaSource		*source;
		int64			video_frame;
		bigtime_t		timestamp;

		FRAME() : bitmap(nullptr), source(nullptr), video_frame(0), timestamp(0) {}
	};

	std::vector<FRAME>	fFrames;
	size_t				fMaxFrames;
	
public:
/*	FUNCTION:		VideoBitmapCache :: VideoBitmapCache
	ARGS:			max_frames
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
	VideoBitmapCache(const size_t max_frames)
	{
		fMaxFrames = max_frames;
		fFrames.reserve(fMaxFrames);		
	}

/*	FUNCTION:		VideoBitmapCache :: ~VideoBitmapCache
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
	~VideoBitmapCache()
	{
		for (auto i : fFrames)
			delete i.bitmap;	
	}
	
/*	FUNCTION:		BitmapCache :: GetFrame
					source
	ARGS:			video_frame
					bitmap_width
					bitmap_height
					found
	RETURN:			iterator to next element in cache to overwrite
	DESCRIPTION:	Get iterator to next element in cache to overwrite (or new item)
*/	
	std::vector<FRAME>::iterator GetFrameLocked(const MediaSource *source, const int64 video_frame, const float bitmap_width, const float bitmap_height, bool &found)
	{
		assert(source != nullptr);
		BMediaTrack *track = source->GetVideoTrack();
		assert(track != nullptr);
		assert(video_frame <= source->GetVideoNumberFrames());

		//	Check if frame already in slot (reuse)
		auto it = fFrames.begin();
		for (std::vector<FRAME>::iterator i = fFrames.begin(); i != fFrames.end(); i++)
		{
			if (((*i).source == source) && ((*i).video_frame == video_frame))
			{
				DEBUG("VideoBitmapCache::GetFrameLocked(%ld) - Reuse slot (%ld)\n", video_frame, i-fFrames.begin());
				(*i).timestamp = system_time();
				found = true;
				return i;
			}
			//	Oldest timestamp?
			if ((*i).timestamp < (*it).timestamp)
				it = i;
		}
		found = false;
	
		//	Reuse or create new slot		
		if (fFrames.size() < fMaxFrames)
		{
			FRAME	aFrame;
			aFrame.bitmap = new BBitmap(BRect(0, 0, bitmap_width - 1, bitmap_height - 1), B_RGBA32);
			fFrames.push_back(aFrame);
			it = fFrames.end();
			it--;
			DEBUG("VideoBitmapCache::GetFrameLocked(%ld) - New entry\n", video_frame);
		}
		else
		{
			DEBUG("VideoBitmapCache::GetFrameLocked(%ld) - Replace entry\n", video_frame);
		}
		return it;	
	}
};

/**********************************
	VideoThumbnailActor
***********************************/

class VideoThumbnailActor : public yarra::Actor
{
	BMessage		*fMessage;
public:
	VideoThumbnailActor()
	{
		fMessage = new BMessage(MedoWindow::eMsgActionAsyncThumbnailReady);
	}
	~VideoThumbnailActor()
	{
		delete fMessage;
	}
	void AsyncGenerateThumbnail(MediaSource *source, const int64 frame_idx, const bool notification)
	{
		BBitmap *bitmap = gVideoManager->CreateThumbnailBitmap(source, frame_idx);
		if (notification)
			MedoWindow::GetInstance()->PostMessage(fMessage);
	}
	void ClearPendingThumbnails()
	{
		ClearAllMessages();
	}
};

/**********************************
	VideoManager
***********************************/

/*	FUNCTION:		VideoManager :: VideoManager
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Constructor
					fQueueSemaphore is used to synchronise access to fFrameCache/fThumbnailCache
					fTaskSemaphore is used to signal fThumbnailThread that work is available
					fDecodeSemaphore is used to synchronise access to ffmpeg decoder (thumbnails thread and preview thread)
*/
VideoManager :: VideoManager()
{
	system_info si;
	get_system_info(&si);

	const size_t kFrameCacheSize = (si.free_memory*0.66f) / (4*3840*2160);								//	66% system memory (UHD RGBA image)
	const size_t kThumbnailCacheSize = (si.free_memory*0.05f) / (4*kThumbnailWidth*kThumbnailHeight);	//	5% system memory, 20Kb/Thumbnail

	printf("[VideoManager] Max Cached Frames = [4K] %ld images / [HD] %ld images\n", kFrameCacheSize, kFrameCacheSize*4);
	printf("[VideoManager] Max Thumbnails = %ld thumbs\n", kThumbnailCacheSize);
	
	fFrameCache = new VideoBitmapCache(kFrameCacheSize);
	fThumbnailCache = new VideoBitmapCache(kThumbnailCacheSize);

	fQueueSemaphore = new yarra::Platform::Semaphore;
	fThumbnailActor = new VideoThumbnailActor;
	fMediaKitSemaphore = new yarra::Platform::Semaphore;
}

/*	FUNCTION:		VideoManager :: ~VideoManager
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
VideoManager :: ~VideoManager()
{
	delete fThumbnailActor;
	delete fFrameCache;
	delete fThumbnailCache;
	delete fQueueSemaphore;
	delete fMediaKitSemaphore;
}

/*	FUNCTION:		VideoManager :: GetFrameBitmap
	ARGS:			source
					frame_idx
					secondary_media
	RETURN:			bitmap
	DESCRIPTION:	Seek to bitmap (or read from cache)
*/	
BBitmap * VideoManager :: GetFrameBitmap(MediaSource *source, const int64 frame_idx, const bool secondary_media)
{
	assert(source != nullptr);
	BMediaTrack *video_track = secondary_media ? source->GetSecondaryVideoTrack() : source->GetVideoTrack();
	assert(video_track != nullptr);

	if ((frame_idx < 0) || (frame_idx >= source->GetVideoDuration()))
		return nullptr;

	//	Convert bigtime_t index to video_frame index
	int64 video_frame = frame_idx / float(kFramesSecond / source->GetVideoFrameRate());
	if (video_frame >= source->GetVideoNumberFrames())
		video_frame = source->GetVideoNumberFrames() - 1;

	int64 requested_video_frame = video_frame;
	const int64 count_frames = video_track->CountFrames();
	if (requested_video_frame >= count_frames)
		requested_video_frame = count_frames - 1;
	if (requested_video_frame < 0)
		requested_video_frame = 0;
	video_frame = requested_video_frame;
	DEBUG("********************\nRequested_video_frame = %ld\nCurrent = %ld\n", requested_video_frame, video_track->CurrentFrame());

	if (!fQueueSemaphore->Lock())
		return nullptr;

	//	Check if frame in cache
	bool found;
	auto it = fFrameCache->GetFrameLocked(source, video_frame, source->GetVideoWidth(), source->GetVideoHeight(), found);
	fQueueSemaphore->Unlock();
	if (found)
		return (*it).bitmap;
		
	//	No match, read frames
	media_header mh;
	int64 num_read;
	int attempt = 0;
	status_t st = B_ERROR;

	if (!LockMediaKit())
		return nullptr;

	if (video_track->CurrentFrame() != video_frame)
	{
		do
		{
			video_frame = requested_video_frame;
			st = video_track->SeekToFrame(&video_frame, B_MEDIA_SEEK_CLOSEST_BACKWARD);
		} while ((st != B_OK) && (++attempt <= kMaxReadAttempts));
		if (st != B_OK)
		{
			printf("Cannot seek to frame %ld, file=%s\n", video_frame, source->GetFilename());
			UnlockMediaKit();
			return nullptr;
		}
	}
	DEBUG("Seek request(%ld), actual (%ld).  status_t=%d\n", requested_video_frame, video_frame, st);

	//	Seek will advance to keyframe, we need to advance to actual frame after keyframe
	while (video_frame < requested_video_frame)
	{
		(*it).bitmap->Lock();
		attempt = 0;
		do 
		{
			st = video_track->ReadFrames((char *)(*it).bitmap->Bits(), &num_read, &mh);
		} while ((st != B_OK) && (++attempt <= kMaxReadAttempts));
		if (st != B_OK)
		{
			printf("Cannot read frame %ld, file=%s\n", video_frame, source->GetFilename());
			(*it).bitmap->Unlock();
			UnlockMediaKit();
			return nullptr;	
		}
		(*it).bitmap->Unlock();
		(*it).video_frame = (st==B_OK ? video_frame : -1);
		(*it).timestamp = system_time();
		(*it).source = source;
		DEBUG("Skip Save(%ld)  status_t=%d\n", video_frame, st);
		
		video_frame++;
		if (!fQueueSemaphore->Lock())	//	safe to request fQueueSemaphore while holding fDecodeSemaphore
		{
			UnlockMediaKit();
			return nullptr;
		}
		it = fFrameCache->GetFrameLocked(source, video_frame, source->GetVideoWidth(), source->GetVideoHeight(), found);
		fQueueSemaphore->Unlock();
	}

	(*it).bitmap->Lock();
	attempt = 0;
	do
	{
		st = video_track->ReadFrames((char *)(*it).bitmap->Bits(), &num_read, &mh);
	} while ((st != B_OK) && (++attempt <= kMaxReadAttempts));
	(*it).bitmap->Unlock();
	(*it).video_frame = (st==B_OK ? video_frame : -1);
	(*it).timestamp = system_time();
	(*it).source = source;
	UnlockMediaKit();
	DEBUG("Final Save(%ld).  status_t=%ld\n", video_frame, st);

	if (st == B_OK)
		return (*it).bitmap;
	else
		return nullptr;
}

/*	FUNCTION:		VideoManager :: CreateThumbnailBitmap
	ARGS:			source
					frame_idx
	RETURN:			bitmap
	DESCRIPTION:	Get thumbnail
*/	
BBitmap * VideoManager :: CreateThumbnailBitmap(MediaSource *source, const int64 frame_idx)
{
	assert(source != nullptr);
	assert(source->GetSecondaryVideoTrack() != nullptr);

	DEBUG("VideoManager::GetThumbnailBitmap(%ld)\n", frame_idx);
	
	//	Convert bigtime_t index to video_frame index
	assert(frame_idx <= source->GetVideoDuration());
	int64 video_frame = frame_idx / float(kFramesSecond / source->GetVideoFrameRate());
	if (video_frame >= source->GetVideoNumberFrames())
		video_frame = source->GetVideoNumberFrames() - 1;

	BBitmap *out = nullptr;

	if (!fQueueSemaphore->Lock())
		return nullptr;

	//	Check if frame in cache
	bool found;
	auto it = fThumbnailCache->GetFrameLocked(source, video_frame, kThumbnailWidth, kThumbnailHeight, found);
	fQueueSemaphore->Unlock();

	if (found)
	{
		DEBUG("Found cached thumbnail\n");
		out = (*it).bitmap;
	}
	else
	{
		//	Create new thumbnail
		BBitmap *frame = nullptr;	
		int attempt = 0;
		while ((frame == nullptr) && (++attempt <= 3))
			frame = GetFrameBitmap(source, frame_idx, true);
			
		if (frame)
		{
			DEBUG("Found frame - generating thumbnail\n");
			out = CreateThumbnail(frame, kThumbnailWidth, kThumbnailHeight, (*it).bitmap);
			(*it).source = source;
			(*it).video_frame = video_frame;
			(*it).timestamp = system_time();
		}
		else
		{
			printf("VideoManager::GetThumbnailBitmap(%s, %ld) - cannot generate bitmap\n", source->GetFilename(), video_frame);
		}
	}
	return out;
}

/*	FUNCTION:		VideoManager :: GetThumbnailAsync
	ARGS:			source
					frame_idx
					callback
	RETURN:			bitmap / nullptr
	DESCRIPTION:	Get thumbnail from cache, otherwise add work queue job
*/	
BBitmap * VideoManager :: GetThumbnailAsync(MediaSource *source, const int64 frame_idx, const bool notification)
{
	assert(source != nullptr);
	assert(source->GetVideoTrack() != nullptr);

	if (!fQueueSemaphore->Lock())
		return nullptr;
	
	//	Convert bigtime_t index to video_frame index
	assert(frame_idx <= source->GetVideoDuration());
	int64 video_frame = frame_idx / float(kFramesSecond / source->GetVideoFrameRate());
	if (video_frame >= source->GetVideoNumberFrames())
		video_frame = source->GetVideoNumberFrames() - 1;

	BBitmap *out = nullptr;
	
	//	Check if frame in cache, otherwise schedule work
	bool found;
	auto it = fThumbnailCache->GetFrameLocked(source, video_frame, kThumbnailWidth, kThumbnailHeight, found);
	fQueueSemaphore->Unlock();

	if (found)
		out = (*it).bitmap;
	else
		fThumbnailActor->Async(&VideoThumbnailActor::AsyncGenerateThumbnail, fThumbnailActor, source, frame_idx, notification);

	return out;
}

/*	FUNCTION:		VideoManager :: ClearPendingThumbnails
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Clear pending thumbnail generation
*/
void VideoManager :: ClearPendingThumbnails()
{
	fThumbnailActor->ClearPendingThumbnails();
}

//	FFmpeg (via BMediaKit) is not thread safe
const bool VideoManager :: LockMediaKit()		{return fMediaKitSemaphore->Lock();}
const bool VideoManager :: UnlockMediaKit()		{return fMediaKitSemaphore->Unlock();}
