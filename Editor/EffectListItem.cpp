/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effects item
 */

#include <cassert>

#include <interface/View.h>
#include <interface/ListItem.h>
#include <interface/Bitmap.h>
#include <support/String.h>
#include <ControlLook.h>

#ifndef _GRAPHICS_DEFS_H
#include <interface/GraphicsDefs.h>
#endif

#include "EffectNode.h"
#include "EffectListItem.h"
#include "Theme.h"

/*	FUNCTION:		EffectListItem :: EffectListItem
	ARGS:			effect_node
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
EffectListItem :: EffectListItem(EffectNode *effect_node)
	: fEffectNode(effect_node), fBitmap(nullptr)
{
	assert(effect_node != nullptr);
		
	fBitmap = fEffectNode->GetIcon();
}

/*	FUNCTION:		EffectListItem :: ~EffectListItem
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
EffectListItem :: ~EffectListItem()
{
	delete fBitmap;
}


/*	FUNCTION:		EffectListItem :: Update
	ARGS:			parent
					font
	RETURN:			n/a
	DESCRIPTION:	Hook called when item added to view
*/
void EffectListItem :: Update(BView *parent, const BFont *font)
{
	SetWidth(EffectNode::kThumbnailWidth + 3*be_control_look->DefaultLabelSpacing() + font->StringWidth(fEffectNode->GetTextB(0)));
	SetHeight(EffectNode::kThumbnailHeight + 2*be_control_look->DefaultLabelSpacing());
	
	font_height h;
	font->GetHeight(&h);
	fBaselineOffset = 0.5f*EffectNode::kThumbnailHeight;
}

/*	FUNCTION:		EffectListItem :: DrawItem
	ARGS:			parent
					frame
					erase_bg
	RETURN:			n/a
	DESCRIPTION:	Draw item
*/
void EffectListItem :: DrawItem(BView *parent, BRect frame, bool erase_bg)
{
	rgb_color lowColor = parent->LowColor();
	erase_bg = true;
	
	if (IsSelected() || erase_bg)
	{
		rgb_color color;
		if (IsSelected())
			color = Theme::GetUiColour(UiColour::eListSelection);
		else
			color = parent->ViewColor();
				
		parent->SetLowColor(color);
		parent->FillRect(frame, B_SOLID_LOW);
	}
	else
		parent->SetLowColor(parent->ViewColor());
	
	const float offset = be_control_look->DefaultLabelSpacing();
	parent->MovePenTo(offset, offset);
	if (fBitmap)
	{	
		parent->DrawBitmap(fBitmap, BRect(frame.left + offset, 
										frame.top + offset, 
										frame.left + EffectNode::kThumbnailWidth + offset, 
										frame.top + EffectNode::kThumbnailHeight + offset));
	}

	parent->SetHighColor(Theme::GetUiColour(UiColour::eListText));

    BString text_a(fEffectNode->GetTextA(0));
    be_plain_font->TruncateString(&text_a, B_TRUNCATE_MIDDLE, frame.Width() - (EffectNode::kThumbnailWidth + 2*offset));
    BString text_b(fEffectNode->GetTextB(0));
    be_plain_font->TruncateString(&text_b, B_TRUNCATE_MIDDLE, frame.Width() - (EffectNode::kThumbnailWidth + 2*offset));

    parent->MovePenTo(EffectNode::kThumbnailWidth + frame.left + 2*offset, frame.top + offset + fBaselineOffset);
	parent->DrawString(text_a);
	parent->MovePenTo(EffectNode::kThumbnailWidth + frame.left + 2*offset, frame.top + offset + fBaselineOffset + be_plain_font->Size());
	parent->DrawString(text_b);
	parent->SetLowColor(lowColor);	
}



