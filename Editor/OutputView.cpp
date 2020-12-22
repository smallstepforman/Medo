/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Output Preview
 */

#include <cstdio>
#include <vector>
#include <cassert>

#include <interface/View.h>
#include <interface/Bitmap.h>

#include "OutputView.h"
#include "TimelineView.h"
#include "MedoWindow.h"
#include "Project.h"

static const float kMinZoomFactor = 0.5f;
static const float kMaxZoomFactor = 50.0f;

/*	FUNCTION:		OutputView :: OutputView
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
OutputView :: OutputView(BRect frame)
: BView(frame, "ControlOutput", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fBitmap(nullptr), fTimelineView(nullptr), fMouseTracking(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);	

	fZoomFactor = 1.0f;
	fZoomOffset.Set(0.0f, 0.0f);
}

/*	FUNCTION:		OutputView :: ~OutputView
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
OutputView :: ~OutputView()
{}

/*	FUNCTION:		OutputView :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw preview frame
*/
void OutputView :: Draw(BRect frame)
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

		if (fZoomFactor <= 1.0f)
		{
			bitmap_rect.left = 0.5f*(frame.Width() - (bitmap_w*fZoomFactor*r));
			bitmap_rect.right = bitmap_rect.left + bitmap_w*fZoomFactor*r;
			bitmap_rect.top = 0.5f*(frame.Height() - (bitmap_h*fZoomFactor*r));
			bitmap_rect.bottom = bitmap_rect.top + bitmap_h*fZoomFactor*r;
			DrawBitmapAsync(fBitmap, bitmap_rect);
		}
		else
		{
			BRect source_rect;
			source_rect.left = 0.5f*(bitmap_w - bitmap_w/fZoomFactor) + fZoomOffset.x;
			source_rect.right = 0.5f*(bitmap_w + bitmap_w/fZoomFactor) + fZoomOffset.x;
			source_rect.top = 0.5f*(bitmap_h - bitmap_h/fZoomFactor) + fZoomOffset.y;
			source_rect.bottom = 0.5f*(bitmap_h + bitmap_h/fZoomFactor) + fZoomOffset.y;

			bitmap_rect = frame;

			if (source_rect.left < 0)
			{
				bitmap_rect.left = -source_rect.left;
				source_rect.left = 0.0f;
				source_rect.right = bitmap_w/frame.Width()*bitmap_rect.Width()/fZoomFactor;
			}
			if (source_rect.right > fBitmap->Bounds().right)
			{
				bitmap_rect.right -= (source_rect.right - fBitmap->Bounds().right);
				source_rect.right = fBitmap->Bounds().right;
				source_rect.left = source_rect.right - bitmap_w/frame.Width()*bitmap_rect.Width()/fZoomFactor;
			}
			if (source_rect.top < 0)
			{
				bitmap_rect.top = -source_rect.top;
				source_rect.top = 0.0f;
				source_rect.bottom = bitmap_h/frame.Height() * bitmap_rect.Height()/fZoomFactor;
			}
			if (source_rect.bottom > fBitmap->Bounds().bottom)
			{
				bitmap_rect.bottom -= (source_rect.bottom - fBitmap->Bounds().bottom);
				source_rect.bottom = fBitmap->Bounds().bottom;
				source_rect.top = source_rect.bottom - bitmap_h/frame.Height()*bitmap_rect.Height()/fZoomFactor;
			}

#if 0
			printf("\nfZoomFactor=%f, fZoomOffset=", fZoomFactor); fZoomOffset.PrintToStream();
			printf("source_rect: ");	source_rect.PrintToStream();
			printf("bitmap_rect: ");	bitmap_rect.PrintToStream();
#endif

			DrawBitmapAsync(fBitmap, source_rect, bitmap_rect);
		}
		
		//	Fill remainder (black bars)
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

		//	Display zoom factor overlay
		if (fZoomFactor != 1.0f)
		{
			char buffer[8];
			sprintf(buffer, "x%0.2f", fZoomFactor);
			font_height fh;
			be_bold_font->GetHeight(&fh);

			FillRect(BRect(frame.right - 4*fh.ascent, 0, frame.right, 1.2f*(fh.ascent + fh.descent)));
			MovePenTo(frame.right - 3.0f*fh.ascent, fh.ascent);
			SetFont(be_bold_font);
			SetHighColor({255, 255, 255, 255});
			DrawString(buffer);
			SetFont(be_plain_font);
		}
	}
	else
		FillRect(frame);
}

/*	FUNCTION:		OutputView :: SetBitmap
	ARGS:			bitmap
	RETURN:			n/a
	DESCRIPTION:	Set bitmap
*/
void OutputView :: SetBitmap(BBitmap *bitmap)
{
	fBitmap = bitmap;
	
	if (fBitmap)
		SetViewColor(B_TRANSPARENT_COLOR);
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

/*	FUNCTION:		OutputView :: SetTimelineView
	ARGS:			view
	RETURN:			n/a
	DESCRIPTION:	Set timeline view (for MouseMoved messages)
*/
void OutputView :: SetTimelineView(TimelineView *view)
{
	fTimelineView = view;
}

/*	FUNCTION:		OutputView :: MouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook function, direct mouse messages to fTimlineView
*/
void OutputView :: MouseDown(BPoint point)
{
	if (!Window()->IsActive())
		Window()->Activate();

	fMouseTracking = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
	fTimelineView->OutputViewMouseDown(point);
	fMouseDownPoint = point;
}

/*	FUNCTION:		OutputView :: MouseMoved
	ARGS:			point
					transit
					message
	RETURN:			n/a
	DESCRIPTION:	Hook function, direct mouse messages to fTimlineView
*/
void OutputView :: MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!fMouseTracking)
		return;

	assert(fTimelineView != nullptr);
	fTimelineView->OutputViewMouseMoved(point);

	if (fZoomFactor > 1.0f)
	{
		fZoomOffset.x -= point.x - fMouseDownPoint.x;
		fZoomOffset.y -= point.y - fMouseDownPoint.y;
		fMouseDownPoint = point;
		Invalidate();
	}
}

/*	FUNCTION:		OutputView :: MouseUp
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook function, direct mouse messages to fTimlineView
*/
void OutputView :: MouseUp(BPoint point)
{
	fMouseTracking = false;
	SetMouseEventMask(B_POINTER_EVENTS, 0);
}

/*	FUNCTION:		OutputView :: MessageReceived
	ARGS:			msg
	RETURN:			none
	DESCRIPTION:	Hook function
*/
void OutputView :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case B_MOUSE_WHEEL_CHANGED:
		{
			bool option_modifier = ((MedoWindow *)Window())->GetKeyModifiers() & B_COMMAND_KEY;
			if (!option_modifier)
			{
				fTimelineView->MessageReceived(msg);
			}
			else
			{
				float delta_y;
				if (msg->FindFloat("be:wheel_delta_y", &delta_y) == B_OK)
				{
					Zoom(delta_y < 0);
				}
			}
			break;
		}

		default:
			BView::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		OutputView :: Zoom
	ARGS:			in
	RETURN:			n/a
	DESCRIPTION:	Zoom in/out
*/
void OutputView :: Zoom(bool in)
{
	fZoomFactor -= (fZoomFactor >= 3.0f ? 1.0f : 0.25f) * (in ? -1.0f : 1.0f);
	if (fZoomFactor < 0.0f)
		fZoomFactor = 0.0f;
	if (fZoomFactor <= 1.0f)
		fZoomOffset.Set(0.0f, 0.0f);
	else if (fZoomFactor < kMinZoomFactor)
		fZoomFactor = kMinZoomFactor;
	else if (fZoomFactor > kMaxZoomFactor)
		fZoomFactor = kMaxZoomFactor;
	fTimelineView->OutputViewZoomed(fZoomFactor);
	Invalidate();
}

/*	FUNCTION:		OutputView :: GetProjectConvertedMouseDown
	ARGS:			point
	RETURN:			converted mouse down (in project coordinates)
	DESCRIPTION:	Convert view mouse down into project coordinates (percentage)
*/
BPoint OutputView :: GetProjectConvertedMouseDown(const BPoint &point)
{
	BRect bounds = Bounds();
	float ratio_x = bounds.Width() / gProject->mResolution.width;
	float ratio_y = bounds.Height() / gProject->mResolution.height;
	float r = (ratio_x < ratio_y) ? ratio_x : ratio_y;
	BRect aRect;
	aRect.left = 0.5f*(bounds.Width() - (gProject->mResolution.width*fZoomFactor*r));
	aRect.right = aRect.left + gProject->mResolution.width*fZoomFactor*r;
	aRect.top = 0.5f*(bounds.Height() - (gProject->mResolution.height*fZoomFactor*r));
	aRect.bottom = aRect.top + gProject->mResolution.height*fZoomFactor*r;
	BPoint converted_point((point.x-aRect.left)/aRect.Width(), (point.y-aRect.top)/aRect.Height());
	return converted_point;
}
