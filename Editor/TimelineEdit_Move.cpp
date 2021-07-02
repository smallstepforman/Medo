/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Timeline edit
 */
 
#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>

#include "Project.h"
#include "TimelineEdit.h"
#include "TimelineView.h"
#include "EffectNode.h"
#include "EffectsManager.h"
#include "MedoWindow.h"
#include "RenderActor.h"

#include "Effects/Effect_Transform.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif


/*	FUNCTION:		TimelineEdit :: MoveClipLinkedEffects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Move fClipLinkedEffects
*/
void TimelineEdit :: MoveClipLinkedEffects()
{
	MediaClip &clip = fActiveClip.track->mClips[fActiveClip.clip_idx];
	for (auto i : fClipLinkedEffects)
	{
		bigtime_t duration = i.effect->Duration();
		i.effect->mTimelineFrameStart = clip.mTimelineFrameStart + i.frame_offset;
		i.effect->mTimelineFrameEnd = i.effect->mTimelineFrameStart + duration;
	}
}

/*	FUNCTION:		TimelineEdit :: MoveClipLinkedEffects
	ARGS:			track
					clip_index
					delta
	RETURN:			n/a
	DESCRIPTION:	Move linked clip effects
*/
void TimelineEdit :: MoveClipLinkedEffects(TimelineTrack *track, int clip_index, int64 delta)
{
	assert(track != nullptr);
	assert(track->mClips.size() > 0);
	assert((clip_index >= 0) && (clip_index < track->mClips.size()));

	const MediaClip &clip = track->mClips[clip_index];
	bigtime_t end_frame = clip.GetTimelineEndFrame();
	for (auto i : track->mEffects)
	{
		if ((i->mTimelineFrameEnd > clip.mTimelineFrameStart) && (i->mTimelineFrameStart < end_frame))
		{
			i->mTimelineFrameStart += delta;
			i->mTimelineFrameEnd += delta;
		}
	}
}
	
/*	FUNCTION:		TimelineEdit :: DragDropClip
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Clip drag 'n' dropped
*/
void TimelineEdit :: DragDropClip(BMessage *msg)
{
	MediaClip c;
	status_t found_start = msg->FindInt64("start", &c.mSourceFrameStart);
	status_t found_end = msg->FindInt64("end", &c.mSourceFrameEnd);
	status_t found_source = msg->FindPointer("source", (void **)&c.mMediaSource);
	if ((found_start == B_OK) && (found_end == B_OK) && (found_source == B_OK))
	{
		bool skip_snapshot;
		if (msg->FindBool("skip_snapshot", &skip_snapshot) != B_OK)
			gProject->Snapshot();

		c.mMediaSourceType = c.mMediaSource->GetMediaType();

		DEBUG("TimelineEdit::DragDropClip() start_frame=%lld, end_frame=%lld\n", c.mSourceFrameStart, c.mSourceFrameEnd);
		//gProject->DebugClips(0);
				
		BPoint drop_point;
		msg->FindPoint("_drop_point_", &drop_point);
		BPoint cdp = ConvertFromScreen(drop_point);
				
		int64 xoffset = 0;
		msg->FindInt64("xoffset", &xoffset);
				
		DEBUG("cdp(%f, %f), xoffset=%lld\n", cdp.x, cdp.y, xoffset);
		c.mTimelineFrameStart = cdp.x*fFramesPixel - xoffset;
		DEBUG("timeline = %lld, fFramesPixel = %lld\n", c.mTimelineFrameStart, fFramesPixel);
		if (c.mTimelineFrameStart < 0)
			c.mTimelineFrameStart = 0;

		if (c.mMediaSourceType != MediaSource::MEDIA_AUDIO)
			c.mTimelineFrameStart = CalculateStickyFrameIndex(c.mTimelineFrameStart, true);
			
		//	find track_idx
		float y_pos = kTimelineTrackInitialY;
		int32 track_idx = 0;
		for (auto track : gProject->mTimelineTracks)
		{
			if ((cdp.y >= y_pos + track->mNumberEffectLayers*kTimelineEffectHeight) && 
				(cdp.y <= y_pos + track->mNumberEffectLayers*kTimelineEffectHeight + kTimelineTrackHeight + kTimelineTrackDeltaY))
			{
				break;
			}
			y_pos += kTimelineTrackDeltaY + (kTimelineTrackHeight + track->mNumberEffectLayers*kTimelineEffectHeight);
			track_idx++;
		}
		//	New track?
		if (track_idx >= gProject->mTimelineTracks.size())
		{
			if (gProject->mTimelineTracks.size() < kMaxNumberTimelineTracks)
			{
				gProject->AddTimelineTrack(new TimelineTrack);
			}
			track_idx = gProject->mTimelineTracks.size() - 1;	
		}
		
		int64 intended_timeline_start = c.mTimelineFrameStart;	//	used to animate drag/drop

		//	Append clip to track, prepare drag/drop animation
		TimelineTrack *track = gProject->mTimelineTracks[track_idx];
		fActiveClip.track = track;

		TimelineTrack *source_track = nullptr;
		status_t found_track = msg->FindPointer("track", (void **)&source_track);
		bool clip_transform_effect = false;

		if (track->mClips.empty())
		{
			fActiveClip.clip_idx = track->AddClip(c);
			if ((found_track == B_OK) && (fClipLinkedEffects.size() > 0))
			{
				for (auto e : fClipLinkedEffects)
				{
					source_track->RemoveEffect(e.effect, false);
					bigtime_t duration = e.effect->Duration();
					e.effect->mTimelineFrameStart = c.mTimelineFrameStart + e.frame_offset;
					e.effect->mTimelineFrameEnd = e.effect->mTimelineFrameStart + duration;
					track->AddEffect(e.effect);

					if (e.effect->mEffectNode->IsSpatialTransform())
						clip_transform_effect = true;
				}
				fClipLinkedEffects.clear();
			}
		}
		else
		{
			//	Prepare animate drag drop
			fAnimateDragDropTrack = track;
			fAnimateDragDropTimestamp = system_time();
			fAnimateDragDropClips.clear();
			fAnimateDragDropClips.reserve(track->mClips.size());
			for (auto i : track->mClips)
				fAnimateDragDropClips.push_back(i);
		
			//	Add clip to project
			fActiveClip.clip_idx = track->AddClip(c);
			if ((found_track == B_OK) && (fClipLinkedEffects.size() > 0))
			{
				for (auto e : fClipLinkedEffects)
				{
					source_track->RemoveEffect(e.effect, false);
					bigtime_t duration = e.effect->Duration();
					e.effect->mTimelineFrameStart = c.mTimelineFrameStart + e.frame_offset;
					e.effect->mTimelineFrameEnd = e.effect->mTimelineFrameStart + duration;
					track->AddEffect(e.effect);

					if (e.effect->mEffectNode->IsSpatialTransform())
						clip_transform_effect = true;
				}
				fClipLinkedEffects.clear();
			}
		
			//	Find where to insert new clip in animation sequence
			size_t source_idx = 0;
			bool found = false;
			for (std::vector<MediaClip>::iterator i = fAnimateDragDropClips.begin(); i != fAnimateDragDropClips.end(); i++)
			{
				const MediaClip &aClip = track->mClips[source_idx];
				if (((*i).mMediaSource != aClip.mMediaSource) ||
					((*i).mSourceFrameStart != aClip.mSourceFrameStart) ||
					((*i).mSourceFrameEnd != aClip.mSourceFrameEnd))
				{
					bool animate = false;
					if ((source_idx > 0) && (fAnimateDragDropClips[source_idx - 1].GetTimelineEndFrame() > intended_timeline_start))
						animate = true;
					if (intended_timeline_start + c.Duration() > fAnimateDragDropClips[source_idx].mTimelineFrameStart)
						animate = true;
					if (animate)
					{
						c.mTimelineFrameStart = intended_timeline_start;
						fAnimateDragDropClips.insert(i, c);
					}
					else
					{
						fAnimateDragDropTrack = nullptr;
						fAnimateDragDropClips.clear();
					}

					found = true;
					break;
				}
				source_idx++;
			}
			if (!found)
			{
				//	Clip at end.  Only animate drag/drop if overlap with last clip
				const MediaClip &second_last = track->mClips[fActiveClip.clip_idx - 1];
				if (second_last.GetTimelineEndFrame() > track->mClips[fActiveClip.clip_idx].mTimelineFrameStart)
				{
					c.mTimelineFrameStart = intended_timeline_start;
					fAnimateDragDropClips.push_back(c);
				}
				else
				{
					fAnimateDragDropTrack = nullptr;
					fAnimateDragDropClips.clear();
				}
			}
		}

		//	For MEDIA_PICTURE clips, automatically append Transform effect
		if (!clip_transform_effect)
			CreateClipTransformEffect(fActiveClip);
		
		MedoWindow::GetInstance()->SetActiveControl(MedoWindow::CONTROL_OUTPUT);
		fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
		MedoWindow::GetInstance()->InvalidatePreview();
		DEBUG("New clip index %d, showing updated clips:\n", fActiveClip.clip_idx);
		//gProject->DebugClips(track_idx);
	}
}

/*	FUNCTION:		TimelineEdit :: CreateClipTransformEffect
	ARGS:			clip
	RETURN:			n/a
	DESCRIPTION:	Called when clip drag 'n' dropped
					For MEDIA_PICTURE clips, automatically append Transform effect when size > project size
*/
void TimelineEdit :: CreateClipTransformEffect(ACTIVE_CLIP &clip)
{
	assert(clip.track && (clip.clip_idx >= 0));

	MediaSource *source = clip.track->mClips[clip.clip_idx].mMediaSource;
	if (source->GetMediaType() == MediaSource::MEDIA_PICTURE)
	{
		if ((source->GetVideoWidth() > gProject->mResolution.width) && (source->GetVideoHeight() > gProject->mResolution.height))
		{
			MediaEffect *media_effect = gEffectsManager->CreateMediaEffect("ZenYes", "Transform");
			media_effect->mTimelineFrameStart = clip.track->mClips[clip.clip_idx].mTimelineFrameStart;
			media_effect->mTimelineFrameEnd = media_effect->mTimelineFrameStart + clip.track->mClips[clip.clip_idx].Duration();
			media_effect->mPriority = 0;
			clip.track->AddEffect(media_effect);
			//	Adjust scale
			((Effect_Transform *)media_effect->mEffectNode)->AutoScale(media_effect, source);

			fActiveEffect.track = clip.track;
			fActiveEffect.clip_idx = clip.clip_idx;
			fActiveEffect.frame_idx = media_effect->mTimelineFrameStart;
			fActiveEffect.media_effect = media_effect;
			int32 effect_idx = 0;
			for (auto &effect : clip.track->mEffects)
			{
				if (effect == media_effect)
					break;
				effect_idx++;
			}
			assert(effect_idx < clip.track->mEffects.size());
			fActiveEffect.effect_idx = effect_idx;

			media_effect->mEffectNode->MediaEffectSelectedBase(media_effect);
		}
	}
}

/*	FUNCTION:		TimelineEdit :: DragDropEffect
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Effect drag 'n' dropped
*/
void TimelineEdit :: DragDropEffect(BMessage *msg)
{
	EffectNode *effect_node;
	status_t found_effect = msg->FindPointer("effect", (void **)&effect_node);
	if (found_effect == B_OK)
	{
		gProject->Snapshot();

		BPoint drop_point;
		msg->FindPoint("_drop_point_", &drop_point);
		BPoint cdp = ConvertFromScreen(drop_point);

		int64 duration = 0;
		msg->FindInt64("duration", &duration);
		
		float xoffset = 0;
		msg->FindFloat("xoffset", &xoffset);
		cdp.x -= xoffset;
		
		//	find track_idx
		float y_pos = kTimelineTrackInitialY;
		int32 track_idx = 0;
		int32 effect_layer = 0;
		for (auto track : gProject->mTimelineTracks)
		{
			if ((cdp.y >= y_pos) &&
				(cdp.y <= y_pos + track->mNumberEffectLayers*kTimelineEffectHeight + kTimelineTrackHeight))
			{
				//	Determine effect_layer
				if (cdp.y < y_pos + track->mNumberEffectLayers*kTimelineEffectHeight)
				{
					effect_layer = track->mNumberEffectLayers - (cdp.y - y_pos)/kTimelineEffectHeight;
				}
				break;
			}
			
			y_pos += kTimelineTrackDeltaY + (kTimelineTrackHeight + track->mNumberEffectLayers*kTimelineEffectHeight);
			track_idx++;
		}

		//	New track?
		if (track_idx >= gProject->mTimelineTracks.size())
		{
			if (gProject->mTimelineTracks.size() < kMaxNumberTimelineTracks)
			{
				gProject->AddTimelineTrack(new TimelineTrack);
			}
			track_idx = gProject->mTimelineTracks.size() - 1;
		}
		
		//	Add effect to track
		int64 frame_index = fLeftFrameIndex + cdp.x*fFramesPixel;
		if (effect_node->GetEffectGroup() != EffectNode::EFFECT_AUDIO)
			frame_index = CalculateStickyFrameIndex(frame_index, true);
		
		MediaEffect *media_effect = gEffectsManager->CreateMediaEffect(effect_node->GetVendorName(), effect_node->GetEffectName());
		media_effect->mTimelineFrameStart = frame_index;
		media_effect->mTimelineFrameEnd = frame_index + duration;
		media_effect->mPriority = effect_layer;
		
		gProject->mTimelineTracks[track_idx]->AddEffect(media_effect);

		//	set as active effect
		fActiveEffect.track = gProject->mTimelineTracks[track_idx];
		fActiveEffect.frame_idx = frame_index;
		fActiveEffect.effect_idx = gProject->mTimelineTracks[track_idx]->GetEffectIndex(media_effect);
		fActiveEffect.media_effect = media_effect;
		fActiveEffect.clip_idx = -1;
		for (size_t ci=0; ci < fActiveEffect.track->mClips.size(); ci++)
		{
			MediaClip &aClip = fActiveEffect.track->mClips[ci];
			if ((media_effect->mTimelineFrameEnd >= aClip.mTimelineFrameStart) && (media_effect->mTimelineFrameStart < aClip.GetTimelineEndFrame()))
			{
				fActiveEffect.clip_idx = (int32)ci;
				break;
			}
		}
		media_effect->mEffectNode->MediaEffectSelectedBase(media_effect);

		fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
		MedoWindow::GetInstance()->InvalidatePreview();
	} 
}

/*	FUNCTION:		TimelineEdit :: EffectMatchClipDuration
	ARGS:			track
					effect
	RETURN:			n/a
	DESCRIPTION:	Force effect duration to clip duration
*/
void TimelineEdit :: EffectMatchClipDuration(TimelineTrack *track, MediaEffect *effect)
{
	assert(effect);
	for (auto &i : track->mClips)
	{
		if ((i.mTimelineFrameStart < effect->mTimelineFrameEnd) && (i.mTimelineFrameStart + i.Duration() > effect->mTimelineFrameStart))
		{
			effect->mTimelineFrameStart = i.mTimelineFrameStart;
			effect->mTimelineFrameEnd = i.mTimelineFrameStart + i.Duration();
			track->RemoveEffect(effect, false);
			track->AddEffect(effect);
			Invalidate();
			break;
		}
	}
}

/*	FUNCTION:		TimelineEdit :: MoveClipUpdate
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Called during MouseMoved() and fMoveClipActive
*/
void TimelineEdit :: MoveClipUpdate(BPoint point)
{
	assert(fState == State::eMoveClip);
	assert(fActiveClip.track != nullptr);
	assert(fActiveClip.clip_idx >= 0);

	//	Check if point outside track
	if (fabs(point.y - fMouseDownPoint.y) > 1.5f*kTimelineTrackHeight)
	{
		fMsgDragDropClip->MakeEmpty();
		gProject->Snapshot();

		int64 frame_idx = fLeftFrameIndex + point.x*fFramesPixel;
		const MediaClip &clip = fActiveClip.track->mClips[fActiveClip.clip_idx];

		fMsgDragDropClip->AddInt64("start", clip.mSourceFrameStart);
		fMsgDragDropClip->AddInt64("end", clip.mSourceFrameEnd);
		fMsgDragDropClip->AddPointer("source", clip.mMediaSource);
		fMsgDragDropClip->AddInt64("xoffset", frame_idx - clip.mTimelineFrameStart);
		fMsgDragDropClip->AddPointer("track", fActiveClip.track);
		fMsgDragDropClip->AddBool("skip_snapshot", true);
			
		BRect aRect;
		aRect.left = point.x - (frame_idx - clip.mTimelineFrameStart)/fFramesPixel;
		aRect.right = aRect.left + clip.Duration()/fFramesPixel;
		aRect.top = point.y - 0.5f*kTimelineTrackHeight;
		aRect.bottom = point.y + 0.5f*kTimelineTrackHeight;

		BRect window_frame = Bounds();
		if (aRect.left < 0)
			aRect.left = 0;
		if (aRect.right > window_frame.Width())
			aRect.right = window_frame.Width();

		BBitmap *drag_bitmap = CreateDragDropClipBitmap(aRect);

		double px = double(frame_idx - clip.mTimelineFrameStart)/(double)clip.Duration();
		fActiveClip.track->mClips.erase(fActiveClip.track->mClips.begin() + fActiveClip.clip_idx);
		fState = State::eIdle;

		DragMessage(fMsgDragDropClip, drag_bitmap, BPoint(aRect.Width()*px, 20));		//	will autodestroy drag_bitmap
		Invalidate();
		fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
		return;
	}

	MediaClip &clip = fActiveClip.track->mClips[fActiveClip.clip_idx];
	int64 frame_index = fLeftFrameIndex + point.x*fFramesPixel;
	if (clip.mMediaSource->GetMediaType() != MediaSource::MEDIA_AUDIO)
		frame_index = CalculateStickyFrameIndex(frame_index, true);
	int64 old_timeline_start = clip.mTimelineFrameStart;
	clip.mTimelineFrameStart = frame_index - fActiveClip.frame_idx;

	//	Check for collision with neighbour clips
	uint32 modifiers = ((MedoWindow *)Window())->GetKeyModifiers();
	bool shift_key = (modifiers & B_LEFT_SHIFT_KEY) | (modifiers & B_RIGHT_SHIFT_KEY);

	if (fActiveClip.clip_idx > 0)
	{
		if (shift_key)
		{
			int64 move_delta = clip.mTimelineFrameStart - old_timeline_start;
			if (move_delta < 0)
			{
				//	move left neighbours
				for (int mi = 0; mi < fActiveClip.clip_idx; mi++)
				{
					fActiveClip.track->mClips[mi].mTimelineFrameStart += move_delta;
					MoveClipLinkedEffects(fActiveClip.track, mi, move_delta);
				}
			}
		}
		else
		{
			const MediaClip &left = fActiveClip.track->mClips[fActiveClip.clip_idx - 1];
			if (left.GetTimelineEndFrame() > clip.mTimelineFrameStart)
				clip.mTimelineFrameStart = left.GetTimelineEndFrame();
		}
	}
	if (fActiveClip.clip_idx < fActiveClip.track->mClips.size() - 1)
	{
		if (shift_key)
		{
			int64 move_delta = clip.mTimelineFrameStart - old_timeline_start;
			if (move_delta > 0)
			{
				//	move right neighbours
				for (int mi = fActiveClip.track->mClips.size() - 1; mi >= fActiveClip.clip_idx + 1; mi--)
				{
					fActiveClip.track->mClips[mi].mTimelineFrameStart += move_delta;
					MoveClipLinkedEffects(fActiveClip.track, mi, move_delta);
				}
			}
		}
		else
		{
			const MediaClip &right = fActiveClip.track->mClips[fActiveClip.clip_idx + 1];
			if (right.mTimelineFrameStart < clip.GetTimelineEndFrame())
				clip.mTimelineFrameStart = right.mTimelineFrameStart - clip.Duration();
		}
	}

	MoveClipLinkedEffects();
	gProject->UpdateDuration();
	fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
	Invalidate();
	gProject->InvalidatePreview();
}

/*	FUNCTION:		TimelineEdit :: MoveEffectUpdate
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Called during MouseMoved() and fMoveEffectActive
*/
void TimelineEdit :: MoveEffectUpdate(BPoint point)
{
	assert(fState == State::eMoveEffect);
	assert(fActiveEffect.track != nullptr);
	assert(fActiveEffect.media_effect != nullptr);
	assert(fActiveEffect.effect_idx >= 0);

	//	Check if point outside track
	if (fabs(point.y - fMouseDownPoint.y) > 1.5f*kTimelineEffectHeight)
	{
		gProject->Snapshot();

		const MediaEffect *effect = fActiveEffect.media_effect;
		fMsgDragDropEffect->MakeEmpty();
		fMsgDragDropEffect->AddPointer("effect", effect->mEffectNode);
		fMsgDragDropEffect->AddInt64("duration", effect->Duration());

		int64 frame_idx = fLeftFrameIndex + point.x*fFramesPixel;
		fMsgDragDropEffect->AddInt64("xoffset", frame_idx - effect->mTimelineFrameStart);

		BRect aRect;
		aRect.left = point.x - (frame_idx - effect->mTimelineFrameStart)/fFramesPixel;
		aRect.right = aRect.left + effect->Duration()/fFramesPixel;
		aRect.top = point.y - 0.5f*kTimelineEffectHeight;
		aRect.bottom = point.y + 0.5f*kTimelineEffectHeight;

		//	Create drag bitmap
		BBitmap *drag_bitmap = new BBitmap(aRect, B_RGBA32);
		drag_bitmap->Lock();
		uint8 *d = (uint8 *)drag_bitmap->Bits();
		for (int32 row=0; row < aRect.Height(); row++)
		{
			for (int32 col=0; col < aRect.Width(); col++)
			{
				*((uint32 *)d)  = 0xff0080ff;	//	Green
				d += 4;
			}
		}
		drag_bitmap->Unlock();
		DragMessage(fMsgDragDropEffect, drag_bitmap, BPoint(20, 20));		//	will autodestroy drag_bitmap

		fActiveEffect.track->mEffects.erase(fActiveEffect.track->mEffects.begin() + fActiveEffect.effect_idx);

		fState = State::eIdle;
		Invalidate();
		fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
		return;
	}
	
	MediaEffect *effect = fActiveEffect.media_effect;
	int64 frame_index = fLeftFrameIndex + point.x*fFramesPixel;
	if (effect->Type() == MediaEffect::MEDIA_EFFECT_IMAGE)
		frame_index = CalculateStickyFrameIndex(frame_index, true);

	bigtime_t effect_duration = effect->Duration();
	effect->mTimelineFrameStart = frame_index - fActiveEffect.frame_idx;
	effect->mTimelineFrameEnd = effect->mTimelineFrameStart + effect_duration;

	//	Elastic link with clip start/end (only useful for audio effects)
	if ((fActiveEffect.clip_idx >= 0) && fActiveEffect.track)
	{
		MediaClip &aClip = fActiveEffect.track->mClips[fActiveEffect.clip_idx];
		bigtime_t delta = effect->mTimelineFrameStart - aClip.mTimelineFrameStart;
		if (abs(delta) < kTimelineElasticGraceX*fFramesPixel)
		{
			effect->mTimelineFrameStart = aClip.mTimelineFrameStart;
			effect->mTimelineFrameEnd = effect->mTimelineFrameStart + effect_duration;
		}

		delta = effect->mTimelineFrameEnd - aClip.GetTimelineEndFrame();
		if (abs(delta) < kTimelineElasticGraceX*fFramesPixel)
		{
			effect->mTimelineFrameEnd = aClip.GetTimelineEndFrame();
			effect->mTimelineFrameStart = effect->mTimelineFrameEnd - effect_duration;
		}
	}

	//	Check for collision with neighbour effects
	if (fActiveEffect.effect_idx > 0)
	{
		for (int left_index = fActiveEffect.effect_idx - 1; left_index >= 0; left_index--)
		{
			const MediaEffect *left = fActiveEffect.track->mEffects[left_index];
			if (left->mPriority == effect->mPriority)
			{
				if (left->mTimelineFrameEnd > effect->mTimelineFrameStart)
				{
					effect->mTimelineFrameStart = left->mTimelineFrameEnd;
					effect->mTimelineFrameEnd = effect->mTimelineFrameStart + effect_duration;
				}
				break;
			}
		}
	}
	if (fActiveEffect.effect_idx < fActiveEffect.track->mEffects.size() - 1)
	{
		for (int right_index = fActiveEffect.effect_idx + 1; right_index < fActiveEffect.track->mEffects.size(); right_index++)
		{
			const MediaEffect *right = fActiveEffect.track->mEffects[right_index];
			if (right->mPriority == effect->mPriority)
			{
				if (right->mTimelineFrameStart < effect->mTimelineFrameEnd)
				{
					effect->mTimelineFrameEnd = right->mTimelineFrameStart;
					effect->mTimelineFrameStart = effect->mTimelineFrameEnd - effect_duration;
				}
				break;
			}
		}
	}

	gProject->UpdateDuration();
	Invalidate();
	fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
	gProject->InvalidatePreview();
}

/*	FUNCTION:		TimelineEdit :: ResizeClipUpdate
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Clip resized
*/
void TimelineEdit :: ResizeClipUpdate(BPoint point)
{
	if (fState != State::eResizeClip)
	{
		printf("TimelineEdit::ResizeClipUpdate() - warning, fState != State::eResizeClip\n");
		return;
	}
	assert(fActiveClip.track != nullptr);
	assert(fActiveClip.clip_idx >= 0);

	MediaClip &media_clip = fActiveClip.track->mClips[fActiveClip.clip_idx];
	int64 frame_idx = fLeftFrameIndex + point.x*fFramesPixel;
	if (media_clip.mMediaSource->GetMediaType() != MediaSource::MEDIA_AUDIO)
		frame_idx = CalculateStickyFrameIndex(frame_idx, fActiveResizeDirection == ResizeDirection::eLeft);

	int64 original_start = media_clip.mSourceFrameStart;
	int64 original_end = media_clip.mSourceFrameEnd;

	if (fActiveResizeDirection == ResizeDirection::eLeft)
	{
		media_clip.mSourceFrameStart = fResizeClipOriginalSourceFrame + (frame_idx - fResizeClipOriginalTimelineFrame);
		if (media_clip.mSourceFrameStart < 0)
			media_clip.mSourceFrameStart = 0;

		media_clip.mTimelineFrameStart = frame_idx;
		if (media_clip.mTimelineFrameStart < fResizeClipOriginalTimelineFrame - fResizeClipOriginalSourceFrame)
			media_clip.mTimelineFrameStart = fResizeClipOriginalTimelineFrame - fResizeClipOriginalSourceFrame;

		if (media_clip.mSourceFrameStart >= media_clip.mSourceFrameEnd)
		{
			media_clip.mSourceFrameStart = media_clip.mSourceFrameEnd - 1;
		}
	}
	else if (fActiveResizeDirection == ResizeDirection::eRight)
	{
		media_clip.mSourceFrameEnd = fResizeClipOriginalSourceFrame + (frame_idx - fResizeClipOriginalTimelineFrame);
		if ((media_clip.mMediaSource->GetMediaType() == MediaSource::MEDIA_VIDEO) || (media_clip.mMediaSource->GetMediaType() == MediaSource::MEDIA_VIDEO_AND_AUDIO))
		{
			if (media_clip.mSourceFrameEnd >= media_clip.mMediaSource->GetVideoDuration())
				media_clip.mSourceFrameEnd = media_clip.mMediaSource->GetVideoDuration();
		}
		else if (media_clip.mMediaSource->GetMediaType() == MediaSource::MEDIA_AUDIO)
		{
			if (media_clip.mSourceFrameEnd >= media_clip.mMediaSource->GetAudioDuration())
				media_clip.mSourceFrameEnd = media_clip.mMediaSource->GetAudioDuration();
		}

		if (media_clip.mSourceFrameEnd <= media_clip.mSourceFrameStart)
		{
			media_clip.mSourceFrameEnd = media_clip.mSourceFrameStart + 1;
		}
	}

	//	Check for collision with neighbour clips
	if (fActiveClip.clip_idx > 0)
	{
		const MediaClip &left = fActiveClip.track->mClips[fActiveClip.clip_idx - 1];
		if (left.GetTimelineEndFrame() > media_clip.mTimelineFrameStart)
		{
			media_clip.mTimelineFrameStart = left.GetTimelineEndFrame();
			media_clip.mSourceFrameStart = original_start;
		}
	}
	if (fActiveClip.clip_idx < fActiveClip.track->mClips.size() - 1)
	{
		const MediaClip &right = fActiveClip.track->mClips[fActiveClip.clip_idx + 1];
		if (right.mTimelineFrameStart < media_clip.GetTimelineEndFrame())
		{
			media_clip.mSourceFrameEnd = original_end;
		}
	}

	gProject->UpdateDuration();
	Invalidate();
	fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
	gProject->InvalidatePreview();
}

/*	FUNCTION:		TimelineEdit :: ResizeEffectUpdate
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Effect resized
*/
void TimelineEdit :: ResizeEffectUpdate(BPoint point)
{
	MediaEffect *effect = fActiveEffect.media_effect;
	if (!effect)
		return;

	int64 frame_idx = fLeftFrameIndex + point.x*fFramesPixel;
	if (effect->Type() == MediaEffect::MEDIA_EFFECT_IMAGE)
		frame_idx = CalculateStickyFrameIndex(frame_idx, fActiveResizeDirection == ResizeDirection::eLeft);

	if (fActiveResizeDirection == ResizeDirection::eLeft)
	{
		effect->mTimelineFrameStart = frame_idx;
		if (effect->mTimelineFrameStart < 0)
			effect->mTimelineFrameStart = 0;
	}
	else if (fActiveResizeDirection == ResizeDirection::eRight)
	{
		effect->mTimelineFrameEnd = frame_idx;
	}

	//	Elastic link with clip start/end (only useful for audio effects)
	if ((fActiveEffect.clip_idx >= 0) && fActiveEffect.track)
	{
		MediaClip &aClip = fActiveEffect.track->mClips[fActiveEffect.clip_idx];

		bigtime_t delta = effect->mTimelineFrameStart - aClip.mTimelineFrameStart;
		if (abs(delta) < kTimelineElasticGraceX*fFramesPixel)
			effect->mTimelineFrameStart = aClip.mTimelineFrameStart;

		delta = effect->mTimelineFrameEnd - aClip.GetTimelineEndFrame();
		if (abs(delta) < kTimelineElasticGraceX*fFramesPixel)
			effect->mTimelineFrameEnd = aClip.GetTimelineEndFrame();
	}

	//	Check for collision with neighbour effects
	if (fActiveEffect.effect_idx > 0)
	{
		for (int left_index = fActiveEffect.effect_idx - 1; left_index >= 0; left_index--)
		{
			const MediaEffect *left = fActiveEffect.track->mEffects[left_index];
			if (left->mPriority == effect->mPriority)
			{
				if (left->mTimelineFrameEnd > effect->mTimelineFrameStart)
				{
					effect->mTimelineFrameStart = left->mTimelineFrameEnd;
				}
				break;
			}
		}
	}
	if (fActiveEffect.effect_idx < fActiveEffect.track->mEffects.size() - 1)
	{
		for (int right_index = fActiveEffect.effect_idx + 1; right_index < fActiveEffect.track->mEffects.size(); right_index++)
		{
			const MediaEffect *right = fActiveEffect.track->mEffects[right_index];
			if (right->mPriority == effect->mPriority)
			{
				if (right->mTimelineFrameStart < effect->mTimelineFrameEnd)
				{
					effect->mTimelineFrameEnd = right->mTimelineFrameStart;
				}
				break;
			}
		}
	}

	gProject->UpdateDuration();
	Invalidate();
	fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
	gProject->InvalidatePreview();
}

/*	FUNCTION:		TimelineEdit :: CalculateStickyFrameIndex
	ARGS:			frame_idx
					left_reference
	RETURN:			sticky frame index
	DESCRIPTION:	Stick to frame edge
*/
int64 TimelineEdit :: CalculateStickyFrameIndex(int64 frame_idx, bool left_reference)
{
	double fi = (double)gProject->mResolution.frame_rate * (double)frame_idx/(double)kFramesSecond;
	int64 fr = int64(fi+0.5);
	if (!left_reference)
		++fr;
	int64 new_frame_idx = kFramesSecond * fr/(double)gProject->mResolution.frame_rate;
	//printf("frame_idx=%ld, fi=%f, fr=%ld, new_frame_idx=%ld\n", frame_idx, fi, fr, new_frame_idx);
	return new_frame_idx;
}

