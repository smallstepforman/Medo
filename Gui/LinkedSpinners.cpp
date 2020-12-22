/*	PROJECT:		Medo
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2019, ZenYes Pty Ltd
	DESCRIPTION:	Linked Spinners
*/

#include <cstdio>

#include <interface/View.h>
#include <translation/TranslationUtils.h>

#include "Spinner.h"
#include "BitmapCheckbox.h"
#include "LinkedSpinners.h"

static const uint32 kMsgStart		= 'glss';
static const uint32 kMsgEnd			= 'glse';
static const uint32 kMsgLink		= 'glsl';

/*	FUNCTION:		LinkedSpinners :: LinkedSpinners
	ARGUMENTS:		frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
LinkedSpinners :: LinkedSpinners(BRect frame, const char *name, const char *start_label, const char *end_label, BMessage *msg, uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags), fMessageNotification(msg)
{
	const float kFontSize = be_plain_font->Size();
	const float kLockIconSize = 32.0f;
	const float kLockIconOffset = frame.Width() - 1.25f*kLockIconSize;

	fSpinnerStart = new Spinner(BRect(0, 0, kLockIconOffset, 1.25f*kFontSize), "start", start_label, new BMessage(kMsgStart));
	fSpinnerStart->SetValue(1.0f);
	fSpinnerStart->SetSteps(0.1f);
	AddChild(fSpinnerStart);

	fSpinnerEnd = new Spinner(BRect(0, 1.75f*kFontSize, kLockIconOffset, (1.75f+1.25f)*kFontSize), "end", end_label, new BMessage(kMsgEnd));
	fSpinnerEnd->SetValue(1.0f);
	fSpinnerEnd->SetSteps(0.1f);
	AddChild(fSpinnerEnd);

	fCheckboxLinked = new BitmapCheckbox(BRect(frame.Width() - kLockIconSize, 0.75f*kFontSize, frame.Width(), 0.75f*kFontSize + kLockIconSize), "linked",
								   BTranslationUtils::GetBitmap("Resources/icon_unlink.png"),
								   BTranslationUtils::GetBitmap("Resources/icon_link.png"),
								   new BMessage(kMsgLink));
	AddChild(fCheckboxLinked);

	fCheckboxLinked->SetValue(1);
	fSpinnerEnd->SetEnabled(false);
	fSpinnerEnd->SetValue(fSpinnerStart->Value());
}

/*	FUNCTION:		LinkedSpinners :: !LinkedSpinners
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
LinkedSpinners :: ~LinkedSpinners()
{
	delete fMessageNotification;
	delete fCheckboxLinked;
}

/*	FUNCTION:		TimelineView :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when attached to window
*/
void LinkedSpinners :: AttachedToWindow()
{
	fSpinnerStart->SetTarget(this, (BLooper *)Window());
	fSpinnerEnd->SetTarget(this, (BLooper *)Window());
	fCheckboxLinked->SetTarget(this, (BLooper *)Window());

	SetViewColor(Parent()->ViewColor());
}

const float	LinkedSpinners :: GetStartValue() const		{return fSpinnerStart->Value();}
const float	LinkedSpinners :: GetEndValue() const		{return fSpinnerEnd->Value();}
const bool	LinkedSpinners :: GetLinked() const			{return fCheckboxLinked->Value() > 0 ? true : false;}

void LinkedSpinners :: SetStartValue(const float value)
{
	fSpinnerStart->SetValue(value);
	if (fCheckboxLinked->Value() > 0)
		fSpinnerEnd->SetValue(value);
}
void LinkedSpinners :: SetEndValue(const float value)
{
	if (fCheckboxLinked->Value() > 0)
		printf("LinkedSpinners::SetEndValue() - cannot modify value since linked to start value\n");
	else
		fSpinnerEnd->SetValue(value);
}
void LinkedSpinners :: SetLinked(const bool linked)
{
	fCheckboxLinked->SetValue(linked);
	fSpinnerEnd->SetEnabled(linked ? false : true);
	if (linked)
		fSpinnerEnd->SetValue(fSpinnerStart->Value());
}

void LinkedSpinners :: SetRange(const float min, const float max)
{
	fSpinnerStart->SetRange(min, max);
	fSpinnerEnd->SetRange(min, max);
}

void LinkedSpinners :: SetSteps(const float steps)
{
	fSpinnerStart->SetSteps(steps);
	fSpinnerEnd->SetSteps(steps);
}

/*	FUNCTION:		LinkedSpinners :: MessageReceived
	ARGUMENTS:		message
	RETURN:			n/a
	DESCRIPTION:	Hook function called when message received
*/
void LinkedSpinners :: MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case kMsgStart:
		{
			const bool linked = fCheckboxLinked->Value();
			if (linked)
			{
				fSpinnerEnd->SetValue(fSpinnerStart->Value());
			}
			Parent()->MessageReceived(fMessageNotification);
			break;
		}

		case kMsgEnd:
		{
			Parent()->MessageReceived(fMessageNotification);
			break;
		}

		case kMsgLink:
		{
			const bool linked = fCheckboxLinked->Value();
			fSpinnerEnd->SetEnabled(linked ? false : true);
			if (linked)
			{
				fSpinnerEnd->SetValue(fSpinnerStart->Value());
			}
			Parent()->MessageReceived(fMessageNotification);
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}
