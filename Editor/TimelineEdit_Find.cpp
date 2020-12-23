/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Timeline edit - find algorithms
 */
 
#include <cstdio>
#include <cassert>

#include "Project.h"
#include "TimelineEdit.h"
#include "EffectNode.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

/*	FUNCTION:		TimelineEdit :: FindTimelineTrack
	ARGS:			point
	RETURN:			track at point (or nullptr)
	DESCRIPTION:	Find track at point
*/
TimelineTrack * TimelineEdit :: FindTimelineTrack(BPoint point)
{
	float y_pos = kTimelineTrackInitialY;
	for (auto &track : gProject->mTimelineTracks)
	{
		if ((point.y >= y_pos + track->mNumberEffectLayers*kTimelineEffectHeight) &&
			(point.y <= y_pos + track->mNumberEffectLayers*kTimelineEffectHeight + kTimelineTrackHeight))
		{
			return track;
		}
		y_pos += kTimelineTrackDeltaY + (kTimelineTrackHeight + track->mNumberEffectLayers*kTimelineEffectHeight);
	}
	return nullptr;
}

/*	FUNCTION:		TimelineEdit :: FindClip
	ARGS:			point
					aClip
					grace_x		- allow x pixels grace when searching (left && right)
	RETURN:			true if clip found
	DESCRIPTION:	Find clip at point
*/
bool TimelineEdit :: FindClip(const BPoint &point, ACTIVE_CLIP &aClip, const float grace_x)
{
	int64 frame_idx = fLeftFrameIndex + point.x*fFramesPixel;
	
	float y_pos = kTimelineTrackInitialY;
	for (auto &track : gProject->mTimelineTracks)
	{
		if ((point.y >= y_pos + track->mNumberEffectLayers*kTimelineEffectHeight) && 
			(point.y <= y_pos + track->mNumberEffectLayers*kTimelineEffectHeight + kTimelineTrackHeight))
		{
			int32 clip_idx = 0;
			for (const auto &clip : track->mClips)
			{
				if ((frame_idx/fFramesPixel + grace_x >= clip.mTimelineFrameStart/fFramesPixel) &&
					(frame_idx/fFramesPixel - grace_x <= clip.GetTimelineEndFrame()/fFramesPixel))
				{
					aClip.track = track;
					aClip.clip_idx = clip_idx;
					aClip.frame_idx = frame_idx - clip.mTimelineFrameStart;
					if (clip.mMediaSource->GetMediaType() != MediaSource::MEDIA_AUDIO)
						aClip.frame_idx = CalculateStickyFrameIndex(aClip.frame_idx, true);
					
					//	catch grace boundary
					if (aClip.frame_idx < 0)
						aClip.frame_idx = 0;
#if 0						
					if ((clip.mMediaSource->GetMediaType() == MediaSource::MEDIA_VIDEO) || (clip.mMediaSource->GetMediaType() == MediaSource::MEDIA_VIDEO_AND_AUDIO))
					{
						if (aClip.frame_idx >= clip.mMediaSource->GetVideoDuration())
							aClip.frame_idx = clip.mMediaSource->GetVideoDuration() - 1;
					}
					else if (clip.mMediaSource->GetMediaType() == MediaSource::MEDIA_AUDIO)
					{
						if (aClip.frame_idx >= clip.mMediaSource->GetAudioDuration())
							aClip.frame_idx = clip.mMediaSource->GetAudioDuration() - 1;	
					}
#endif
					fMouseDownPoint.y = y_pos + track->mNumberEffectLayers*kTimelineEffectHeight + 0.5f*kTimelineTrackHeight - fScrollViewOffsetY;
					return true;	
				}
				clip_idx++; 
			}
		}
		y_pos += kTimelineTrackDeltaY + (kTimelineTrackHeight + track->mNumberEffectLayers*kTimelineEffectHeight);	
	}
	return false;	
}

/*	FUNCTION:		TimelineEdit :: FindEffect
	ARGS:			point
					anEffect
					effect_track
					grace_x
	RETURN:			true if effect found
	DESCRIPTION:	Find effect at point
*/
bool TimelineEdit :: FindEffect(const BPoint &point, ACTIVE_EFFECT &anEffect, const float grace_x)
{
	int64 frame_idx = fLeftFrameIndex + point.x*fFramesPixel;
	
	float y_pos = kTimelineTrackInitialY;
	for (auto &track : gProject->mTimelineTracks)
	{
		//	Any effect layer?
		if ((point.y >= y_pos) && (point.y <= y_pos + track->mNumberEffectLayers*kTimelineEffectHeight))
		{
			for (int32 layer = 0; layer < track->mNumberEffectLayers; layer++)
			{
				//	Specific effect layer
				if ((point.y >= y_pos + (track->mNumberEffectLayers - layer - 1)*kTimelineEffectHeight) && 
					(point.y <= y_pos + (track->mNumberEffectLayers - layer)*kTimelineEffectHeight))
				{
					//	Find effect
					int32 effect_idx = 0;
					for (auto effect : track->mEffects)
					{
						if ((effect->mPriority == layer) && 
							(frame_idx/fFramesPixel + grace_x >= effect->mTimelineFrameStart/fFramesPixel) &&
							(frame_idx/fFramesPixel - grace_x <= effect->mTimelineFrameEnd/fFramesPixel))
						{
							anEffect.track = track;
							anEffect.effect_idx = effect_idx;
							anEffect.frame_idx = frame_idx - effect->mTimelineFrameStart;
							anEffect.media_effect = effect;
							if (effect->Type() == MediaEffect::MEDIA_EFFECT_IMAGE)
								anEffect.frame_idx = CalculateStickyFrameIndex(anEffect.frame_idx, true);
							if (anEffect.frame_idx < 0)
								anEffect.frame_idx = 0;

							anEffect.clip_idx = -1;
							for (size_t ci=0; ci < track->mClips.size(); ci++)
							{
								MediaClip &aClip = track->mClips[ci];
								if ((effect->mTimelineFrameEnd >= aClip.mTimelineFrameStart) && (effect->mTimelineFrameStart < aClip.GetTimelineEndFrame()))
								{
									anEffect.clip_idx = (int32)ci;
									break;
								}
							}

							//printf("Effect found: mPriority=%d, effect.index=%d, Frame=%d\n", effect->mPriority, anEffect.effect_idx, anEffect.frame_idx);
							return true;
						}
						effect_idx++; 
					}
				}
			}
		}
		y_pos += kTimelineTrackDeltaY + (kTimelineTrackHeight + track->mNumberEffectLayers*kTimelineEffectHeight);	
	}
	return false;		
}

/*	FUNCTION:		TimelineEdit :: FindClipLinkedEffects
	ARGS:			via fActiveClip
	RETURN:			n/a
	DESCRIPTION:	Populate fClipLinkedEffects
*/
void TimelineEdit :: FindClipLinkedEffects()
{
	assert(fActiveClip.track != nullptr);
	assert((fActiveClip.clip_idx >= 0) && (fActiveClip.clip_idx < fActiveClip.track->mClips.size()));

	fClipLinkedEffects.clear();
	const MediaClip &clip = fActiveClip.track->mClips[fActiveClip.clip_idx];
	bigtime_t end_frame = clip.GetTimelineEndFrame();
	for (auto i : fActiveClip.track->mEffects)
	{
		if ((i->mTimelineFrameEnd > clip.mTimelineFrameStart) && (i->mTimelineFrameStart < end_frame))
		{
			fClipLinkedEffects.push_back(LINKED_EFFECT(i, i->mTimelineFrameStart - clip.mTimelineFrameStart));
		}
	}
}

/*	FUNCTION:		TimelineEdit :: GetTrackOffsets
	ARGS:			offsets
	RETURN:			n/a
	DESCRIPTION:	Calculate track Y offsets
*/
void TimelineEdit :: GetTrackOffsets(std::vector<float> &offsets)
{
	offsets.clear();
	float y_pos = kTimelineTrackInitialY;
	for (auto track : gProject->mTimelineTracks)
	{
		y_pos += track->mNumberEffectLayers*kTimelineEffectHeight;
		offsets.push_back(y_pos);
		y_pos += kTimelineTrackDeltaY + kTimelineTrackHeight;
	}
}

