/*	PROJECT:		Medo
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2019-2021, ZenYes Pty Ltd
	DESCRIPTION:	BBitmap Checkbox
*/

#include <cassert>
#include <interface/Control.h>
#include <interface/Bitmap.h>

#include "BitmapCheckbox.h"

/*	FUNCTION:		BitmapCheckbox :: BitmapCheckbox
	ARGS:			frame
					name
	RETURN:			n/a
	DESCRIPTION:	constructor
*/
BitmapCheckbox :: BitmapCheckbox(BRect frame, const char *name, BBitmap *off, BBitmap *on, BMessage *msg, uint32 resizingMode, uint32 flags)
	: BControl(frame, name, nullptr, msg, resizingMode, flags)
{
	assert(on);
	assert(off);
	fBitmapOn = on;
	fBitmapOff = off;
}

/*	FUNCTION:		BitmapCheckbox :: ~BitmapCheckbox
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
BitmapCheckbox :: ~BitmapCheckbox()
{
	delete fBitmapOff;
	delete fBitmapOn;
}

/*	FUNCTION:		BitmapCheckbox :: MouseDown
	ARGS:			where
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
void BitmapCheckbox :: MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	int value = Value();
	SetValue(value ? 0 : 1);
	Invalidate();
	Invoke();
}

/*	FUNCTION:		BitmapCheckbox :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw checkbox
*/
void BitmapCheckbox :: Draw(BRect frame)
{
	if (Value())
		DrawBitmapAsync(fBitmapOn, frame);
	else
		DrawBitmapAsync(fBitmapOff, frame);
}

/*	FUNCTION:		BitmapCheckbox :: SetState
	ARGS:			on
	RETURN:			n/a
	DESCRIPTION:	Set state
*/
void BitmapCheckbox :: SetState(bool on)
{
	SetValue(on ? 1 : 0);
	Invalidate();
}
