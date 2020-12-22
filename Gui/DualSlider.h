/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	GUI Dual Sliders
 */

#ifndef DUAL_SLIDER_H
#define DUAL_SLIDER_H

#ifndef _VIEW_H
#include <interface/View.h>
#endif
#ifndef _INVOKER_H
#include <app/Invoker.h>
#endif

class BSlider;
class BitmapCheckbox;
class MouseInterceptView;

//=======================
class DualSlider : public BView, public BInvoker
{
public:
				DualSlider(BRect frame, const char* name, const char* label, BMessage* message, int32 minValue, int32 maxValue, const char *label_left, const char *label_right);
				~DualSlider()	override;

	void		AttachedToWindow()				override;
	void		MessageReceived(BMessage *msg)	override;
	void		MouseDown(BPoint where)			override;
	void		MouseMoved(BPoint where, uint32 code, const BMessage *)	override;
	void		MouseUp(BPoint where)			override;
	void		DrawAfterChildren(BRect frame)	override;

	void		SetValue(int index, int value);
	int			GetValue(int index);
	void		SetEnabled(const bool enabled);

private:
	BSlider				*fSliders[2];
	BitmapCheckbox		*fCheckboxLinked;
	MouseInterceptView	*fMouseInterceptView;
	bool				fTrackingLinked;
	int					fMouseTracking;
};

#endif	//#ifndef DUAL_SLIDER_H
