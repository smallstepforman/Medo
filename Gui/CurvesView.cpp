#include <cstdio>
#include <cassert>

#include <interface/View.h>
#include <interface/Window.h>
#include "CurvesView.h"

static const float	kControlPointSize = 8.0f;

static rgb_color kColours[CurvesView::NUMBER_CURVES] =
{
	{255, 32, 32, 255},
	{32, 255, 32, 255},
	{64, 64, 255, 255},
};

/*	FUNCTION:		CurvesView :: CurvesView
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
CurvesView :: CurvesView(BRect frame, BHandler *parent, BMessage *msg)
	: BView(frame, "CurvesView", B_FOLLOW_NONE, B_WILL_DRAW),
	  fParent(parent), fMessage(msg)
{
	SetViewColor(32, 32, 32, 255);

	Reset();
	fMouseTracking = false;
	fInterpolation = INTERPOLATION_CATMULL_ROM;
	fColourIndex = 0;
}

/*	FUNCTION:		CurvesView :: ~CurvesView
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
CurvesView :: ~CurvesView()
{ }

/*	FUNCTION:		CurvesView :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void CurvesView :: AttachedToWindow()
{ }

/*	FUNCTION:		CurvesView :: Reset
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Reset to default
*/
void CurvesView :: Reset()
{
	BRect frame = Bounds();

	for (int c=0; c < NUMBER_CURVES; c++)
	{
		COLOUR_COMPONENT &cc = fColourControls[c];

		cc.v[0] = 0.0f;
		cc.v[1] = 0.33f;
		cc.v[2] = 0.66f;
		cc.v[3] = 1.0f;

		cc.points[0].Set(kControlPointSize, frame.Height()-kControlPointSize);
		cc.points[1].Set(0.33f*frame.Width(), frame.Height() - cc.v[1]*frame.Height());
		cc.points[2].Set(0.66f*frame.Width(), frame.Height() - cc.v[2]*frame.Height());
		cc.points[3].Set(frame.Width() - kControlPointSize, kControlPointSize);
	}

	if (Window())
		Invalidate();
}

/*	FUNCTION:		CurvesView :: SetColourValues
	ARGS:			colour
					values
	RETURN:			n/a
	DESCRIPTION:	Set colour values
*/
void CurvesView :: SetColourValues(CURVE_COLOUR colour, float *values)
{
	COLOUR_COMPONENT &cc = fColourControls[colour];
	for (int i=0; i < 4; i++)
		cc.v[i] = values[i];

	BRect frame = Bounds();
	cc.points[0].Set(kControlPointSize, frame.Height()-kControlPointSize);
	cc.points[1].Set(0.33f*frame.Width(), frame.Height() - cc.v[1]*frame.Height());
	cc.points[2].Set(0.66f*frame.Width(), frame.Height() - cc.v[2]*frame.Height());
	cc.points[3].Set(frame.Width() - kControlPointSize, kControlPointSize);
}

/*	FUNCTION:		CurvesView :: SetInterpolation
	ARGS:			interpolation
	RETURN:			n/a
	DESCRIPTION:	Modify interpolation method
*/
void CurvesView :: SetInterpolation(INTERPOLATION interpolation)
{
	fInterpolation = interpolation;
	Invalidate();
}

/*	FUNCTION:		CurvesView :: SetActiveColour
	ARGS:			colour
	RETURN:			n/a
	DESCRIPTION:	Modify colour method
*/
void CurvesView :: SetActiveColour(CURVE_COLOUR colour)
{
	fColourIndex = colour;
	Invalidate();
}

/*	FUNCTION:		CurvesView :: SetWhiteBalance
	ARGS:			white
					send_msg
	RETURN:			n/a
	DESCRIPTION:	Set white balance
*/
void CurvesView :: SetWhiteBalance(const rgb_color &white, bool send_msg)
{
	//printf("CurvesView :: SetWhiteBalanc (%d, %d, %d)\n", white.red, white.green, white.blue);

	BRect frame = Bounds();

	fColourControls[CURVE_RED].points[0].Set(kControlPointSize, frame.Height()-kControlPointSize);
	fColourControls[CURVE_RED].points[2].Set(white.red/255.0f*frame.Width(), kControlPointSize);
	fColourControls[CURVE_RED].points[1].Set(0.5f*white.red/255.0f*frame.Width(), 0.5f*frame.Height());
	fColourControls[CURVE_RED].points[3].Set(frame.Width() - kControlPointSize, kControlPointSize);
	fColourControls[CURVE_RED].v[0] = 0.0f;
	fColourControls[CURVE_RED].v[1] = 0.5f*255.0f/white.red;
	fColourControls[CURVE_RED].v[2] = 255.0f/white.red;
	fColourControls[CURVE_RED].v[3] = 1.0f;

	fColourControls[CURVE_GREEN].points[0].Set(kControlPointSize, frame.Height()-kControlPointSize);
	fColourControls[CURVE_GREEN].points[2].Set(white.green/255.0f*frame.Width(), kControlPointSize);
	fColourControls[CURVE_GREEN].points[1].Set(0.5f*white.green/255.0f*frame.Width(), 0.5f*frame.Height());
	fColourControls[CURVE_GREEN].points[3].Set(frame.Width() - kControlPointSize, kControlPointSize);
	fColourControls[CURVE_GREEN].v[0] = 0.0f;
	fColourControls[CURVE_GREEN].v[1] = 0.5f*255.0f/white.green;
	fColourControls[CURVE_GREEN].v[2] = 255.0f/white.green;
	fColourControls[CURVE_GREEN].v[3] = 1.0f;

	fColourControls[CURVE_BLUE].points[0].Set(kControlPointSize, frame.Height()-kControlPointSize);
	fColourControls[CURVE_BLUE].points[2].Set(white.blue/255.0f*frame.Width(), kControlPointSize);
	fColourControls[CURVE_BLUE].points[1].Set(0.5f*white.blue/255.0f*frame.Width(), 0.5f*frame.Height());
	fColourControls[CURVE_BLUE].points[3].Set(frame.Width() - kControlPointSize, kControlPointSize);
	fColourControls[CURVE_BLUE].v[0] = 0.0f;
	fColourControls[CURVE_BLUE].v[1] = 0.5f*255.0f/white.blue;
	fColourControls[CURVE_BLUE].v[2] = 255.0f/white.blue;
	fColourControls[CURVE_BLUE].v[3] = 1.0f;

	Invalidate();
	if (fParent && fMessage && send_msg)
		Window()->PostMessage(fMessage, fParent);
}

/*	FUNCTION:		CurvesView :: MouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void CurvesView :: MouseDown(BPoint point)
{
	BRect frame = Bounds();
	float w= frame.Width();
	float h = frame.Height();

	COLOUR_COMPONENT &cc = fColourControls[fColourIndex];

	const float kGrace = 4.0f;
	for (int i=0; i < 4; i++)
	{
		if ((point.x >= cc.points[i].x - kGrace*kControlPointSize) &&
			(point.x < cc.points[i].x + kGrace*kControlPointSize) &&
			(point.y >= cc.points[i].y - kGrace*kControlPointSize) &&
			(point.y < cc.points[i].y + kGrace*kControlPointSize))
		{
			fMouseTracking = true;
			fMouseTrackingIndex = i;
			break;
		}
	}
	if (fMouseTracking)
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

/*	FUNCTION:		CurvesView :: MouseUp
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void CurvesView :: MouseUp(BPoint)
{
	fMouseTracking = false;
}

/*	FUNCTION:		CurvesView :: MouseMoved
	ARGS:			where
					code
					dragMessage
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void CurvesView :: MouseMoved(BPoint where, uint32 code, const BMessage *dragMessage)
{
	if (!fMouseTracking)
		return;

	COLOUR_COMPONENT &cc = fColourControls[fColourIndex];

	BRect frame = Bounds();
	switch (fMouseTrackingIndex)
	{
		case 0:
			where.x = kControlPointSize;
			break;
		case 1:
			if (where.x > cc.points[2].x)
				where.x = cc.points[2].x - 1.0f;
			break;
		case 2:
			if (where.x < cc.points[1].x)
				where.x = cc.points[1].x + 1.0f;
			break;
		case 3:
			where.x = frame.Width() - kControlPointSize;
			break;
		default:
			break;
	}

	cc.v[fMouseTrackingIndex] = (frame.Height() - where.y) / frame.Height();
	if (cc.v[fMouseTrackingIndex] < 0.0f)
		cc.v[fMouseTrackingIndex] = 0.0f;
	if (cc.v[fMouseTrackingIndex] > 1.0f)
		cc.v[fMouseTrackingIndex] = 1.0f;

	if (where.y < kControlPointSize)
		where.y = kControlPointSize;
	if (where.y > frame.Height() - kControlPointSize)
		where.y = frame.Height() - kControlPointSize;

	cc.points[fMouseTrackingIndex].Set(where.x, where.y);

	Invalidate();

	if (fParent && fMessage)
		Window()->PostMessage(fMessage, fParent);
}

/*	FUNCTION:		CatmullRomSpline
	ARGS:			x	0...1
					v[4]
	RETURN:			n/a
	DESCRIPTION:	Catmull Rom Spline interpolation
*/
static float CatmullRomSpline(float x, float v0, float v1, float v2, float v3)
{
#define M11		0.0f
#define M12		1.0f
#define M13		0.0f
#define M14		0.0f

#define M21		-0.5f
#define M22		0.0f
#define M23		0.5f
#define M24		0.0f

#define M31		1.0f
#define M32		-2.5f
#define M33		2.0f
#define M34		-0.5f

#define M41		-0.5f
#define M42		1.5f
#define M43		-1.5f
#define M44		0.5f

	float c1 =          M12*v1;
	float c2 = M21*v0          + M23*v2;
	float c3 = M31*v0 + M32*v1 + M33*v2 + M34*v3;
	float c4 = M41*v0 + M42*v1 + M43*v2 + M44*v3;

	float cms =  ((c4*x + c3)*x + c2)*x + c1;
	return cms;
}

/*	FUNCTION:		CurvesView :: Draw
	ARGS:			rect
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void CurvesView :: Draw(BRect)
{
	BRect frame = Bounds();
	const float h = frame.Height();
	float pen_size = PenSize();
	SetPenSize(4.0f);

	for (int c = NUMBER_CURVES - 1; c >= 0; c--)
	{
		SetHighColor(kColours[c]);

		COLOUR_COMPONENT &cc = fColourControls[c];

		if (fInterpolation == INTERPOLATION_CATMULL_ROM)
		{
			float w = cc.points[1].x;
			for (int x=0; x < (int)w; x++)
			{
				float y = frame.bottom * CatmullRomSpline((float)x/w, 2*cc.v[0]-cc.v[1], cc.v[0], cc.v[1], cc.v[2]);
				MovePenTo(x, h-y);
				StrokeLine(BPoint(x, h-y));
			}
			w = cc.points[2].x - cc.points[1].x;
			for (int x=0; x < (int)w; x++)
			{
				float y = frame.bottom * CatmullRomSpline((float)x/w, cc.v[0], cc.v[1], cc.v[2], cc.v[3]);
				MovePenTo(x+cc.points[1].x, h-y);
				StrokeLine(BPoint(x+cc.points[1].x, h-y));
			}
			w = frame.Width() - cc.points[2].x;
			for (int x=0; x < (int)w; x++)
			{
				float y = frame.bottom * CatmullRomSpline((float)x/w, cc.v[1], cc.v[2], cc.v[3], 2*cc.v[3]-cc.v[2]);
				MovePenTo(x+cc.points[2].x, h-y);
				StrokeLine(BPoint(x+cc.points[2].x, h-y));
			}
		}
		else	//	Beizer
		{
			for (int x=0; x < frame.Width(); x++)
			{
				float t = (float)x / frame.Width();
				float q = 1-t;
				float y = q*q*q*cc.v[0] + 3*t*q*q*cc.v[1] + 3*t*t*q*cc.v[2] + t*t*t*cc.v[3];

				MovePenTo(x, h-y*h);
				StrokeLine(BPoint(x, h-y*h));
			}
		}

		//	Control points
		if (fColourIndex == c)
		{
			for (int i=0; i < 4; i++)
			{
				FillRect(BRect(cc.points[i].x - kControlPointSize, cc.points[i].y - kControlPointSize,
							   cc.points[i].x + kControlPointSize, cc.points[i].y + kControlPointSize));
			}
		}
	}

	SetPenSize(pen_size);
}

