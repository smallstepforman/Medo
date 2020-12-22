/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	GUI Keyframe slider
 */

#include <cassert>
#include <cstdio>

#include <InterfaceKit.h>

#include  "KeyframeSlider.h"

static const float kTriangleSize = 16.0f;


/*	FUNCTION:		KeyframeSlider :: KeyframeSlider
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
KeyframeSlider :: KeyframeSlider(BRect frame)
	: BView(frame, "keyframe_slider", B_FOLLOW_NONE, B_WILL_DRAW),
	  fTargetLooper(nullptr), fTargetHandler(nullptr), fTargetMessage(nullptr)
{
	SetViewColor(ui_color(B_CONTROL_BACKGROUND_COLOR));

	fMouseTrackingIndex = -1;
	fSelectIndex = 0;
}

/*	FUNCTION:		KeyframeSlider :: ~KeyframeSlider
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
KeyframeSlider :: ~KeyframeSlider()
{ }

/*	FUNCTION:		PathView :: SetObserver
	ARGS:			looper, handler
					message
	RETURN:			n/a
	DESCRIPTION:	Notify target
*/
void KeyframeSlider :: SetObserver(BLooper *looper, BHandler *handler, BMessage *message)
{
	assert(looper);
	assert(handler);
	assert(message);

	fTargetLooper = looper;
	fTargetHandler = handler;
	fTargetMessage = message;
}

/*	FUNCTION:		KeyframeSlider :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw
*/
void KeyframeSlider :: Draw(BRect)
{
	BRect frame = Bounds();
	float w = frame.Width();
	float h = frame.Height();

	if (fPoints.size() == 0)
	{
		//assert(0);
		return;
	}

	SetLowColor(255, 0, 0);
	if (fPoints.size() <= 1)
	{
		SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
		DrawString("Single keyframe", BPoint(kTriangleSize, h-0.5f*kTriangleSize));
		return;
	}

	SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
	frame.left += 0.5f*kTriangleSize;
	frame.right -= 0.5f*kTriangleSize;
	frame.bottom -= kTriangleSize;
	frame.top = frame.bottom - 0.5f*kTriangleSize;
	FillRect(frame);


	//	Assumption - begin=0, end=1
	SetPenSize(1.0f);
	if (fSelectIndex == 0)
		SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR));
	else
		SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
	StrokeTriangle(BPoint(0.5f*kTriangleSize, h - kTriangleSize), BPoint(0, h), BPoint(kTriangleSize, h));

	if (fSelectIndex == (int)(fPoints.size() - 1))
		SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR));
	else
		SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
	StrokeTriangle(BPoint(w-0.5f*kTriangleSize, h - kTriangleSize), BPoint(w-kTriangleSize, h), BPoint(w, h));


	if (fPoints.size() > 2)
	{
		w -= kTriangleSize;
		for (int i=1; i < int(fPoints.size() - 1); i++)
		{
			float p = 0.5f*kTriangleSize + fPoints[i]*w;
			if (i == fSelectIndex)
				SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR));
			else
				SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
			FillTriangle(BPoint(p, h - kTriangleSize), BPoint(p-0.5f*kTriangleSize, h), BPoint(p+0.5f*kTriangleSize, h));
		}
	}
}

/*	FUNCTION:		KeyframeSlider :: MouseDown
	ARGS:			where
	RETURN:			n/a
	DESCRIPTION:	Hook functio when mouse down
*/
void KeyframeSlider :: MouseDown(BPoint where)
{
	if (fPoints.size() < 2)
		return;

	BRect bounds = Bounds();
	const float kGrace = 4.0f / bounds.Width();
	float point = (where.x-0.5f*kTriangleSize)/(bounds.Width() - kTriangleSize);		//	0..1
	//printf("where=%f, bounds=%f, p=%f\n", where.x, bounds.Width(), point);
	for (int i=0; i < fPoints.size(); i++)
	{
		if ((point > fPoints[i] - kGrace) && (point < fPoints[i]+kGrace))
		{
			fMouseTrackingIndex = i;
			fSelectIndex = i;
			if (fTargetMessage->HasInt32("selection"))
				fTargetMessage->ReplaceInt32("selection", fSelectIndex);
			else
				fTargetMessage->AddInt32("selection", fSelectIndex);
			fTargetLooper->PostMessage(fTargetMessage, fTargetHandler);
			break;
		}
	}

	if (fPoints.size() < 3)
		fSelectIndex = -1;	//	we allow PostMessage with 2 items to select

	if (fMouseTrackingIndex >= 0)
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

/*	FUNCTION:		KeyframeSlider :: MouseMoved
	ARGS:			where
	RETURN:			n/a
	DESCRIPTION:	Hook functio when mouse moved
*/
void KeyframeSlider :: MouseMoved(BPoint where, uint32 code, const BMessage *)
{
	if (fMouseTrackingIndex < 0)
		return;

	BRect bounds = Bounds();
	float point = (where.x-0.5f*kTriangleSize)/(bounds.Width() - kTriangleSize);		//	0..1

	if (point < 0.0f)	point = 0.0f;
	if (point > 1.0f)	point = 1.0f;
	if (point < fPoints[fMouseTrackingIndex-1])		point = fPoints[fMouseTrackingIndex - 1];
	if (point > fPoints[fMouseTrackingIndex + 1])	point = fPoints[fMouseTrackingIndex + 1];

	fPoints[fMouseTrackingIndex] = point;
	fTargetLooper->PostMessage(fTargetMessage, fTargetHandler);
	Invalidate();
}

/*	FUNCTION:		KeyframeSlider :: MouseUp
	ARGS:			where
	RETURN:			n/a
	DESCRIPTION:	Hook functio when mouse up
*/
void KeyframeSlider :: MouseUp(BPoint where)
{
	fMouseTrackingIndex = -1;
}

void KeyframeSlider :: Select(int index)
{
	assert((index >= 0) && (index < fPoints.size()));
	fSelectIndex = index;
	Invalidate();
}

/*	FUNCTION:		KeyframeSlider :: SetPoints
	ARGS:			points
	RETURN:			n/a
	DESCRIPTION:	Set from parent
*/
void KeyframeSlider :: SetPoints(std::vector<float> &points)
{
	fPoints.clear();
	fPoints = points;

	if (points.size() > 1)
	{
		assert(points[0] == 0.0f);
		assert(points[points.size()-1] == 1.0f);
	}
	Invalidate();
}

/*	FUNCTION:		KeyframeSlider :: GetPoints
	ARGS:			points
	RETURN:			n/a
	DESCRIPTION:	Update parents version
*/
void KeyframeSlider :: GetPoints(std::vector<float> &points)
{
	points.clear();
	points = fPoints;
}
