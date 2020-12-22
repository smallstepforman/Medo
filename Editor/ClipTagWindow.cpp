/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	TimelineEdit clip tag window
 */

#include <cassert>

#include <InterfaceKit.h>

#include "TimelineEdit.h"
#include "ClipTagWindow.h"
#include "Project.h"

static const uint32_t	kMsgButtonOk		= 'cwm1';
static const uint32_t	kMsgButtonCancel	= 'cwm2';

struct TAG_WINDOW_DEFINITIONS
{
	float		window_width;
	float		window_height;
	const char	*window_title;
	float		text_width;
	float		text_height;
};
static const TAG_WINDOW_DEFINITIONS kTagWindow[] =
{
	{320,	120,	"Edit tag",		300, 32},
	{320,	220,	"Edit note",	300, 128},
};

/*	FUNCTION:		ClipTagWindow :: ClipTagWindow
	ARGS:			pos
					type
					parent
					text
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ClipTagWindow :: ClipTagWindow(BPoint pos, CLIP_TAG_TYPE type, TimelineEdit *parent, const char *text)
	: BWindow(BRect(pos.x, pos.y, pos.x + kTagWindow[type].window_width, pos.y+kTagWindow[type].window_height),
			  kTagWindow[type].window_title, B_FLOATING_WINDOW, B_WILL_ACCEPT_FIRST_CLICK),
	  fClipTagType(type), fParent(parent), fReallyQuit(false)
{

	BView *view = new BView(Bounds(), "clip_tag_view", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
	AddChild(view);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fParentMessage = new BMessage(kMsgButtonOk);
	fParentMessage->AddString("tag", "");

	//	Text control (single line)
	if (type == CLIP_TAG)
	{
		fTextControl = new BTextControl(BRect(10, 10, 10+kTagWindow[CLIP_TAG].text_width, 10+kTagWindow[CLIP_TAG].text_height),
							  "fTextControl", nullptr, text, fParentMessage);
		view->AddChild(fTextControl);
		fTextControl->SetTarget(nullptr, this);
		fTextView = nullptr;
	}
	else
	{	//	Text view (multiline)
		fTextView = new BTextView(BRect(10, 10, 10+kTagWindow[CLIP_NOTE].text_width, 10+kTagWindow[CLIP_NOTE].text_height),
							  "fTextView", BRect(0, 0, kTagWindow[CLIP_NOTE].text_width, kTagWindow[CLIP_NOTE].text_height), B_FOLLOW_LEFT_TOP);
		fTextView->SetText(text);
		view->AddChild(fTextView);
		fTextControl = nullptr;
	}

	//	Cancel
	BButton *button_cancel = new BButton(BRect(kTagWindow[type].window_width - 170, kTagWindow[type].window_height - 50,
											   kTagWindow[type].window_width - 90, kTagWindow[type].window_height - 20),
											"button_cancel", "Cancel", new BMessage(kMsgButtonCancel));
	view->AddChild(button_cancel);

	//	OK
	BButton *button_ok = new BButton(BRect(kTagWindow[type].window_width - 80, kTagWindow[type].window_height - 50,
										   kTagWindow[type].window_width - 20, kTagWindow[type].window_height - 20),
											"button_ok", "OK", new BMessage(kMsgButtonOk));
	view->AddChild(button_ok);
}
	
/*	FUNCTION:		ClipTagWindow :: ~ClipTagWindow
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ClipTagWindow :: ~ClipTagWindow()
{
}


/*	FUNCTION:		ClipTagWindow :: QuitRequested
	ARGS:			none
	RETURN:			false
	DESCRIPTION:	Trap window close button (dont quit, hide window)
*/
bool ClipTagWindow :: QuitRequested()
{
	if (!fReallyQuit)
	{
		fParentMessage->what = TimelineEdit::eMsgClipEditTagCancelled;
		fParent->Window()->PostMessage(fParentMessage, fParent);
	}

	return fReallyQuit;
}

/*	FUNCTION:		ClipTagWindow :: Terminate
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when fParent (fTimelineEdit) destroyed
*/
void ClipTagWindow :: Terminate()
{
	fReallyQuit = true;
	PostMessage(B_QUIT_REQUESTED);
}

/*	FUNCTION:		ClipTagWindow :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process window messages
*/
void ClipTagWindow :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgButtonOk:
			if (fClipTagType == CLIP_TAG)
			{
				fParentMessage->what = TimelineEdit::eMsgClipEditTagComplete;
				fParentMessage->ReplaceString("tag", fTextControl->Text());
			}
			else
			{
				fParentMessage->what = TimelineEdit::eMsgClipEditNoteComplete;
				fParentMessage->ReplaceString("tag", fTextView->Text());
			}
			fParent->Window()->PostMessage(fParentMessage, fParent);
			break;

		case kMsgButtonCancel:
			fParentMessage->what = TimelineEdit::eMsgClipEditTagCancelled;
			fParent->Window()->PostMessage(fParentMessage, fParent);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}

