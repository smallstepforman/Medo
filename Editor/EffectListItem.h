/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Effects item
 */
 
#ifndef _EFFECT_LIST_ITEM_H_
#define _EFFECT_LIST_ITEM_H_

#ifndef _LIST_ITEM_H
#include <interface/ListItem.h>
#endif

class BBitmap;
class EffectNode;

//=======================
class EffectListItem : public BListItem
{
public:
	
					EffectListItem(EffectNode *effects_node);
					~EffectListItem();
			
	void			DrawItem(BView *parent, BRect frame, bool erase_bg=false);
	void			Update(BView *parent, const BFont *font);
	EffectNode		*GetEffectNode() {return fEffectNode;}
	
private:
	EffectNode		*fEffectNode;
	BBitmap			*fBitmap;
	float			fBaselineOffset;
};

#endif	//#ifndef _EFFECT_LIST_ITEM_H_
