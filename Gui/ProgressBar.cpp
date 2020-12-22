/*	PROJECT:		Medo
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2019, ZenYes Pty Ltd
	DESCRIPTION:	Progress bar
*/

#include <cstdio>

#include <interface/View.h>

#include "ProgressBar.h"

/*	FUNCTION:		ProgressBar :: ProgressBar
	ARGS:			frame
					name
					resizingMode
					flags
	RETURN:			n/a
	DESCRIPTION:	constructor
*/
ProgressBar :: ProgressBar(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags)
{
	fPercentage = 0.0f;
}

/*	FUNCTION:		ProgressBar :: ~ProgressBar
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ProgressBar :: ~ProgressBar()
{ }

/*	FUNCTION:		ProgressBar :: SetValue
	ARGS:			where
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
void ProgressBar :: SetValue(float percentage)
{
	if (percentage < 0.0f)
		percentage = 0.0f;
	if (percentage > 1.0f)
		percentage = 1.0f;
	fPercentage = percentage;
	Invalidate();
}

/*	FUNCTION:		ProgressBar :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw progress bar
*/
void ProgressBar :: Draw(BRect frame)
{
	BRect bounds = Bounds();
	float right = bounds.right;
	float seperator = right*fPercentage;
	bounds.right = seperator;
	SetHighColor(make_color(160, 160, 255, 255));
	FillRect(bounds);
	bounds.left = seperator;
	bounds.right = right;
	SetHighColor(make_color(216, 216, 216));
	FillRect(bounds);

	//	Frame
	bounds.left = 0;
	SetHighColor(make_color(255, 255, 255));
	float pen_size = PenSize();
	SetPenSize(2.0f);
	StrokeRect(bounds);
	SetPenSize(pen_size);
}
