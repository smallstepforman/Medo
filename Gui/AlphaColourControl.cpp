/*	PROJECT:		Medo
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2020-2021, ZenYes Pty Ltd
	DESCRIPTION:	Alpha Colour Control
*/

#include <cstdio>

#include <interface/ColorControl.h>
#include <interface/Slider.h>
#include <interface/TextControl.h>
#include <interface/View.h>
#include <interface/Window.h>

#include "AlphaColourControl.h"
#include "Editor/Language.h"

enum kAlphaColourMessages
{
	kMsgAlphaSlider = 'gals',
	kMsgAlphaText,
};

/**********************************
	AlphaSlider
***********************************/
class AlphaSlider : public BSlider
{
public:
	AlphaSlider(BRect frame, const char* name, const char* label, BMessage* message, int minValue, int maxValue)
		: BSlider(frame, name, label, message, minValue, maxValue)
	{ }
	void DrawBar() override
	{
		BRect r = Frame();
		r.left += 2;
		r.right -= 2;
		r.top += 2;
		r.bottom -= 2;
		BView *v = OffscreenView();
		v->SetHighColor({255,255,255,255});
		v->FillRect(r);
	}
	void DrawThumb() override
	{
		BRect r = ThumbFrame();
		BView *v = OffscreenView();

		// Draw the black shadow
		if (IsEnabled())
			v->SetHighColor({0,0,0,255});
		else
			v->SetHighColor({128,128,129,255});
		r.top += 4;
		r.left += 4;
		r.right -= 2;
		r.bottom -= 2;
		v->StrokeEllipse(r);

		// Draw the dark grey edge
		if (IsEnabled())
			v->SetHighColor({100,100,100,255});
		else
			v->SetHighColor({192,192,192,255});
		r.bottom--;
		r.right--;
		v->StrokeEllipse(r);

		// Fill the inside of the thumb
		v->SetHighColor({235,235,235,255});
		r.InsetBy(1,1);
		v->FillEllipse(r);
	}
};

/*	FUNCTION:		AlphaColourControl :: AlphaColourControl
	ARGS:			point
					name
					msg
					resizingMode
					flags
	RETURN:			n/a
	DESCRIPTION:	constructor
*/
AlphaColourControl :: AlphaColourControl(BPoint point, const char *name, BMessage *msg, uint32 resizingMode, uint32 flags)
	: BView(BRect(point.x, point.y, point.x + 480, point.y + 132), name, resizingMode, flags)
{
	fColourControl = new BColorControl(BPoint(0, 32), B_CELLS_32x8, 6.0f, "ColorControl", msg, true);
	AddChild(fColourControl);

	//	Colour labels
	BView *view_red = fColourControl->FindView("_red");
	BView *view_green = fColourControl->FindView("_green");
	BView *view_blue = fColourControl->FindView("_blue");
	if (view_red)		((BTextControl *)view_red)->SetLabel(GetText(TXT_EFFECTS_COMMON_RED));
	if (view_green)		((BTextControl *)view_green)->SetLabel(GetText(TXT_EFFECTS_COMMON_GREEN));
	if (view_blue)		((BTextControl *)view_blue)->SetLabel(GetText(TXT_EFFECTS_COMMON_BLUE));

	float scale = be_plain_font->Size()/16;
	fAlphaSlider = new AlphaSlider(BRect(0, 0, 256*scale, 32), "AlphaSlider", nullptr, nullptr, 0, 255);
	fAlphaSlider->SetModificationMessage(new BMessage(kMsgAlphaSlider));
	AddChild(fAlphaSlider);

	fTextAlpha = new BTextControl(BRect(264*scale, 0, 380*scale, 32), "alpha", GetText(TXT_EFFECTS_COMMON_ALPHA), "0", new BMessage(kMsgAlphaText), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	float labelWidth = StringWidth("Green: ");
	fTextAlpha->SetDivider(labelWidth);
	for (int32 i = 0; i < 256; i++)
		fTextAlpha->TextView()->DisallowChar(i);
	for (int32 i = '0'; i <= '9'; i++)
		fTextAlpha->TextView()->AllowChar(i);
	fTextAlpha->TextView()->SetMaxBytes(3);
	AddChild(fTextAlpha);
}

/*	FUNCTION:		AlphaColourControl :: ~AlphaColourControl
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
AlphaColourControl :: ~AlphaColourControl()
{ }

/*	FUNCTION:		AlphaColourControl :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void AlphaColourControl :: AttachedToWindow()
{
	AdoptParentColors();
	fColourControl->SetTarget(Parent(), Window());
	fAlphaSlider->SetTarget(this, Window());
}

/*	FUNCTION:		AlphaColourControl :: SetValue
	ARGS:			where
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
void AlphaColourControl :: SetValue(rgb_color colour)
{
	fColourControl->SetValue(colour);
	fAlphaSlider->SetValue(colour.alpha);
	char buffer[8];
	sprintf(buffer, "%d", colour.alpha);
	fTextAlpha->SetText(buffer);
}

/*	FUNCTION:		AlphaColourControl :: ValueAsColor
	ARGS:			none
	RETURN:			rgb_color
	DESCRIPTION:	Get value as RGB colour
*/
rgb_color AlphaColourControl :: ValueAsColor() const
{
	rgb_color colour = fColourControl->ValueAsColor();
	colour.alpha = fAlphaSlider->Value();
	return colour;
}

/*	FUNCTION:		AlphaColourControl :: SetEnabled
	ARGS:			enable
	RETURN:			n/a
	DESCRIPTION:	Enable/disable control
*/
void AlphaColourControl :: SetEnabled(const bool enable)
{
	fColourControl->SetEnabled(enable);
	fAlphaSlider->SetEnabled(enable);
	fTextAlpha->SetEnabled(enable);
}

void AlphaColourControl :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgAlphaSlider:
		{
			char buffer[8];
			sprintf(buffer, "%d", fAlphaSlider->Value());
			fTextAlpha->SetText(buffer);
			fColourControl->Invoke();
			break;
		}

		default:
			BView::MessageReceived(msg);
	}
}

