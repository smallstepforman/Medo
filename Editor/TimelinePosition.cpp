/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Timeline Position
 */

#include <cassert>

#include <app/Application.h>
#include <app/Cursor.h>
#include <interface/Window.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/Screen.h>
#include <interface/View.h>

#include "Project.h"
#include "TimelineEdit.h"
#include "TimelinePosition.h"
#include "TimelineView.h"
#include "Theme.h"

static const float kTimelinePositionGraceX = 8.0f;	//	pixels
static const float kMarkerPosY = 42.0f;

static const uint32 kMessageContextPositionA	= 'tpca';
static const uint32 kMessageContextPositionB	= 'tpcb';

struct ZOOM_TIMING
{
	int64		frames_view;		//	number of frames across screen (eg. 30 seconds)
	int64		frames_tick;		//	number of frames per tick
	int			number_subticks;	//	number of ticks between text marks
};
static const ZOOM_TIMING kZoomTiming[] =
{
	{1*kFramesSecond,	kFramesSecond/5,	6},
	{2*kFramesSecond,	kFramesSecond/3,	10},
	{5*kFramesSecond,	kFramesSecond/3,	10},
	{10*kFramesSecond,	kFramesSecond,		10},
	{30*kFramesSecond,	5*kFramesSecond,	5},
	{60*kFramesSecond,	5*kFramesSecond,	5},
	{120*kFramesSecond,	5*kFramesSecond,	5},
	{300*kFramesSecond,	10*kFramesSecond,	2},
};
static_assert(sizeof(kZoomTiming)/sizeof(ZOOM_TIMING) == 8, "sizeof(kZoomTiming) != 8");	//	must match TimelineView::kZoomValues

/*	FUNCTION:		TimelinePosition :: TimelinePosition
	ARGS:			frame
					parent
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
TimelinePosition :: TimelinePosition(BRect frame, TimelineView *parent)
	: BView(frame, "TimelinePosition", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_TRANSPARENT_BACKGROUND)
{
	fTimelineView = parent;

	BScreen screen(B_MAIN_SCREEN_ID);
	fScreenWidth = screen.Frame().Width();

	fZoomTimingIndex = 5;	//	60 seconds
	fFramesPixel = kZoomTiming[fZoomTimingIndex].frames_view / frame.Width();
	fCurrentPosition = 1*kFramesSecond;

	fDragState = DragState::eIdle;
	fDragCursor = new BCursor(B_CURSOR_ID_GRAB);

	fKeyframeMarkers[0] = 2*kFramesSecond;
	fKeyframeMarkers[1] = 5*kFramesSecond;

	//	Caller must call InitTimelineValues
}

/*	FUNCTION:		TimelinePosition :: ~TimelinePosition
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
TimelinePosition :: ~TimelinePosition()
{
	delete fDragCursor;
	if (fDragState != DragState::eIdle)
		be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
}

/*	FUNCTION:		TimelineEdit :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Called when frame resized
*/
void TimelinePosition :: FrameResized(float width, float height)
{
	//printf("TimelinePosition::FrameResized(%f, %f)\n", width, height);
	InitTimelineLabels();
}

/*	FUNCTION:		TimelineEdit :: SetZoomFactor
	ARGS:			visible_frames
	RETURN:			n/a
	DESCRIPTION:	Called when zoom slider update
*/
void TimelinePosition :: SetZoomFactor(const int64 visible_frames)
{
	fZoomTimingIndex = 0;
	while (fZoomTimingIndex < sizeof(kZoomTiming)/sizeof(ZOOM_TIMING))
	{
		if (visible_frames == kZoomTiming[fZoomTimingIndex].frames_view)
			break;
		fZoomTimingIndex++;
	}
	assert(fZoomTimingIndex < sizeof(kZoomTiming)/sizeof(ZOOM_TIMING));

	fFramesPixel = kZoomTiming[fZoomTimingIndex].frames_view / fScreenWidth;
	InitTimelineLabels();
}

/*	FUNCTION:		TimelineEdit :: SetPosition
	ARGS:			position
	RETURN:			n/a
	DESCRIPTION:	Set timeline position
*/
void TimelinePosition :: SetPosition(const int64 position)
{
	fCurrentPosition = position;
}

/*	FUNCTION:		TimelinePosition :: MouseDown
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw timeline information
*/
void TimelinePosition :: MouseDown(BPoint point)
{
	if (!Window()->IsActive())
		Window()->Activate();

	int64 frame_idx = fTimelineView->GetLeftFrameIndex() + (point.x-4)*fFramesPixel;

	uint32 buttons;
	BMessage* msg = Window()->CurrentMessage();
	msg->FindInt32("buttons", (int32*)&buttons);
	if (buttons & B_SECONDARY_MOUSE_BUTTON)
	{
		fKeyframeMarkerEditPosition = frame_idx;
		ContextMenu(point);
		return;
	}

	if (fDragState == DragState::eShowPosition)
	{
		fDragState = DragState::eMovePosition;
		SetMouseEventMask(B_POINTER_EVENTS,	B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		fTimelineView->GetTimelineEdit()->SetTimelineScrub(true);
		return;
	}
	else if (fDragState == DragState::eShowMarkerA)
	{
		fDragState = DragState::eMoveMarkerA;
		SetMouseEventMask(B_POINTER_EVENTS,	B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		return;
	}
	else if (fDragState == DragState::eShowMarkerB)
	{
		fDragState = DragState::eMoveMarkerB;
		SetMouseEventMask(B_POINTER_EVENTS,	B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		return;
	}

	UpdateDragPosition(frame_idx);
}

/*	FUNCTION:		TimelineEdit :: ContextMenuClip
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Show context menu for clip
*/
void TimelinePosition :: ContextMenu(BPoint point)
{
	ConvertToScreen(&point);

	//	Initialise PopUp menu
	BPopUpMenu *aPopUpMenu = new BPopUpMenu("ContextMenuPosition", false, false);
	aPopUpMenu->SetAsyncAutoDestruct(true);

	BMenuItem *aMenuItem_1 = new BMenuItem("Marker A", new BMessage(kMessageContextPositionA));
	aPopUpMenu->AddItem(aMenuItem_1);
	BMenuItem *aMenuItem_2 = new BMenuItem("Market B", new BMessage(kMessageContextPositionB));
	aPopUpMenu->AddItem(aMenuItem_2);

	aPopUpMenu->SetTargetForItems(this);
	aPopUpMenu->Go(point, true /*notify*/, false /*stay open when mouse away*/, true /*async*/);

	//All BPopUpMenu items are freed when the popup is closed
}

/*	FUNCTION:		TimelinePosition :: MouseMoved
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw timeline information
*/
void TimelinePosition :: MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (point.x < 4)
		point.x = 4;
	int64 frame_idx = fTimelineView->GetLeftFrameIndex() + (point.x-4)*fFramesPixel;
	bool dragPos = ((frame_idx/fFramesPixel + kTimelinePositionGraceX >= fCurrentPosition/fFramesPixel) &&
				(frame_idx/fFramesPixel - kTimelinePositionGraceX <= fCurrentPosition/fFramesPixel));
	bool dragMarkerA = ((frame_idx/fFramesPixel + kTimelinePositionGraceX >= fKeyframeMarkers[0]/fFramesPixel) &&
				(frame_idx/fFramesPixel - kTimelinePositionGraceX <= fKeyframeMarkers[0]/fFramesPixel));
	bool dragMarkerB = ((frame_idx/fFramesPixel + kTimelinePositionGraceX >= fKeyframeMarkers[1]/fFramesPixel) &&
				(frame_idx/fFramesPixel - kTimelinePositionGraceX <= fKeyframeMarkers[1]/fFramesPixel));

	if (dragPos && dragMarkerA)
	{
		if (point.y > kMarkerPosY - 8.0f)
			dragPos = false;
		else
			dragMarkerA = false;
	}
	if (dragPos && dragMarkerB)
	{
		if (point.y > kMarkerPosY - 8.0f)
			dragPos = false;
		else
			dragMarkerB = false;
	}

	switch (fDragState)
	{
		case DragState::eIdle:
		{
			if (transit == B_INSIDE_VIEW)
			{
				if (dragPos)
				{
					be_app->SetCursor(fDragCursor);
					fDragState = DragState::eShowPosition;
					return;
				}
				else if (dragMarkerA)
				{
					be_app->SetCursor(fDragCursor);
					fDragState = DragState::eShowMarkerA;
					return;
				}
				else if (dragMarkerB)
				{
					be_app->SetCursor(fDragCursor);
					fDragState = DragState::eShowMarkerB;
					return;
				}
			}
			break;
		}

		case DragState::eMovePosition:
		case DragState::eMoveMarkerA:
		case DragState::eMoveMarkerB:
			UpdateDragPosition(frame_idx);
			return;


		case DragState::eShowPosition:
			if (transit == B_EXITED_VIEW)
				break;
			if (dragPos)
				return;
			break;

		case DragState::eShowMarkerA:
			if (transit == B_EXITED_VIEW)
				break;
			if (dragMarkerA)
				return;
			break;

		case DragState::eShowMarkerB:
			if (transit == B_EXITED_VIEW)
				break;
			if (dragMarkerB)
				return;
			break;

		default:
			assert(0);
	}

	if (fDragState != DragState::eIdle)
	{
		be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
		if ((fDragState == DragState::eMovePosition) || (fDragState == DragState::eMoveMarkerA))
			SetMouseEventMask(B_POINTER_EVENTS, 0);
		fDragState = DragState::eIdle;
	}
}

/*	FUNCTION:		TimelinePosition :: MouseUp
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw timeline information
*/
void TimelinePosition :: MouseUp(BPoint point)
{
	if ((fDragState == DragState::eMovePosition) || (fDragState == DragState::eMoveMarkerA) || (fDragState == DragState::eMoveMarkerB))
	{
		be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
		fDragState = DragState::eIdle;
		SetMouseEventMask(B_POINTER_EVENTS, 0);
		fTimelineView->GetTimelineEdit()->SetTimelineScrub(false);
	}
}

/*	FUNCTION:		TimelinePosition :: UpdateDragPosition
	ARGS:			timeline
	RETURN:			n/a
	DESCRIPTION:	Called when position indicator dragged
*/
void TimelinePosition :: UpdateDragPosition(int64 timeline)
{
	switch (fDragState)
	{
		case DragState::eShowMarkerA:
		case DragState::eShowMarkerB:
			break;
		case DragState::eMoveMarkerA:
			fKeyframeMarkers[0] = timeline;
			fTimelineView->PositionKeyframeUpdate();
			break;
		case DragState::eMoveMarkerB:
			fKeyframeMarkers[1] = timeline;
			fTimelineView->PositionKeyframeUpdate();
			break;

		default:
			fCurrentPosition = timeline;
			fTimelineView->PositionUpdate(fCurrentPosition, true);
			break;
	}
}

/*	FUNCTION:		TimelinePosition :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw timeline information
*/
void TimelinePosition :: Draw(BRect frame)
{
	BRect bound = Bounds();
	bound.left += 4;
	bound.right -= 4;

	int64 visible_left = fTimelineView->GetLeftFrameIndex();
	int64 visible_right = visible_left + bound.Width()*fFramesPixel;

	//	Bar
	BRect bar = bound;
	bar.top = 16;
	bar.bottom = 40;
	SetHighColor(Theme::GetUiColour(UiColour::eTimelinePosition));
	FillRect(bar);

	//	Limits
	SetHighColor(255, 255, 255);
	for (const auto &label : fLabels)
	{
		DrawString(label.text, BPoint(label.position, 16));
	}

	//	Tick marks major
	for (const auto &mark : fMarks)
	{
		StrokeLine(BPoint(mark.x, mark.y0), BPoint(mark.x, mark.y1));
	}

	//	end project marker
	const int64 duration = gProject->mTotalDuration;
	if ((visible_left < duration) && (visible_right > duration))
	{
		SetHighColor(0, 0, 0);
		float p = (duration-visible_left)/fFramesPixel;
		StrokeLine(BPoint(p, bar.top), BPoint(p, bar.bottom));
	}

	float pen_size = PenSize();
	float font_size = be_plain_font->Size();

	//	Keyframe markers
	for (int i=0; i < NUMBER_KEYFRAME_MARKERS; i++)
	{
		if ((fKeyframeMarkers[i] >= 0) && (fKeyframeMarkers[i] >= visible_left) && (fKeyframeMarkers[i] <= visible_right))
		{
			SetHighColor(255, 255, 0);
			SetLowColor(192, 192, 192);
			SetPenSize(2.0f);
			float px = (fKeyframeMarkers[i]-visible_left)/fFramesPixel + 4;
			const pattern p = {255, 255, 0, 0, 0, 0, 0, 0};
			StrokeLine(BPoint(px, 38), BPoint(px, bound.bottom - 2), p);
			SetPenSize(6.0f);
			StrokeTriangle(BPoint(px - 3, kMarkerPosY+8), BPoint(px + 3, kMarkerPosY+8), BPoint(px, kMarkerPosY));
			SetHighColor(128, 64, 0);
			char label[2] = {char('A' + i), 0};
			SetFontSize(12.0f);
			DrawString(label, BPoint(px - 3, kMarkerPosY+10));
		}
	}

	if ((fCurrentPosition >= visible_left) && (fCurrentPosition <= visible_right))
	{
		//	Line
		float posx = (fCurrentPosition-visible_left)/fFramesPixel + 4;
		SetHighColor(212, 32, 32);
		SetPenSize(2.0f);
		StrokeLine(BPoint(posx, 0), BPoint(posx, bound.bottom - 2));
		SetPenSize(6.0f);
		StrokeTriangle(BPoint(posx - 6, 16), BPoint(posx + 6, 16), BPoint(posx, 28));

		//	Current time
		char buffer[20];
		gProject->CreateTimeString(fCurrentPosition, buffer, true);
		char *strarray[1] = {buffer};
		int len[1] = {20};
		float width;
		GetStringWidths(strarray, len, 1, &width);

		MovePenTo(posx - 0.5f*width, 28+12);
		SetFont(be_bold_font);
		float bold_font_size = be_bold_font->Size();
		SetFontSize(0.8f*bold_font_size);
		SetHighColor({255, 255, 255, 255});
		DrawString(buffer);
		SetFontSize(bold_font_size);
		SetFont(be_plain_font);
	}
	else
	{
		SetHighColor(212, 32, 32);
		SetPenSize(6.0f);
		if (fCurrentPosition < visible_left)
			StrokeTriangle(BPoint(bar.left+2, bar.top + 0.5f*bar.Height()), BPoint(bar.left + 10, bar.top + 6), BPoint(bar.left + 10, bar.bottom - 6));
		else
			StrokeTriangle(BPoint(bar.right-2, bar.top + 0.5f*bar.Height()), BPoint(bar.right - 10, bar.top + 6), BPoint(bar.right - 10, bar.bottom - 6));
	}
	//	restore
	SetPenSize(pen_size);
	SetFontSize(font_size);
}


/*	FUNCTION:		TimelinePosition :: InitTimelineLabels
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Init timeline labels
*/
void TimelinePosition :: InitTimelineLabels()
{
	BRect frame = Bounds();
	fLabels.clear();
	fMarks.clear();

	//	Determine number of text markers
	int64 left_frame_idx = fTimelineView->GetLeftFrameIndex();
	int64 visible_frames = fFramesPixel*frame.Width();

	int64 xpos = 0;
	int64 mod = left_frame_idx%kZoomTiming[fZoomTimingIndex].frames_tick;
	int64 display_time = left_frame_idx - mod;
	const float dx = (kZoomTiming[fZoomTimingIndex].frames_tick / fFramesPixel) / kZoomTiming[fZoomTimingIndex].number_subticks;
	MARK aMark;

	//	Position left most label
	if (mod > 0)
	{
		xpos = (kZoomTiming[fZoomTimingIndex].frames_tick - mod)/fFramesPixel;
		display_time += kZoomTiming[fZoomTimingIndex].frames_tick;

		//	Tick marks left of first label
		float tx = xpos - dx;
		while (tx > 0)
		{
			aMark.x = tx;
			aMark.y0 = 22.0f;
			aMark.y1 = 26.0f;
			fMarks.push_back(aMark);
			tx -= dx;
		}
	}

	LABEL aLabel;
	while (xpos < frame.Width())
	{
		int64 round_factor = kFramesSecond/(2.0f*gProject->mResolution.frame_rate);
		gProject->CreateTimeString(display_time + round_factor, aLabel.text, fZoomTimingIndex < 3);	//	0.75 add small offset to round frame time to decimal-like frame numbers

		aLabel.width = StringWidth(aLabel.text);
		aLabel.position = xpos - (xpos > 0 ? 0.25f : 0.0f)*aLabel.width;
		fLabels.push_back(aLabel);

		aMark.x = xpos;
		aMark.y0 = 18.0f;
		aMark.y1 = 32.0f;
		fMarks.push_back(aMark);

		for (int st=1; st < kZoomTiming[fZoomTimingIndex].number_subticks; st++)
		{
			aMark.x = xpos + st*dx;
			aMark.y0 = 22.0f;
			aMark.y1 = 26.0f;
			fMarks.push_back(aMark);
		}

		xpos += kZoomTiming[fZoomTimingIndex].frames_tick / fFramesPixel;
		display_time += kZoomTiming[fZoomTimingIndex].frames_tick;
	}
}

/*	FUNCTION:		TimelinePosition :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void TimelinePosition :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMessageContextPositionA:
			fKeyframeMarkers[0] = fKeyframeMarkerEditPosition;
			if (fKeyframeMarkers[0] > fKeyframeMarkers[1])
				fKeyframeMarkers[1] = fKeyframeMarkers[0];
			fTimelineView->PositionKeyframeUpdate();
			break;

		case kMessageContextPositionB:
			fKeyframeMarkers[1] = fKeyframeMarkerEditPosition;
			if (fKeyframeMarkers[1] < fKeyframeMarkers[0])
				fKeyframeMarkers[0] = fKeyframeMarkers[1];
			fTimelineView->PositionKeyframeUpdate();
			break;

		default:
#if 0
			printf("TimelineEdit::MessageReceived()\n");
			msg->PrintToStream();
#endif
			BView::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		TimelinePosition :: GetKeyframeMarkerPosition
	ARGS:			index
	RETURN:			timeline position
	DESCRIPTION:	Get keyframe market position
*/
const int64 TimelinePosition :: GetKeyframeMarkerPosition(const uint32 index)
{
	assert(index < NUMBER_KEYFRAME_MARKERS);
	return fKeyframeMarkers[index];
}

/*	FUNCTION:		TimelinePosition :: SetKeyframeMarkerPosition
	ARGS:			index
					position
	RETURN:			n/a
	DESCRIPTION:	Set keyframe market position
*/
void TimelinePosition :: SetKeyframeMarkerPosition(const uint32 index, const int64 position)
{
	assert(index < NUMBER_KEYFRAME_MARKERS);
	fKeyframeMarkers[index] = position;
}
