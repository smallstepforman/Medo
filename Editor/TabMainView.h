/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016
 *	DESCRIPTION:	Tab main view
 */

#ifndef _TAB_MAIN_VIEW_H_
#define _TAB_MAIN_VIEW_H_

#ifndef _TAB_VIEW_H
#include <interface/TabView.h>
#endif

class BScrollView;

class SourceListView;
class EffectsTab;
class TextTab;
class MediaEffect;

//=======================
class TabMainView : public BTabView
{
public:
					TabMainView(BRect tab_frame);
					~TabMainView() override;
	void			AttachedToWindow() override;
	void			FrameResized(float width, float height) override;
	void			Draw(BRect frame) override;

	SourceListView	*GetSourceListView() {return fSourceView;}
	EffectsTab		*GetEffectsTab() {return fEffectsTab;}
	TextTab			*GetTextTab() {return fTextTab;}
	void			SelectEffect(MediaEffect *effect);
	void			Select(int32 tab) override;
	
	enum {TAB_SOURCE, TAB_EFFECTS, TAB_TEXT};
					
private:
	SourceListView	*fSourceView;
	BScrollView     *fSourceScrollView;
	EffectsTab      *fEffectsTab;
	TextTab			*fTextTab;
};

#endif	//#ifndef _TAB_MAIN_VIEW_H_
