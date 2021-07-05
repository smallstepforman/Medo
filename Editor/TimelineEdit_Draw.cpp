/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Timeline edit
 */
 
#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <support/StringList.h>

#include "AudioManager.h"
#include "EffectNode.h"
#include "Language.h"
#include "Project.h"
#include "RenderActor.h"
#include "Theme.h"
#include "TimelineEdit.h"
#include "VideoManager.h"


#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

static const unsigned char kImageEffectsColours[][3] =
{
	{32, 128, 255},
	{30, 120, 240},
	{28, 112, 224},
	{26, 104, 208},
	{24, 96, 192},
	{22, 88, 176},
	{20, 80, 160},
	{18, 72, 144},
	{16, 64, 128},
	{14, 56, 112},
	{12, 48, 96},
	{10, 40, 80},
	{8, 32, 64},
	{6, 24, 48},
	{4, 16, 32},
	{2, 8, 16},
};
static_assert(sizeof(kImageEffectsColours) == 16*3*sizeof(char), "sizeof(kImageEffectsColours != 16");

static const unsigned char kAudioEffectsColours[][3] =
{
	{176, 64, 0},
	{160, 64, 0},
	{144, 64, 0},
	{128, 64, 0},
	{112, 64, 0},
	{96, 64, 0},
	{80, 64, 0},
	{64, 64, 0},
};
static_assert(sizeof(kAudioEffectsColours) == 8*3*sizeof(char), "sizeof(kAudioEffectsColours != 8");


/*	FUNCTION:		TimelineEdit :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw view
*/
void TimelineEdit :: Draw(BRect frame)
{
	//	Draw tracks
	BRect track_frame = frame;
	track_frame.top += kTimelineTrackInitialY - fScrollViewOffsetY;
	for (auto &track : gProject->mTimelineTracks)
	{
		track_frame.bottom = track_frame.top + kTimelineTrackHeight;
		if (track != fAnimateDragDropTrack)	
			DrawTrack(track, track_frame);
		else
			AnimateDragDropDrawTrack(track, track_frame);
		
		track_frame.top += kTimelineTrackDeltaY + (kTimelineTrackHeight + track->mNumberEffectLayers*kTimelineEffectHeight);
	}
}

/*	FUNCTION:		TimelineEdit :: DrawTrackEffects
	ARGS:			track
					frame
	RETURN:			n/a
	DESCRIPTION:	Draw track effects
*/
void TimelineEdit :: DrawTrackEffects(TimelineTrack *track, BRect frame)
{
	const int64 kNumberVisibleFrames = fViewWidth*fFramesPixel;
	fActiveEffect.highlight_rect_visible = false;
	
	for (auto &i : track->mEffects)
	{
		if ((i->mTimelineFrameStart > fLeftFrameIndex + kNumberVisibleFrames) ||
			(i->mTimelineFrameEnd < fLeftFrameIndex))
			continue;
			
		int64 left = i->mTimelineFrameStart;
		int64 right = i->mTimelineFrameEnd;
		int64 mid = left + (i->mTimelineFrameEnd - i->mTimelineFrameStart)/2;

		if (left < fLeftFrameIndex)
		{
			left = fLeftFrameIndex;
		}
		if (right >= fLeftFrameIndex + kNumberVisibleFrames)
		{
			right = fLeftFrameIndex + kNumberVisibleFrames;
		}
		
		BRect clip_frame = frame;
		clip_frame.left = (left - fLeftFrameIndex)/fFramesPixel;
		clip_frame.right = (right - fLeftFrameIndex)/fFramesPixel;
		clip_frame.top += (track->mNumberEffectLayers - i->mPriority - 1)*kTimelineEffectHeight;
		clip_frame.bottom = clip_frame.top + kTimelineEffectHeight;
		
		if (i->Type() == MediaEffect::MEDIA_EFFECT_IMAGE)
			SetHighColor(kImageEffectsColours[i->mPriority][0],
						kImageEffectsColours[i->mPriority][1],
						kImageEffectsColours[i->mPriority][2]);
		else	//	MediaEffect::MEDIA_EFFECT_AUDIO
			SetHighColor(kAudioEffectsColours[i->mPriority][0],
						kAudioEffectsColours[i->mPriority][1],
						kAudioEffectsColours[i->mPriority][2]);
		FillRoundRect(clip_frame, kRoundRectRadius, kRoundRectRadius);

		if (!i->mEnabled)
		{
			SetHighColor(255, 255, 0);
			StrokeLine(BPoint(clip_frame.left, clip_frame.top), BPoint(clip_frame.right, clip_frame.bottom));
			StrokeLine(BPoint(clip_frame.left, clip_frame.bottom), BPoint(clip_frame.right, clip_frame.top));
		}
		
		SetHighColor(255, 255, 255);
		BString label(i->mEffectNode->GetTextEffectName(gLanguageManager->GetCurrentLanguageIndex()));
		TruncateString(&label, B_TRUNCATE_MIDDLE, clip_frame.Width() - 2);
		float text_width = StringWidth(label.String());
		float text_xpos = (mid - fLeftFrameIndex)/fFramesPixel - 0.5f*text_width;
		if (text_xpos < 0.0f)
			text_xpos = 0.0f;
		if (text_xpos > clip_frame.right - 0.5f*text_width - 2*B_V_SCROLL_BAR_WIDTH)
		{
			text_xpos = clip_frame.right - 0.5f*text_width - 2*B_V_SCROLL_BAR_WIDTH;
			if (text_xpos < clip_frame.left)
				text_xpos = clip_frame.left;
		}
		MovePenTo(text_xpos, clip_frame.top+kTimelineEffectHeight - 8);
		DrawString(label);

		//	Frame
		if (fActiveEffect.media_effect == i)
		{
			clip_frame.left += 2.0f;
			clip_frame.top += 2.0f;
			clip_frame.right -= 2.0f;
			clip_frame.bottom -= 2.0f;
			fActiveEffect.highlight_rect = clip_frame;
			fActiveEffect.highlight_rect_visible = true;
		}
	}		
}

/*	FUNCTION:		TimelineEdit :: DrawTrack
	ARGS:			track
					frame
	RETURN:			n/a
	DESCRIPTION:	Draw track
*/
void TimelineEdit :: DrawTrack(TimelineTrack *track, BRect frame)
{
	assert(track != nullptr);

	DrawTrackEffects(track, frame);
	const float kEffectLayersOffset = track->mNumberEffectLayers*kTimelineEffectHeight;
	frame.top += kEffectLayersOffset;
	frame.bottom += kEffectLayersOffset;
		
	//	Draw track
	SetHighColor(Theme::GetUiColour(UiColour::eTimelineTrack));
	FillRect(frame);
	
	const int64 kNumberVisibleFrames = fViewWidth*fFramesPixel;
	fActiveClip.highlight_rect_visible = false;
	
	if (fDrawAllVideoThumbnails)
		gVideoManager->ClearPendingThumbnails();

	font_height text_height;
	be_plain_font->GetHeight(&text_height);

	//	Now draw tracks
	int32 clip_index = 0;
	for (auto &i : track->mClips)
	{
		const bool has_video = (i.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (i.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO);
		const bool has_audio = (i.mMediaSourceType == MediaSource::MEDIA_AUDIO) || (i.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO);
		const bool has_picture = (i.mMediaSourceType == MediaSource::MEDIA_PICTURE);

		if ((i.mTimelineFrameStart > fLeftFrameIndex + kNumberVisibleFrames) ||
			(i.GetTimelineEndFrame() < fLeftFrameIndex))
			continue;
			
		int64 left = i.mTimelineFrameStart;
		int64 left_thumb = i.mSourceFrameStart;
		if (left < fLeftFrameIndex)
		{
			left = fLeftFrameIndex;
			left_thumb += fLeftFrameIndex - i.mTimelineFrameStart;
		}
		
		int64 right = i.GetTimelineEndFrame();
		int64 right_thumb = i.mSourceFrameEnd - 1;
		if (right > fLeftFrameIndex + kNumberVisibleFrames)
		{
			right = fLeftFrameIndex + kNumberVisibleFrames;
			right_thumb = i.mSourceFrameStart + (right - i.mTimelineFrameStart);	
		}
		
		BRect clip_frame = frame;
		clip_frame.left = (left - fLeftFrameIndex)/fFramesPixel;
		clip_frame.right = (right - fLeftFrameIndex)/fFramesPixel;
		
		if (has_video)
		{
			SetHighColor(32, 192, 32);
			FillRoundRect(clip_frame, kRoundRectRadius, kRoundRectRadius);
			DrawTrackThumbnails(clip_frame, i.mMediaSource, left_thumb, right_thumb);

			if (!i.mVideoEnabled)
			{
				SetHighColor(255, 0, 0);
				StrokeLine(BPoint(clip_frame.left, clip_frame.top), BPoint(clip_frame.right, clip_frame.bottom));
				StrokeLine(BPoint(clip_frame.left, clip_frame.bottom), BPoint(clip_frame.right, clip_frame.top));
			}
		}
		else if (has_picture)
		{
			BBitmap *bitmap = i.mMediaSource->GetBitmap();
			float aspect = (float)i.mMediaSource->GetVideoWidth()/(float)i.mMediaSource->GetVideoHeight();
			float w = aspect * clip_frame.Height();
			BRect thumb_rect = clip_frame;
			thumb_rect.left = (clip_frame.left + clip_frame.right)/2 - 0.5f*w;		if (thumb_rect.left < clip_frame.left)		thumb_rect.left = clip_frame.left;
			thumb_rect.right = (clip_frame.left + clip_frame.right)/2 + 0.5f*w;		if (thumb_rect.right > clip_frame.right)	thumb_rect.right = clip_frame.right;
			SetHighColor(128, 64+(clip_index%10)*10, 32+(clip_index%10)*20);
			if (bitmap)
			{
				FillRoundRect(clip_frame, kRoundRectRadius, kRoundRectRadius);
				DrawBitmapAsync(i.mMediaSource->GetBitmap(), thumb_rect);
			}
			else
			{
				FillRoundRect(clip_frame, kRoundRectRadius, kRoundRectRadius);
			}
		}

		//	Audio thumbnail
		if (has_audio)
		{
			BRect audio_frame = clip_frame;
			if (has_video)
				audio_frame.top += kTimelineTrackHeight;
			audio_frame.bottom += kTimelineTrackSoundY;

			//	Corrupt MediaSource may have shorter audio tracks ...
			const int64 audio_end_frame = i.mSourceFrameStart + i.mMediaSource->GetAudioDuration();
			if (right_thumb >= audio_end_frame)
				right_thumb = audio_end_frame - 1;
			if (left_thumb < audio_end_frame)
			{
				gAudioManager->ClearPendingThumbnails();
				BBitmap *audio_bitmap = gAudioManager->GetBitmapAsync(i.mMediaSource, left_thumb, right_thumb, audio_frame.Width() - 2*kRoundRectRadius, audio_frame.Height());
				if (audio_bitmap)
				{
					DrawBitmapAsync(audio_bitmap, audio_frame);

					//	Draw ears
					if (has_video)
						SetHighColor(Theme::GetUiColour(UiColour::eTimelineView));
					else
						SetHighColor(Theme::GetUiColour(UiColour::eTimelineTrack));
					FillTriangle(BPoint(audio_frame.left-1, audio_frame.top-1),
								 BPoint(audio_frame.left+0.75f*kRoundRectRadius, audio_frame.top-1),
								 BPoint(audio_frame.left-1, audio_frame.top + 0.75f*kRoundRectRadius));
					FillTriangle(BPoint(audio_frame.right+1, audio_frame.top-1),
								 BPoint(audio_frame.right-0.75f*kRoundRectRadius, audio_frame.top-1),
								 BPoint(audio_frame.right+1, audio_frame.top + 0.75f*kRoundRectRadius));

					SetHighColor(Theme::GetUiColour(UiColour::eTimelineView));
					FillTriangle(BPoint(audio_frame.left-1, audio_frame.bottom+1),
								 BPoint(audio_frame.left+0.75f*kRoundRectRadius, audio_frame.bottom+1),
								 BPoint(audio_frame.left-1, audio_frame.bottom - 0.75f*kRoundRectRadius));
					FillTriangle(BPoint(audio_frame.right+1, audio_frame.bottom+1),
								 BPoint(audio_frame.right-0.75f*kRoundRectRadius, audio_frame.bottom+1),
								 BPoint(audio_frame.right+1, audio_frame.bottom - 0.75f*kRoundRectRadius));
				}
				else
				{
					SetHighColor(255, 192, 0);
					FillRoundRect(audio_frame, kRoundRectRadius, kRoundRectRadius);
				}

				if (!i.mAudioEnabled)
				{
					SetHighColor(255, 0, 0);
					StrokeLine(BPoint(audio_frame.left, audio_frame.top), BPoint(audio_frame.right, audio_frame.bottom));
					StrokeLine(BPoint(audio_frame.left, audio_frame.bottom), BPoint(audio_frame.right, audio_frame.top));
				}
			}
		}

		//	Indicate left/right clip boundaries (relative to source media and visible viewport)
		SetHighColor(255, 255, 255);
		if (left > i.mTimelineFrameStart)
		{
			//	left not visible
			MovePenTo(BPoint(clip_frame.left, clip_frame.top+0.5f*clip_frame.Height()));
			DrawString("<");
		}
		else if (i.mSourceFrameStart > 0)
		{
			//	Clip can be resized left
			MovePenTo(BPoint(clip_frame.left, clip_frame.top+0.5f*clip_frame.Height()));
			DrawString("*");
		}
		if (right < i.mTimelineFrameStart + i.Duration())
		{
			//	Right not visible
			MovePenTo(BPoint(clip_frame.right - (0.5f*text_height.ascent), clip_frame.top+0.5f*clip_frame.Height()));
			DrawString(">");
		}
		else if (i.mSourceFrameEnd < i.mMediaSource->GetTotalDuration())
		{
			//	Right can be resized right
			MovePenTo(BPoint(clip_frame.right - (0.5f*text_height.ascent), clip_frame.top+0.5f*clip_frame.Height()));
			DrawString("*");
		}

		//	Tag
		if (fClipTagsVisible && !i.mTag.IsEmpty())
		{
			int64 mid = clip_frame.left + 0.5f*clip_frame.Width();
			SetHighColor(255, 255, 255);
			BString tag(i.mTag);
			TruncateString(&tag, B_TRUNCATE_MIDDLE, clip_frame.Width() - 2);
			float text_width = StringWidth(tag.String());
			float text_xpos = mid - 0.5f*text_width;
			if (text_xpos < 0.0f)
				text_xpos = 0.0f;
			if (text_xpos > clip_frame.right - 0.5f*text_width - 2*B_V_SCROLL_BAR_WIDTH)
			{
				text_xpos = clip_frame.right - 0.5f*text_width - 2*B_V_SCROLL_BAR_WIDTH;
				if (text_xpos < clip_frame.left)
					text_xpos = clip_frame.left;
			}
			MovePenTo(text_xpos, clip_frame.top + 0.5f*clip_frame.Height() + 0.5f*text_height.ascent);
			SetFont(be_bold_font);
			DrawString(tag);
			SetFont(be_plain_font);
		}

		//	Frame
		if ((fActiveClip.track == track) && (fActiveClip.clip_idx == clip_index))
		{
			clip_frame.left += 2.0f;
			clip_frame.top += 2.0f;
			clip_frame.right -= 2.0f;
			clip_frame.bottom -= 2.0f;
			if (has_audio && !has_video)
				clip_frame.bottom += kTimelineTrackSoundY;
			fActiveClip.highlight_rect = clip_frame;
			fActiveClip.highlight_rect_visible = true;
		}
		clip_index++;
	}

	//	Highlight active clip
	if (fActiveClip.highlight_rect_visible && (fActiveClip.track == track) && (fActiveClip.clip_idx >= 0))
	{
		SetHighColor(255, 255, 0);
		SetPenSize(4.0f);
		StrokeRoundRect(fActiveClip.highlight_rect, kRoundRectRadius, kRoundRectRadius);
	}
	//	Highlight active effect
	if (fActiveEffect.highlight_rect_visible && (fActiveEffect.track == track) && fActiveEffect.media_effect)
	{
		SetHighColor(255, 255, 0);
		SetPenSize(4.0f);
		StrokeRoundRect(fActiveEffect.highlight_rect, kRoundRectRadius, kRoundRectRadius);
	}

	//	Notes
	if (fTrackNotesVisible && !track->mNotes.empty())
		DrawTrackNotes(track, frame);
}

/*	FUNCTION:		TimelineEdit :: DrawTrackThumbnails
	ARGS:			rect
					source
					left_frame
					right_frame
	RETURN:			n/a
	DESCRIPTION:	Draw track thumbnails
					TODO align thumb left to timeline
*/
void TimelineEdit :: DrawTrackThumbnails(const BRect &rect, MediaSource *source, const int64 left_frame, const int64 right_frame)
{
#define DRAW_THUMB_PLACEHOLDER		0

	if (fDrawAllVideoThumbnails)
	{
		BRect thumb_rect = rect;

		int num_thumbs = rect.Width() / (16*6);
		double frame_time = kFramesSecond / gProject->mResolution.frame_rate;
		int clip_thumbs = (right_frame - left_frame + 0.5f*frame_time) / frame_time;
		if (clip_thumbs < num_thumbs)
			num_thumbs = clip_thumbs;
		float padding = rect.Width() - num_thumbs*(16*6);
		if (num_thumbs > 1)
			padding /= (num_thumbs-1);
		const int64 frames_per_thumb = fFramesPixel*(16*6);
		int pending_notifications = 0;

		for (int c=0; c < num_thumbs; c++)
		{
			thumb_rect.left = rect.left + c*16*6 + c*padding;
			thumb_rect.right = thumb_rect.left + (16*6);
			int64 thumb_frame = left_frame + c*(right_frame - left_frame)/num_thumbs;
			if (c==0)
				thumb_frame = left_frame;
			else if (c == num_thumbs - 1)
				thumb_frame = right_frame;
			else if (c > 0)
			{
				//	round thumb_frame to nearest threshold (prevent regenerating thumbs for adjacent frames when slight clip resize)
				int64 rnd = thumb_frame/frames_per_thumb;
				if (rnd > 0)
					thumb_frame = rnd*frames_per_thumb;
			}

			BBitmap *thumb = gVideoManager->GetThumbnailAsync(source, thumb_frame, (pending_notifications == 0));
			if (thumb)
			{
				DrawBitmapAsync(thumb, thumb_rect);
				if (c == 0)
				{
					SetHighColor(Theme::GetUiColour(UiColour::eTimelineTrack));
					FillTriangle(BPoint(thumb_rect.left - 1, thumb_rect.top - 1),
								 BPoint(thumb_rect.left + kRoundRectRadius, thumb_rect.top - 1),
								 BPoint(thumb_rect.left - 1, thumb_rect.top + kRoundRectRadius));
					FillTriangle(BPoint(thumb_rect.left - 1, thumb_rect.bottom + 1),
								 BPoint(thumb_rect.left + kRoundRectRadius, thumb_rect.bottom + 1),
								 BPoint(thumb_rect.left - 1, thumb_rect.bottom - kRoundRectRadius));
				}
				else if (c == num_thumbs-1)
				{
					SetHighColor(Theme::GetUiColour(UiColour::eTimelineTrack));
					FillTriangle(BPoint(thumb_rect.right + 1, thumb_rect.top - 1),
								 BPoint(thumb_rect.right - kRoundRectRadius, thumb_rect.top - 1),
								 BPoint(thumb_rect.right + 1, thumb_rect.top + kRoundRectRadius));
					FillTriangle(BPoint(thumb_rect.right + 1, thumb_rect.bottom + 1),
								 BPoint(thumb_rect.right - kRoundRectRadius, thumb_rect.bottom + 1),
								 BPoint(thumb_rect.right + 1, thumb_rect.bottom - kRoundRectRadius));
				}
			}
#if DRAW_THUMB_PLACEHOLDER
			else
			{
				SetHighColor(32, 176, 32);
				FillRect(thumb_rect);
				pending_notifications++;
			}
#endif
		}
	}
	else	//fDrawAllVideoThumbnails
	{
		//	Left thumbnail
		BRect thumb_rect = rect;
		thumb_rect.right = rect.left + 16*6;
		if (thumb_rect.right > rect.right)
			thumb_rect.right = rect.right;
		if (rect.Width() < 2*16*6)
			thumb_rect.right = thumb_rect.left + 0.5f*rect.Width();
		BBitmap *thumb = gVideoManager->GetThumbnailAsync(source, left_frame, true);
		if (thumb)
			DrawBitmapAsync(thumb, thumb_rect);
#if DRAW_THUMB_PLACEHOLDER
		else
		{
			SetHighColor(32, 176, 32);
			FillRect(thumb_rect);
		}
#endif

		//	Right thumbnail
		thumb_rect = rect;
		thumb_rect.left = rect.right - 16*6;
		if (thumb_rect.left < rect.left)
			thumb_rect.left = rect.left;
		if (rect.Width() < 2*16*6)
			thumb_rect.left = thumb_rect.right - 0.5f*rect.Width();
		thumb = gVideoManager->GetThumbnailAsync(source, right_frame, true);
		if (thumb)
			DrawBitmapAsync(thumb, thumb_rect);
#if DRAW_THUMB_PLACEHOLDER
		else
		{
			SetHighColor(32, 176, 32);
			FillRect(thumb_rect);
		}
#endif
	}
}

/*	FUNCTION:		TimelineEdit :: CreateDragDropClipBitmap
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Create drag drop clip bitmap
*/
BBitmap * TimelineEdit :: CreateDragDropClipBitmap(BRect frame)
{
	const MediaClip &clip = fActiveClip.track->mClips[fActiveClip.clip_idx];

	uint32 colour;
	BBitmap *thumb = nullptr;
	switch (clip.mMediaSource->GetMediaType())
	{
		case MediaSource::MEDIA_AUDIO:
			colour = 0xffffc000;
			break;

		case MediaSource::MEDIA_PICTURE:
			colour = 0xff804020;
			break;

		default:
			//	Only video clips show a thumb (if available)
			thumb = gVideoManager->GetThumbnailAsync(clip.mMediaSource, clip.mSourceFrameStart, false);
			colour = 0xff00ff00;
			break;
	}

	BBitmap *drag_bitmap = new BBitmap(frame, B_RGBA32);
	drag_bitmap->Lock();
	uint8 *d = (uint8 *)drag_bitmap->Bits();
	if (thumb)
	{
		uint8 *s = (uint8 *)thumb->Bits();
		int32 bytes_per_row = thumb->BytesPerRow();
		int32 bw = bytes_per_row / 4;
		int32 bh = thumb->BitsLength() / bytes_per_row;
		assert(bh <= frame.Height());
		int32 w = bw < frame.Width() ? bw : 0;
		for (int32 row=0; row < frame.Height()+1; row++)
		{
			if (w > 0)
			{
				uint8 *bs = s + row*bytes_per_row;
				for (int32 col=0; col < w; col++)
				{
					*((uint32 *)d) = *((uint32 *)bs);
					d += 4;
					bs += 4;
				}
			}
			for (int32 col=w; col < frame.Width()+1; col++)
			{
				*((uint32 *)d)  = colour;
				d += 4;
			}
		}
	}
	else
	{
		for (int32 row=0; row < frame.Height()+1; row++)
		{
			for (int32 col=0; col < frame.Width()+1; col++)
			{
				*((uint32 *)d)  = colour;
				d += 4;
			}
		}
	}
	drag_bitmap->Unlock();
	return drag_bitmap;
}

/*	FUNCTION:		TimelineEdit :: DrawTrackNotes
	ARGS:			track
					frame
	RETURN:			n/a
	DESCRIPTION:	Draw track notes
*/
void TimelineEdit :: DrawTrackNotes(TimelineTrack *track, BRect frame)
{
	font_height fh;
	be_plain_font->GetHeight(&fh);

	for (auto &n : track->mNotes)
	{
		BPoint pos((n.mTimelineFrame - fLeftFrameIndex)/fFramesPixel, frame.top + 0.5f*frame.Height());

		SetHighColor(255, 255, 32);
		BRect fill_rect(pos.x - n.mWidth, pos.y - n.mHeight, pos.x + n.mWidth, pos.y + n.mHeight);
		FillRect(fill_rect);

		SetHighColor(0, 0, 0);
		BStringList string_list;
		n.mText.Split("\n", true, string_list);
		assert(string_list.CountStrings() == n.mTextWidths.size());
		for (int si = string_list.CountStrings() - 1; si >= 0; si--)	//	backward due to descend characters (yg)
		{
			float y_offset = (fh.ascent + 0.5f*fh.descent) * 1.025f * si;
			if (string_list.CountStrings() == 1)
				y_offset += 0.5f*(fh.ascent + fh.descent);
			MovePenTo(pos.x - 0.5f*n.mTextWidths[si], pos.y - n.mHeight + fh.ascent + y_offset);
			DrawString(string_list.StringAt(si).String());
		}
	}
}

/*	FUNCTION:		TimelineEdit :: AnimateDragDropDrawTrack
	ARGS:			track_idx
					frame
	RETURN:			n/a
	DESCRIPTION:	Draw animation to represent drag drop
*/
void TimelineEdit :: AnimateDragDropDrawTrack(TimelineTrack *track, BRect frame)
{
	assert(track != nullptr);
	assert(fAnimateDragDropTrack != nullptr);
	assert(track->mClips.size() == fAnimateDragDropClips.size());
	
	//	TODO need to animate effects track as well
	DrawTrackEffects(track, frame);
	const float kEffectLayersOffset = track->mNumberEffectLayers*kTimelineEffectHeight;
	frame.top += kEffectLayersOffset;
	frame.bottom += kEffectLayersOffset;
	
	const int64 kNumberVisibleFrames = fViewWidth*fFramesPixel;
	
	float t = (float)(system_time() - fAnimateDragDropTimestamp) / 250000.0f;		//	 quarter second animation
	if (t > 1.0f)
		t = 1.0f;
		
	
	//	Draw track
	SetHighColor(176, 176, 176);
	FillRect(frame);
	
	//	Now draw tracks
	int32 idx = 0;
	for (auto &i : fAnimateDragDropClips)
	{
		int64 left = i.mTimelineFrameStart;
		int64 left_thumb = i.mSourceFrameStart;
		if (left < fLeftFrameIndex)
		{
			left = fLeftFrameIndex;
			left_thumb += fLeftFrameIndex - i.mTimelineFrameStart;
		}
		int64 right = i.GetTimelineEndFrame();
		int64 right_thumb = i.mSourceFrameEnd;
		if (right >= fLeftFrameIndex + kNumberVisibleFrames)
		{
			right = fLeftFrameIndex + kNumberVisibleFrames;
			right_thumb = i.mSourceFrameStart + (right - i.mTimelineFrameStart);
		}
		int64 left_target = track->mClips[idx].mTimelineFrameStart;
		if (left_target < fLeftFrameIndex)
			left_target = fLeftFrameIndex;
		int64 right_target = track->mClips[idx].GetTimelineEndFrame();
		if (right_target >= fLeftFrameIndex + kNumberVisibleFrames)
			right_target = fLeftFrameIndex + kNumberVisibleFrames;
		
		float pos_left = left + t*(left_target - left);
		float pos_right = right + t*(right_target - right);
		
		BRect clip_frame = frame;
		clip_frame.left = (pos_left - fLeftFrameIndex)/fFramesPixel;
		clip_frame.right = (pos_right - fLeftFrameIndex)/fFramesPixel;
		
		if (idx != fActiveClip.clip_idx)
		{
			SetHighColor(32, 192, 32);
			FillRect(clip_frame);
		}
		else
		{
			//	TODO some form of highlight
			SetHighColor(255, 255, 0);
			//StrokeRect(clip_frame);
			clip_frame.top += 20;
			clip_frame.bottom += 20;
			FillRect(clip_frame);
			idx++;
			continue;	//	dont show thumbs
		}
		idx++;
			
		//	Thumbnails
		const bool has_video = (i.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (i.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO);
		if (has_video)
		{
			BBitmap *thumb = gVideoManager->GetThumbnailAsync(i.mMediaSource, left_thumb, true);
			BRect thumb_frame;
			if (thumb)
			{
				thumb_frame = clip_frame;
				thumb_frame.right = clip_frame.left + 16*6;
				if (thumb_frame.right > clip_frame.right)
					thumb_frame.right = clip_frame.right;
				DrawBitmapAsync(thumb, thumb_frame);
			}
		
			//	Test thumbnail end
			thumb = gVideoManager->GetThumbnailAsync(i.mMediaSource, right_thumb, true);
			if (thumb)
			{
				thumb_frame = clip_frame;
				thumb_frame.left = clip_frame.right - 16*6;
				if (thumb_frame.left < clip_frame.left)
					thumb_frame.left = clip_frame.left;
				DrawBitmapAsync(thumb, thumb_frame);
			}
		}
	}
	
	if (t >= 1.0f)
	{
		fAnimateDragDropTrack = nullptr;
		fAnimateDragDropClips.clear();
	}
	Invalidate();	
}

