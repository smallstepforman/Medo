/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Node
 */

#include <InterfaceKit.h>

#include "EffectNode.h"
#include "Language.h"
#include "MedoWindow.h"
#include "Project.h"
#include "TimelineEdit.h"
#include "Theme.h"

MediaEffect *	EffectNode :: sCurrentMediaEffect = nullptr;
BMessage *		EffectNode :: sMsgEffectSelected = nullptr;

/****************************
	Drag drop control prototype
*****************************/
class EffectDragDropButton : public BControl
{
private:
	EffectNode		*fEffectNode;
	BMessage		*fDragMessage;
	BPoint			fStringOffset;

public:
	EffectDragDropButton(BRect frame, EffectNode *effect)
	: BControl(frame, "Apply", "Apply", new BMessage('deft'), B_FOLLOW_NONE, B_WILL_DRAW)
	{
		fEffectNode = effect;
		fDragMessage = new BMessage(TimelineEdit::eMsgDragDropEffect);

		fStringOffset.x = 0.5f*(frame.Width() - be_plain_font->StringWidth(GetText(TXT_DRAG_DROP)));
		fStringOffset.y = be_plain_font->Size();
	}
	
	~EffectDragDropButton()
	{
		delete fDragMessage;	
	}
	
	
	void Draw(BRect frame)
	{
		SetHighColor(32, 128, 255);
			
		FillRect(frame);
		MovePenTo(frame.left + fStringOffset.x, frame.top + fStringOffset.y);
		SetHighColor(255, 255, 255);
		DrawString(GetText(TXT_DRAG_DROP));
	}
	
	void MouseDown(BPoint point)
	{
		SetMouseEventMask(B_POINTER_EVENTS, 0);
		
		fDragMessage->MakeEmpty();
		fDragMessage->AddPointer("effect", fEffectNode);
		fDragMessage->AddInt64("duration", TimelineEdit::kDefaultNewEffectDuration);
		
		float xoffset = point.x;
		fDragMessage->AddFloat("xoffset", xoffset);
		 
		BRect frame = Bounds();
		DragMessage(fDragMessage, frame, this);
	}
	
	void MouseUp(BPoint point)
	{ }
		
};

/********************************************
 *	EffectNode
 ********************************************/

/*	FUNCTION:		EffectNode :: EffectNode
	ARGS:			frame
					view_name
					is_window
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
EffectNode :: EffectNode(BRect frame, const char *view_name, const bool scroll_view)
	: BView(frame, view_name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_SCROLL_VIEW_AWARE | B_FULL_UPDATE_ON_RESIZE)
{
	if (scroll_view)
	{
		BRect scroll_view = Bounds();
		float scroll_scale = be_plain_font->Size()/12.0f;
		scroll_view.right -= scroll_scale*B_V_SCROLL_BAR_WIDTH;
		scroll_view.bottom -= scroll_scale*B_H_SCROLL_BAR_HEIGHT;
		mEffectView = new BView(scroll_view, "transform_view", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_SCROLL_VIEW_AWARE);
		mEffectView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		mScrollView = new BScrollView(nullptr, mEffectView, B_FOLLOW_LEFT | B_FOLLOW_LEFT_TOP, B_WILL_DRAW | B_FRAME_EVENTS, true, true);
		mScrollView->ScrollBar(B_HORIZONTAL)->SetRange(0.0f, 0.0f);
		mScrollView->ScrollBar(B_VERTICAL)->SetRange(0.0f, 0.0f);
		AddChild(mScrollView);
		
		fEffectDragDropButton = new EffectDragDropButton(BRect(10, frame.Height() - (40+scroll_scale*B_H_SCROLL_BAR_HEIGHT), 160, frame.Height() - (10+scroll_scale*B_H_SCROLL_BAR_HEIGHT)), this);
		mEffectView->AddChild(fEffectDragDropButton);
	}
	else
	{
		mScrollView = nullptr;
		mEffectView = nullptr;
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		
		fEffectDragDropButton = new EffectDragDropButton(BRect(10, frame.Height() - 40, 160, frame.Height() - 10), this);
		AddChild(fEffectDragDropButton);
	}

    const float kFontFactor = be_plain_font->Size()/20.0f;
    mEffectViewIdealWidth = frame.Width()*kFontFactor;
	mEffectViewIdealHeight = frame.Height();

	mRenderObjectsInitialised = false;
	mSwapTexturesCheckbox = nullptr;

	if (!sMsgEffectSelected)
	{
		sMsgEffectSelected = new BMessage(MedoWindow::eMsgActionTimelineEffectSelected);
		sMsgEffectSelected->AddPointer("MediaEffect", nullptr);
	}
}

/*	FUNCTION:		EffectNode :: MouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook function, gain focus
*/
void EffectNode :: MouseDown(BPoint point)
{
	if (!Window()->IsActive())
		Window()->Activate();
}

/*	FUNCTION:		MedoWindow :: FrameResized
	ARGS:			width
					height
	RETURN:			none
	DESCRIPTION:	Hook function
*/
void EffectNode :: FrameResized(float width, float height)
{
	if (mScrollView)
	{
		float scroll_scale = be_plain_font->Size()/12.0f;
		mEffectView->ResizeTo(width - scroll_scale*B_V_SCROLL_BAR_WIDTH, height - scroll_scale*B_H_SCROLL_BAR_HEIGHT);
		mScrollView->ResizeTo(width+4, height+4);


		float w = width / mEffectViewIdealWidth;
		mScrollView->ScrollBar(B_HORIZONTAL)->SetRange(0.0f, w < 1.0f ? mEffectViewIdealWidth - width : 0.0f);
		mScrollView->ScrollBar(B_HORIZONTAL)->SetProportion(w);

		float h = height / mEffectViewIdealHeight;
		mScrollView->ScrollBar(B_VERTICAL)->SetRange(0.0f, h < 1.0f ? mEffectViewIdealHeight - height : 0.0f);
		mScrollView->ScrollBar(B_VERTICAL)->SetProportion(h);
		
		SetDragDropButtonPosition(BPoint(10, height - (40+scroll_scale*B_H_SCROLL_BAR_HEIGHT)));
	}
	else
		SetDragDropButtonPosition(BPoint(10, height - 40));

	BView::FrameResized(width, height);
}

/*	FUNCTION:		EffectNode :: SetViewIdealSize
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Used to automatically set scroll bar proportion
*/
void EffectNode :: SetViewIdealSize(const float width, const float height)
{
	mEffectViewIdealWidth = width;
	mEffectViewIdealHeight = height;

	BRect bounds = Bounds();
	float w = bounds.Width() / mEffectViewIdealWidth;
	mScrollView->ScrollBar(B_HORIZONTAL)->SetRange(0.0f, w < 1.0f ? mEffectViewIdealWidth - bounds.Width() : 0.0f);
	mScrollView->ScrollBar(B_HORIZONTAL)->SetProportion(w);

	float h = bounds.Height() / mEffectViewIdealHeight;
	mScrollView->ScrollBar(B_VERTICAL)->SetRange(0.0f, h < 1.0f ? mEffectViewIdealHeight - bounds.Height() : 0.0f);
	mScrollView->ScrollBar(B_VERTICAL)->SetProportion(h);
}

/*	FUNCTION:		EffectNode :: SetDragDropButtonPosition
	ARGS:			position
	RETURN:			n/a
	DESCRIPTION:	Set drag/drop button position
*/
void EffectNode :: SetDragDropButtonPosition(BPoint position)
{
	fEffectDragDropButton->MoveTo(position);		
}

/*	FUNCTION:		EffectNode :: InitSwapTexturesCheckbox
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Add swaap textures checkbox
*/
void EffectNode :: InitSwapTexturesCheckbox()
{
	BRect frame = Bounds();
	mSwapTexturesCheckbox = new BCheckBox(BRect(180, frame.Height() - (50+B_V_SCROLL_BAR_WIDTH+2), 380, frame.Height() - (20+B_V_SCROLL_BAR_WIDTH+2)),
										  "swap_textures", GetText(TXT_EFFECTS_COMMON_SWAP_TEXTURES), new BMessage(kMsgSwapTextureUnits));
	if (mEffectView)
		mEffectView->AddChild(mSwapTexturesCheckbox);
	else
		AddChild(mSwapTexturesCheckbox);
}

/*	FUNCTION:		EffectNode :: AreTexturesSwapped
	ARGS:			none
	RETURN:			true if swapped
	DESCRIPTION:	Return state of mSwapTexturesCheckbox
*/
bool EffectNode :: AreTexturesSwapped()
{
	if (mSwapTexturesCheckbox && (mSwapTexturesCheckbox->Value() > 0))
		return true;

	return false;
}

/*	FUNCTION:		EffectNode :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Media effect selected
*/
void EffectNode :: MediaEffectSelectedBase(MediaEffect *effect)
{
	sCurrentMediaEffect = effect;
	sMsgEffectSelected->ReplacePointer("MediaEffect", effect);
	MedoWindow::GetInstance()->PostMessage(sMsgEffectSelected);
}

/*	FUNCTION:		EffectNode :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Media effect selected
*/
void EffectNode :: MediaEffectDeselectedBase(MediaEffect *effect)
{
	sCurrentMediaEffect = nullptr;
	sMsgEffectSelected->ReplacePointer("MediaEffect", nullptr);
	MedoWindow::GetInstance()->PostMessage(sMsgEffectSelected);
}

/*	FUNCTION:		EffectNode :: InvalidatePreview
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Redraw preview
*/
void EffectNode :: InvalidatePreview()
{
	gProject->InvalidatePreview();
}
