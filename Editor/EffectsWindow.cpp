/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	Window container for EffectNode (view)
 */
 
#include <InterfaceKit.h>

#include "EffectNode.h"
#include "EffectsWindow.h"
#include "Language.h"
#include "MedoWindow.h"
#include "PersistantWindow.h"
#include "Project.h"

EffectsWindow * EffectsWindow :: sWindowInstance = nullptr;

/*	FUNCTION:		EffectsWindow :: EffectsWindow
	ARGS:			frame
					name
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
EffectsWindow :: EffectsWindow(BRect frame)
	: PersistantWindow(frame, "Effect Window")
{
	assert(sWindowInstance == nullptr);
	sWindowInstance = this;

	fEffectNode = nullptr;

	SetSizeLimits(frame.Width(), 2.0f*frame.Width(), frame.Height(), 2.0f*frame.Height());
}
	
/*	FUNCTION:		EffectsWindow :: ~EffectsWindow
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
EffectsWindow :: ~EffectsWindow()
{
}

/*	FUNCTION:		EffectsWindow :: Terminate
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when MainWindow->EffectsTab destroyed
*/
void EffectsWindow :: Terminate()
{
	LockLooper();
	//	EffectsManager owns all EffectNode's, remove child
	if (fEffectNode)
		RemoveChild(fEffectNode);
	UnlockLooper();

	PersistantWindow::Terminate();
}

/*	FUNCTION:		EffectsWindow :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process window messages
*/
void EffectsWindow :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case eMsgShowEffect:
		{
			EffectNode *node;
			MediaEffect *effect;
			if ((msg->FindPointer("EffectNode", (void **)&node) == B_OK) &&
				(msg->FindPointer("MediaEffect", (void **)&effect) == B_OK))
			{
				//printf("EffectsWindow::MessageReceived() node=%p, fEffectNode=%p\n", node, fEffectNode);
				if (node != fEffectNode)
				{
					if (fEffectNode)
					{
						RemoveChild(fEffectNode);
					}
					fEffectNode = node;
					if (fEffectNode)
					{
						AddChild(fEffectNode);
						SetTitle(fEffectNode->GetTextEffectName(GetLanguage()));
						fEffectNode->ResizeTo(Bounds().Width(), Bounds().Height());
					}
					else
					{
						while (!IsHidden())
							Hide();
						return;
					}
				}

				//	Effect GUI is updated from BWindow::BLooper
				if (effect)
					fEffectNode->MediaEffectSelected(effect);
			}

			while (IsHidden())
				Show();

			MedoWindow::GetInstance()->Activate(true);
			break;
		}

		case eMsgActivateMedoWindow:
			MedoWindow::GetInstance()->Activate(true);
			break;

		case eMsgOutputViewMouseDown:
		{
			BPoint aPoint;
			MediaEffect *effect;
			if (fEffectNode && (msg->FindPoint("point", &aPoint) == B_OK) && (msg->FindPointer("effect", (void **)&effect) == B_OK))
			{
				fEffectNode->OutputViewMouseDown(effect, aPoint);
			}
			else
				printf("EffectsWindow::MessageReceived(eMsgOutputViewMouseDown) invalid msg\n");
			break;
		}

		case eMsgOutputViewMouseMoved:
		{
			BPoint aPoint;
			MediaEffect *effect;
			if (fEffectNode && (msg->FindPoint("point", &aPoint) == B_OK) && (msg->FindPointer("effect", (void **)&effect) == B_OK))
			{
				fEffectNode->OutputViewMouseMoved(effect, aPoint);
			}
			else
				printf("EffectsWindow::MessageReceived(eMsgOutputViewMouseMoved) invalid msg\n");
			break;
		}

		default:
			//printf("EffectsWindow::MessageReceived()\n");
			//msg->PrintToStream();
			BWindow::MessageReceived(msg);
	}
}

/*	FUNCTION:		EffectsWindow :: GetKeyModifiers
	ARGS:			none
	RETURN:			modifier key
	DESCRIPTION:	Get modifier keys (B_SHIFT_KEY, B_CONTROL_KEY etc)
*/
const uint32 EffectsWindow :: GetKeyModifiers()
{
	return modifiers();
}

