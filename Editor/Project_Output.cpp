/*	PROJECT:		Medo
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2018
	DESCRIPTION:	Project Output Frame
*/

#include <algorithm>
#include <deque>

#include <interface/Bitmap.h>

#include "Yarra/Platform.h"

#include "Project.h"
#include "MediaSource.h"
#include "VideoManager.h"
#include "MedoOpenGlView.h"
#include "EffectsManager.h"
#include "EffectNode.h"
#include "Effects/Effect_None.h" 
#include "MedoWindow.h"

#if 1
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

/*	FUNCTION:		Project :: GetOutputFrame
	ARGS:			frame_idx
	RETURN:			Output frame
	DESCRIPTION:	Create output frame
*/
BBitmap * Project :: GetOutputFrame(int64 frame_idx, MedoOpenGlView *gl_view)
{
	std::deque<FRAME_ITEM>		frame_items;

	//	Reverse iterate each track, find clips and effects
	for (std::vector<TimelineTrack *>::const_reverse_iterator t = mTimelineTracks.rbegin(); t < mTimelineTracks.rend(); ++t)
	{
		//	Find clips (TODO binary search)
		for (auto &clip : (*t)->mClips)
		{
			if ((frame_idx >= clip.mTimelineFrameStart) && (frame_idx < clip.GetTimelineEndFrame()) &&
				((clip.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO) || (clip.mMediaSourceType == MediaSource::MEDIA_PICTURE)))
			{
				frame_items.emplace_back(FRAME_ITEM(*t, &clip, nullptr));
				break;
			}
			else if (clip.mTimelineFrameStart > frame_idx)
				break;
		}

		//	Find effects (TODO binary search, TODO early exit)
		for (auto e : (*t)->mEffects)
		{
			if (((frame_idx >= e->mTimelineFrameStart) && (frame_idx < e->mTimelineFrameEnd)) &&
					(e->Type() == MediaEffect::MEDIA_EFFECT_IMAGE))
				frame_items.emplace_back(FRAME_ITEM(*t, nullptr, e));
		}
	}

	if (frame_items.empty())
		return fBackgroundBitmap;

	BBitmap *bitmap = fBackgroundBitmap;

	//	Early exit if single (or final) full screen frame
	const FRAME_ITEM &last = frame_items[frame_items.size() - 1];
	if (last.clip)
	{
		const MediaClip &clip = *last.clip;
		bool last_fullscreen = (clip.mMediaSource->GetVideoWidth() >= mResolution.width) && (clip.mMediaSource->GetVideoHeight() >= mResolution.height);
		if (last_fullscreen)
		{
			int64 requested_frame = (frame_idx - clip.mTimelineFrameStart) + clip.mSourceFrameStart;
			if ((clip.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO))
				bitmap = gVideoManager->GetFrameBitmap(clip.mMediaSource, requested_frame);
			else
				bitmap = clip.mMediaSource->GetBitmap();
			return bitmap;
		}
	}

	DEBUG("*** Output ***\n");
	for (auto i : frame_items)
	{
		DEBUG("   %s\n", i.clip ? i.clip->mMediaSource->GetFilename() : i.effect->mEffectNode->GetEffectName());
	}

	BBitmap *frame_bitmap = bitmap;
	bool initial_item = true;
	double ts = yplatform::GetElapsedTime();
	gl_view->LockGL();
	while (!frame_items.empty())
	{
		const FRAME_ITEM &item = frame_items.front();
		frame_items.pop_front();

		DEBUG("%s\n", item.clip ? item.clip->mMediaSource->GetFilename() : item.effect->mEffectNode->GetEffectName());

		if (item.clip)
		{
			const MediaClip &clip = *item.clip;
			int64 requested_frame = (frame_idx - clip.mTimelineFrameStart) + clip.mSourceFrameStart;
			if ((clip.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO))
				frame_bitmap = gVideoManager->GetFrameBitmap(clip.mMediaSource, requested_frame);
			else
				frame_bitmap = clip.mMediaSource->GetBitmap();

			gl_view->ActivateFrameBuffer(MedoOpenGlView::PRIMARY_FRAME_BUFFER, initial_item, false);
			gEffectsManager->GetEffectNone()->RenderEffect(frame_bitmap, nullptr, frame_idx, frame_items);
			gl_view->DeactivateFrameBuffer(MedoOpenGlView::PRIMARY_FRAME_BUFFER);
			bitmap = gl_view->GetFrameBufferBitmap(MedoOpenGlView::PRIMARY_FRAME_BUFFER);
		}
		else
		{
			//	effect
			if (item.effect->Type() == MediaEffect::MEDIA_EFFECT_IMAGE)
			{
				gl_view->ActivateFrameBuffer(MedoOpenGlView::PRIMARY_FRAME_BUFFER, initial_item, false);
				item.effect->mEffectNode->RenderEffect(bitmap, item.effect, frame_idx, frame_items);
				gl_view->DeactivateFrameBuffer(MedoOpenGlView::PRIMARY_FRAME_BUFFER);
				bitmap = gl_view->GetFrameBufferBitmap(MedoOpenGlView::PRIMARY_FRAME_BUFFER);
			}
		}

		initial_item = false;
	}
	gl_view->UnlockGL();
	DEBUG("RenderTime[3] = %fms\n", 1000.0 * (yplatform::GetElapsedTime() - ts));
	return bitmap;
}
