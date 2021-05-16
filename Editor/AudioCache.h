/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Audio manager (accurate seeking + cache frames)
 */

#ifndef _AUDIO_CACHE_H_
#define _AUDIO_CACHE_H_

#ifndef _GLIBCXX_DEQUE
#include <deque>
#endif

class BBitmap;
class MediaSource;

/**********************************
	Audio Cache
***********************************/
class AudioCache
{
	struct BITMAP_ITEM
	{
		BBitmap				*bitmap;
		int32				width;
		int32				height;
		int32				samples_pixel;
		const MediaSource	*source;
		int64				audio_start;
		int64				audio_end;
	};
	struct AUDIO_ITEM
	{
		uint8				*buffer;
		size_t				buffer_size;
		const MediaSource	*source;
		int64				audio_start;	//	in source frames (eg. 44100)
		int64				audio_end;		//	in source frames
		void				PrintToStream();
	};
	std::deque<AUDIO_ITEM>		fAudioCache;
	std::deque<BITMAP_ITEM>		fBitmapCache;

	std::size_t					fAudioCacheMaxSize;
	std::size_t					fAudioCacheCurrentSize;
	std::size_t					fCacheMaxBitmaps;

	status_t			ReadFile(uint8 *destination, const MediaSource *source, int64 start, int64 end, const size_t buffer_size);
	const AUDIO_ITEM	 CreateAudioItem(const MediaSource *source, const int64 audio_start, const int64 audio_end);

	uint8				*GetAudioBufferUnlocked(sem_id manager_semaphore, const MediaSource *source, const int64 audio_start, int64 &audio_end, size_t &size);
	BBitmap				*CreateBitmap(sem_id manager_semaphore, const MediaSource *source, const int64 audio_start, const int64 audio_end, const int64 samples_pixel, const int32 width, const int32 height);

public:
				AudioCache();
				~AudioCache();
				
	BBitmap		*FindBitmapLocked(const MediaSource *source, const int64 audio_start, const int64 audio_end, const int32 width, const int32 height);
	BBitmap		*FindSimilarBitmapLocked(const MediaSource *source, const int64 audio_start, const int64 audio_end, const int32 width, const int32 height);
	BBitmap		*CreateBitmapUnlocked(sem_id manager_semaphore, const MediaSource *source, const int64 audio_start, const int64 audio_end, const int32 width, const int32 height);
	uint8		*GetAudioBufferLocked(const MediaSource *source, const int64 audio_start, int64 &audio_end, size_t &size);
};

#endif	//#ifndef _AUDIO_CACHE_H_
