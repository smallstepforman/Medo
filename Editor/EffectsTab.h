/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Tab effect
 */

#ifndef _EFFECTS_TAB_H_
#define _EFFECTS_TAB_H_

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class BScrollView;
class BOutlineListView;

class EffectNode;
class MediaEffect;
class EffectsWindow;

//==========================
class EffectsTab : public BView
{
	BScrollView			*fScrollView;
	BOutlineListView	*fOutlineListView;
	BMessage			*fMsgDragDrop;
	BMessage			*fMsgWindowNotification;
	EffectsWindow		*fEffectsWindow;
	BMessage			*fMsgEffectsWindow;

public:
					EffectsTab(BRect tab_frame);
					~EffectsTab()								override;
	void			FrameResized(float width, float height)		override;

	void			TabSelected();
	void			SelectionChanged();
	void			SelectEffect(MediaEffect *effect);

	void			UpdateEffectWindow(EffectNode *node, MediaEffect *effect);

	void			DragInitiated(const int index);
};

#endif	//#ifndef _EFFECTS_TAB_H_
