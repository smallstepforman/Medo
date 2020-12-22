/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	BListView with tool tip
 */

#include <interface/ListView.h>
#include "ListViewToolTip.h"

#include <private/interface/ToolTip.h>
 
/*	FUNCTION:		ListViewToolTip :: ListViewToolTip
    ARGS:			frame
                    name
    RETURN:			n/a
    DESCRIPTION:	constructor
*/
ListViewToolTip :: ListViewToolTip(BRect frame, const char *name, std::function<const char *(BListItem *)> func)
	: BListView(frame, name)
{
    fToolTipIndex = -1;
    fTextFunction = func;
}

/*	FUNCTION:		ListViewToolTip :: MouseMoved
    ARGS:			where
                    code
                    dragMessage
    RETURN:			n/a
    DESCRIPTION:	Hook function called when mouse moved
*/
void ListViewToolTip :: MouseMoved(BPoint where, uint32 code, const BMessage *dragMessage)
{
    int32 count = CountItems();
    if (count == 0)
        return;

    int index = 0;
    BRect itemFrame(0, 0, Bounds().right, -1);
    while (index < count)
    {
        BListItem *item = ItemAt(index);
        itemFrame.bottom = itemFrame.top + ceilf(item->Height()) - 1;
        if (itemFrame.Contains(where))
        {
            if (fToolTipIndex == index)
                break;

            fToolTipIndex = index;
            SetToolTip(fTextFunction(ItemAt(fToolTipIndex)));
            break;
        }
        itemFrame.top = itemFrame.bottom + 1;
        index++;
    }
    BListView::MouseMoved(where, code, dragMessage);

    if ((fToolTipIndex != index) && (fToolTipIndex >= 0))
    {
        SetToolTip("");
        fToolTipIndex = -1;
    }
}
