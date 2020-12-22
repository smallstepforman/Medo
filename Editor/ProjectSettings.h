/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Project Settings Window
 */

#ifndef _PROJECT_SETTINGS_H_
#define _PROJECT_SETTINGS_H_

#ifndef PERSISTANT_WINDOW_H
#include "PersistantWindow.h"
#endif

class MedoWindow;
class BView;
class BOptionPopUp;
class BCheckBox;
class BTextControl;
class BitmapCheckbox;

class ProjectSettings : public PersistantWindow
{
public:
				ProjectSettings(MedoWindow *parent);
				~ProjectSettings();

	void		MessageReceived(BMessage *msg) override;
	void		Show() override;

private:
	MedoWindow		*fMedoWindow;
	BView			*fBackgroundView;
	BOptionPopUp	*fOptionVideoResolution;
	BCheckBox		*fEnableCustomVideoResolution;
	BTextControl	*fTextVideoCustomWidth;
	BTextControl	*fTextVideoCustomHeight;
	BitmapCheckbox	*fCheckboxCustomResolutionLinked;
	BOptionPopUp	*fOptionVideoFrameRate;
	BOptionPopUp	*fOptionAudioSampleRate;

	void			ValidateTextField(BTextControl *control, uint32 what);
	void			UpdateCustomVideoResolution(uint32 msg);
};

#endif // #ifndef _PROJECT_SETTINGS_H_
