/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Splitter View
 */

#include <cstdio>

#include <app/Application.h>
#include <app/Cursor.h>
#include <interface/Window.h>
#include <interface/View.h>

#include "DividerView.h"

/*	FUNCTION:		DividerView :: DividerView
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
DividerView :: DividerView(BRect frame, BMessage *msg)
	: BView(frame, "DividerView", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS)
{
	fMessage = msg;
	fResizeCursor = new BCursor(B_CURSOR_ID_RESIZE_NORTH_SOUTH);
	fState = STATE_IDLE;
}

/*	FUNCTION:		DividerView :: ~DividerView
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
DividerView :: ~DividerView()
{
	delete fResizeCursor;

	if (fState != STATE_IDLE)
		be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
}

/*	FUNCTION:		DividerView :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Draw divider
*/
void DividerView :: Draw(BRect frame)
{
	SetHighColor(160, 160, 160);
	FillRect(frame);
}

/*	FUNCTION:		DividerView :: MouseMoved
	ARGS:			where
					transit
					dragMessage
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse moved
*/
void DividerView :: MouseMoved(BPoint where, uint32 transit, const BMessage *dragMessage)
{
	if (fState == STATE_RESIZING)
	{
		fMessage->SetPoint("point", where);
		((BLooper *)Window())->PostMessage(fMessage);
		return;
	}

	switch (transit)
	{
		case B_ENTERED_VIEW:
			be_app->SetCursor(fResizeCursor);
			fState = STATE_CAN_RESIZE;
			break;

		case B_EXITED_VIEW:
			be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
			fState = STATE_IDLE;
			break;

		case B_INSIDE_VIEW:
			break;

		default:
			break;
	}
}

/*	FUNCTION:		DividerView :: MouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse down
*/
void DividerView :: MouseDown(BPoint point)
{
	printf("DividerView::MouseDown() %d\n", fState);

	if (fState == STATE_CAN_RESIZE)
	{
		fState = STATE_RESIZING;
		SetMouseEventMask(B_POINTER_EVENTS,	B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY | B_SUSPEND_VIEW_FOCUS);
	}
}

/*	FUNCTION:		DividerView :: MouseUp
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse up
*/
void DividerView :: MouseUp(BPoint point)
{
	if (fState != STATE_IDLE)
	{

		fState = STATE_IDLE;
		be_app->SetCursor(B_HAND_CURSOR);
		SetMouseEventMask(B_POINTER_EVENTS, 0);
	}
	printf("DividerView::MouseUp() %d\n", fState);
}
		
