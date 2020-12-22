/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	GUI Value Slider
 */

#ifndef VALUE_SLIDER_H
#define VALUE_SLIDER_H

#ifndef _SLIDER_H
#include <interface/Slider.h>
#endif

#ifndef _B_STRING_H
#include <support/String.h>
#endif

class BMessage;

//=======================
class ValueSlider : public BSlider
{
public:
				ValueSlider(BRect frame, const char* name, const char* label, BMessage* message, int32 minValue, int32 maxValue);
	const char	*UpdateText() const		override;
	void		DrawText()				override;

	void		UpdateTextValue(const float value);
	void		SetMidpointLabel(const char *label);
	void		SetFloatingPointPrecision(const int precision = 1)	{fFloatPrecision = precision;}
	void		SetValueUpdateText(const float value);

private:
	BString		fText;
	BString		fMidpointLabel;
	int			fFloatPrecision;
};

#endif	//#ifndef VALUE_SLIDER_H
