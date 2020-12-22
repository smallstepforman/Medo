/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2020
 *	DESCRIPTION:	Monitor Controls
 */

#include <cstdio>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Gui/BitmapCheckbox.h"
#include "Gui/BitmapButton.h"
#include "MonitorControls.h"
#include "TimelinePlayer.h"
#include "Project.h"

static constexpr uint32	kPulseDuration = 2;
static constexpr float	kButtonXoffset = 2;
static constexpr float	kButtonWidth = 32.0f;
static constexpr float	kButtonHeight = 32.0f;
static constexpr float	kProgressHeight = 12.0f;
static constexpr float	kProgressOffset = 4.0f;
static constexpr float	kMouseOffset = 16.0f;
static_assert(kMouseOffset + kButtonHeight + kProgressHeight + kProgressOffset == MonitorControls::kControlHeight);

enum
{
	kMessageButtonRewind		= 'mcbr',
	kMessageButtonPlay			= 'mcbp',
};

/*	FUNCTION:		MonitorControls :: MonitorControls
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
MonitorControls :: MonitorControls(BRect frame, TimelinePlayer *player)
	: BView(frame, "MonitorControls", B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_DRAW_ON_CHILDREN | B_TRANSPARENT_BACKGROUND),
	  fTimelinePlayer(player)
{
	SetViewColor(B_TRANSPARENT_COLOR);

	float x = 0.5f*frame.Width();

	fButtonRewind = new BitmapButton(BRect(x - kButtonWidth - kButtonXoffset, kMouseOffset, x - kButtonXoffset, kMouseOffset + kButtonHeight), "rewind",
									   BTranslationUtils::GetBitmap("Resources/icon_frame_left.png"),
									   BTranslationUtils::GetBitmap("Resources/icon_frame_left_down.png"),
									   new BMessage(kMessageButtonRewind));
	AddChild(fButtonRewind);

	fButtonPlay = new BitmapCheckbox(BRect(x + kButtonXoffset, kMouseOffset, x + kButtonXoffset + kButtonWidth, kMouseOffset + kButtonHeight), "play",
									 BTranslationUtils::GetBitmap("Resources/icon_play.png"),
									 BTranslationUtils::GetBitmap("Resources/icon_pause.png"),
									 new BMessage(kMessageButtonPlay));
	AddChild(fButtonPlay);

	fPulseCount = kPulseDuration;
}

/*	FUNCTION:		MonitorControls :: ~MonitorControls
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
MonitorControls :: ~MonitorControls()
{ }

/*	FUNCTION:		MonitorControls :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorControls :: AttachedToWindow()
{
	fButtonPlay->SetTarget(this, Window());
	fButtonRewind->SetTarget(this, Window());
}

/*	FUNCTION:		MonitorControls :: WindowActivated
	ARGS:			enable
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorControls :: WindowActivated(bool)
{
	fButtonPlay->SetState(fTimelinePlayer->IsPlaying());
}

/*	FUNCTION:		MonitorControls :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorControls :: FrameResized(float width, float height)
{
	ResizeTo(width, height);
	if (fPulseCount > 0)
	{
		float x = 0.5f*width;
		fButtonRewind->MoveTo(x - kButtonWidth - kButtonXoffset, kMouseOffset);
		fButtonPlay->MoveTo(x + kButtonXoffset, kMouseOffset);
	}
}

/*	FUNCTION:		MonitorControls :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorControls :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMessageButtonRewind:
			fTimelinePlayer->Async(&TimelinePlayer::AsyncSetFrame, fTimelinePlayer, 0);
			break;

		case kMessageButtonPlay:
			if (fButtonPlay->Value() == 1)
			{
				if (fCurrentFrame >= gProject->mTotalDuration)
					fCurrentFrame = 0;
				fTimelinePlayer->Async(&TimelinePlayer::AsyncPlay, fTimelinePlayer, fCurrentFrame, -1, false);
			}
			else
				fTimelinePlayer->Async(&TimelinePlayer::AsyncStop, fTimelinePlayer);
			break;

		case B_MOUSE_WHEEL_CHANGED:
		{
			float delta_y;
			if (msg->FindFloat("be:wheel_delta_y", &delta_y) == B_OK)
			{
				int64 when = fCurrentFrame + kFramesSecond/gProject->mResolution.frame_rate*delta_y*-1.0f;
				int64 max_time = gProject->mTotalDuration;
				if (when > max_time)	when = max_time;
				if (when < 0)			when = 0;
				fTimelinePlayer->Async(&TimelinePlayer::AsyncSetFrame, fTimelinePlayer, when);
			}
			break;
		}

		default:
			BView::MessageReceived(msg);
	}
}

/*	FUNCTION:		MonitorControls :: SetCurrentFrame
	ARGS:			frame_idx
	RETURN:			n/a
	DESCRIPTION:	Called when frame position updated (TimelinePlayer or TimelineView)
*/
void MonitorControls :: SetCurrentFrame(int64 frame_idx)
{
	fCurrentFrame = frame_idx;
}

/*	FUNCTION:		MonitorControls :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorControls :: MouseDown(BPoint point)
{
	BRect bounds = Bounds();
	if (point.y > bounds.bottom - (kProgressHeight + kProgressOffset))
	{
		float p = point.x / bounds.Width();
		fTimelinePlayer->Async(&TimelinePlayer::AsyncSetFrame, fTimelinePlayer, gProject->mTotalDuration*p);
	}
}

/*	FUNCTION:		MonitorControls :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorControls :: MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	switch (transit)
	{
		case B_EXITED_VIEW:
		{
			if ((point.y < 0) || (point.y > kControlHeight))
				fPulseCount = kPulseDuration;
			break;
		}

		case B_ENTERED_VIEW:
		{
			fPulseCount = uint32(-1);
			BRect frame = Bounds();
			float x = 0.5f*frame.Width();
			fButtonRewind->MoveTo(x - kButtonWidth - kButtonXoffset, kMouseOffset);
			fButtonPlay->MoveTo(x + kButtonXoffset, kMouseOffset);
			Invalidate();
			break;
		}

		case B_INSIDE_VIEW:
			fPulseCount = uint32(-1);
			break;

		case B_OUTSIDE_VIEW:
		default:
			break;
	}
}

/*	FUNCTION:		MonitorControls :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorControls :: MouseUp(BPoint point)
{
}

/*	FUNCTION:		MonitorControls :: Pulse
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorControls :: Pulse()
{
	if (fPulseCount > 0)
	{
		if (--fPulseCount == 0)
		{
			fButtonRewind->MoveTo(-200, -100);
			fButtonPlay->MoveTo(-100, -100);
		}
	}
}

/*	FUNCTION:		MonitorControls :: DrawAfterChildren
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorControls :: DrawAfterChildren(BRect frame)
{
	if (fPulseCount > 0)
	{
		//	Progress bar
		SetHighColor(192, 128, 32);
		double pos = (double)fCurrentFrame / (double)gProject->mTotalDuration;
		frame.right *= pos;
		frame.top = frame.bottom - (kProgressHeight + kProgressOffset);
		FillRect(frame);

		//	Time
		char buffer[20];
		gProject->CreateTimeString(fCurrentFrame, buffer);
		char *strarray[1] = {buffer};
		int len[1] = {20};
		float width;
		GetStringWidths(strarray, len, 1, &width);

		SetHighColor(128, 64, 16);
		FillRect(BRect(frame.right - (width+8), frame.top, frame.right, frame.bottom));
		MovePenTo(frame.right - (width+4), frame.bottom - 2);
		SetFont(be_bold_font);
		SetHighColor({255, 255, 255, 255});
		DrawString(buffer);
		SetFont(be_plain_font);
	}
}

