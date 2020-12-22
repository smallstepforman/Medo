/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Timeline view
 */

#include <cstdio>
#include <cassert>

#include <interface/View.h>
#include <interface/Slider.h>
#include <interface/ScrollView.h>
#include <interface/Bitmap.h>
#include <private/interface/ToolTip.h>
#include <translation/TranslationUtils.h>

#include "Gui/BitmapCheckbox.h"
#include "Gui/BitmapButton.h"

#include "AudioManager.h"
#include "EffectsWindow.h"
#include "Language.h"
#include "MedoWindow.h"
#include "OutputView.h"
#include "Project.h"
#include "RenderActor.h"
#include "Theme.h"
#include "TimelineEdit.h"
#include "TimelinePlayer.h"
#include "TimelinePosition.h"
#include "TimelineView.h"
#include "VideoManager.h"

static const uint32 kMessageZoomSlider			= 'mtzs';
static const uint32 kMessageCheckboxVideo		= 'mtcv';
static const uint32 kMessageCheckboxAudio		= 'mtca';
static const uint32 kMessageButtonPlay			= 'mtb0';
static const uint32 kMessageButtonPlayAb		= 'mtb1';
static const uint32 kMessageButtonFrameNext		= 'mtb2';
static const uint32 kMessageButtonFramePrev		= 'mtb3';

static const float kStatusViewWidth = 200.0f;	//	must match width in MedoWindow.cpp
static const float kTimelineOffsetX = 80.0f;
static const float kTimelineOffsetY = 64.0f;
static const float kZoomSliderIconWidth = 18.0f;
static const float kZoomSliderWidth = 140.0f;

struct ZoomValue
{
	int64			value;
	const char		*label;
};
static const ZoomValue kZoomValues[] = {
	{1*kFramesSecond,	"1 sec"},
	{2*kFramesSecond,	"2 secs"},
	{5*kFramesSecond,	"5 secs"},
	{10*kFramesSecond,	"10 secs"},
	{30*kFramesSecond,	"30 sec"},
	{60*kFramesSecond,	"1 min"},
	{120*kFramesSecond,	"2 min"},
	{300*kFramesSecond,	"5 min"},
};	//	frames to display in view
static_assert(sizeof(kZoomValues)/sizeof(ZoomValue) == 8, "sizeof(kZoomValues) != 8");	//	must match TimelinePosition::kZoomTiming
static const uint32 kDefaultZoomIndex = 5;	//	60 seconds

//================================
class HorizontalScrollView : public BView
{
	TimelineView	*fParent;
public:
	HorizontalScrollView(BRect frame, TimelineView *parent)
	: BView(frame, "HorizontalScrollView", B_FOLLOW_NONE, 0), fParent(parent) { }
	void	ScrollTo(BPoint point) {fParent->ScrollToHorizontal(point.x);}
};

//================================
class VerticalScrollView : public BView
{
	TimelineView	*fParent;
public:
	VerticalScrollView(BRect frame, TimelineView *parent)
	: BView(frame, "VerticalScrollView", B_FOLLOW_NONE, 0), fParent(parent) { }
	void	ScrollTo(BPoint point) {fParent->ScrollToVertical(point.y);}
};

//=================================
class TimelineControlView : public BView
{
	std::vector<float>	fTrackOffsets;
	float				fScrollYOffset;
public:
	TimelineControlView(BRect frame)
	: BView(frame, "TimelineControlView", B_FOLLOW_NONE, B_WILL_DRAW | B_TRANSPARENT_BACKGROUND)
	{
		fScrollYOffset = 0.0f;
	}
	void	SetTrackOffsets(const std::vector<float> &offsets, float scroll_y_offset)
	{
		fTrackOffsets = offsets;
		fScrollYOffset = scroll_y_offset;
	}
	void Draw(BRect frame) override
	{
		assert(gProject->mTimelineTracks.size() == fTrackOffsets.size());
		SetHighColor(Theme::GetUiColour(UiColour::eListText));
		for (int i=0; i < gProject->mTimelineTracks.size(); i++)
		{
			MovePenTo(4, fTrackOffsets[i] + 32+28 - fScrollYOffset);
			DrawString(gProject->mTimelineTracks[i]->mName);
		}
	}
};

/*	FUNCTION:		TimelineView :: TimelineView
	ARGS:			frame
					view
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
TimelineView :: TimelineView(BRect frame, MedoWindow *parent)
	: BView(frame, "TimelineView", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fParent(parent), fLeftFrameIndex(0), fEditViewScrollOffsetY(0), fCurrentFrame(0), fZoomSliderValue(kDefaultZoomIndex)
{
	SetViewColor(Theme::GetUiColour(UiColour::eTimelineView));

	frame = Bounds();
	fViewWidth = frame.Width();
	
	//	Timeline edit
	BRect frame_edit(frame.left + kTimelineOffsetX, frame.top + kTimelineOffsetY, frame.right - (B_V_SCROLL_BAR_WIDTH + 4), frame.bottom - (B_H_SCROLL_BAR_HEIGHT + 4));
	fTimelineEdit = new TimelineEdit(frame_edit, this);
	fTimelineEdit->SetViewColor(B_TRANSPARENT_COLOR);
	AddChild(fTimelineEdit);
	fTimelineEdit->SetZoomFactor(kZoomValues[fZoomSliderValue].value);

	//	TimelinePosition
	frame_edit.top -= kTimelineOffsetY;
	frame_edit.left -= 4;
	frame_edit.right += 4;
	frame_edit.bottom += 4;
	fTimelinePosition = new TimelinePosition(frame_edit, this);
	fTimelinePosition->SetViewColor(B_TRANSPARENT_COLOR);
	AddChild(fTimelinePosition);
	fTimelinePosition->SetZoomFactor(kZoomValues[fZoomSliderValue].value);

	//	Zoom slider
	const BRect zoom_slider_rect(frame.right - (kZoomSliderWidth + B_V_SCROLL_BAR_WIDTH),
								frame.bottom - B_H_SCROLL_BAR_HEIGHT-2,
								frame.right - B_V_SCROLL_BAR_WIDTH,
								frame.bottom);
	fZoomSlider = new BSlider(zoom_slider_rect, "ZoomSlider", nullptr, nullptr, 0, sizeof(kZoomValues)/sizeof(ZoomValue) - 1);
	fZoomSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fZoomSlider->SetHashMarkCount(sizeof(kZoomValues)/sizeof(ZoomValue));
	fZoomSlider->SetModificationMessage(fMsgZoomSlider = new BMessage(kMessageZoomSlider));
	fZoomSlider->SetValue(fZoomSliderValue);
	fZoomSlider->SetToolTip(kZoomValues[fZoomSliderValue].label);
	AddChild(fZoomSlider);
	
	//	Zoom icon
	fZoomIcon = BTranslationUtils::GetBitmap("Resources/icon_zoom.png");
	
	//	Horizontal scroll bar
	const BRect horizontal_view_rect(frame.left + kStatusViewWidth,
									frame.bottom - (B_H_SCROLL_BAR_HEIGHT + 4),
									frame.right - (kZoomSliderIconWidth + kZoomSliderWidth + B_V_SCROLL_BAR_WIDTH + 6),
									frame.bottom);
	HorizontalScrollView *horizontal_view = new HorizontalScrollView(horizontal_view_rect, this);
	fHorizontalScrollView = new BScrollView("TimlineViewHorizontalScrollView", horizontal_view, B_FOLLOW_NONE, 0, true, false);
	fHorizontalScrollView->ScrollBar(B_HORIZONTAL)->SetRange(0.0f, 0.0f);
	AddChild(fHorizontalScrollView);
	
	//	Vertical scroll bar
	const BRect vertical_view_rect(frame.right - (B_V_SCROLL_BAR_WIDTH + 4),
									frame.top + kTimelineOffsetY,
									frame.right,
									frame.bottom - (B_H_SCROLL_BAR_HEIGHT + 4));
	VerticalScrollView *vertical_view = new VerticalScrollView(horizontal_view_rect, this);
	fVerticalScrollView = new BScrollView("TimlineViewVerticalScrollView", vertical_view, B_FOLLOW_NONE, 0, false, true);
	fVerticalScrollView->ScrollBar(B_VERTICAL)->SetRange(0.0f, 0.0f);
	AddChild(fVerticalScrollView);

	fControlView = new TimelineControlView(BRect(0, kTimelineOffsetY, kTimelineOffsetX, frame.bottom));
	fControlView->SetViewColor(B_TRANSPARENT_COLOR);
	AddChild(fControlView);

	//	Play buttons
	fButtonPlay = new BitmapCheckbox(BRect(0, 0, 32, 32), "play",
									 BTranslationUtils::GetBitmap("Resources/icon_play.png"),
									 BTranslationUtils::GetBitmap("Resources/icon_pause.png"),
									 new BMessage(kMessageButtonPlay));
	fButtonPlay->SetToolTip(GetText(TXT_TIMELINE_TOOLTIP_PLAY));
	fButtonPlayAb = new BitmapCheckbox(BRect(32+4, 0, 64+4, 32), "playAB",
									 BTranslationUtils::GetBitmap("Resources/icon_play_ab.png"),
									 BTranslationUtils::GetBitmap("Resources/icon_pause_ab.png"),
									 new BMessage(kMessageButtonPlayAb));
	fButtonPlayAb->SetToolTip(GetText(TXT_TIMELINE_TOOLTIP_PLAY_AB));

	fButtonFrameNext = new BitmapButton(BRect(32+4, 32+4, 64+4, 64+4), "frame_next",
									 BTranslationUtils::GetBitmap("Resources/icon_skip_right.png"),
									 BTranslationUtils::GetBitmap("Resources/icon_skip_right_down.png"),
									 new BMessage(kMessageButtonFrameNext));
	fButtonFrameNext->SetToolTip(GetText(TXT_TIMELINE_TOOLTIP_NEXT_FRAME));
	fButtonFramePrev = new BitmapButton(BRect(0, 32+4, 32, 64+4), "frame_prev",
									 BTranslationUtils::GetBitmap("Resources/icon_skip_left.png"),
									 BTranslationUtils::GetBitmap("Resources/icon_skip_left_down.png"),
									 new BMessage(kMessageButtonFramePrev));
	fButtonFramePrev->SetToolTip(GetText(TXT_TIMELINE_TOOLTIP_PREVIOUS_FRAME));
	AddChild(fButtonPlay);
	AddChild(fButtonPlayAb);
	AddChild(fButtonFrameNext);
	AddChild(fButtonFramePrev);

	fTimelinePlayer = new TimelinePlayer(parent);
	fPlayMode = PLAY_OFF;

	UpdateControlView();
}

/*	FUNCTION:		TimelineView :: ~TimelineView
	ARGS:			N/A
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
TimelineView :: ~TimelineView()
{
	if (fTimelinePlayer->IsPlaying())
	{
		fTimelinePlayer->Async(&TimelinePlayer::AsyncStop, fTimelinePlayer);
		usleep(1000*1000);
	}
	delete fTimelinePlayer;
	delete fZoomIcon;
}

/*	FUNCTION:		TimelineView :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when attached to window
*/
void TimelineView :: AttachedToWindow()
{
	fZoomSlider->SetTarget(this, (BLooper *)Window());
	fButtonPlay->SetTarget(this, (BLooper *)Window());
	fButtonPlayAb->SetTarget(this, (BLooper *)Window());
	fButtonFrameNext->SetTarget(this, (BLooper *)Window());
	fButtonFramePrev->SetTarget(this, (BLooper *)Window());
	FrameResized(Bounds().Width(), Bounds().Height());

	for (auto &i : fTrackSettings)
	{
		i.visual->SetTarget(this, (BLooper *)Window());
		i.audio->SetTarget(this, (BLooper *)Window());
	}
}

void TimelineView :: MouseDown(BPoint point)
{
	if (!Window()->IsActive())
		Window()->Activate();
	BView::MouseDown(point);
}

/*	FUNCTION:		TimelineView :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process messages
*/
void TimelineView :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMessageZoomSlider:
			MessageZoomSlider(msg);
			break;

		case kMessageButtonPlay:
		{
			if (fPlayMode != PLAY_ALL)
			{
				fPlayMode = PLAY_ALL;
				fTimelinePlayer->Async(&TimelinePlayer::AsyncPlay, fTimelinePlayer, fCurrentFrame, -1, false);
				fButtonPlay->SetState(true);
				fButtonPlayAb->SetState(false);
			}
			else
				PlayComplete();
			break;
		}
		case kMessageButtonPlayAb:
		{
			if (fPlayMode != PLAY_AB)
			{
				fPlayMode = PLAY_AB;
				int64 posA = fTimelinePosition->GetKeyframeMarkerPosition(0);
				int64 posB = fTimelinePosition->GetKeyframeMarkerPosition(1);
				if ((fCurrentFrame < posA) || (fCurrentFrame >= posB))
					fCurrentFrame = posA;
				fTimelinePlayer->Async(&TimelinePlayer::AsyncPlay, fTimelinePlayer, posA, posB, true);
				fButtonPlayAb->SetState(true);
				fButtonPlay->SetState(false);
			}
			else
				PlayComplete();
			break;
		}

		case kMessageButtonFrameNext:
		{
			int64 when = fCurrentFrame + kFramesSecond/gProject->mResolution.frame_rate;
			int64 max_time = gProject->mTotalDuration;
			if (when > max_time)
				when = max_time;
			fTimelinePosition->SetPosition(when);
			PositionUpdate(when, true);
			break;
		}
		case kMessageButtonFramePrev:
		{
			int64 when = fCurrentFrame - kFramesSecond/gProject->mResolution.frame_rate;
			if (when < 0)
				when = 0;
			fTimelinePosition->SetPosition(when);
			PositionUpdate(when, true);
			break;
		}

		case B_MOUSE_WHEEL_CHANGED:
		{
			bool option_modifier = ((MedoWindow *)Window())->GetKeyModifiers() & B_COMMAND_KEY;
			if (option_modifier)
			{
				((MedoWindow *)Window())->GetOutputView()->MessageReceived(msg);
				break;
			}

			float delta_y;
			if (msg->FindFloat("be:wheel_delta_y", &delta_y) == B_OK)
			{
				int64 when = fCurrentFrame + kFramesSecond/gProject->mResolution.frame_rate*delta_y*-1.0f;
				int64 max_time = gProject->mTotalDuration;
				if (when > max_time)	when = max_time;
				if (when < 0)			when = 0;
				fTimelinePosition->SetPosition(when);
				PositionUpdate(when, true);
			}
			break;
		}

		case B_MOUSE_IDLE:		//	BViews with tooltips send this 1s after mouse idle
		case B_MOUSE_MOVED:
			break;

		case kMessageCheckboxVideo:
		{
			uint32 index;
			if (msg->FindUInt32("index", &index) == B_OK)
			{
				printf("kMessageCheckboxVideo(%d)\n", index);
				assert(index < gProject->mTimelineTracks.size());
				gProject->mTimelineTracks[index]->mVideoEnabled = fTrackSettings[index].visual->Value();
				gProject->InvalidatePreview();
			}
			break;
		}
		case kMessageCheckboxAudio:
		{
			uint32 index;
			if (msg->FindUInt32("index", &index) == B_OK)
			{
				printf("kMessageCheckboxAudio(%d)\n", index);
				assert(index < gProject->mTimelineTracks.size());
				gProject->mTimelineTracks[index]->mAudioEnabled = fTrackSettings[index].audio->Value();
			}
			break;
		}

		default:
#if 0
			printf("TimelineView::MessageReceived()\n");
			msg->PrintToStream();
#endif
			BView::MessageReceived(msg);
	}
}

/*	FUNCTION:		TimelineView :: PlayComplete
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Make sure all buttons off
*/
void TimelineView :: PlayComplete()
{
	fTimelinePlayer->Async(&TimelinePlayer::AsyncStop, fTimelinePlayer);
	fPlayMode = PLAY_OFF;
	fButtonPlay->SetState(false);
	fButtonPlayAb->SetState(false);
}

/*	FUNCTION:		TimelineView :: KeyDownMessage
	ARGS:			msg
	RETURN:			true if processed
	DESCRIPTION:	Called by MedoWindow, intercept keydown messages
*/
bool TimelineView :: KeyDownMessage(BMessage *msg)
{
	const char *bytes;
	msg->FindString("bytes",&bytes);
	switch (bytes[0])
	{
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		{
			int64 when = fCurrentFrame + kFramesSecond/gProject->mResolution.frame_rate*(bytes[0]==B_LEFT_ARROW?-1:1);
			int64 max_time = gProject->mTotalDuration;
			if (when > max_time)
				when = max_time;
			fTimelinePosition->SetPosition(when);
			PositionUpdate(when, true);
			return true;
		}
		case '-':
			((MedoWindow *)Window())->GetOutputView()->Zoom(false);
			return true;
		case '=':
		case '+':
			((MedoWindow *)Window())->GetOutputView()->Zoom(true);
			return true;

		default:
			return fTimelineEdit->KeyDownMessage(msg);

	}
}

/*	FUNCTION:		TimelineView :: SetSession
	ARGS:			session
	RETURN:			n/a
	DESCRIPTION:	Called when project loaded
*/
void TimelineView :: SetSession(const SESSION &session)
{
	//printf("SetSession() h=%f, v=%f, current=%ld\n", session.horizontal_scroll, session.vertical_scroll, session.current_frame);

	fZoomSliderValue = session.zoom_index;
	fZoomSlider->SetValue(fZoomSliderValue);

	fTimelineEdit->SetZoomFactor(kZoomValues[fZoomSliderValue].value);
	fTimelinePosition->SetZoomFactor(kZoomValues[fZoomSliderValue].value);
	fZoomSlider->SetToolTip(kZoomValues[fZoomSliderValue].label);

	fCurrentFrame = session.current_frame;
	fTimelinePosition->SetPosition(fCurrentFrame);

	fTimelinePosition->SetKeyframeMarkerPosition(0, session.marker_a);
	fTimelinePosition->SetKeyframeMarkerPosition(1, session.marker_b);
	
	InvalidateItems(-1);

	fHorizontalScrollView->ScrollBar(B_HORIZONTAL)->SetValue(session.horizontal_scroll*100);
	fVerticalScrollView->ScrollBar(B_VERTICAL)->SetValue(session.vertical_scroll*100);
}

/*	FUNCTION:		TimelineView :: GetSession
	ARGS:			none
	RETURN:			current session
	DESCRIPTION:	Called when project saved
*/
const TimelineView::SESSION TimelineView :: GetSession()
{
	SESSION session;
	session.zoom_index = fZoomSliderValue;
	session.horizontal_scroll = fHorizontalScrollView->ScrollBar(B_HORIZONTAL)->Value();
	session.vertical_scroll = fVerticalScrollView->ScrollBar(B_VERTICAL)->Value();
	session.current_frame = fCurrentFrame;
	session.marker_a = fTimelinePosition->GetKeyframeMarkerPosition(0);
	session.marker_b = fTimelinePosition->GetKeyframeMarkerPosition(1);
	return session;
}

/*	FUNCTION:		TimelineView :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void TimelineView :: Draw(BRect frame)
{
	BRect bound = Bounds();
	DrawBitmapAsync(fZoomIcon, BPoint(bound.right - (kZoomSliderWidth + (kZoomSliderIconWidth - 2) + B_V_SCROLL_BAR_WIDTH), bound.bottom - B_H_SCROLL_BAR_HEIGHT));
}

/*	FUNCTION:		TimelineView :: GetTimelineEditWidth
	ARGS:			none
	RETURN:			fTimelineEdit view width
	DESCRIPTION:	Get timeline edit view width
*/
float TimelineView :: GetTimelineEditWidth() const
{
	return fViewWidth - (kTimelineOffsetX + B_V_SCROLL_BAR_WIDTH + 4);
}

/*	FUNCTION:		TimelineView :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void TimelineView :: FrameResized(float width, float height)
{
	fViewWidth = width;

	fZoomSlider->MoveTo(width - (kZoomSliderWidth + B_V_SCROLL_BAR_WIDTH), height - B_H_SCROLL_BAR_HEIGHT - 2);
	
	fTimelineEdit->ResizeTo(GetTimelineEditWidth(), height - (kTimelineOffsetY + B_H_SCROLL_BAR_HEIGHT + 4));
	fTimelineEdit->MoveTo(kTimelineOffsetX, kTimelineOffsetY);


	//fTimelinePosition
	
	fHorizontalScrollView->ResizeTo(width - (kStatusViewWidth + kZoomSliderIconWidth + kZoomSliderWidth + B_V_SCROLL_BAR_WIDTH + 6), B_H_SCROLL_BAR_HEIGHT + 6);
	fHorizontalScrollView->MoveTo(kStatusViewWidth, height - (B_H_SCROLL_BAR_HEIGHT+4));
	
	fVerticalScrollView->ResizeTo(B_V_SCROLL_BAR_WIDTH+6, height - (kTimelineOffsetY + B_H_SCROLL_BAR_HEIGHT + 2));
	fVerticalScrollView->MoveTo(width - (B_V_SCROLL_BAR_WIDTH+4), kTimelineOffsetY);
	
	InvalidateItems(INVALIDATE_VERTICAL_SLIDER | INVALIDATE_HORIZONTAL_SLIDER | INVALIDATE_EDIT_TRACKS | INVALIDATE_POSITION_SLIDER);

	fControlView->ResizeTo(kTimelineOffsetX, height - kTimelineOffsetY);
	fControlView->MoveTo(0, kTimelineOffsetY);
}

/*	FUNCTION:		TimelineView :: InvalidateItems
	ARGS:			mask
	RETURN:			n/a
	DESCRIPTION:	Invalidate view items
*/
void TimelineView :: InvalidateItems(uint32 mask)
{
	if (mask & INVALIDATE_VIEW)
		Invalidate();
	if (mask & INVALIDATE_VERTICAL_SLIDER)
		UpdateVerticalScrollBar();
	if (mask & INVALIDATE_HORIZONTAL_SLIDER)
		UpdateHorizontalScrollBar();
	if (mask & INVALIDATE_EDIT_TRACKS)
		fTimelineEdit->Invalidate();
	if (mask & INVALIDATE_POSITION_SLIDER)
		fTimelinePosition->Invalidate();
	if (mask & INVALIDATE_CONTROL_VIEW)
		UpdateControlView();
}

/*	FUNCTION:		TimelineView :: PositionUpdate
	ARGS:			position
					generate_output_preview
	RETURN:			n/a
	DESCRIPTION:	Position slider updated
*/
void TimelineView :: PositionUpdate(const int64 position, const bool generate_output_preview)
{
	int64 old_frame = fCurrentFrame;
	fCurrentFrame = position;

	//	Switch parent to output preview
	fParent->SetActiveControl(MedoWindow::CONTROL_OUTPUT);

	//	Invalidate preview
	if (generate_output_preview)
	{
		gRenderActor->AsyncPriority(&RenderActor::AsyncPrepareFrame, gRenderActor, fCurrentFrame);

		//	If playing, update current position
		if ((fPlayMode == PLAY_ALL) || (fPlayMode == PLAY_AB))
			fTimelinePlayer->Async(&TimelinePlayer::AsyncSetFrame, fTimelinePlayer, fCurrentFrame);
		else
			fTimelinePosition->SetPosition(position);
	}
	else
		fTimelinePosition->SetPosition(position);

	//	Preview audio
	int64 next_frame = fCurrentFrame + kFramesSecond/gProject->mResolution.frame_rate;
	gAudioManager->PlayPreview(fCurrentFrame, next_frame);

	//	Invalidate children
	InvalidateItems(INVALIDATE_POSITION_SLIDER | INVALIDATE_EDIT_TRACKS);
}

/*	FUNCTION:		TimelineView :: PositionKeyframeUpdate
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called by TimelinePosition when keyframe position update
*/
void TimelineView :: PositionKeyframeUpdate()
{
	if (fPlayMode == PLAY_AB)
	{
		int64 posA = fTimelinePosition->GetKeyframeMarkerPosition(0);
		int64 posB = fTimelinePosition->GetKeyframeMarkerPosition(1);
		if ((fCurrentFrame < posA) || (fCurrentFrame >= posB))
			fCurrentFrame = posA;
		fTimelinePlayer->Async(&TimelinePlayer::AsyncPlay, fTimelinePlayer, posA, posB, true);
		fTimelinePlayer->Async(&TimelinePlayer::AsyncSetFrame, fTimelinePlayer, fCurrentFrame);
	}
	InvalidateItems(INVALIDATE_POSITION_SLIDER);
}

/*	FUNCTION:		TimelineView :: MessageZoomSlider
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Zoom slider updated
*/
void TimelineView :: MessageZoomSlider(BMessage *msg)
{
	//	Get value (plus sanity checks)
	assert(msg->FindInt32("be:value", &fZoomSliderValue) == B_OK);
	assert((fZoomSliderValue >= 0) && (fZoomSliderValue < sizeof(kZoomValues)/sizeof(ZoomValue)));
	
	fTimelineEdit->SetZoomFactor(kZoomValues[fZoomSliderValue].value);
	fTimelinePosition->SetZoomFactor(kZoomValues[fZoomSliderValue].value);

	const int64 total_frames = gProject->mTotalDuration + kFramesSecond;	//	add 1s to right (space for append)
	const int64 visible_frames = fViewWidth * fTimelineEdit->GetFramesPixel();
	double pos = double(fTimelinePosition->GetCurrrentPosition() - 0.5*visible_frames)/(double)total_frames;
	if (pos < 0.0)
		pos = 0.0;
	fLeftFrameIndex = pos*total_frames;
	fTimelineEdit->SetScrollViewOrigin(fLeftFrameIndex, fEditViewScrollOffsetY);
	fHorizontalScrollView->ScrollBar(B_HORIZONTAL)->SetValue(pos*100.0f + 1);
	
	InvalidateItems(INVALIDATE_EDIT_TRACKS | INVALIDATE_HORIZONTAL_SLIDER | INVALIDATE_POSITION_SLIDER);

	fZoomSlider->SetToolTip(kZoomValues[fZoomSliderValue].label);
	fZoomSlider->ToolTip()->SetSticky(true);
	fZoomSlider->ShowToolTip(fZoomSlider->ToolTip());
}

/*	FUNCTION:		TimelineView :: UpdateHorizontalScrollBar
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Adjust size / pos of horizontal scroll bar
*/
void TimelineView :: UpdateHorizontalScrollBar()
{
	BScrollBar *bar = fHorizontalScrollView->ScrollBar(B_HORIZONTAL); 
	const int64 total_frames = gProject->mTotalDuration + kFramesSecond;	//	add 1s to right (space for append)
	const int64 visible_frames = fViewWidth * fTimelineEdit->GetFramesPixel();
	float ratio = (float)visible_frames/(float)total_frames;
	
	if (ratio < 1.0f)
	{
		bar->SetRange(0.0f, 101.0f);	//	values will be from 1.0 to 101.0
		bar->SetProportion(ratio);
	}
	else
	{
		fLeftFrameIndex = 0;
		fTimelineEdit->SetScrollViewOrigin(fLeftFrameIndex, fEditViewScrollOffsetY);
		bar->SetRange(0.0f, 0.0f);
	}
}

/*	FUNCTION:		TimelineView :: UpdateVerticalScrollBar
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Adjust size / pos of vertical scroll bar
*/
void TimelineView :: UpdateVerticalScrollBar()
{
	BScrollBar *bar = fVerticalScrollView->ScrollBar(B_VERTICAL);
	std::vector<float> track_offsets;
	fTimelineEdit->GetTrackOffsets(track_offsets);
	float total_height = 0.0f;
	for (auto &t : track_offsets)
		total_height += t;
	const float kTrackExtraY = 9*6 + 64;	//	kTimelineTrackHeight + kTimelineTrackDeltaY (from TimelineEdit.cpp
	float ratio = fVerticalScrollView->Frame().Height() / (total_height + kTrackExtraY);

	if (ratio < 1.0f)
	{
		bar->SetRange(0.0f, 101.0f);
		bar->SetProportion(ratio);
	}
	else
	{
		fEditViewScrollOffsetY = 0;
		fTimelineEdit->SetScrollViewOrigin(fLeftFrameIndex, fEditViewScrollOffsetY);
		bar->SetRange(0, 0);
	}
}

/*	FUNCTION:		TimelineView :: ScrollToHorizontal
	ARGS:			x
	RETURN:			n/a
	DESCRIPTION:	Horizontal scroll bar moved
*/
void TimelineView :: ScrollToHorizontal(const float x)
{
	const int64 total_frames = gProject->mTotalDuration + kFramesSecond;	//	add 1s to right (space for append)
	const int64 visible_frames = fViewWidth * fTimelineEdit->GetFramesPixel();
	fLeftFrameIndex = (total_frames - visible_frames)*(x-1.0f)/100.0f;
	fTimelineEdit->SetScrollViewOrigin(fLeftFrameIndex, fEditViewScrollOffsetY);
	fTimelinePosition->InitTimelineLabels();
	InvalidateItems(INVALIDATE_EDIT_TRACKS | INVALIDATE_POSITION_SLIDER);
}

/*	FUNCTION:		TimelineView :: ScrollToVertical
	ARGS:			y
	RETURN:			n/a
	DESCRIPTION:	Vertical scroll bar moved
*/
void TimelineView :: ScrollToVertical(const float y)
{
	std::vector<float> track_offsets;
	fTimelineEdit->GetTrackOffsets(track_offsets);
	float total_height = 0.0f;
	for (auto &t : track_offsets)
		total_height += t;
	const float kTrackExtraY = 9*6 + 64;	//	kTimelineTrackHeight + kTimelineTrackDeltaY (from TimelineEdit.cpp
	total_height += kTrackExtraY;
	float visible_height = fVerticalScrollView->Frame().Height();
	fEditViewScrollOffsetY = (total_height - visible_height)*(y-1.0f)/100.0f;
	fTimelineEdit->SetScrollViewOrigin(fLeftFrameIndex, fEditViewScrollOffsetY);
	InvalidateItems(INVALIDATE_EDIT_TRACKS | INVALIDATE_CONTROL_VIEW);
}

/*	FUNCTION:		TimelineView :: OutputViewMouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Mouse down initiated on Output view
*/
void TimelineView :: OutputViewMouseDown(const BPoint &point)
{
	if (fTimelineEdit->OutputViewMouseDown(point))
	{
		gRenderActor->AsyncPriority(&RenderActor::AsyncPrepareFrame, gRenderActor, fCurrentFrame);
	}
}

/*	FUNCTION:		TimelineView :: OutputViewMouseMoved
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Mouse move initiated on Output view
*/
void TimelineView :: OutputViewMouseMoved(const BPoint &point)
{
	if (fTimelineEdit->OutputViewMouseMoved(point))
	{
		gRenderActor->AsyncPriority(&RenderActor::AsyncPrepareFrame, gRenderActor, fCurrentFrame);
	}
}

/*	FUNCTION:		TimelineView :: OutputViewZoomed
	ARGS:			zoom_factor
	RETURN:			n/a
	DESCRIPTION:	Called when Output view zoomed
*/
void TimelineView :: OutputViewZoomed(const float zoom_factor)
{
	if (fTimelineEdit->OutputViewZoomed(zoom_factor))
	{
		gRenderActor->AsyncPriority(&RenderActor::AsyncPrepareFrame, gRenderActor, fCurrentFrame);
	}
}

/*	FUNCTION:		TimelineView :: ProjectLoaded
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	New project
*/
void TimelineView :: ProjectLoaded()
{
	fTimelineEdit->ProjectInvalidated();
	PositionUpdate(fCurrentFrame, true);

	const int64 visible_frames = fViewWidth * fTimelineEdit->GetFramesPixel();
	if (visible_frames < gProject->mTotalDuration)
	{
		double scroll_offset = (fCurrentFrame - 0.5f*visible_frames)/(double)gProject->mTotalDuration;
		if (scroll_offset < 0)
			scroll_offset = 0;
		fHorizontalScrollView->ScrollBar(B_HORIZONTAL)->SetValue(1.0 + 100.0*scroll_offset);
		fVerticalScrollView->ScrollBar(B_VERTICAL)->SetValue(0);
		ScrollToHorizontal(1.0 + 100.0*scroll_offset);
		ScrollToVertical(0);
	}
	else
	{
		fHorizontalScrollView->ScrollBar(B_HORIZONTAL)->SetValue(0.0f);
		fVerticalScrollView->ScrollBar(B_VERTICAL)->SetValue(0.0f);
	}
	UpdateHorizontalScrollBar();
	UpdateVerticalScrollBar();


	assert(gProject->mTimelineTracks.size() == fTrackSettings.size());
	for (size_t i=0; i < gProject->mTimelineTracks.size(); i++)
	{
		fTrackSettings[i].visual->SetValue(gProject->mTimelineTracks[i]->mVideoEnabled);
		fTrackSettings[i].audio->SetValue(gProject->mTimelineTracks[i]->mAudioEnabled);
	}
}

/*	FUNCTION:		TimelineView :: UpdateControlView
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Create/reposition track settings icons
*/
void TimelineView :: UpdateControlView()
{
	BRect frame = Bounds();
	const float kPadX = 12;

	std::vector<float> track_offsets;
	fTimelineEdit->GetTrackOffsets(track_offsets);
	fControlView->SetTrackOffsets(track_offsets, fEditViewScrollOffsetY);

	if (fTrackSettings.size() > track_offsets.size())
	{
		std::vector<TRACK_SETTINGS>::iterator i = fTrackSettings.begin() + track_offsets.size();
		while (i != fTrackSettings.end())
		{
			fControlView->RemoveChild((*i).visual);	delete (*i).visual;
			fControlView->RemoveChild((*i).audio);	delete (*i).audio;
			i++;
		}
		fTrackSettings.resize(track_offsets.size());
	}

	const float kIconSize = 32.0f;
	while(fTrackSettings.size() < track_offsets.size())
	{
		uint32 idx = uint32(fTrackSettings.size());
		float posy = track_offsets[idx];
		TRACK_SETTINGS setting;
		BMessage *msg_video = new BMessage(kMessageCheckboxVideo);
		msg_video->AddUInt32("index", idx);
		BMessage *msg_audio = new BMessage(kMessageCheckboxAudio);
		msg_audio->AddUInt32("index", idx);

		setting.visual = new BitmapCheckbox(BRect(kTimelineOffsetX - (kIconSize + kPadX), posy, kTimelineOffsetX - kPadX, posy + kIconSize), "video",
											BTranslationUtils::GetBitmap("Resources/icon_eye_off.png"),
											BTranslationUtils::GetBitmap("Resources/icon_eye.png"),
											msg_video);
		setting.visual->SetValue(1);
		fControlView->AddChild(setting.visual);
		setting.audio = new BitmapCheckbox(BRect(kTimelineOffsetX - 2*(kIconSize + kPadX) - 4, posy, kTimelineOffsetX - (kIconSize + kPadX) - 4, posy + kIconSize), "audio",
											BTranslationUtils::GetBitmap("Resources/icon_ear_off.png"),
											BTranslationUtils::GetBitmap("Resources/icon_ear.png"),
											msg_audio);
		setting.audio->SetValue(1);
		fControlView->AddChild(setting.audio);
		fTrackSettings.push_back(setting);

		if (Window())
		{
			setting.visual->SetTarget(this, (BLooper *)Window());
			setting.audio->SetTarget(this, (BLooper *)Window());
		}
	}

	if (Window())
	{
		assert(fTrackSettings.size() == track_offsets.size());
		for (size_t i=0; i < fTrackSettings.size(); i++)
		{
			float posy = track_offsets[i] - fEditViewScrollOffsetY;
			fTrackSettings[i].visual->MoveTo(BPoint(kTimelineOffsetX - (kIconSize + kPadX), posy));
			fTrackSettings[i].audio->MoveTo(BPoint(kTimelineOffsetX - 2*(kIconSize + kPadX) - 4, posy));
		}
	}

	fControlView->Invalidate();
	fButtonPlay->Invalidate();
	fButtonPlayAb->Invalidate();
}

