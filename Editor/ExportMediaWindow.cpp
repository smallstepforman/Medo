/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Export Media Window
 */

#include <cstdio>
#include <vector>
#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <OptionPopUp.h>
#include <StorageKit.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Math/Math.h"
#include "MedoWindow.h"
#include "Project.h"
#include "Language.h"
#include "Gui/BitmapCheckbox.h"
#include "Gui/ProgressBar.h"

#include "ExportMediaWindow.h"
#include "ExportMedia_MediaKit.h"
#include "ExportMedia_ffmpeg.h"

enum kExportWindowMessages
{
	kMsgEnableVideo	= 'exwm',
	kMsgEnableAudio,
	kMsgSelectFileButton,
	kMsgSelectFileReturn,
	kMsgStartEncode,
	kMsgExportEngine,

	kMsgPopupVideoResolution,
	kMsgPopupVideoFrameRate,
	kMsgPopupVideoCodec,
	kMsgCustomVideoResolution,
	kMsgCustomVideoWidth,
	kMsgCustomVideoHeight,
	kMsgCustomVideoResolutionLinked,
	kMsgPopupVideoBitrate,
	kMsgCustomVideoBitrateEnable,
	kMsgCustomVideoBitrateValue,

	kMsgPopupAudioSampleRate,
	kMsgPopupAudioChannelCount,
	kMsgPopupAudioCodec,
	kMsgPopupFileFormat,
	kMsgPopupAudioBitrate,
	kMsgCustomAudioBitrateEnable,
	kMsgCustomAudioBitrateValue,
};

static const float kGuiHeight = 44;
static const float kGuiOffset = 10;

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
static const float kVideoFrameRates[] = {24, 25, 30, 60, 23.976f, 29.970f, 59.940f};
static const uint32 kVideoBitrates[] = {512, 756, 1024, 1536, 2048, 3072, 4096, 5120, 6144, 8192, 10240, 12288, 16384, 32768};
static const uint32 kDefaultVideoBitrate = 8192;
static const uint32 kAudioSampleRates[] = {22050, 44100, 48000, 96000, 192000};
static const uint32 kDefaultAudioSampleRate = 48000;
static const uint32 kAudioBitrates[] = {96, 128, 160, 192, 224, 256, 320};
static const uint32 kDefaultAudioBitrate = 128;
static const uint32 kAudioNumberChannels[] = {1, 2};

/*	FUNCTION:		ExportMediaWindow :: ExportMediaWindow
	ARGS:			medo_window
					engine
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ExportMediaWindow :: ExportMediaWindow(MedoWindow *parent, EXPORT_ENGINE engine)
	: BWindow(BRect(128, 64, 128+640, 64+940),
				engine == EXPORT_USING_FFMPEG ? GetText(TXT_MENU_PROJECT_EXPORT_FFMPEG) : GetText(TXT_MENU_PROJECT_EXPORT_MEDIA_KIT),
				B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	fMedoWindow = parent;
	PreprocessProject();

	switch (engine)
	{
		case EXPORT_USING_MEDIA_KIT:	fExportEngine = new Export_MediaKit(this);		break;
		case EXPORT_USING_FFMPEG:		fExportEngine = new Export_ffmpeg(this);		break;
	}
	fMsgExportEngine = new BMessage(kMsgExportEngine);
	fMsgExportEngine->AddFloat("progress", 0.0f);
	fState = STATE_INPUT;

	//	Background view
	fBackgroundView = new BView(Bounds(), nullptr, B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_DRAW_ON_CHILDREN);
	fBackgroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fBackgroundView);

	float start_y = 0;
	start_y = CreateFileFormatGui(start_y);
	start_y = CreateVideoGui(start_y);
	start_y = CreateAudioGui(start_y);
	start_y = CreateFileSaveGui(start_y);

	fExportEngine->BuildFileFormatOptions();
}

/*	FUNCTION:		ExportMediaWindow :: ~ExportMediaWindow
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ExportMediaWindow :: ~ExportMediaWindow()
{
	delete fFilePanel;
	delete fExportEngine;
	if (fState == STATE_INPUT)
		delete fExportProgressBar;
}

/*	FUNCTION:		ExportMediaWindow :: PreprocessProject
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Parse project to determine video and audio properties
*/
void ExportMediaWindow :: PreprocessProject()
{
	//	Has Video ?
	fHasVideo = false;
	for (size_t track_idx=0; track_idx < gProject->mTimelineTracks.size(); track_idx++)
	{
		for (const auto &clip : gProject->mTimelineTracks[track_idx]->mClips)
		{
			if ((clip.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO) ||
				(clip.mMediaSourceType == MediaSource::MEDIA_PICTURE))
			{
				fHasVideo = true;
				break;
			}
		}

		if (!fHasVideo)
		{
			for (const auto &effect : gProject->mTimelineTracks[track_idx]->mEffects)
			{
				if (effect->Type() == MediaEffect::MEDIA_EFFECT_IMAGE)
				{
					fHasVideo = true;
					break;
				}
			}
		}
		if (fHasVideo)
			break;
	}

	//	Has Audio ?
	fHasAudio = false;
	for (size_t track_idx=0; track_idx < gProject->mTimelineTracks.size(); track_idx++)
	{
		for (const auto &clip : gProject->mTimelineTracks[track_idx]->mClips)
		{
			if ((clip.mMediaSourceType == MediaSource::MEDIA_AUDIO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO))
			{
				fHasAudio = true;
				break;
			}
		}

		if (!fHasAudio)
		{
			for (const auto &effect : gProject->mTimelineTracks[track_idx]->mEffects)
			{
				if (effect->Type() == MediaEffect::MEDIA_EFFECT_AUDIO)
				{
					fHasAudio = true;
					break;
				}
			}
		}
		if (fHasAudio)
			break;
	}
}

/*	FUNCTION:		ExportMediaWindow :: CreateFileFormatGui
	ARGS:			start_y
	RETURN:			start_y
	DESCRIPTION:	Create file format GUI
*/
float ExportMediaWindow :: CreateFileFormatGui(float start_y)
{
	BStringView *title = new BStringView(BRect(20, start_y, 600, start_y + kGuiHeight), nullptr, GetText(TXT_EXPORT_FILE_FORMAT));
	title->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	title->SetFont(be_bold_font);
	AddChild(title);
	start_y += 50;

	fEnableVideo = new BCheckBox(BRect(20, start_y, 200, start_y + kGuiHeight), "enable_video",
								 GetText(TXT_EXPORT_ENABLE_VIDEO), new BMessage(kMsgEnableVideo));
	fEnableAudio = new BCheckBox(BRect(220, start_y, 400, start_y + kGuiHeight), "enable_audio",
								 GetText(TXT_EXPORT_ENABLE_AUDIO), new BMessage(kMsgEnableAudio));
	if (fHasVideo)
		fEnableVideo->SetValue(1);
	else
		fEnableVideo->SetEnabled(false);
	if (fHasAudio)
		fEnableAudio->SetValue(1);
	else
		fEnableAudio->SetEnabled(false);
	fBackgroundView->AddChild(fEnableVideo);
	fBackgroundView->AddChild(fEnableAudio);
	start_y += kGuiHeight + kGuiOffset;

	fOptionFileFormat = new BOptionPopUp(BRect(20, start_y, 480, start_y+kGuiHeight), "file_format",
										 GetText(TXT_EXPORT_FILE_FORMAT), new BMessage(kMsgPopupFileFormat));
	fBackgroundView->AddChild(fOptionFileFormat);
	start_y += kGuiHeight + kGuiOffset;

	return start_y;
}

/*	FUNCTION:		ExportMediaWindow :: CreateVideoGui
	ARGS:			start_y
	RETURN:			n/a
	DESCRIPTION:	Create Video GUI widgets
*/
float ExportMediaWindow :: CreateVideoGui(float start_y)
{
	BStringView *title = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_EXPORT_VIDEO_SETTINGS));
	title->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	title->SetFont(be_bold_font);
	AddChild(title);
	start_y += kGuiHeight;

	//	Check if video settings required
	if (!fHasVideo)
	{
		BStringView *out_text = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_EXPORT_VIDEO_NO_SOURCES));
		out_text->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(out_text);
		start_y += kGuiHeight;
		return start_y;
	}

	//	Resolution
	fOptionVideoResolution = new BOptionPopUp(BRect(20, start_y, 520, start_y+kGuiHeight), "video_resolution",
											  GetText(TXT_EXPORT_VIDEO_RESOLUTION), new BMessage(kMsgPopupVideoResolution));
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
												 GetText(TXT_EXPORT_VIDEO_CUSTOM_RESOLUTION), new BMessage(kMsgCustomVideoResolution));
	fBackgroundView->AddChild(fEnableCustomVideoResolution);
	fEnableCustomVideoResolution->SetValue(0);

	fTextVideoCustomWidth = new BTextControl(BRect(280, start_y, 420, start_y + kGuiHeight), nullptr,
											 GetText(TXT_EXPORT_VIDEO_CUSTOM_WIDTH), nullptr, new BMessage(kMsgCustomVideoWidth));
	sprintf(text_buffer, "%d", gProject->mResolution.width);
	fTextVideoCustomWidth->SetText(text_buffer);
	fTextVideoCustomWidth->SetEnabled(false);
	fBackgroundView->AddChild(fTextVideoCustomWidth);

	fTextVideoCustomHeight = new BTextControl(BRect(440, start_y, 580, start_y + kGuiHeight), nullptr,
											  GetText(TXT_EXPORT_VIDEO_CUSTOM_HEIGHT), nullptr, new BMessage(kMsgCustomVideoHeight));
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
													  GetText(TXT_EXPORT_VIDEO_FRAME_RATE), new BMessage(kMsgPopupVideoFrameRate));
	int default_frame_rate_selection = 0;
	for (int i=0; i < sizeof(kVideoFrameRates)/sizeof(float); i++)
	{
		if (float(int(kVideoFrameRates[i])) == kVideoFrameRates[i])
			sprintf(text_buffer, "%3.0f %s", kVideoFrameRates[i], GetText(TXT_EXPORT_VIDEO_FPS));
		else
			sprintf(text_buffer, "%0.3f %s", kVideoFrameRates[i], GetText(TXT_EXPORT_VIDEO_FPS));
		fOptionVideoFrameRate->AddOption(text_buffer, i);

		if (ymath::YIsEqual(gProject->mResolution.frame_rate, kVideoFrameRates[i]))
			default_frame_rate_selection = i;
	}
	fOptionVideoFrameRate->SelectOptionFor(default_frame_rate_selection);
	fBackgroundView->AddChild(fOptionVideoFrameRate);
	start_y += kGuiHeight + kGuiOffset;

	//	Video codec
	fOptionVideoCodec = new BOptionPopUp(BRect(20, start_y, 480, start_y+kGuiHeight), "video_codec",
										 GetText(TXT_EXPORT_VIDEO_CODEC), new BMessage(kMsgPopupVideoCodec));
	//	BuildVideoCodecOptions() will populate popup
	fBackgroundView->AddChild(fOptionVideoCodec);
	start_y += kGuiHeight + kGuiOffset;

	//	Video bitrate
	fOptionVideoBitrate = new BOptionPopUp(BRect(20, start_y, 480, start_y+kGuiHeight), "video_bitrate",
										   GetText(TXT_EXPORT_VIDEO_BITRATE), new BMessage(kMsgPopupVideoBitrate));
	int default_bitrate_index = 0;
	for (int i=0; i < sizeof(kVideoBitrates)/sizeof(int32); i++)
	{
		if (kVideoBitrates[i] >= 1000)
			sprintf(text_buffer, "%d,%03d %s", kVideoBitrates[i]/1000, kVideoBitrates[i]%1000, GetText(TXT_EXPORT_VIDEO_KBPS));
		else
			sprintf(text_buffer, "%d %s", kVideoBitrates[i], GetText(TXT_EXPORT_VIDEO_KBPS));
		fOptionVideoBitrate->AddOption(text_buffer, i);

		if (kVideoBitrates[i] == kDefaultVideoBitrate)
			default_bitrate_index = i;
	}
	fOptionVideoBitrate->SelectOptionFor(default_bitrate_index);
	fBackgroundView->AddChild(fOptionVideoBitrate);
	start_y += kGuiHeight;

	//	Custom bitrate
	fEnableCustomVideoBitrate = new BCheckBox(BRect(20, start_y, 260, start_y+kGuiHeight), nullptr,
											  GetText(TXT_EXPORT_VIDEO_CUSTOM_BITRATE), new BMessage(kMsgCustomVideoBitrateEnable));
	fBackgroundView->AddChild(fEnableCustomVideoBitrate);
	fEnableCustomVideoBitrate->SetValue(0);
	fTextVideoCustomBitrate = new BTextControl(BRect(280, start_y, 480, start_y+kGuiHeight), nullptr,
											   GetText(TXT_EXPORT_VIDEO_KBPS), nullptr, new BMessage(kMsgCustomVideoBitrateValue));
	fBackgroundView->AddChild(fTextVideoCustomBitrate);
	sprintf(text_buffer, "%d", kDefaultVideoBitrate);
	fTextVideoCustomBitrate->SetText(text_buffer);
	fTextVideoCustomBitrate->SetEnabled(false);
	//fTextVideoCustomBitrate->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	start_y += kGuiHeight + kGuiOffset;

	//	Engine Custom GUI
	start_y = fExportEngine->AddCustomVideoGui(start_y);

	return start_y;
}

/*	FUNCTION:		ExportMediaWindow :: CreateAudioGui
	ARGS:			start_y
	RETURN:			adjusted start_y
	DESCRIPTION:	Create Audio GUI widgets
*/
float ExportMediaWindow :: CreateAudioGui(float start_y)
{
	BStringView *title = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_EXPORT_AUDIO_SETTINGS));
	title->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	title->SetFont(be_bold_font);
	AddChild(title);
	start_y += kGuiHeight + kGuiOffset;

	//	Check if video settings required
	if (!fHasAudio)
	{
		BStringView *out_text = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_EXPORT_AUDIO_NO_SOURCES));
		out_text->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(out_text);
		start_y += kGuiHeight;
		return start_y;
	}

	//	Audio sample rate
	fOptionAudioSampleRate = new BOptionPopUp(BRect(20, start_y, 440, start_y+kGuiHeight), "audio_frame_rate",
													  GetText(TXT_EXPORT_AUDIO_SAMPLE_RATE), new BMessage(kMsgPopupAudioSampleRate));
	int default_sample_rate_selection = 0;
	for (int i=0; i < sizeof(kAudioSampleRates)/sizeof(float); i++)
	{
		char buffer[32];
		sprintf(buffer, "%d,%03d %s", kAudioSampleRates[i]/1000, kAudioSampleRates[i]%1000, GetText(TXT_EXPORT_AUDIO_HZ));
		fOptionAudioSampleRate->AddOption(buffer, i);

		if (kDefaultAudioSampleRate == kAudioSampleRates[i])
			default_sample_rate_selection = i;
	}
	fOptionAudioSampleRate->SelectOptionFor(default_sample_rate_selection);
	fBackgroundView->AddChild(fOptionAudioSampleRate);

	//	Audio Channel count
	fOptionAudioChannelCount = new BOptionPopUp(BRect(460, start_y, 630, start_y+kGuiHeight), "audio_channels",
													GetText(TXT_EXPORT_AUDIO_CHANNEL_COUNT), new BMessage(kMsgPopupAudioChannelCount));
	int default_channel_count = 0;
	for (int i=0; i < sizeof(kAudioNumberChannels)/sizeof(float); i++)
	{
		char buffer[32];
		sprintf(buffer, "%d", kAudioNumberChannels[i]);
		fOptionAudioChannelCount->AddOption(buffer, i);

		if (kAudioNumberChannels[i] == 2)
			default_channel_count = i;
	}
	fOptionAudioChannelCount->SelectOptionFor(default_channel_count);
	fBackgroundView->AddChild(fOptionAudioChannelCount);
	start_y += kGuiHeight + kGuiOffset;

	// Audio codec
	fOptionAudioCodec = new BOptionPopUp(BRect(20, start_y, 480, start_y+kGuiHeight), "audio_codec",
										 GetText(TXT_EXPORT_AUDIO_CODEC), new BMessage(kMsgPopupAudioCodec));
	//	BuildAudioCodecOptoins() will populate popup
	fBackgroundView->AddChild(fOptionAudioCodec);
	start_y += kGuiHeight + kGuiOffset;

	//	Audio bitrate
	fOptionAudioBitrate = new BOptionPopUp(BRect(20, start_y, 480, start_y+kGuiHeight), "audio_bitrate",
										   GetText(TXT_EXPORT_AUDIO_BITRATE), new BMessage(kMsgPopupAudioBitrate));
	int default_bitrate_index = 0;
	char text_buffer[16];
	for (int i=0; i < sizeof(kAudioBitrates)/sizeof(int32); i++)
	{
		sprintf(text_buffer, "%d %s", kAudioBitrates[i], GetText(TXT_EXPORT_AUDIO_KBPS));
		fOptionAudioBitrate->AddOption(text_buffer, i);

		if (kDefaultAudioBitrate == kAudioBitrates[i])
			default_bitrate_index = i;
	}
	fOptionAudioBitrate->SelectOptionFor(default_bitrate_index);
	fBackgroundView->AddChild(fOptionAudioBitrate);
	start_y += kGuiHeight;

	//	Custom bitrate
	fEnableCustomAudioBitrate = new BCheckBox(BRect(20, start_y, 260, start_y+kGuiHeight), nullptr,
											  GetText(TXT_EXPORT_AUDIO_CUSTOM_BITRATE), new BMessage(kMsgCustomAudioBitrateEnable));
	fBackgroundView->AddChild(fEnableCustomAudioBitrate);
	fEnableCustomAudioBitrate->SetValue(0);
	fTextAudioCustomBitrate = new BTextControl(BRect(280, start_y, 480, start_y+kGuiHeight), nullptr,
											   GetText(TXT_EXPORT_AUDIO_KBPS), nullptr, new BMessage(kMsgCustomAudioBitrateValue));
	fBackgroundView->AddChild(fTextAudioCustomBitrate);
	sprintf(text_buffer, "%d", kDefaultAudioBitrate);
	fTextAudioCustomBitrate->SetText(text_buffer);
	fTextAudioCustomBitrate->SetEnabled(false);
	//fTextAudioCustomBitrate->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	start_y += kGuiHeight + kGuiOffset;

	//	Engine Custom GUI
	start_y = fExportEngine->AddCustomAudioGui(start_y);

	return start_y;
}

/*	FUNCTION:		ExportMediaWindow :: CreateFileSaveGui
	ARGS:			start_y
	RETURN:			none
	DESCRIPTION:	Adjusted start_y
*/
float ExportMediaWindow :: CreateFileSaveGui(float start_y)
{
	fFilePanel = nullptr;
	fButtonStartEncode = nullptr;
	fTextOutFile = nullptr;
	fExportProgressBar = nullptr;

	if (!fHasVideo && !fHasAudio)
		return start_y;

	BStringView *title = new BStringView(BRect(20, start_y, 600, start_y+kGuiHeight), nullptr, GetText(TXT_EXPORT_OUT_TITLE));
	title->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	title->SetFont(be_bold_font);
	AddChild(title);
	start_y += kGuiHeight;

	BButton *select_file_button = new BButton(BRect(20, start_y, 200, start_y+kGuiHeight), nullptr,
									   GetText(TXT_EXPORT_OUT_SAVE_FILE), new BMessage(kMsgSelectFileButton));
	fBackgroundView->AddChild(select_file_button);

	//	TODO cache out file
	fTextOutFile = new BStringView(BRect(220, start_y, 630, start_y+kGuiHeight), nullptr, "/boot/home/video.out");
	fTextOutFile->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fTextOutFile);
	start_y += kGuiHeight + 2*kGuiOffset;

	//`Start encode button
	fButtonStartEncode = new BButton(BRect(430, start_y, 630, start_y+kGuiHeight), nullptr, GetText(TXT_EXPORT_OUT_START_BUTTON), new BMessage(kMsgStartEncode));
	fBackgroundView->AddChild(fButtonStartEncode);

	//	Export progress
	fExportProgressBar = new ProgressBar(BRect(20, start_y, 420, start_y+kGuiHeight), "ProgressBar");
	//start_y += 0.8f*kGuiHeight;
	fTextExportProgress = new BStringView(BRect(180, start_y-4, 300, start_y+kGuiHeight-4), nullptr, "");
	fTextExportProgress->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fTextExportProgress);

	start_y += kGuiHeight + kGuiOffset;

	return start_y;
}

/*	FUNCTION:		ExportMediaWindow :: QuitRequested
	ARGS:			msg
	RETURN:			none
	DESCRIPTION:	Hook function
*/
bool ExportMediaWindow :: QuitRequested()
{
	fMedoWindow->PostMessage(new BMessage(MedoWindow::eMsgActionExportWindowClosed));
	return true;
}

/*	FUNCTION:		ExportMediaWindow :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function called when message received
*/
void ExportMediaWindow :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgEnableVideo:
			fExportEngine->BuildFileFormatOptions();
			break;
		case kMsgEnableAudio:
			fExportEngine->BuildFileFormatOptions();
			break;
		case kMsgPopupFileFormat:
			fExportEngine->FileFormatSelectionChanged();
			break;
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
		case kMsgCustomVideoBitrateValue:	ValidateTextField(fTextVideoCustomBitrate, kMsgCustomVideoBitrateValue);	break;
		case kMsgCustomAudioBitrateValue:	ValidateTextField(fTextAudioCustomBitrate, kMsgCustomAudioBitrateValue);	break;

		case kMsgCustomVideoResolutionLinked:
			UpdateCustomVideoResolution(kMsgCustomVideoWidth);
			break;

		case kMsgCustomVideoBitrateEnable:
			if (fEnableCustomVideoBitrate->Value() == 0)
			{
				fOptionVideoBitrate->SetEnabled(true);
				fTextVideoCustomBitrate->SetEnabled(false);
			}
			else
			{
				fOptionVideoBitrate->SetEnabled(false);
				fTextVideoCustomBitrate->SetEnabled(true);
			}
			break;
		case kMsgCustomAudioBitrateEnable:
			if (fEnableCustomAudioBitrate->Value() == 0)
			{
				fOptionAudioBitrate->SetEnabled(true);
				fTextAudioCustomBitrate->SetEnabled(false);
			}
			else
			{
				fOptionAudioBitrate->SetEnabled(false);
				fTextAudioCustomBitrate->SetEnabled(true);
			}
			break;

		case kMsgSelectFileButton:
		{
			if (fFilePanel)
			{
				fFilePanel->Show();
			}
			else
			{
				fFilePanel = new BFilePanel(B_SAVE_PANEL,	//	file_panel_mode
								nullptr,		//	target
								nullptr,		//	directory
								B_FILE_NODE,				//	nodeFlavors
								false,			//	allowMultipleSelection
								new BMessage(kMsgSelectFileReturn),		//	message
								nullptr,		//	refFilter
								true,			//	modal
								true);			//	hideWhenDone
				fFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Save");
				fFilePanel->Window()->SetTitle(GetText(TXT_EXPORT_OUT_SAVE_FILE));
				fFilePanel->SetTarget(this);
				fFilePanel->Show();
			}
			break;
		}
		case kMsgSelectFileReturn:
		{
			entry_ref ref;
			BString name;
			if ((msg->FindRef("directory", &ref) != B_OK) || (msg->FindString("name", &name) != B_OK))
			{
				BAlert *alert = new BAlert("Export Media Error", "BMessage::missing entry_ref(\"directory\")/string(\"name\")", "OK");
				alert->Go();
				break;
			}
			BEntry entry(&ref);
			if (entry.InitCheck() != B_NO_ERROR)
			{
				BAlert *alert = new BAlert("Export Media Error", "BMessage::invalid BEntry(directory)", "OK");
				alert->Go();
				break;
			}
			BPath path;
			entry.GetPath(&path);
			path.Append(name);
			BString aString(path.Path());
			be_plain_font->TruncateString(&aString, B_TRUNCATE_SMART, fTextOutFile->Bounds().Width());
			fTextOutFile->SetText(aString.String());
			break;
		}

		case kMsgStartEncode:
			switch (fState)
			{
				case STATE_INPUT:		StartEncode();			break;
				case STATE_ENCODING:	StopEncode(false);		break;
			}
			break;

		case kMsgExportEngine:
		{
			float progress;
			if (msg->FindFloat("progress", &progress) == B_OK)
			{
				fExportProgressBar->SetValue(progress/100.0f);
				char buffer[16];
				sprintf(buffer, "%0.2f%%", progress);
				fTextExportProgress->SetText(buffer);
				fTextExportProgress->Invalidate(fTextOutFile->Bounds());
				if (progress >= 100.0f)
					StopEncode(true);
			}
			break;
		}

		default:
			if (!fExportEngine->MessageRedirect(msg))
			{
				//printf("ExportMediaWindow::MessageReceived()\n");
				//msg->PrintToStream();
				BWindow::MessageReceived(msg);
			}
	}
}

/*	FUNCTION:		ExportMediaWindow :: ValidateTextField
	ARGS:			control
					what
	RETURN:			n/a
	DESCRIPTION:	Validate text field
*/
void ExportMediaWindow :: ValidateTextField(BTextControl *control, uint32 what)
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
		case kMsgCustomVideoBitrateValue:
			sprintf(buffer, "%d", kDefaultVideoBitrate);
			fTextVideoCustomBitrate->SetText(buffer);
			break;
		case kMsgCustomAudioBitrateValue:
			sprintf(buffer, "%d", kDefaultAudioBitrate);
			fTextAudioCustomBitrate->SetText(buffer);
			break;
		default:
			assert(0);
			break;
	}
	control->MakeFocus(false);
}

/*	FUNCTION:		ExportMediaWindow :: UpdateCustomVideoResolution
	ARGS:			msg (kMsgCustomVideoWidth or kMsgCustomVideoHeight)
	RETURN:			n/a
	DESCRIPTION:	Ensure width/height maintains aspect ratio when linked
*/
void ExportMediaWindow :: UpdateCustomVideoResolution(uint32 msg)
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

/*	FUNCTION:		ExportMediaWindow :: GetSelectedVideoWidth
	ARGS:			none
	RETURN:			width
	DESCRIPTION:	Get Video Width
*/
uint32 ExportMediaWindow :: GetSelectedVideoWidth() const
{
	if (fEnableCustomVideoResolution->Value())
	{
		return uint32(atoi(fTextVideoCustomWidth->Text()));
	}
	else
	{
		int32 index = fOptionVideoResolution->SelectedOption();
		assert((index >= 0) && (index < sizeof(kVideoResolutions)/sizeof(VIDEO_RESOLUTION)));
		return kVideoResolutions[index].width;
	}
}

/*	FUNCTION:		ExportMediaWindow :: GetSelectedVideoHeight
	ARGS:			none
	RETURN:			height
	DESCRIPTION:	Get Video height
*/
uint32 ExportMediaWindow :: GetSelectedVideoHeight() const
{
	if (fEnableCustomVideoResolution->Value())
	{
		return uint32(atoi(fTextVideoCustomHeight->Text()));
	}
	else
	{
		int32 index = fOptionVideoResolution->SelectedOption();
		assert((index >= 0) && (index < sizeof(kVideoResolutions)/sizeof(VIDEO_RESOLUTION)));
		return kVideoResolutions[index].height;
	}
}

/*	FUNCTION:		ExportMediaWindow :: GetSelectedVideoFrameRate
	ARGS:			none
	RETURN:			frame rate
	DESCRIPTION:	Get Video frame rate
*/
float ExportMediaWindow :: GetSelectedVideoFrameRate() const
{
	int index = fOptionVideoFrameRate->SelectedOption();
	assert((index >= 0) && (index < sizeof(kVideoFrameRates)/sizeof(float)));
	return kVideoFrameRates[index];
}

/*	FUNCTION:		ExportMediaWindow :: GetSelectedVideoBitrate
	ARGS:			none
	RETURN:			video bit rate
	DESCRIPTION:	Get Video bitrate
*/
uint32 ExportMediaWindow :: GetSelectedVideoBitrate() const
{
	if (fEnableCustomVideoBitrate)
	{
		return uint32(atoi(fTextVideoCustomBitrate->Text()) * 1024);
	}
	else
	{
		int32 index = fOptionVideoBitrate->SelectedOption();
		assert((index >= 0) && (index < sizeof(kVideoBitrates)/sizeof(uint32)));
		return kVideoBitrates[index] * 1024;
	}
}

/*	FUNCTION:		ExportMediaWindow :: GetSelectedAudioSampleRate
	ARGS:			none
	RETURN:			frame rate
	DESCRIPTION:	Get Video frame rate
*/
uint32 ExportMediaWindow :: GetSelectedAudioSampleRate() const
{
	int index = fOptionAudioSampleRate->SelectedOption();
	assert((index >= 0) && (index < sizeof(kAudioSampleRates)/sizeof(float)));
	return kAudioSampleRates[index];
}

/*	FUNCTION:		ExportMediaWindow :: GetSelectedAudioNumberChannels
	ARGS:			none
	RETURN:			number channels
	DESCRIPTION:	Get Number of Audio channels
*/
uint32 ExportMediaWindow :: GetSelectedAudioNumberChannels() const
{
	int index = fOptionAudioChannelCount->SelectedOption();
	assert((index >= 0) && (index < sizeof(kAudioNumberChannels)/sizeof(float)));
	return kAudioNumberChannels[index];
}

/*	FUNCTION:		ExportMediaWindow :: GetSelectedAudioBitrate
	ARGS:			none
	RETURN:			frame rate
	DESCRIPTION:	Get Video frame rate
*/
uint32 ExportMediaWindow :: GetSelectedAudioBitrate() const
{
	if (fEnableCustomVideoBitrate)
	{
		return uint32(atoi(fTextAudioCustomBitrate->Text()) * 1024);
	}
	else
	{
		int index = fOptionAudioBitrate->SelectedOption();
		assert((index >= 0) && (index < sizeof(kAudioBitrates)/sizeof(float)));
		return kAudioBitrates[index] * 1024;
	}
}

/*	FUNCTION:		ExportMediaWindow :: StartEncode
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Start encode process
*/
void ExportMediaWindow :: StartEncode()
{
	fExportEngine->StartEncode();
	fButtonStartEncode->SetLabel(GetText(TXT_EXPORT_OUT_CANCEL_BUTTON));
	fState = STATE_ENCODING;
	AddChild(fExportProgressBar);
	RemoveChild(fTextExportProgress);
	AddChild(fTextExportProgress);
	fExportProgressBar->SetValue(0.0f);
}

/*	FUNCTION:		ExportMediaWindow :: StopEncode
	ARGS:			complete
	RETURN:			n/a
	DESCRIPTION:	Stop encode process
*/
void ExportMediaWindow :: StopEncode(const bool complete)
{
	fExportEngine->StopEncode(complete);
	RemoveChild(fExportProgressBar);
	fState = STATE_INPUT;
	fButtonStartEncode->SetLabel(GetText(TXT_EXPORT_OUT_START_BUTTON));
	if (complete)
		fTextExportProgress->SetText(GetText(TXT_EXPORT_OUT_PROGRESS_COMPLETE));
	else
		fTextExportProgress->SetText(GetText(TXT_EXPORT_OUT_PROGRESS_CANCELLED));
}
