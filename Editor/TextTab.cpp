/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Background Tab
 */
 
#include <cstdio>
#include <cassert>
#include <algorithm>

#include <interface/View.h>
#include <interface/ScrollView.h>
#include <interface/OutlineListView.h>
#include <interface/Bitmap.h>

#include "MedoWindow.h"
#include "Project.h"
#include "EffectNode.h"
#include "EffectListItem.h"
#include "TextTab.h"
#include "Language.h"
#include "EffectsManager.h"
#include "EffectsTab.h"
#include "TimelineEdit.h"
#include "Theme.h"

/****************************
	DraggerListView
*****************************/
class DraggerListView : public BListView
{
	TextTab		*fParent;
public:
	DraggerListView(BRect frame, const char *name, TextTab *parent)
		: BListView(frame, name), fParent(parent)
	{ }

	bool InitiateDrag(BPoint point, int32 index, bool wasSelected) override
	{
		fParent->DragInitiated(index);
		return true;
	}

	void MouseDown(BPoint point) override
	{
		if (!Window()->IsActive())
			Window()->Activate();
		BListView::MouseDown(point);
	}

	void KeyDown(const char* bytes, int32 numBytes) override
	{
		BMessage *msg = Window()->CurrentMessage();
		Window()->PostMessage(msg);
	}
};

static int sort_text_nodes(const void *a, const void *b)
{
	EffectNode *node_a = ((EffectListItem *)a)->GetEffectNode();
	EffectNode *node_b= ((EffectListItem *)b)->GetEffectNode();
	if (node_a->GetEffectListPriority() == node_b->GetEffectListPriority())
		return strcmp(node_a->GetEffectName(), node_b->GetEffectName());
	else
		return node_b->GetEffectListPriority() - node_a->GetEffectListPriority();
}

/*	FUNCTION:		TextTab :: TextTab
	ARGS:			tab_frame
					preview_frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
TextTab :: TextTab(BRect tab_frame, EffectsTab *effects_tab)
	: BView(tab_frame, GetText(TXT_TAB_TEXT), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS)
{
	fEffectsTab = effects_tab;

	BRect bframe = Bounds();
	fListView = new DraggerListView(BRect(bframe.left, bframe.top, bframe.right, bframe.bottom), "EffectsListView", this);
	fMsgWindowNotification = new BMessage(MedoWindow::eMsgActionTabTextSelected);
	fListView->SetSelectionMessage(fMsgWindowNotification);		//	BListView takes ownership of fMsgWindowNotification
	fScrollView = new BScrollView("TextTabScrollView", fListView, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);
	AddChild(fScrollView);
	fScrollView->ScrollBar(B_VERTICAL)->SetRange(0.0f, 0.0f);

	fListView->SetViewColor(Theme::GetUiColour(UiColour::eListBackground));

#if 0
	//	TODO investigate why sort doesn't work in this scenario
	std::vector<EffectListItem *> text_items;
	for (auto i : gEffectsManager->fEffectNodes)
	{
		if (i->GetEffectGroup() == EffectNode::EFFECT_TEXT)
			text_items.emplace_back(new EffectListItem(i));
	}
	std::sort(text_items.begin(), text_items.end(), [](const void *a, const void *b)->int
				{
					EffectNode *node_a = ((EffectListItem *)a)->GetEffectNode();
					EffectNode *node_b= ((EffectListItem *)b)->GetEffectNode();
					if (node_a->GetEffectListPriority() == node_b->GetEffectListPriority())
						return strcmp(node_a->GetEffectName(), node_b->GetEffectName());
					else
						return node_a->GetEffectListPriority() > node_b->GetEffectListPriority() ? -1 : 1;
				});
	for (auto &i : text_items)
		fListView->AddItem(i);
#else
	for (auto i : gEffectsManager->fEffectNodes)
	{
		if (i->GetEffectGroup() == EffectNode::EFFECT_TEXT)
			fListView->AddItem(new EffectListItem(i));
	}
#endif

	//	Drag/drop message
	fMsgDragDrop = new BMessage(TimelineEdit::eMsgDragDropEffect);
}

/*	FUNCTION:		TextTab :: ~TextTab
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
TextTab :: ~TextTab()
{
	delete fMsgDragDrop;
	//delete fMsgWindowNotification;	//	BListView takes ownership of fMsgWindowNotification
}

/*	FUNCTION:		TextTab :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void TextTab :: FrameResized(float width, float height)
{
	float scale = be_plain_font->Size()/12.0f;
	fListView->ResizeTo(width - (scale*B_V_SCROLL_BAR_WIDTH) - 4, height);
	fScrollView->ResizeTo(width, height);
}

/*	FUNCTION:		TextTab :: TabSelected
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when tab selected
*/
void TextTab :: TabSelected()
{
	printf("TextTab::TabSelected()\n");
	Window()->PostMessage(fMsgWindowNotification);
}

/*	FUNCTION:		TextTab :: SelectionChanged
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when user selects list view item
*/
void TextTab :: SelectionChanged()
{
	int32 index = fListView->CurrentSelection();
	EffectListItem *item = dynamic_cast<EffectListItem *>(fListView->ItemAt(index));
	if (item)
	{
		EffectNode *effect = item->GetEffectNode();
		printf("TextTab::SelectionChanged(%s)\n", effect->GetTextA(index));
		fEffectsTab->UpdateEffectWindow(effect, nullptr);
	}
	else
	{
		printf("TextTab::SelectionChanged() no item found\n");
	}
}

/*	FUNCTION:		TextTab :: SelectEffect
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called by TimelineEdit when effect selected
*/
void TextTab :: SelectEffect(MediaEffect *effect)
{
	printf("TextTab::SelectEffect()\n");

	for (int i=0; i < fListView->CountItems(); i++)
	{
		EffectListItem *item = (EffectListItem *) fListView->ItemAt(i);
		if (item->GetEffectNode() == effect->mEffectNode)
		{
			printf("SelectEffect() Found \n");

			fListView->Select(i);
			fEffectsTab->UpdateEffectWindow(effect->mEffectNode, effect);
			break;
		}
	}
}

/*	FUNCTION:		TextTab :: DragInitiated
	ARGS:			index
	RETURN:			n/a
	DESCRIPTION:	Called by DraggerListView
*/
void TextTab :: DragInitiated(int index)
{
	//int32 index = fListView->CurrentSelection();
	EffectListItem *item = dynamic_cast<EffectListItem *>(fListView->ItemAt(index));
	if (item)
	{
		EffectNode *effect = item->GetEffectNode();
		SetMouseEventMask(B_POINTER_EVENTS, 0);
		fMsgDragDrop->MakeEmpty();
		fMsgDragDrop->AddPointer("effect", effect);
		fMsgDragDrop->AddInt64("duration", TimelineEdit::kDefaultNewEffectDuration);
		float xoffset = 0.0f;
		fMsgDragDrop->AddFloat("xoffset", xoffset);
		BBitmap *icon = new BBitmap(effect->GetIcon());
		DragMessage(fMsgDragDrop, icon, BPoint(0.5f*icon->Bounds().Width(), 0.5f*icon->Bounds().Height()));
	}
}
