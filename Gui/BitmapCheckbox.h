/*	PROJECT:		Medo
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2019-2021, ZenYes Pty Ltd
	DESCRIPTION:	BBitmap checkbox
*/

#ifndef BITMAP_CHECK_BOX_H
#define BITMAP_CHECK_BOX_H

#ifndef _CONTROL_H
#include <interface/Control.h>
#endif

class BBitmap;

class BitmapCheckbox : public BControl
{
public:
			BitmapCheckbox(BRect frame, const char *name, BBitmap *off, BBitmap *on, BMessage *msg, uint32 resizingMode = B_FOLLOW_LEFT_TOP, uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);
			~BitmapCheckbox() override;

	void	Draw(BRect frame) override;
	void	MouseDown(BPoint where) override;
	void	SetState(bool on);

private:
	BBitmap		*fBitmapOff;
	BBitmap		*fBitmapOn;
};

#endif //#ifndef BITMAP_CHECK_BOX_H
