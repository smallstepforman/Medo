/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	Monitor Window
 */

#include <cstdio>

#include <InterfaceKit.h>

#include "PersistantWindow.h"
#include "MonitorWindow.h"
#include "MonitorControls.h"
#include "MedoWindow.h"
#include "Project.h"

/*************************************
	MonitorView
**************************************/
class MonitorView : public BView
{
	BBitmap				*fBitmap;
	MonitorControls		*fMonitorControls;
public:
	MonitorView(BRect frame, TimelinePlayer *player)
	: BView(frame, nullptr, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		fBitmap = nullptr;

		fMonitorControls  = new MonitorControls(BRect(frame.left, frame.bottom - (MonitorControls::kControlHeight), frame.right, frame.bottom), player);
		AddChild(fMonitorControls);
	}

	~MonitorView()	override
	{ }

	void AttachedToWindow() override
	{
		MakeFocus(true);
	}

	void FrameResized(float width, float height) override
	{
		fMonitorControls->FrameResized(width, MonitorControls::kControlHeight);
		fMonitorControls->MoveTo(0, height - MonitorControls::kControlHeight);
	}

	void Draw(BRect frame) override
	{
		SetHighColor(0x30, 0x30, 0x30);
		frame = Bounds();

		if (fBitmap)
		{
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
			DrawBitmap(fBitmap, bitmap_rect);

			if (bitmap_rect == frame)
				return;

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
		}
		else
			FillRect(frame);
	}
	void SetBitmap(BBitmap *bitmap, int64 frame_idx)
	{
		fBitmap = bitmap;
		fMonitorControls->SetCurrentFrame(frame_idx);
		Invalidate();
	}
	void KeyDown(const char *bytes, int32 numBytes)
	{
		switch ((char)*bytes)
		{
			case B_ESCAPE:
				((MonitorWindow *)Window())->RestoreZoom();
				break;

			default:
				printf("KeyDown(%s)\n", bytes);
				break;
		}
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what)
		{
			case B_MOUSE_WHEEL_CHANGED:
				fMonitorControls->MessageReceived(msg);
				break;
			default:
				BView::MessageReceived(msg);
		}
	}
};

/*************************************
	Monitor Window
**************************************/

/*	FUNCTION:		MonitorWindow :: MonitorWindow
	ARGS:			frame
					player
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
MonitorWindow :: MonitorWindow(BRect frame, const char *title, TimelinePlayer *player)
	: PersistantWindow(frame, title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	fPreZoomFrame = frame;
	fFullscreen = false;

	fMonitorView = new MonitorView(Bounds(), player);
	AddChild(fMonitorView);

	AddShortcut('f', 0, new BMessage('full'));
}

/*	FUNCTION:		MonitorWindow :: ~MonitorWindow
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
MonitorWindow :: ~MonitorWindow()
{ }

/*	FUNCTION:		MonitorWindow :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process window messages
*/
void MonitorWindow :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case MedoWindow::eMsgActionAsyncPreviewReady:	//	caution - must be unique msg->what for this window
		{
			BBitmap *bitmap;
			int64 frame_idx;
			if ((msg->FindPointer("BBitmap", (void **)&bitmap) == B_OK) &&
				(msg->FindInt64("frame", &frame_idx) == B_OK))
			{
				fMonitorView->SetBitmap(bitmap, frame_idx);
			}
			break;
		}

		case 'full':
		{
			BScreen screen;
			BRect frame = screen.Frame();
			if (!fFullscreen)
				Zoom(BPoint(0,0), frame.Width(), frame.Height());
			else
				RestoreZoom();
			break;
		}

		default:
			BWindow::MessageReceived(msg);
	}
}

/*	FUNCTION:		MonitorWindow :: Zoom
	ARGS:			origin
					width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void MonitorWindow :: Zoom(BPoint origin, float width, float height)
{
	fPreZoomFrame = Frame();
	fFullscreen = true;

	BScreen screen;
	BRect frame = screen.Frame();
	BWindow::Zoom(BPoint(0, 0), frame.Width(), frame.Height());
}

/*	FUNCTION:		MonitorWindow :: RestoreZoom
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when B_ESCAPE key received
*/
void MonitorWindow :: RestoreZoom()
{
	ResizeTo(fPreZoomFrame.Width(), fPreZoomFrame.Height());
	MoveTo(fPreZoomFrame.left, fPreZoomFrame.top);
	fFullscreen = false;

	gProject->InvalidatePreview();
}

