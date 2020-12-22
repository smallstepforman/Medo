/*	PROJECT:		Medo
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2019, ZenYes Pty Ltd
	DESCRIPTION:	Linked Spinners
*/

#ifndef GUI_LINKED_SPINNER_H
#define GUI_LINKED_SPINNER_H

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class Spinner;
class BitmapCheckbox;

class LinkedSpinners : public BView
{
public:
					LinkedSpinners(BRect frame, const char* name, const char *start_label, const char *end_label, BMessage *msg,
								   uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP, uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
					~LinkedSpinners() override;

	void			AttachedToWindow() override;
	void			MessageReceived(BMessage* message) override;

	const float		GetStartValue() const;
	const float		GetEndValue() const;
	const bool		GetLinked() const;

	void			SetStartValue(const float value);
	void			SetEndValue(const float value);
	void			SetLinked(const bool linked);
	void			SetRange(const float min, const float max);
	void			SetSteps(const float steps);
	
private:
	Spinner				*fSpinnerStart;
	Spinner				*fSpinnerEnd;
	BitmapCheckbox		*fCheckboxLinked;
	BMessage			*fMessageNotification;
};

#endif	//#ifndef GUI_LINKED_SPINNER_H
