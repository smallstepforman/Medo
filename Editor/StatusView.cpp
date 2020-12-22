/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Status view
 */
 
#include <interface/View.h>
#include <interface/StringView.h>

#include "StatusView.h"


/*	FUNCTION:		StatusView :: StatusView
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
StatusView :: StatusView(BRect frame)
    : BView(frame, "StatusView", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_FRAME_EVENTS | B_DRAW_ON_CHILDREN)
{
	frame = Bounds();
    frame.bottom += 0.33f*be_plain_font->Size();
	
	SetViewColor(B_TRANSPARENT_COLOR);
		
    fStringView = new BStringView(frame, "StatusText", "Status");
    AddChild(fStringView);
}

/*	FUNCTION:		StatusView :: SetText
	ARGS:			text
	RETURN:			n/a
	DESCRIPTION:	Set text
*/
void StatusView :: SetText(const char *text)
{
	fStringView->SetText(text);	
}

void StatusView :: Draw(BRect frame)
{
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(frame);	
}

