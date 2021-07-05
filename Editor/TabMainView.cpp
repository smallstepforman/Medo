/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Tab main view
 */
 
#include <cstdio>
 
#include <InterfaceKit.h>

#include "TabMainView.h"
#include "Project.h"
#include "EffectsTab.h"
#include "TextTab.h"
#include "Gui/ListViewToolTip.h"
#include "Language.h"
#include "SourceListView.h"
#include "MediaSource.h"
#include "EffectNode.h"

enum kMsgSelectTab
{
	kMsgSelectedSource		= 'mts1',
	kMsgSelectedEffects,
	kMsgSelectedText,
};

//********************************

/*	FUNCTION:		TabMainView :: TabMainView
	ARGS:			tab_frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
TabMainView :: TabMainView(BRect tab_frame)
	: BTabView(tab_frame, "TabView", B_WIDTH_FROM_WIDEST, B_FOLLOW_NONE)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	auto text_func = [](BListItem *item){return static_cast<SourceListItem *>(item)->GetMediaSource()->GetFilename().String();};
	
	//	TAB_SOURCE
	fSourceView = new SourceListView(BRect(tab_frame.left, tab_frame.top, tab_frame.right, tab_frame.bottom - TabHeight()), "SourceTab", text_func);
	fSourceScrollView = new BScrollView(GetText(TXT_TAB_MEDIA_SOURCES), fSourceView, B_FOLLOW_LEFT_TOP, 0, false, true);
	AddTab(fSourceScrollView);
	fSourceScrollView->ScrollBar(B_VERTICAL)->SetRange(0.0f, 0.0f);
	
	//	TAB_EFFECTS
	fEffectsTab = new EffectsTab(BRect(tab_frame.left, tab_frame.top, tab_frame.right, tab_frame.bottom - TabHeight()));
	AddTab(fEffectsTab);

	//	TAB TEXT
	fTextTab = new TextTab(BRect(tab_frame.left, tab_frame.top, tab_frame.right, tab_frame.bottom - TabHeight()), fEffectsTab);
	AddTab(fTextTab);
		
	Select(0);

//	SetViewColor(fSourceView->ViewColor());
}

/*	FUNCTION:		TabMainView :: ~TabMainView
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
TabMainView :: ~TabMainView()
{
}

/*	FUNCTION:		TabMainView :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when attached to window
*/
void TabMainView :: AttachedToWindow()
{
	fSourceView->SetTarget(this, Window());
}

/*	FUNCTION:		TabMainView :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void TabMainView :: FrameResized(float width, float height)
{
	const float tab_height = TabHeight();
	float scale = be_plain_font->Size()/12.0f;

	fSourceView->ResizeTo(width - (scale*B_V_SCROLL_BAR_WIDTH) - 4, height - tab_height);
	fSourceScrollView->ResizeTo(width, height - tab_height);
	fSourceScrollView->MoveTo(0, -2);
	
	fEffectsTab->ResizeTo(width - 4, height - tab_height);
	fEffectsTab->MoveTo(2, -2);

	fTextTab->ResizeTo(width - 4, height - tab_height);
	fTextTab->MoveTo(2, -2);
}

/*	FUNCTION:		TabMainView :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void TabMainView :: Draw(BRect frame)
{
	const float kTabHeight = TabHeight();

	SetHighColor(fSourceView->ViewColor());
	BRect fill_rect = frame;
	fill_rect.top = kTabHeight;
	fill_rect.bottom = kTabHeight + 2;
	FillRect(fill_rect);

	BTabView::Draw(frame);
}

/*	FUNCTION:		TabMainView :: Select
	ARGS:			tab
	RETURN:			n/a
	DESCRIPTION:	Hook function called when tab selected
*/
void TabMainView :: Select(int32 tab)
{
	if (Window() && !Window()->IsActive())
		Window()->Activate();

	BRect frame = Bounds();
	frame.bottom -= TabHeight();

	BTabView::Select(tab);
	switch (tab)
	{
		case TAB_SOURCE:
			//fSourceView->ResizeTo(frame.Width() - (2*B_V_SCROLL_BAR_WIDTH), frame.Height());
			//fSourceScrollView->ResizeTo(frame.Width() - 4, frame.Height());
			break;

		case TAB_EFFECTS:
			fEffectsTab->TabSelected();
			fEffectsTab->ResizeTo(frame.Width(), frame.Height());
			fEffectsTab->MoveTo(2, -2);
			break;

		case TAB_TEXT:
			fTextTab->TabSelected();
			fTextTab->ResizeTo(frame.Width(), frame.Height());
			fTextTab->MoveTo(2, -2);
			break;

		default:
			assert(0);
	}
}

/*	FUNCTION:		TabMainView :: SelectEffect
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Select effect (in appropriate tab)
*/
void TabMainView :: SelectEffect(MediaEffect *effect)
{
	if (effect && (effect->mEffectNode->GetEffectGroup() == EffectNode::EFFECT_TEXT))
	{
		fTextTab->SelectEffect(effect);
		Select(TAB_TEXT);
	}
	else
	{
		fEffectsTab->SelectEffect(effect);
		Select(TAB_EFFECTS);
	}
}

