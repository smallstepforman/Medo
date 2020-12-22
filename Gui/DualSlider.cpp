/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	GUI Dual Slider
 */

#include <cstdio>
#include <cassert>

#include <interface/Window.h>
#include <interface/Slider.h>
#include <private/interface/ToolTip.h>
#include <translation/TranslationUtils.h>

#include "DualSlider.h"
#include "BitmapCheckbox.h"

enum {eMsgSliderLeft = 'dsm_', eMsgSliderRight, eMsgCheckboxLink};

static const float kSliderWidth = 20.0f;
static const float kMidPoint = -12.0f;
static const float kLeftStart = kMidPoint - kSliderWidth;
static const float kRightEnd = kMidPoint + kSliderWidth;

/**********************************
	MouseInterceptView
***********************************/
class MouseInterceptView : public BView
{
	DualSlider	*fParent;
public:
	MouseInterceptView(BRect frame, DualSlider *parent)
		: BView(frame, nullptr, 0, 0), fParent(parent)
	{ }
	void MouseDown(BPoint where)	override											{fParent->MouseDown(where);}
	void MouseMoved(BPoint where, uint32 code, const BMessage *drag_msg)	override	{fParent->MouseMoved(where, code, drag_msg);}
	void MouseUp(BPoint where)		override											{fParent->MouseUp(where);}
};

/*	FUNCTION:		DualSlider :: DualSlider
	ARGS:			frame
					name
					label
					message
					minValue
					maxValue
					label_left
					label_right
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
DualSlider :: DualSlider(BRect frame, const char* name, const char* label, BMessage* message, int32 minValue, int32 maxValue, const char *label_left, const char *label_right)
	: BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW | B_DRAW_ON_CHILDREN),
	  BInvoker()
{
	fMouseInterceptView = new MouseInterceptView(BRect(0, 0, frame.Width(), frame.Height()), this);
	AddChild(fMouseInterceptView);
	fMouseTracking = -1;

	float x = 0.5f*frame.Width();
	fSliders[0] = new BSlider(BRect(x+kLeftStart, 0, x+kMidPoint, frame.Height()-32), nullptr, label_left, nullptr, minValue, maxValue);
	fSliders[0]->SetModificationMessage(new BMessage(eMsgSliderLeft));
	fSliders[0]->SetOrientation(orientation::B_VERTICAL);
	fSliders[0]->SetHashMarks(B_HASH_MARKS_LEFT);
	fSliders[0]->SetHashMarkCount(9);
	fSliders[0]->SetBarColor({255, 0, 0, 255});
	fSliders[0]->UseFillColor(true);
	AddChild(fSliders[0]);

	fSliders[1] = new BSlider(BRect(x+kMidPoint, 0, x+kRightEnd, frame.Height()-32), nullptr, label_right, nullptr, minValue, maxValue);
	fSliders[1]->SetModificationMessage(new BMessage(eMsgSliderRight));
	fSliders[1]->SetOrientation(orientation::B_VERTICAL);
	fSliders[1]->SetHashMarks(B_HASH_MARKS_RIGHT);
	fSliders[1]->SetHashMarkCount(9);
	fSliders[1]->SetBarColor({255, 0, 0, 255});
	fSliders[1]->UseFillColor(true);
	AddChild(fSliders[1]);

	fCheckboxLinked = new BitmapCheckbox(BRect(x+kMidPoint - 16, frame.bottom - 40, x+kMidPoint + 16, frame.bottom), "linked",
								   BTranslationUtils::GetBitmap("Resources/icon_unlink.png"),
								   BTranslationUtils::GetBitmap("Resources/icon_link.png"),
								   new BMessage(eMsgCheckboxLink));
	AddChild(fCheckboxLinked);
	fCheckboxLinked->SetValue(1);

	SetMessage(message);
}

/*	FUNCTION:		DualSlider :: ~DualSlider
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
DualSlider :: ~DualSlider()
{
	//	Children are deleted automatically
}

/*	FUNCTION:		DualSlider :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void DualSlider :: AttachedToWindow()
{
	fSliders[0]->SetTarget(this, Window());
	fSliders[1]->SetTarget(this, Window());
	fCheckboxLinked->SetTarget(this, Window());

	SetViewColor(Parent()->ViewColor());
	fMouseInterceptView->SetViewColor(Parent()->ViewColor());
}

/*	FUNCTION:		DualSlider :: MouseDown
	ARGS:			where
	RETURN:			n/a
	DESCRIPTION:	Invoked by MouseInterceptView, allow catching right mouse down / ctrl modifier to control linked sliders
*/
void DualSlider :: MouseDown(BPoint where)
{
	if (where.y > Frame().Height() - 32)
	{
		fCheckboxLinked->MouseDown(where);
		return;
	}
	float mid = 0.5f*Frame().Width() + kMidPoint;

	fTrackingLinked = fCheckboxLinked->Value();
	uint32 buttons;
	BMessage* msg = Window()->CurrentMessage();
	msg->FindInt32("buttons", (int32*)&buttons);
	bool ctrl_modifier = modifiers() & B_CONTROL_KEY;
	if ((buttons & B_SECONDARY_MOUSE_BUTTON) || (modifiers() & B_CONTROL_KEY))
		fTrackingLinked = false;

	if ((where.x > mid - kSliderWidth) && (where.x < mid))
	{
		fSliders[0]->MouseDown(where);
		fMouseTracking = 0;
	}
	else if ((where.x > mid) && (where.x < mid + kSliderWidth))
	{
		fSliders[1]->MouseDown(where);
		fMouseTracking = 1;
	}

	if (fMouseTracking >= 0)
	{
		char buffer[8];
		sprintf(buffer, "%d", fSliders[fMouseTracking]->Value());
		fSliders[fMouseTracking]->SetToolTip(buffer);
		fSliders[fMouseTracking]->ToolTip()->SetSticky(true);
		fSliders[fMouseTracking]->ShowToolTip(fSliders[fMouseTracking]->ToolTip());
	}
}
void DualSlider :: MouseMoved(BPoint where, uint32 code, const BMessage *drag_msg)
{
	if (fMouseTracking < 0)
		return;

	char buffer[8];
	fSliders[fMouseTracking]->MouseMoved(where, code, drag_msg);
	sprintf(buffer, "%d", fSliders[fMouseTracking]->Value());
	fSliders[fMouseTracking]->SetToolTip(buffer);
	fSliders[fMouseTracking]->ToolTip()->SetSticky(true);
	fSliders[fMouseTracking]->ShowToolTip(fSliders[fMouseTracking]->ToolTip());
}

void DualSlider :: MouseUp(BPoint where)
{
	fMouseTracking = -1;
}

/*	FUNCTION:		DualSlider :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void DualSlider :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case eMsgSliderLeft:
		{
			if (fTrackingLinked)
				fSliders[1]->SetValue(fSliders[0]->Value());

			char buffer[8];
			sprintf(buffer, "%d", fSliders[0]->Value());
			fSliders[0]->SetToolTip(buffer);
			fSliders[0]->ShowToolTip();
			Invoke();
			break;
		}
		case eMsgSliderRight:
		{
			if (fTrackingLinked)
				fSliders[0]->SetValue(fSliders[1]->Value());
			char buffer[8];
			sprintf(buffer, "%d", fSliders[1]->Value());
			fSliders[1]->SetToolTip(buffer);
			fSliders[1]->ShowToolTip();
			Invoke();
			break;
		}
		case eMsgCheckboxLink:
			if (fCheckboxLinked->Value())
			{
				fSliders[1]->SetValue(fSliders[0]->Value());
			}
			break;
		default:
			BView::MessageReceived(msg);
	}
}

void DualSlider :: DrawAfterChildren(BRect frame)
{
	BRect bounds = Bounds();
	float x = 0.5f*bounds.Width() + kRightEnd + 2;
	float h = bounds.Height();
	SetHighColor(128, 128, 128);
	int32 min_limit, max_limit;
	fSliders[0]->GetLimits(&min_limit, &max_limit);
	char buffer[32];

	MovePenTo(x, 32+8);		//	top
	sprintf(buffer, "%d", max_limit);
	DrawString(buffer);

	MovePenTo(x, 32+8 + 0.5f*(0.5f*h-(32+8)));	//	half point between top and middle
	int three_quarters = min_limit + 0.75f*(max_limit - min_limit);
	sprintf(buffer, "%d", three_quarters);
	DrawString(buffer);

	MovePenTo(x, 0.5f*h);	//	middle
	int mid_point = 0.5f*(max_limit - min_limit);
	sprintf(buffer, "%d", mid_point);
	DrawString(buffer);

	MovePenTo(x,  0.5f*h + 0.5f*(h - (32+4) - 0.5f*h));		//	half point between middle and bottom
	int one_quarter = min_limit + 0.25f*(max_limit - min_limit);
	sprintf(buffer, "%d", one_quarter);
	DrawString(buffer);

	MovePenTo(x, h - (32+4));	//	bottom
	sprintf(buffer, "%d", min_limit);
	DrawString(buffer);
}

/*	FUNCTION:		DualSlider :: SetValue
	ARGS:			index, value
	RETURN:			n/a
	DESCRIPTION:	Set value
*/
void DualSlider :: SetValue(int index, int value)
{
	assert((index >= 0) && (index < 2));
	fSliders[index]->SetValue(value);
	fCheckboxLinked->SetValue(fSliders[0]->Value() == fSliders[1]->Value() ? 1 : 0);
}

/*	FUNCTION:		DualSlider :: GetValue
	ARGS:			n/a
	RETURN:			value
	DESCRIPTION:	Get value
*/
int DualSlider :: GetValue(int index)
{
	assert((index >= 0) && (index < 2));
	return fSliders[index]->Value();
}

/*	FUNCTION:		DualSlider :: SetEnabled
	ARGS:			enabled
	RETURN:			n/a
	DESCRIPTION:	Enable/Disable sliders
*/
void DualSlider :: SetEnabled(const bool enabled)
{
	fSliders[0]->SetEnabled(enabled);
	fSliders[1]->SetEnabled(enabled);
}

