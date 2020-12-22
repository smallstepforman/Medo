/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Control soiurce + clip generator
 */
 
#include <cassert>

#include <interface/View.h>
#include <interface/Bitmap.h>
#include <app/Cursor.h>
#include <app/Application.h>
#include <MediaKit.h>

#include "MediaSource.h"
#include "Project.h"
#include "CursorDefinitions.inc"
#include "MediaUtility.h"
#include "VideoManager.h"
#include "AudioManager.h"
#include "TimelineEdit.h"
#include "ControlSource.h"

static const float kClipTimelineOffsetX = 0.0f;
static const float kClipTimelineHeight = 50.0f;
static const float kClipAdjustRectHeight = 30.0f;
static const float kClipAdjustFrameSize = 4.0f;

/***********************************
	ClipTimeline
************************************/
class ClipTimeline : public BView
{
	BRect			fTimelineClipRect;
	BRect			fTimelineUserRect;
	bigtime_t		fDuration;
	MediaSource		*fMediaSource;
	BMediaTrack		*fMediaTrack;
	float			fFrameRateFactor;
	
	BCursor			*fCursor;
	bool			fCursorActive;
	
	enum DRAG_TYPE {DRAG_INACTIVE, DRAG_LEFT, DRAG_RIGHT, DRAG_DROP};
	DRAG_TYPE		fDragType, fNextDragType;
	BMessage		*fMsgDragDrop;
	
public:
/*	FUNCTION:		ClipTimeline :: ClipTimeline
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
	ClipTimeline(BRect frame)
		: BView(frame, "ClipTimeline", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		
		fCursor = new BCursor(kCursorResizeHorizontal);
		fCursorActive = false;
		fDragType = DRAG_INACTIVE;
		
		fMsgDragDrop = new BMessage(TimelineEdit::eMsgDragDropClip);
			
		fDuration = 0;
		fMediaTrack = nullptr;
		fMediaSource = nullptr;

		fTimelineClipRect = Bounds();
		fTimelineClipRect.bottom -= (kClipTimelineHeight - kClipAdjustRectHeight);
	}
	
/*	FUNCTION:		ClipTimeline :: ~ClipTimeline
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
	~ClipTimeline()
	{
		if (fCursorActive)
			be_app->SetCursor(B_HAND_CURSOR);
		delete fCursor;
		delete fMsgDragDrop;	
	}
	
/*	FUNCTION:		ClipTimeline :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
	void FrameResized(float width, float height)
	{
		float r = fTimelineClipRect.Width()/width;
		
		fTimelineClipRect = Bounds();
		fTimelineClipRect.left;
		fTimelineClipRect.right;
		fTimelineClipRect.top;
		fTimelineClipRect.bottom -= (kClipTimelineHeight - kClipAdjustRectHeight);
		
		fTimelineUserRect.left /= r;
		fTimelineUserRect.right /= r;	
	}
	
/*	FUNCTION:		ClipTimeline :: Init
	ARGS:			media_track
					media_source
	RETURN:			n/a
	DESCRIPTION:	New track
*/
	void Init(BMediaTrack *media_track, MediaSource *media_source)
	{
		assert(media_source != nullptr);
		fMediaTrack = media_track;
		fMediaSource = media_source;
		if (media_track)
			fDuration = media_track->Duration();
		else
			fDuration = 1.0f;
		fTimelineUserRect = fTimelineClipRect;

		switch (media_source->GetMediaType())
		{
			case MediaSource::MEDIA_VIDEO:
			case MediaSource::MEDIA_VIDEO_AND_AUDIO:
				fFrameRateFactor = kFramesSecond/media_source->GetVideoFrameRate();
				break;
			case MediaSource::MEDIA_AUDIO:
				fFrameRateFactor = kFramesSecond/media_source->GetAudioFrameRate();
				break;
			case MediaSource::MEDIA_PICTURE:
				fFrameRateFactor = 1.0f;
			default:
				break;
		}
	}

/*	FUNCTION:		ClipTimeline :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw clip timeline
*/	
	void Draw(BRect frame)
	{
		if (fMediaTrack == nullptr)
		{
			SetHighColor(216, 216, 216);
			FillRect(frame);

			char buffer[40];
			SetHighColor(0, 0, 0);
			sprintf(buffer, "Resolution: %d x %d\n", (int)fMediaSource->GetVideoWidth(), (int)fMediaSource->GetVideoHeight());
			font_height fh;
			be_plain_font->GetHeight(&fh);
			MovePenTo(frame.left + 0.5f*frame.Width() - 0.5f*StringWidth(buffer), frame.bottom - 0.5f*frame.Height() + 0.5f*fh.ascent);
			DrawString(buffer);
			return;
		}

		SetHighColor(216, 216, 216);
		frame.top = fTimelineClipRect.bottom;
		FillRect(frame);
		
		SetHighColor(32, 192, 32);
		FillRect(fTimelineClipRect);
		
		SetHighColor(255, 255, 0);
		SetPenSize(kClipAdjustFrameSize);
		StrokeRect(fTimelineUserRect);
		
		char buffer[40];
		SetHighColor(0, 0, 0);

		double p = (double)fTimelineUserRect.left/(double)fTimelineClipRect.Width();
		MediaDuration start(p*fDuration, fMediaSource->GetVideoTrack() ? fMediaSource->GetVideoFrameRate() : 0);
		if (fMediaSource->GetVideoTrack())
			start.PrepareVideoTimestamp(buffer);
		else
			start.PrepareAudioTimestamp(buffer);
		float buffer_width = StringWidth(buffer);
		float x_pos = buffer_width > fTimelineUserRect.left ? 0.0f : fTimelineUserRect.left - buffer_width;
		MovePenTo(x_pos, fTimelineUserRect.bottom + (kClipTimelineHeight-kClipAdjustRectHeight) - 3);
		DrawString(buffer);
		
		p = (double)fTimelineUserRect.right/(double)fTimelineClipRect.Width();
		MediaDuration end(p*fDuration, fMediaSource->GetVideoTrack() ? fMediaSource->GetVideoFrameRate() : 0);
		if (fMediaSource->GetVideoTrack())
			end.PrepareVideoTimestamp(buffer);
		else
			end.PrepareAudioTimestamp(buffer);
		buffer_width = StringWidth(buffer);
		x_pos = buffer_width < (fTimelineClipRect.right - fTimelineUserRect.right) ? fTimelineUserRect.right : fTimelineClipRect.right - buffer_width;
		MovePenTo(x_pos, fTimelineUserRect.bottom  + (kClipTimelineHeight-kClipAdjustRectHeight) - 3);
		DrawString(buffer);
	}
	
/*	FUNCTION:		ClipTimeline :: MouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Trap clip rect adjustment
*/	
	void MouseDown(BPoint point)
	{
		if (fMediaTrack == nullptr)
			return;

		if (fCursorActive)
		{
			fDragType = fNextDragType;
			SetMouseEventMask(B_POINTER_EVENTS,	B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		}
		else if (fTimelineUserRect.Contains(point))
		{
			SetMouseEventMask(B_POINTER_EVENTS, 0);
			fDragType = DRAG_DROP;
			fMsgDragDrop->MakeEmpty();
			
			int64 clip_start = ((double)fTimelineUserRect.left/(double)fTimelineClipRect.Width()) * fMediaTrack->CountFrames()*fFrameRateFactor;
			fMsgDragDrop->AddInt64("start", clip_start);
			int64 clip_end = ((double)fTimelineUserRect.right/(double)fTimelineClipRect.Width()) * fMediaTrack->CountFrames()*fFrameRateFactor;
			fMsgDragDrop->AddInt64("end", clip_end);
			fMsgDragDrop->AddPointer("source", fMediaSource);
			
			//	number of frames from fTimelineUserRect.left to mouse.x
			int64 xoffset = fMediaTrack->CountFrames()*fFrameRateFactor * (point.x - fTimelineUserRect.left)/fTimelineClipRect.Width();
			fMsgDragDrop->AddInt64("xoffset", xoffset);
			
			printf("Initiate Drag-Drop.  Data: clip_start=%ld, frame_end=%ld, xoffset=%ld\n", clip_start, clip_end, xoffset);
			DragMessage(fMsgDragDrop, fTimelineUserRect, this);
		}	
	}

/*	FUNCTION:		ClipTimeline :: MouseMoved
	ARGS:			point
					transit
					message
	RETURN:			n/a
	DESCRIPTION:	When not dragging, modify mouse cursor
					When dragging, adjust clip rect
*/
	void MouseMoved(BPoint point, uint32 transit, const BMessage *message)
	{
		switch (transit)
		{
			case B_ENTERED_VIEW:
				break;
				
			case B_EXITED_VIEW:
				if (fCursorActive && (fDragType == DRAG_INACTIVE))
				{
					be_app->SetCursor(B_HAND_CURSOR);
					fCursorActive = false;
				}
				break;
				
			case B_INSIDE_VIEW:
				if (fDragType == DRAG_INACTIVE)
				{
					if ((point.x > fTimelineUserRect.left - 2*kClipAdjustFrameSize) && (point.x < fTimelineUserRect.left + 2*kClipAdjustFrameSize))
					{
						if (!fCursorActive)
						{
							be_app->SetCursor(fCursor);
							fCursorActive = true;
							fNextDragType = DRAG_LEFT;	
						}	
					}
					else if ((point.x > fTimelineUserRect.right - 2*kClipAdjustFrameSize) && (point.x < fTimelineUserRect.right + 2*kClipAdjustFrameSize))
					{
						if (!fCursorActive)
						{
							be_app->SetCursor(fCursor);
							fCursorActive = true;
							fNextDragType = DRAG_RIGHT;	
						}	
					}
					else if (fCursorActive)
					{
						be_app->SetCursor(B_HAND_CURSOR);
						fCursorActive = false;
					}
					
					break;
				}
				//	else fall through
				
			case B_OUTSIDE_VIEW:
				if (fCursorActive)
				{
					if (fDragType == DRAG_LEFT)
					{
						if (point.x < fTimelineUserRect.right)
						{
							if (point.x < fTimelineClipRect.left)
								point.x = fTimelineClipRect.left;
							fTimelineUserRect.left = point.x;
							
							int64 clip_start = ((double)fTimelineUserRect.left/(double)fTimelineClipRect.Width()) * fMediaTrack->CountFrames()*fFrameRateFactor;
							((ControlSource *)Parent())->ShowPreview(clip_start);
						}
						Invalidate();	
					}
					else if (fDragType == DRAG_RIGHT)
					{
						if (point.x > fTimelineUserRect.left)
						{
							if (point.x > fTimelineClipRect.right)
								point.x = fTimelineClipRect.right;
							fTimelineUserRect.right = point.x;
							
							int64 clip_end = ((double)fTimelineUserRect.right/(double)fTimelineClipRect.Width()) * fMediaTrack->CountFrames()*fFrameRateFactor;
							((ControlSource *)Parent())->ShowPreview(clip_end);
						}
						Invalidate();	
					}
				} 
				break;
			default:
				assert(0);				
		}	
	}

/*	FUNCTION:		ClipTimeline :: MouseUp
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Restore mouse cursor when
*/	
	void MouseUp(BPoint point)
	{
		if (fDragType != DRAG_INACTIVE)
		{
			be_app->SetCursor(B_HAND_CURSOR);
			fCursorActive = false;
			fDragType = DRAG_INACTIVE;
		}	
	}
	
};


/***********************************
	ControlSource
************************************/

/*	FUNCTION:		ControlSource :: ControlSource
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ControlSource :: ControlSource(BRect frame)
	: BView(frame, "ControlSource", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fBitmap(nullptr)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	
	BRect timeline_frame = Bounds();
	timeline_frame.left += kClipTimelineOffsetX;
	timeline_frame.top = (timeline_frame.bottom - kClipTimelineHeight);
	fClipTimeline = new ClipTimeline(timeline_frame);
}

/*	FUNCTION:		ControlSource :: ~ControlSource
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ControlSource :: ~ControlSource()
{
}

/*	FUNCTION:		ControlSource :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void ControlSource :: FrameResized(float width, float height)
{
	fClipTimeline->ResizeTo(width - kClipTimelineOffsetX, kClipTimelineHeight);
	fClipTimeline->MoveTo(kClipTimelineOffsetX, height - kClipTimelineHeight);		
}

/*	FUNCTION:		ControlSource :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw view
*/
void ControlSource :: Draw(BRect frame)
{
	SetHighColor(0x30, 0x30, 0x30);
	if (fBitmap)
	{
		frame = Bounds();
		frame.bottom -= kClipTimelineHeight;
		BRect bitmap_rect = fBitmap->Bounds();
		float bitmap_w = bitmap_rect.Width();
		float bitmap_h = bitmap_rect.Height();
		
		float ratio_x = frame.Width() / bitmap_w; 
		float ratio_y = frame.Height() / bitmap_h;
		float r = (ratio_x < ratio_y) ? ratio_x : ratio_y;
		
		bitmap_rect.left = 0.5f*(frame.Width() - (bitmap_w*r));
		bitmap_rect.right = bitmap_rect.left + bitmap_w*r;
		bitmap_rect.top = 0.5f*(frame.Height() - (bitmap_h*r));
		bitmap_rect.bottom = bitmap_rect.top + bitmap_h*r;
		
		DrawBitmapAsync(fBitmap, bitmap_rect);
		
		//	Fill remainder
		BRect fill_rect = frame;
		fill_rect.bottom = bitmap_rect.top;
		FillRect(fill_rect);
		fill_rect.top = bitmap_rect.bottom;
		fill_rect.bottom = frame.bottom;
		FillRect(fill_rect);
		fill_rect.right = bitmap_rect.left;
		fill_rect.top = bitmap_rect.top;
		fill_rect.bottom = bitmap_rect.bottom;
		FillRect(fill_rect);
		fill_rect.left = bitmap_rect.right;
		fill_rect.right = frame.right;
		FillRect(fill_rect);
		
		//	left of clip selection
		fill_rect.left = 0;
		fill_rect.top = frame.bottom;
		fill_rect.right = kClipTimelineOffsetX;
		fill_rect.bottom = fill_rect.top + kClipTimelineHeight;
		FillRect(fill_rect);
		
		//	Clip selection
		fClipTimeline->Draw(frame);
	}
	else
		FillRect(frame);	
}

/*	FUNCTION:		ControlSource :: SetMediaSource
	ARGS:			media
	RETURN:			n/a
	DESCRIPTION:	Set media source
*/
void ControlSource :: SetMediaSource(MediaSource *media)
{
	if (fClipTimeline->Parent() == this)
		RemoveChild(fClipTimeline);
	
	fMediaSource = media;
	
	switch (fMediaSource->GetMediaType())
	{
		case MediaSource::MEDIA_VIDEO:
		case MediaSource::MEDIA_VIDEO_AND_AUDIO:
		{
			assert(fMediaSource->GetVideoTrack() != nullptr);
			fBitmap = gVideoManager->GetFrameBitmap(fMediaSource, 0);
			if (fClipTimeline->Parent() == nullptr)
				AddChild(fClipTimeline);
			fClipTimeline->Init(fMediaSource->GetVideoTrack(), fMediaSource);
			break;
		}
		case MediaSource::MEDIA_AUDIO:
		{
			assert(fMediaSource->GetAudioTrack() != nullptr);
			BRect frame = Bounds();
			fBitmap = gAudioManager->GetBitmapAsync(fMediaSource, 0, fMediaSource->GetAudioNumberSamples() * kFramesSecond/fMediaSource->GetAudioFrameRate(), frame.Width(), frame.Height());
			if (fClipTimeline->Parent() == nullptr)
				AddChild(fClipTimeline);
			fClipTimeline->Init(fMediaSource->GetAudioTrack(), fMediaSource);
			break;
		}
		case MediaSource::MEDIA_PICTURE:
		{
			fBitmap = fMediaSource->GetBitmap();
			if (fClipTimeline->Parent() == nullptr)
				AddChild(fClipTimeline);
			fClipTimeline->Init(nullptr, fMediaSource);
			break;
		}
		default:
			assert(0);
	}
	
	if (fBitmap)
		Invalidate();
}

/*	FUNCTION:		ControlSource :: ShowPreview
	ARGS:			frame_idx
	RETURN:			n/a
	DESCRIPTION:	Show frame preview
*/
void ControlSource :: ShowPreview(int64 frame_idx)
{
	//printf("ShowPreview(%ld)\n", frame_idx);
	if (fMediaSource->GetVideoTrack())
	{
		fBitmap = gVideoManager->GetFrameBitmap(fMediaSource, frame_idx);
	}
	else if (fMediaSource->GetAudioTrack())
	{
		BRect frame = Bounds();
		fBitmap = gAudioManager->GetBitmapAsync(fMediaSource, 0, fMediaSource->GetAudioNumberSamples() * kFramesSecond/fMediaSource->GetAudioFrameRate(), frame.Width(), frame.Height());
	}
	
	Invalidate();
}

