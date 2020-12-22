/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Tab effects
 */
 
#include <cstdio>
#include <cassert>

#include <interface/View.h>
#include <interface/Window.h>
#include <interface/ScrollView.h>
#include <interface/OutlineListView.h>
#include <interface/Screen.h>
#include <interface/Bitmap.h>
#include <ControlLook.h>

#include "MedoWindow.h"
#include "Project.h"
#include "EffectsManager.h"
#include "EffectsTab.h"
#include "EffectsWindow.h"
#include "EffectListItem.h"
#include "EffectNode.h"
#include "Language.h"
#include "TimelineEdit.h"
#include "Theme.h"

EffectsManager	*gEffectsManager = nullptr;

/**************************************
	DraggerOutlineListView
***************************************/
class DraggerOutlineListView : public BOutlineListView
{
	EffectsTab		*fParent;
public:
	DraggerOutlineListView(BRect frame, const char *name, EffectsTab *parent)
		: BOutlineListView(frame, name), fParent(parent)
	{ }

	bool InitiateDrag(BPoint point, int32 index, bool wasSelected) override
	{
		printf("InitiateDrag() index=%d, selected=%d\n", index, wasSelected);
		fParent->DragInitiated(FullListCurrentSelection());
		return true;
	}

	void MouseDown(BPoint point) override
	{
		if (!Window()->IsActive())
			Window()->Activate();
		BOutlineListView::MouseDown(point);
	}

	void KeyDown(const char* bytes, int32 numBytes) override
	{
		BMessage *msg = Window()->CurrentMessage();
		Window()->PostMessage(msg);
	}

	//	Modify BOutlineListView::DrawLatch() to use theme colour
	void DrawLatch(BRect itemRect, int32 level, bool collapsed, bool highlighted, bool misTracked) override
	{
		BRect latchRect(LatchRect(itemRect, level));
		rgb_color base = Theme::GetUiColour(UiColour::eListOutlineTriangle);	//	was ui_color(B_PANEL_BACKGROUND_COLOR)
		int32 arrowDirection = collapsed ? BControlLook::B_RIGHT_ARROW : BControlLook::B_DOWN_ARROW;
		float tintColor = B_DARKEN_4_TINT;
		if (base.red + base.green + base.blue <= 128 * 3)
		{
			tintColor = B_LIGHTEN_2_TINT;
		}
		be_control_look->DrawArrowShape(this, latchRect, itemRect, base, arrowDirection, 0, tintColor);
	}

	//	Modify BOutlineListView::DrawItem() to use theme colour
	void DrawItem(BListItem* item, BRect itemRect, bool complete) override
	{
		//	BOutlineListView::DrawItem()
		if (item->OutlineLevel() == 0)
		{
			DrawLatch(itemRect, item->OutlineLevel(), !item->IsExpanded(), item->IsSelected() || complete, false);
		}
		itemRect.left += LatchRect(itemRect, item->OutlineLevel()).right;

		//	BListView::DrawItem()
		if (!item->IsEnabled())
		{
			rgb_color textColor = Theme::GetUiColour(UiColour::eListText);	//	was ui_color(B_LIST_ITEM_TEXT_COLOR)
			rgb_color disabledColor;
			if (textColor.red + textColor.green + textColor.blue > 128 * 3)
				disabledColor = tint_color(textColor, B_DARKEN_2_TINT);
			else
				disabledColor = tint_color(textColor, B_LIGHTEN_2_TINT);
			SetHighColor(disabledColor);
		}
		else if (item->IsSelected())
			SetHighColor(Theme::GetUiColour(UiColour::eListSelection));	//	was ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR)
		else
			SetHighColor(Theme::GetUiColour(UiColour::eListText));	//	was ui_color(B_LIST_ITEM_TEXT_COLOR)

		//	trick BStringItem::DrawItem() to not draw selected background, since it uses ui_color(B_LIST_SELECTED_BACKGROUND_COLOR
		if (item->OutlineLevel() == 0)
		{
			if (item->IsSelected() || complete)
			{
				item->Deselect();
				SetHighColor(Theme::GetUiColour(UiColour::eListSelection));
				FillRect(itemRect);
				SetHighColor(Theme::GetUiColour(UiColour::eListText));
				item->DrawItem(this, itemRect, false);
				item->Select();
			}
			else
				item->DrawItem(this, itemRect, complete);
		}
		else
			item->DrawItem(this, itemRect, complete);
	}
};

/*	FUNCTION:		EffectsTab :: EffectsTab
	ARGS:			tab_frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
EffectsTab :: EffectsTab(BRect tab_frame)
	: BView(tab_frame, GetText(TXT_TAB_EFFECTS), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	BScreen screen;

	fEffectsWindow = new EffectsWindow(BRect(screen.Frame().right - 740, 64, screen.Frame().right, 64 + 700));
	fMsgEffectsWindow = new BMessage(EffectsWindow::eMsgShowEffect);
	fMsgEffectsWindow->AddPointer("EffectNode", nullptr);
	fMsgEffectsWindow->AddPointer("MediaEffect", nullptr);
	fEffectsWindow->Show();	//	Need to start looper
	fEffectsWindow->Hide();

	assert(gEffectsManager == nullptr);
	gEffectsManager = new EffectsManager(BRect(0, 0, be_plain_font->Size() > 16 ? 740 : 640, 700));
	
	BRect bframe = Bounds();
	fOutlineListView = new DraggerOutlineListView(BRect(bframe.left, bframe.top, bframe.right, bframe.bottom), "EffectsListView", this);
	fMsgWindowNotification = new BMessage(MedoWindow::eMsgActionTabEffectSelected);
	fOutlineListView->SetSelectionMessage(fMsgWindowNotification);		//	BOutlineListView takes ownership of fMsgWindowNotification
	fScrollView = new BScrollView("EffectsScrollView", fOutlineListView, B_FOLLOW_LEFT_TOP, 0, false, true);
	AddChild(fScrollView);
	fScrollView->ScrollBar(B_VERTICAL)->SetRange(0.0f, 0.0f);

	fOutlineListView->SetViewColor(Theme::GetUiColour(UiColour::eListBackground));

	//	Populate items
	std::vector<BStringItem *>	groups;
	std::vector<BListItem *> sort_superitems;
	for (int g=0; g < EffectNode::NUMBER_EFFECT_GROUPS - 1; g++)
	{
		BStringItem *group = new BStringItem(GetText((LANGUAGE_TEXT)(TXT_TAB_EFFECTS_SPATIAL + g)));
		groups.push_back(group);
		fOutlineListView->AddItem(group);
		fOutlineListView->Collapse(fOutlineListView->ItemAt(g));

		sort_superitems.push_back(group);
	}
	for (auto i : gEffectsManager->fEffectNodes)
	{
		if (i->GetEffectGroup() != EffectNode::EFFECT_TEXT)
		{
			//printf("EffectsTab() Adding Effect %s [%d], priority=%d\n", i->GetEffectName(), i->GetEffectGroup(), i->GetEffectListPriority());
			fOutlineListView->AddUnder(new EffectListItem(i), groups[i->GetEffectGroup()]);
		}
	}

	for (auto i : sort_superitems)
	{
		fOutlineListView->SortItemsUnder(i, true, [](const BListItem *a, const BListItem *b) -> int
			{
				EffectNode *node_a = ((EffectListItem *)a)->GetEffectNode();
				EffectNode *node_b= ((EffectListItem *)b)->GetEffectNode();
				if (node_a->GetEffectListPriority() == node_b->GetEffectListPriority())
					return strcmp(node_a->GetEffectName(), node_b->GetEffectName());
				else
					return node_b->GetEffectListPriority() - node_a->GetEffectListPriority();
			});
	}

	//	Drag/drop message
	fMsgDragDrop = new BMessage(TimelineEdit::eMsgDragDropEffect);
}

/*	FUNCTION:		EffectsTab :: EffectsTab
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
EffectsTab :: ~EffectsTab()
{
	delete fMsgDragDrop;
	delete fMsgEffectsWindow;
	fEffectsWindow->Terminate();	//	Remove child view since EffectsManager owns all EffectNodes
	delete gEffectsManager;
	//delete fMsgWindowNotification;	//	BOutlineListView takes ownership of fMsgWindowNotification
}

/*	FUNCTION:		EffectsTab :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void EffectsTab :: FrameResized(float width, float height)
{
	float scale = be_plain_font->Size()/12.0f;
	fOutlineListView->ResizeTo(width - scale*B_V_SCROLL_BAR_WIDTH - 4, height);
	fScrollView->ResizeTo(width, height);
}

/*	FUNCTION:		EffectsTab :: UpdateEffectWindow
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Update effect window to show effect
*/
void EffectsTab :: UpdateEffectWindow(EffectNode *node, MediaEffect *effect)
{
	fMsgEffectsWindow->ReplacePointer("EffectNode", node);
	fMsgEffectsWindow->ReplacePointer("MediaEffect", effect);
	fEffectsWindow->PostMessage(fMsgEffectsWindow);
}

/*	FUNCTION:		EffectsTab :: TabSelected
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when tab selected
*/
void EffectsTab :: TabSelected()
{
	//printf("EffectsTab::TabSelected()\n");
}

/*	FUNCTION:		EffectsTab :: SelectionChanged
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when user selects list view item
*/
void EffectsTab :: SelectionChanged()
{
	int32 index = fOutlineListView->FullListCurrentSelection();
	EffectListItem *item = dynamic_cast<EffectListItem *>(fOutlineListView->FullListItemAt(index));
	if (item)
	{
		EffectNode *effect = item->GetEffectNode();
		UpdateEffectWindow(effect, nullptr);
	}
}

/*	FUNCTION:		EffectsTab :: DragInitiated
	ARGS:			index
	RETURN:			n/a
	DESCRIPTION:	Called by DraggerOutlineListView
*/
void EffectsTab :: DragInitiated(const int index)
{
	EffectListItem *item = dynamic_cast<EffectListItem *>(fOutlineListView->FullListItemAt(index));
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

/*	FUNCTION:		EffectsTab :: SelectEffect
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called by TimelineEdit when effect selected
*/
void EffectsTab :: SelectEffect(MediaEffect *effect)
{
	if (!effect)
	{
		UpdateEffectWindow(nullptr, nullptr);
		return;
	}

	for (int i=0; i < fOutlineListView->FullListCountItems(); i++)
	{
		EffectListItem *item = (EffectListItem *) fOutlineListView->FullListItemAt(i);
		if (item->GetEffectNode() == effect->mEffectNode)
		{
			fOutlineListView->DeselectAll();
			if (fOutlineListView->Superitem(item))
				fOutlineListView->Expand(fOutlineListView->Superitem(item));
			fOutlineListView->Select(fOutlineListView->IndexOf(item));

			UpdateEffectWindow(effect->mEffectNode, effect);
			return;
		}
	}
	printf("EffectsTab :: SelectEffect() Failed to select %s", effect->mEffectNode->GetEffectName());
	assert(0);
}
