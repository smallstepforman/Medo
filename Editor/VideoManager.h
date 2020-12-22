/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Video manager (accurate seeking + cache frames)
 */

#ifndef _VIDEO_MANAGER_H_
#define _VIDEO_MANAGER_H_

namespace yarra
{
	namespace Platform
	{
		class Semaphore;
	};
};

class BMediaTrack;
class BBitmap;
class MediaSource;
class VideoBitmapCache;
class VideoThumbnailActor;

//===================
class VideoManager
{
private:
	VideoBitmapCache			*fFrameCache;
	VideoBitmapCache			*fThumbnailCache;
	yarra::Platform::Semaphore	*fQueueSemaphore;
	VideoThumbnailActor			*fThumbnailActor;
	yarra::Platform::Semaphore	*fMediaKitSemaphore;
	friend class				VideoThumbnailActor;

	BBitmap						*CreateThumbnailBitmap(MediaSource *source, const int64 frame_idx);

public:
				VideoManager();
				~VideoManager();
			
	BBitmap		*GetFrameBitmap(MediaSource *source, const int64 frame_idx, const bool secondary_media = false);
	BBitmap		*GetThumbnailAsync(MediaSource *source, const int64 frame_idx, const bool notification);
	void		ClearPendingThumbnails();

	//	FFMpeg (via BMediaKit) doesn't like multithreaded access
	const bool	LockMediaKit();
	const bool	UnlockMediaKit();
};

extern VideoManager *gVideoManager;

#endif	//#ifndef _VIDEO_MANAGER_H_
