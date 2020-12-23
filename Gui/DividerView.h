/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Divider View
 */

#ifndef DIVIDER_VIEW_H_
#define DIVIDER_VIEW_H_

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class BCursor;

class DividerView : public BView
{
public:
				DividerView(BRect frame, BMessage *msg);
				~DividerView() override;
		
	void		Draw(BRect frame) override;
	void		MouseMoved(BPoint where, uint32 code, const BMessage *dragMessage) override;
	void		MouseDown(BPoint point) override;
	void		MouseUp(BPoint point) override;

private:
	BMessage	*fMessage;
	BCursor		*fResizeCursor;

	enum STATE {STATE_IDLE, STATE_CAN_RESIZE, STATE_RESIZING};
	STATE		fState;
};

#endif	//#ifndef DIVIDER_VIEW_H_
