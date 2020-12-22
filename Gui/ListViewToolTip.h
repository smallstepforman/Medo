/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	List view with tool tip
 */

#ifndef LIST_VIEW_TOOLTIP_H
#define LIST_VIEW_TOOLTIP_H

#ifndef _LIST_VIEW_H
#include <interface/ListView.h>
#endif

#include <functional>

class ListViewToolTip : public BListView
{
public:
					ListViewToolTip(BRect frame, const char *name, std::function<const char *(BListItem *)> func);
	virtual			~ListViewToolTip() {}
	virtual void	MouseMoved(BPoint where, uint32 code, const BMessage *dragMessage)	override;

	virtual	bool	InitiateDrag(BPoint point, int32 index, bool wasSelected) override {return false;}

private:
    int     fToolTipIndex;
    std::function<const char * (BListItem *)> fTextFunction;
};

#endif	//#ifndef LIST_VIEW_TOOLTIP_H
