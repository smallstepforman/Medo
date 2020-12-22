/*	PROJECT:		Medo
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2020-2021, ZenYes Pty Ltd
	DESCRIPTION:	Alpha colour control
*/

#ifndef ALPHA_COLOUR_CONTROL_H
#define ALPHA_COLOUR_CONTROL_H

#ifndef _INVOKER_H
#include <Invoker.h>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class BColorControl;
class AlphaSlider;
class BTextControl;

class AlphaColourControl : public BView, public BInvoker
{
public:
				AlphaColourControl(BPoint point, const char *name, BMessage *msg, uint32 resizingMode = B_FOLLOW_LEFT_TOP, uint32 flags = B_WILL_DRAW);
				~AlphaColourControl() override;
	void		AttachedToWindow() override;

	void		SetValue(rgb_color colour);
	rgb_color	ValueAsColor() const;
	void		SetEnabled(const bool enable);
	void		MessageReceived(BMessage *msg)	override;

private:
	BColorControl	*fColourControl;
	AlphaSlider		*fAlphaSlider;
	BTextControl	*fTextAlpha;
};

#endif //#ifndef ALPHA_COLOUR_CONTROL_H
