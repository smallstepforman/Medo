/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	GUI Value Slider
 */

#include <cassert>

#include <interface/Slider.h>
#include <interface/ControlLook.h>
#include <support/String.h>

#include "ValueSlider.h"

/*	FUNCTION:		ValueSlider :: ValueSlider
	ARGS:			frame
					name
					label
					message
					minValue
					maxValue
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ValueSlider :: ValueSlider(BRect frame, const char* name, const char* label, BMessage* message, int32 minValue, int32 maxValue)
	: BSlider(frame, name, label, message, minValue, maxValue, B_TRIANGLE_THUMB), fFloatPrecision(1)
{ }

/*	FUNCTION:		ValueSlider :: UpdateText
	ARGS:			none
	RETURN:			Text to display
	DESCRIPTION:	Hook function
*/
const char * ValueSlider :: UpdateText() const
{
	return fText.String();
}

/*	FUNCTION:		ValueSlider :: UpdateTextValue
	ARGS:			value
	RETURN:			n/a
	DESCRIPTION:	Modify text message
*/
void ValueSlider :: UpdateTextValue(const float value)
{
	switch (fFloatPrecision)
	{
		case 3:		fText.SetToFormat("%0.3f", value);		break;
		case 2:		fText.SetToFormat("%0.2f", value);		break;
		case 0:		fText.SetToFormat("%d", (int)value);	break;
		default:	fText.SetToFormat("%0.1f", value);
	}
}

/*	FUNCTION:		ValueSlider :: SetValueUpdateText
	ARGS:			value
	RETURN:			n/a
	DESCRIPTION:	Set value and update text
					Note - only use this when text linear with value
*/
void ValueSlider :: SetValueUpdateText(const float value)
{
	SetValue(value);
	UpdateTextValue(value);
}

void ValueSlider :: SetMidpointLabel(const char *label)
{
	fMidpointLabel.SetTo(label);
}

/*	FUNCTION:		ValueSlider :: DrawText
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
					Derived from interface/Slider.cpp
*/
void ValueSlider :: DrawText()
{
	BRect bounds(Bounds());
	BView *view = OffscreenView();
	rgb_color base = LowColor();
	rgb_color high = HighColor();


	uint32 flags = be_control_look->Flags(this);
	flags &= ~BControlLook::B_IS_CONTROL;

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	assert(Orientation() == B_HORIZONTAL);

	SetFont(be_bold_font);
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));

	if (Label())
	{
		//be_control_look->DrawLabel(view, Label(), base, flags,
		//						   BPoint(0.0f, ceilf(fontHeight.ascent)));
		DrawString(Label(), BPoint(0.0f, ceilf(fontHeight.ascent)));
	}

	const char *update_text = UpdateText();
	if (update_text)
	{
		//be_control_look->DrawLabel(view, update_text, base, flags,
		//						   BPoint(bounds.right - StringWidth(update_text), ceilf(fontHeight.ascent)));
		DrawString(update_text, BPoint(bounds.right - StringWidth(update_text), ceilf(fontHeight.ascent)));
	}

	SetFont(be_plain_font);
	SetHighColor(high);

	const char *min_label = MinLimitLabel();
	if (min_label)
	{
		be_control_look->DrawLabel(view, min_label, base, flags,
								   BPoint(0.0f, bounds.bottom - fontHeight.descent));
	}

	const char *max_label = MaxLimitLabel();
	if (max_label)
	{
		be_control_look->DrawLabel(view, max_label, base, flags,
								   BPoint(bounds.right - StringWidth(max_label), bounds.bottom - fontHeight.descent));
	}

	if (fMidpointLabel.Length() > 0)
	{
		//be_control_look->DrawLabel(view, fMidpointLabel.String(), base, flags,
		//						   BPoint(0.5f*bounds.Width() - 0.5f*StringWidth(fMidpointLabel.String()), bounds.bottom - fontHeight.descent));
		SetHighColor({128, 128, 128, 255});
		DrawString(fMidpointLabel.String(), BPoint(0.5f*bounds.Width() - 0.5f*StringWidth(fMidpointLabel.String()), bounds.bottom - fontHeight.descent));
		SetHighColor(high);

	}
}

