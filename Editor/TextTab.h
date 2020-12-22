/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Text tab
 */

#ifndef _TEXT_TAB_H_
#define _TEXT_TAB_H_

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class BScrollView;
class BListView;
class EffectsTab;
class MediaEffect;

//==========================
class TextTab : public BView
{
	BScrollView		*fScrollView;
	BListView		*fListView;
	BMessage		*fMsgWindowNotification;
	EffectsTab		*fEffectsTab;
	BMessage		*fMsgDragDrop;
	
public:
					TextTab(BRect tab_frame, EffectsTab *effects_tab);
					~TextTab();
			
	void			FrameResized(float width, float height) override;
	
	void			SelectionChanged();
	void			SelectEffect(MediaEffect *effect);
	void			TabSelected();
	void			DragInitiated(int index);
};

#endif	//#ifndef _TEXT_TAB_H_
