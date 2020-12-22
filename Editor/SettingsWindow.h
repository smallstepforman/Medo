/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Settings window
 */

#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#ifndef PERSISTANT_WINDOW_H
#include "PersistantWindow.h"
#endif

class SettingsView;

class SettingsWindow : public PersistantWindow
{
public:
						SettingsWindow(BRect frame, const char *title);
						~SettingsWindow()				override;

private:
	SettingsView		*fSettingsView;
};

struct GlobalSettings
{
	bool		export_enable_media_kit;

	GlobalSettings();
};

const GlobalSettings *GetSettings();
void	LoadSettings();

#endif	//#ifndef SETTINGS_WINDOW_H
