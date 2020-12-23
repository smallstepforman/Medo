/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Settings Window
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <interface/TabView.h>
#include <StorageKit.h>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "FileUtility.h"
#include "Language.h"
#include "MedoWindow.h"
#include "PersistantWindow.h"
#include "Project.h"
#include "SettingsWindow.h"
#include "Theme.h"

static const char *kThemes[] =
{
	"Dark",
	"Lite",
	"System",
};

GlobalSettings :: GlobalSettings()
	: export_enable_media_kit(false)
{ }
static GlobalSettings sGlobalSettings;
const GlobalSettings * GetSettings() {return &sGlobalSettings;}

/*************************************
	Settings functions
**************************************/

/*	FUNCTION:		LoadSettings
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
void SaveSettings()
{
	BPath config_path;
	find_directory(B_USER_CONFIG_DIRECTORY, &config_path);
	BString settings_path(config_path.Path());

	//	Check if settings directory exists?
	settings_path.Append("/settings/Medo");
	if (!BEntry(settings_path.String()).Exists())
	{
		if (B_OK != create_directory(settings_path.String(), 0777))
		{
			printf("SaveSettings(): Cannot create dir: %s\n", settings_path.String());
			return;
		}
	}

	settings_path.Append("/medo.json");
	FILE *file = fopen(settings_path.String(), "wb");
	if (file)
	{
		char buffer[0x200];	//	512 bytes
		sprintf(buffer, "{\n");
		fwrite(buffer, strlen(buffer), 1, file);

		//	"medo"
		sprintf(buffer, "\t\"medo\": {\n");
		fwrite(buffer, strlen(buffer), 1, file);
			sprintf(buffer, "\t\t\"theme\": %d,\n", Theme::GetTheme());
			fwrite(buffer, strlen(buffer), 1, file);
			sprintf(buffer, "\t\t\"language\": %d,\n", GetLanguage());
			fwrite(buffer, strlen(buffer), 1, file);
			sprintf(buffer, "\t\t\"menu_export_media_kit\": %s\n", sGlobalSettings.export_enable_media_kit ? "true" : "false");
			fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t}\n");
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "}\n");
		fwrite(buffer, strlen(buffer), 1, file);
		fclose(file);
	}
	else
	{
		printf("SaveSettings() error - unable to open \"%s\"\n", settings_path.String());
	}
}

/*	FUNCTION:		LoadSettings
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
void LoadSettings()
{
	BPath config_path;
	find_directory(B_USER_CONFIG_DIRECTORY, &config_path);
	BString settings_path(config_path.Path());
	settings_path.Append("/settings/Medo/medo.json");

	char *data = ReadFileToBuffer(settings_path.String());
	if (data)
	{
#define ERROR_EXIT(a) {printf("LoadSettings() Error(%s)\n", a);	return;}
		rapidjson::Document document;
		rapidjson::ParseResult res = document.Parse<rapidjson::kParseTrailingCommasFlag>(data);
		if (!res)
		{
			printf("JSON parse error: %s (Byte offset: %lu)\n", rapidjson::GetParseError_En(res.Code()), res.Offset());

			for (int c=-20; c < 20; c++)
				printf("%c", data[res.Offset() + c]);
			printf("\n");
			return;
		}

		//	"medo" (header)
		if (!document.HasMember("medo"))
			ERROR_EXIT("Missing object \"medo\"");
		{
			const rapidjson::Value &header = document["medo"];

			//	theme
			if (!header.HasMember("theme") || !header["theme"].IsUint())
				ERROR_EXIT("Missing attribute medo::theme");
			uint32 theme = header["theme"].GetUint();
			if (theme > (uint32)Theme::Preset::eSystem)
				ERROR_EXIT("medo::theme invalid");
			Theme::SetTheme((Theme::Preset)theme);

			//	language
			if (!header.HasMember("language") || !header["language"].IsUint())
				ERROR_EXIT("Missing attribute medo::language");
			uint32 language = header["language"].GetUint();
			if (language >= (uint32)GetAvailableLanguages().size())
				ERROR_EXIT("medo::theme language");
			SetLangauge(language);

			//	export_media_kit
			if (!header.HasMember("menu_export_media_kit") || !header["menu_export_media_kit"].IsBool())
				ERROR_EXIT("Missing attribute medo::menu_export_media_kit");
			sGlobalSettings.export_enable_media_kit = header["menu_export_media_kit"].GetBool();
		}
	}
	else
		SaveSettings();
}

/*************************************
	SettingsView
**************************************/
class SettingsView : public BTabView
{
	BOptionPopUp	*fAppearancePopupTheme;
	BOptionPopUp	*fAppearancePopupLanguage;
	BButton			*fAppearanceButtonApply;
	BCheckBox		*fExportCheckboxMediaKit;

	enum SettingsMessages
	{
		eMsgAppearanceTheme,
		eMsgAppearanceLanguage,
		eMsgAppearanceApply,
		eMsgExportMediaKit
	};

public:
	SettingsView(BRect bounds)
	: BTabView(bounds, "settings_tabs", B_WIDTH_FROM_WIDEST, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		//	Tab Appearance
		{
			BView *tab_appearance = new BView(BRect(bounds.left, bounds.top, bounds.right, bounds.bottom - TabHeight()), GetText(TXT_SETTINGS_APPEARANCE), B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);

			//	Theme popup
			fAppearancePopupTheme = new BOptionPopUp(BRect(20, 20, 320, 60), "theme", GetText(TXT_SETTINGS_APPEARANCE_THEME), new BMessage(eMsgAppearanceTheme));
			for (size_t i=0; i < sizeof(kThemes)/sizeof(char *); i++)
			{
				fAppearancePopupTheme->AddOption(GetText((LANGUAGE_TEXT)(TXT_SETTINGS_APPEARANCE_THEME_DARK + i)), i);
			}
			fAppearancePopupTheme->SelectOptionFor((int)Theme::GetTheme());
			tab_appearance->AddChild(fAppearancePopupTheme);

			//	Language popup
			fAppearancePopupLanguage = new BOptionPopUp(BRect(20, 70, 320, 110), "language", GetText(TXT_SETTINGS_APPEARANCE_LANGUAGE), new BMessage(eMsgAppearanceLanguage));
			const std::vector<const char *> kAvailableLanguages = GetAvailableLanguages();
			for (int i=0; i < (int)kAvailableLanguages.size(); i++)
			{
				fAppearancePopupLanguage->AddOption(kAvailableLanguages[i], i);
			}
			fAppearancePopupLanguage->SelectOptionFor(GetLanguage());
			tab_appearance->AddChild(fAppearancePopupLanguage);

			fAppearanceButtonApply = new BButton(BRect(20, 150, 200, 190), "apply", GetText(TXT_SETTINGS_APPEARANCE_RESTART), new BMessage(eMsgAppearanceApply));
			tab_appearance->AddChild(fAppearanceButtonApply);
			fAppearanceButtonApply->Hide();

			AddTab(tab_appearance);
		}

		//	Tab Export
		{
			BView *tab_export = new BView(BRect(bounds.left, bounds.top, bounds.right, bounds.bottom - TabHeight()), GetText(TXT_SETTINGS_EXPORT), B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);

			//	Export BMediaKit
			fExportCheckboxMediaKit = new BCheckBox(BRect(20, 20, 320, 60), "checkbox_media_kit", GetText(TXT_SETTINGS_EXPORT_USE_BMEDIA_KIT), new BMessage(eMsgExportMediaKit));
			fExportCheckboxMediaKit->SetValue(sGlobalSettings.export_enable_media_kit);
			tab_export->AddChild(fExportCheckboxMediaKit);

			AddTab(tab_export);
		}

		Select(0);
	}
	~SettingsView()	override
	{
	}
	void AttachedToWindow() override
	{
		fAppearancePopupTheme->SetTarget(this, Window());
		fAppearancePopupLanguage->SetTarget(this, Window());
		fAppearanceButtonApply->SetTarget(this, Window());
		fExportCheckboxMediaKit->SetTarget(this, Window());
	}
	void MessageReceived(BMessage *msg) override
	{
		switch (msg->what)
		{
			case eMsgAppearanceTheme:
				Theme::SetTheme((Theme::Preset)fAppearancePopupTheme->SelectedOption());
				SaveSettings();
				if (fAppearanceButtonApply->IsHidden())
					fAppearanceButtonApply->Show();
				break;

			case eMsgAppearanceLanguage:
				SetLangauge(fAppearancePopupLanguage->SelectedOption());
				SaveSettings();
				if (fAppearanceButtonApply->IsHidden())
					fAppearanceButtonApply->Show();
				break;

			case eMsgAppearanceApply:
				MedoWindow::GetInstance()->PostMessage(B_QUIT_REQUESTED);
				break;

			case eMsgExportMediaKit:
				sGlobalSettings.export_enable_media_kit = fExportCheckboxMediaKit->Value();
				MedoWindow::GetInstance()->PostMessage(MedoWindow::eMsgActionMedoSettingsChanged);
				SaveSettings();
				break;

			default:
				BView::MessageReceived(msg);
		}
	}
};

/*************************************
	Settings Window
**************************************/

/*	FUNCTION:		SettingsWindow :: SettingsWindow
	ARGS:			frame
					title
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
SettingsWindow :: SettingsWindow(BRect frame, const char *title)
	: PersistantWindow(frame, title, B_DOCUMENT_WINDOW, B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS)
{
	fSettingsView = new SettingsView(Bounds());
	AddChild(fSettingsView);
}

/*	FUNCTION:		SettingsWindow :: ~SettingsWindow
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
SettingsWindow :: ~SettingsWindow()
{
}

