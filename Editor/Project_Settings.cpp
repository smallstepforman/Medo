/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Project Settings Window
 */

#include <cassert>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Math/Math.h"
#include "Project.h"
#include "ProjectSettings.h"
#include "MedoWindow.h"
#include "Language.h"
#include "Gui/BitmapCheckbox.h"

static const uint32 kMsgPopupVideoResolution		= 'psvr';
static const uint32 kMsgPopupVideoFrameRate			= 'psvf';
static const uint32 kMsgCustomVideoResolution		= 'psv1';
static const uint32 kMsgCustomVideoWidth			= 'psv2';
static const uint32 kMsgCustomVideoHeight			= 'psv3';
static const uint32 kMsgCustomVideoResolutionLinked	= 'psv4';
static const uint32 kMsgPopupAudioSampleRate		= 'psas';
static const uint32 kMsgApply						= 'psok';
static const uint32 kMsgCancel						= 'psno';

static const float kGuiHeight = 44;
static const float kGuiOffset = 16;

struct VIDEO_RESOLUTION
{
	uint32		width;
	uint32		height;
	const char	*description;
};
//	16:9 resolutions supported by YouTube
static const VIDEO_RESOLUTION kVideoResolutions[] =
{
	{3840, 2160,	"2160p (4K Ultra HD)"},
	{2560, 1440,	"1440p (2K)"},
	{1920, 1080,	"1080p (Full HD)"},
	{1280, 720,		"   720p (HD Ready)"},
	{854, 480,		"    480p"},
//	{720, 576,		"    DVD (PAL)"},
//	{720, 480,		"    DVD (NTSC)"},
	{640, 360,		"    360p"},
//	{426, 240,		"    240p"},
};
static const float kVideoFrameRates[] = {24, 25, 29.97f, 30, 60};
static const float kDefaultVideoFrameRateIndex = 3;	//	30fps
static const uint32 kAudioSampleRates[] = {22050, 44100, 48000, 96000, 192000};
static const uint32 kDefaultAudioSampleRate = 48000;

/*	FUNCTION:		ProjectSettings :: ProjectSettings
	ARGS:			parent
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ProjectSettings :: ProjectSettings(MedoWindow *parent)
	: PersistantWindow(BRect(96, 96, 96+640, 96+540), GetText(TXT_PROJECT_SETTINGS_WINDOW), B_DOCUMENT_WINDOW, 0)
{
	fMedoWindow = parent;

	//	Background view
	fBackgroundView = new BView(Bounds(), nullptr, B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_DRAW_ON_CHILDREN);
	fBackgroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fBackgroundView);

	//	Populate view
	float start_y = kGuiOffset;

	//	Video Settings
	BStringView *title = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_PROJECT_VIDEO_SETTINGS));
	title->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	title->SetFont(be_bold_font);
	AddChild(title);
	start_y += kGuiHeight + kGuiOffset;

	//	Video Resolution
	fOptionVideoResolution = new BOptionPopUp(BRect(20, start_y, 520, start_y+kGuiHeight), "video_resolution",
											  GetText(TXT_PROJECT_VIDEO_RESOLUTION), new BMessage(kMsgPopupVideoResolution));
	int default_resolution_index = 0;
	for (int i=0; i < sizeof(kVideoResolutions)/sizeof(VIDEO_RESOLUTION); i++)
	{
		char buffer[40];
		sprintf(buffer, "%dx%d   %s", kVideoResolutions[i].width, kVideoResolutions[i].height, kVideoResolutions[i].description);
		fOptionVideoResolution->AddOption(buffer, i);

		if ((gProject->mResolution.width == kVideoResolutions[i].width) && (gProject->mResolution.height == kVideoResolutions[i].height))
			default_resolution_index = i;
	}
	fOptionVideoResolution->SelectOptionFor(default_resolution_index);
	fBackgroundView->AddChild(fOptionVideoResolution);
	start_y += kGuiHeight;

	//	Custom Resolution
	char text_buffer[32];
	fEnableCustomVideoResolution = new BCheckBox(BRect(20, start_y, 260, start_y + kGuiHeight), nullptr,
												 GetText(TXT_PROJECT_VIDEO_CUSTOM_RESOLUTION), new BMessage(kMsgCustomVideoResolution));
	fBackgroundView->AddChild(fEnableCustomVideoResolution);
	fEnableCustomVideoResolution->SetValue(0);

	fTextVideoCustomWidth = new BTextControl(BRect(280, start_y, 420, start_y + kGuiHeight), nullptr,
											 GetText(TXT_PROJECT_VIDEO_CUSTOM_WIDTH), nullptr, new BMessage(kMsgCustomVideoWidth));
	sprintf(text_buffer, "%d", gProject->mResolution.width);
	fTextVideoCustomWidth->SetText(text_buffer);
	fTextVideoCustomWidth->SetEnabled(false);
	fBackgroundView->AddChild(fTextVideoCustomWidth);

	fTextVideoCustomHeight = new BTextControl(BRect(440, start_y, 580, start_y + kGuiHeight), nullptr,
											  GetText(TXT_PROJECT_VIDEO_CUSTOM_HEIGHT), nullptr, new BMessage(kMsgCustomVideoHeight));
	sprintf(text_buffer, "%d", gProject->mResolution.height);
	fTextVideoCustomHeight->SetText(text_buffer);
	fTextVideoCustomHeight->SetEnabled(false);
	fBackgroundView->AddChild(fTextVideoCustomHeight);

	//	Custom Resolution Linked button
	fCheckboxCustomResolutionLinked = new BitmapCheckbox(BRect(600, start_y, 600+0.8f*kGuiHeight, start_y + 0.8f*kGuiHeight), "linked_resolution",
								   BTranslationUtils::GetBitmap("Resources/icon_unlink.png"),
								   BTranslationUtils::GetBitmap("Resources/icon_link.png"),
								   new BMessage(kMsgCustomVideoResolutionLinked));
	fCheckboxCustomResolutionLinked->SetState(true);
	fCheckboxCustomResolutionLinked->SetEnabled(false);
	fBackgroundView->AddChild(fCheckboxCustomResolutionLinked);

	start_y += kGuiHeight + kGuiOffset;

	//	Frame rate
	fOptionVideoFrameRate = new BOptionPopUp(BRect(20, start_y, 480, start_y+kGuiHeight), "video_frame_rate",
													  GetText(TXT_PROJECT_VIDEO_FRAME_RATE), new BMessage(kMsgPopupVideoFrameRate));
	int default_frame_rate_selection = 0;
	for (int i=0; i < sizeof(kVideoFrameRates)/sizeof(float); i++)
	{
		if (float(int(kVideoFrameRates[i])) == kVideoFrameRates[i])
			sprintf(text_buffer, "%3.0f %s", kVideoFrameRates[i], GetText(TXT_PROJECT_VIDEO_FPS));
		else
			sprintf(text_buffer, "%0.3f %s", kVideoFrameRates[i], GetText(TXT_PROJECT_VIDEO_FPS));
		fOptionVideoFrameRate->AddOption(text_buffer, i);

		if (ymath::YIsEqual(gProject->mResolution.frame_rate, kVideoFrameRates[i]))
			default_frame_rate_selection = i;
	}
	fOptionVideoFrameRate->SelectOptionFor(default_frame_rate_selection);
	fBackgroundView->AddChild(fOptionVideoFrameRate);
	start_y += kGuiHeight + kGuiOffset;

#if 0
	//	Audio settings
	title = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_PROJECT_AUDIO_SETTINGS));
	title->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	title->SetFont(be_bold_font);
	AddChild(title);
	start_y += kGuiHeight + kGuiOffset;

	//	Audio sample rate
	fOptionAudioSampleRate = new BOptionPopUp(BRect(20, start_y, 440, start_y+kGuiHeight), "audio_frame_rate",
													  GetText(TXT_PROJECT_AUDIO_SAMPLE_RATE), new BMessage(kMsgPopupAudioSampleRate));
	int default_sample_rate_selection = 0;
	for (int i=0; i < sizeof(kAudioSampleRates)/sizeof(float); i++)
	{
		char buffer[32];
		sprintf(buffer, "%d,%03d %s", kAudioSampleRates[i]/1000, kAudioSampleRates[i]%1000, GetText(TXT_PROJECT_AUDIO_HZ));
		fOptionAudioSampleRate->AddOption(buffer, i);

		if (kDefaultAudioSampleRate == kAudioSampleRates[i])
			default_sample_rate_selection = i;
	}
	fOptionAudioSampleRate->SelectOptionFor(default_sample_rate_selection);
	fBackgroundView->AddChild(fOptionAudioSampleRate);
	start_y += 2*kGuiHeight;
#endif

	//	Video performance text
	title = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_PROJECT_SETTINGS_INTRO));
	title->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	title->SetFont(be_bold_font);
	AddChild(title);
	start_y += kGuiHeight;
	//	Line 2
	title = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_PROJECT_SETTINGS_VIDEO_1));
	title->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	title->SetFont(be_plain_font);
	AddChild(title);
	start_y += kGuiHeight;
	//	Line 3
	title = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_PROJECT_SETTINGS_VIDEO_2));
	title->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	title->SetFont(be_plain_font);
	AddChild(title);
	start_y += 2*kGuiHeight + kGuiOffset;

	//	Buttons
	BButton *button_save = new BButton(BRect(430, start_y, 630, start_y+kGuiHeight), nullptr, GetText(TXT_APPLY), new BMessage(kMsgApply));
	fBackgroundView->AddChild(button_save);
	BButton *button_cancel = new BButton(BRect(220, start_y, 420, start_y+kGuiHeight), nullptr, GetText(TXT_CANCEL), new BMessage(kMsgCancel));
	fBackgroundView->AddChild(button_cancel);
	start_y += kGuiHeight + kGuiOffset;

	assert(start_y <= Bounds().Height());
}

/*	FUNCTION:		ProjectSettings :: ~ProjectSettings
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ProjectSettings :: ~ProjectSettings()
{ }

/*	FUNCTION:		ProjectSettings :: Show
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when window shown
*/
void ProjectSettings :: Show()
{
	LockLooper();
	//	Resolution
	bool found = false;
	for (int i=0; i < sizeof(kVideoResolutions)/sizeof(VIDEO_RESOLUTION); i++)
	{
		if ((kVideoResolutions[i].width == gProject->mResolution.width) &&
			(kVideoResolutions[i].height == gProject->mResolution.height))
		{
			fTextVideoCustomWidth->SetEnabled(false);
			fTextVideoCustomHeight->SetEnabled(false);
			fOptionVideoResolution->SetEnabled(true);
			fCheckboxCustomResolutionLinked->SetEnabled(false);
			fEnableCustomVideoResolution->SetValue(0);
			fOptionVideoResolution->SelectOptionFor(i);
			found = true;
			break;
		}
	}
	if (!found)
	{
		fTextVideoCustomWidth->SetEnabled(true);
		fTextVideoCustomHeight->SetEnabled(true);
		fOptionVideoResolution->SetEnabled(false);
		fCheckboxCustomResolutionLinked->SetEnabled(true);
		fCheckboxCustomResolutionLinked->SetValue(0);
		fEnableCustomVideoResolution->SetValue(1);
		char buffer[16];
		sprintf(buffer, "%d", gProject->mResolution.width);
		fTextVideoCustomWidth->SetText(buffer);
		sprintf(buffer, "%d", gProject->mResolution.height);
		fTextVideoCustomHeight->SetText(buffer);
	}

	//	Frame rate
	found = false;
	for (int i=0; i < sizeof(kVideoFrameRates)/sizeof(float); i++)
	{
		if (ymath::YIsEqual(gProject->mResolution.frame_rate, kVideoFrameRates[i]))
		{
			fOptionVideoFrameRate->SelectOptionFor(i);
			found = true;
			break;
		}
	}
	if (!found)
		fOptionVideoFrameRate->SelectOptionFor(kDefaultVideoFrameRateIndex);

	UnlockLooper();
	BWindow::Show();
}

/*	FUNCTION:		ProjectSettings :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void ProjectSettings :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgCustomVideoResolution:
			if (fEnableCustomVideoResolution->Value() == 0)
			{
				fTextVideoCustomWidth->SetEnabled(false);
				fTextVideoCustomHeight->SetEnabled(false);
				fOptionVideoResolution->SetEnabled(true);
				fCheckboxCustomResolutionLinked->SetEnabled(false);
			}
			else
			{
				fTextVideoCustomWidth->SetEnabled(true);
				fTextVideoCustomHeight->SetEnabled(true);
				fOptionVideoResolution->SetEnabled(false);
				fCheckboxCustomResolutionLinked->SetEnabled(true);
			}
			break;
		case kMsgCustomVideoWidth:			ValidateTextField(fTextVideoCustomWidth, kMsgCustomVideoWidth);				break;
		case kMsgCustomVideoHeight:			ValidateTextField(fTextVideoCustomHeight, kMsgCustomVideoHeight);			break;

		case kMsgCustomVideoResolutionLinked:
			UpdateCustomVideoResolution(kMsgCustomVideoWidth);
			break;

		case kMsgApply:
		{
			Project::RESOLUTION old_resolution = gProject->mResolution;
			if (fEnableCustomVideoResolution->Value() == 0)
			{
				int selected_resolution = fOptionVideoResolution->SelectedOption();
				gProject->mResolution.width = kVideoResolutions[selected_resolution].width;
				gProject->mResolution.height = kVideoResolutions[selected_resolution].height;
			}
			else
			{
				gProject->mResolution.width = atoi(fTextVideoCustomWidth->Text());
				gProject->mResolution.height = atoi(fTextVideoCustomHeight->Text());
			}
			gProject->mResolution.frame_rate = kVideoFrameRates[fOptionVideoFrameRate->SelectedOption()];

			if ((old_resolution.width != gProject->mResolution.width) ||
				(old_resolution.height != gProject->mResolution.height) ||
				!ymath::YIsEqual(old_resolution.frame_rate, gProject->mResolution.frame_rate))
			{
				MedoWindow::GetInstance()->PostMessage(MedoWindow::eMsgActionProjectSettingsChanged);
			}
		}
		[[fallthrough]];
		case kMsgCancel:
			Hide();
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}

/*	FUNCTION:		ProjectSettings :: ValidateTextField
	ARGS:			control
					what
	RETURN:			n/a
	DESCRIPTION:	Validate text field
*/
void ProjectSettings :: ValidateTextField(BTextControl *control, uint32 what)
{
	//	Validate number
	const char *p = control->Text();
	while (*p && isdigit(*p))
		p++;
	bool even_error = false;
	if ((*p == 0) && (control->Text() != nullptr))
	{
		int32 value = atoi(control->Text());

		switch (what)
		{
			case kMsgCustomVideoWidth:
			case kMsgCustomVideoHeight:
				if (value % 2)
					even_error = true;
				else
					UpdateCustomVideoResolution(what);
				break;
			default:
				break;
		}

		//	Valid number ?
		if ((value > 0) && !even_error)
		{
			control->MakeFocus(false);
			return;
		}
	}

	BAlert *alert = new BAlert(nullptr,
							   even_error ? GetText(TXT_EXPORT_INVALID_EVEN_NUMBER) : GetText(TXT_EXPORT_INVALID_NUMBER),
							   "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->Go();

	//	Restore default value
	char buffer[16];
	switch (what)
	{
		case kMsgCustomVideoWidth:
			sprintf(buffer, "%d", gProject->mResolution.width);
			fTextVideoCustomWidth->SetText(buffer);
			UpdateCustomVideoResolution(0);
			break;
		case kMsgCustomVideoHeight:
			sprintf(buffer, "%d", gProject->mResolution.height);
			fTextVideoCustomHeight->SetText(buffer);
			UpdateCustomVideoResolution(1);
			break;
		default:
			assert(0);
			break;
	}
	control->MakeFocus(false);
}

/*	FUNCTION:		ProjectSettings :: UpdateCustomVideoResolution
	ARGS:			msg (kMsgCustomVideoWidth or kMsgCustomVideoHeight)
	RETURN:			n/a
	DESCRIPTION:	Ensure width/height maintains aspect ratio when linked
*/
void ProjectSettings :: UpdateCustomVideoResolution(uint32 msg)
{
	if (fCheckboxCustomResolutionLinked->Value() > 0)
	{
		float aspect = float(gProject->mResolution.width) / float(gProject->mResolution.height);
		char buffer[16];
		switch (msg)
		{
			case kMsgCustomVideoWidth:
			{
				int32 width = atoi(fTextVideoCustomWidth->Text());
				int32 height = int32(width/aspect);
				if (height%2)
					height++;
				sprintf(buffer, "%d", height);
				fTextVideoCustomHeight->SetText(buffer);
				break;
			}
			case kMsgCustomVideoHeight:
			{
				int32 height = atoi(fTextVideoCustomHeight->Text());
				int32 width = int32(aspect*height);
				if (width%2)
					width++;
				sprintf(buffer, "%d", width);
				fTextVideoCustomWidth->SetText(buffer);
				break;
			}
			default:
				assert(0);
		}
	}
}

