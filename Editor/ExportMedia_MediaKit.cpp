/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Export Media Thread
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include "interface/OptionPopUp.h"
#include <StorageKit.h>
#include <MediaKit.h>

#include "Project.h"
#include "AudioManager.h"
#include "Language.h"
#include "ExportMediaWindow.h"
#include "ExportMedia_MediaKit.h"

#include "RenderActor.h"

/*	FUNCTION:		Export_MediaKit :: Export_MediaKit
	ARGS:			parent
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Export_MediaKit :: Export_MediaKit(ExportMediaWindow *parent)
	: ExportEngine(parent)
{
	if ((fRenderSemaphore = create_sem(0, "ExportFfmpeg Semaphore")) < B_OK)
	{
		printf("Export_MediaKit() Cannot create fRenderSemaphore\n");
		exit(1);
	}
}

/*	FUNCTION:		Export_MediaKit :: ~Export_MediaKit
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Export_MediaKit :: ~Export_MediaKit()
{
	if (fRenderSemaphore >= B_OK)
		delete_sem(fRenderSemaphore);
}

/*	FUNCTION:		Export_MediaKit :: BuildFileFormatOptions
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Build file format popup
*/
void Export_MediaKit :: BuildFileFormatOptions()
{
	while (mParent->fOptionFileFormat->CountOptions() > 0)
		mParent->fOptionFileFormat->RemoveOptionAt(0);
	fFileFormatCookies.clear();

	media_file_format mfi;
	media_format format, outfmt;
	media_codec_info mci;
	int32 cookie = 0;
	int add_option_index = 0;
	int default_format_index = -1;

	int32 enable_video = mParent->fEnableVideo->Value();
	int32 enable_audio = mParent->fEnableAudio->Value();

	while (get_next_file_format(&cookie, &mfi) == B_OK)
	{
		if ((mfi.capabilities & media_file_format::B_WRITABLE) == 0)
			continue;
#if 0
		printf(" ********** AddOption(%s) ********** (%s)\n", mfi.short_name, mfi.pretty_name);
		printf("read=%d, write=%d, raw_video=%d, encodec_video=%d, raw_audio=%d, encoded_audio=%d\n",
			   mfi.capabilities & media_file_format::B_READABLE,
			   mfi.capabilities & media_file_format::B_WRITABLE,
			   mfi.capabilities & media_file_format::B_KNOWS_RAW_VIDEO,
			   mfi.capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO,
			   mfi.capabilities & media_file_format::B_KNOWS_RAW_AUDIO,
			   mfi.capabilities & media_file_format::B_KNOWS_ENCODED_AUDIO);
#endif
		bool valid = false;
		if (enable_video && ((mfi.capabilities & media_file_format::B_KNOWS_RAW_VIDEO) || (mfi.capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO)))
			valid = true;
		if (enable_audio && ((mfi.capabilities & media_file_format::B_KNOWS_RAW_AUDIO) || (mfi.capabilities & media_file_format::B_KNOWS_ENCODED_AUDIO)))
			valid = true;

		if (valid)
		{
			if ((default_format_index < 0) && (strcmp(mfi.short_name, "avi") == 0))
				default_format_index = add_option_index;

			mParent->fOptionFileFormat->AddOption(mfi.pretty_name, add_option_index);
			add_option_index++;
			fFileFormatCookies.push_back(cookie);
		}
	}


	if (mParent->fOptionFileFormat->CountOptions() > 0)
	{
		if (default_format_index >=0)
			mParent->fOptionFileFormat->SelectOptionFor(default_format_index);
		FileFormatSelectionChanged();
	}
	else
	{
		//	Workaround for Haiku bug, when popup empty it will still display previous entry until clicked
		mParent->fOptionFileFormat->AddOption(GetText(TXT_EXPORT_FILE_FORMAT_NONE), 0);
		mParent->fOptionFileFormat->RemoveOptionAt(0);
		FileFormatSelectionChanged();
	}
}

/*	FUNCTION:		Export_MediaKit :: FileFormatSelectionChanged
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Rebuild video/audio codecs
*/
void Export_MediaKit :: FileFormatSelectionChanged()
{
	int selected_option = mParent->fOptionFileFormat->SelectedOption();
	if (selected_option >= 0)
	{
		int32 cookie = 0;
		media_file_format mfi;
		while (get_next_file_format(&cookie, &mfi) == B_OK)
		{
			if (fFileFormatCookies[selected_option] == cookie)
			{
				BString aString("/boot/home/video.");
				aString.Append(mfi.short_name);
				mParent->fTextOutFile->SetText(aString);
				break;
			}
		}
	}
	else
	{
		if (mParent->fTextOutFile)
			mParent->fTextOutFile->SetText("");
	}

	if (mParent->fHasVideo)
		BuildVideoCodecOptions();
	if (mParent->fHasAudio)
		BuildAudioCodecOptions();
}

/*	FUNCTION:		Export_MediaKit :: BuildVideoCodecOptions
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Build video codec popup
*/
void Export_MediaKit :: BuildVideoCodecOptions()
{
	while (mParent->fOptionVideoCodec->CountOptions() > 0)
		mParent->fOptionVideoCodec->RemoveOptionAt(0);
	if (fFileFormatCookies.empty())
	{
		//	Workaround for Haiku bug, when popup empty it will still display previous entry until clicked
		mParent->fOptionVideoCodec->AddOption(GetText(TXT_EXPORT_FILE_FORMAT_NONE), 0);
		mParent->fOptionVideoCodec->RemoveOptionAt(0);
		return;
	}

	int32 selected_cookie = fFileFormatCookies[mParent->fOptionFileFormat->SelectedOption()];
	media_file_format mfi;
	media_format format, outfmt;
	media_codec_info mci;
	int32 cookie = 0;
	int add_option_index = 0;
	int default_codec_index = 0;
	while (get_next_file_format(&cookie, &mfi) == B_OK)
	{
		if (cookie != selected_cookie)
			continue;

		int cookie2 = 0;
		memset(&format, 0, sizeof(media_format));
		format.type = B_MEDIA_RAW_VIDEO;
		format.u.raw_video = media_raw_video_format::wildcard;
		while (get_next_encoder(&cookie2, &mfi, &format, &outfmt, &mci) == B_OK)
		{
			mParent->fOptionVideoCodec->AddOption(mci.pretty_name, add_option_index);
			if (strstr(mci.pretty_name, "MPEG-4") != nullptr)
				default_codec_index = add_option_index;
			add_option_index++;
		}
		break;
	}
	if (mParent->fOptionVideoCodec->CountOptions() > 0)
		mParent->fOptionVideoCodec->SelectOptionFor(default_codec_index);
	else
	{
		//	Workaround for Haiku bug, when popup empty it will still display previous entry until clicked
		mParent->fOptionVideoCodec->AddOption(GetText(TXT_EXPORT_FILE_FORMAT_NONE), 0);
		mParent->fOptionVideoCodec->RemoveOptionAt(0);
	}
}

/*	FUNCTION:		Export_MediaKit :: BuildAudioCodecOptions
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Build audio codec popup
*/
void Export_MediaKit :: BuildAudioCodecOptions()
{
	while (mParent->fOptionAudioCodec->CountOptions() > 0)
		mParent->fOptionAudioCodec->RemoveOptionAt(0);
	if (fFileFormatCookies.empty())
	{
		//	Workaround for Haiku bug, when popup empty it will still display previous entry until clicked
		mParent->fOptionAudioCodec->AddOption(GetText(TXT_EXPORT_FILE_FORMAT_NONE), 0);
		mParent->fOptionAudioCodec->RemoveOptionAt(0);
		return;
	}

	int selected_cookie = fFileFormatCookies[mParent->fOptionFileFormat->SelectedOption()];
	media_file_format mfi;
	media_format format, outfmt;
	media_codec_info mci;
	int32 cookie = 0;
	int add_option_index = 0;
	int default_codec_index = 0;
	while (get_next_file_format(&cookie, &mfi) == B_OK)
	{
		if (cookie != selected_cookie)
			continue;

		int cookie2 = 0;
		memset(&format, 0, sizeof(media_format));
		format.type = B_MEDIA_RAW_AUDIO;
		while (get_next_encoder(&cookie2, &mfi, &format, &outfmt, &mci) == B_OK)
		{
			if (strcmp(mci.short_name, "flac") == 0)
			{
				BString aString("<** broken **>");
				aString.Append(mci.pretty_name);
				mParent->fOptionAudioCodec->AddOption(aString.String(), add_option_index);
			}
			else
				mParent->fOptionAudioCodec->AddOption(mci.pretty_name, add_option_index);
			if (strcmp(mci.short_name, "ac3") == 0)
				default_codec_index = add_option_index;
			add_option_index++;
		}
	}
	if (mParent->fOptionAudioCodec->CountOptions() > 0)
		mParent->fOptionAudioCodec->SelectOptionFor(default_codec_index);
	else
	{
		//	Workaround for Haiku bug, when popup empty it will still display previous entry until clicked
		mParent->fOptionAudioCodec->AddOption(GetText(TXT_EXPORT_FILE_FORMAT_NONE), 0);
		mParent->fOptionAudioCodec->RemoveOptionAt(0);
	}
}

float Export_MediaKit :: AddCustomAudioGui(float start_y)
{
	BStringView *string_view = new BStringView(BRect(150, start_y, 620, start_y + 40), "warning", "WARNING - current BMediaKit Audio Encoder is broken");
	string_view->SetFont(be_bold_font);
	mParent->fBackgroundView->AddChild(string_view);
	//start_y += 40;

	return start_y;
}

/*	FUNCTION:		Export_MediaKit :: StartEncode
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Export Media
*/
void Export_MediaKit :: StartEncode()
{
	printf("Export_MediaKit::StartEncode(%s)\n", mParent->fTextOutFile->Text());
	fKeepAlive = true;
	fThread = spawn_thread((status_t (*)(void *))WorkThread, "Export_MediaKit::WorkThread", B_NORMAL_PRIORITY, this);
	resume_thread(fThread);
}

/*	FUNCTION:		Export_MediaKit :: StartEncode
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Export Media
*/
void Export_MediaKit :: StopEncode(const bool complete)
{
	printf("Export_MediaKit::StopEncode(%d)\n", complete);
	if (!complete)
	{
		fKeepAlive = false;
		int attempt = 0;
		while ((fThread > 0) && (attempt < 2000))
		{
			snooze(1000);
			attempt++;
		}
		if (fThread > B_OK)
			kill_thread(fThread);
	}
}


/**************************************************************************
	The Export_MediaKit::WorkThread() needs to be replaced with Actors
****************************************************************************/

/*	FUNCTION:		Export_MediaKit :: WorkThread
	ARGS:			arg
	RETURN:			thread exit status
	DESCRIPTION:	Export Media thread
*/
int32 Export_MediaKit :: WorkThread(void *arg)
{
	assert(arg != nullptr);
	Export_MediaKit *instance = (Export_MediaKit *) arg;
	ExportMediaWindow *mParent = instance->mParent;

	//	entry_ref
	entry_ref ref;
	status_t err;
	err = get_ref_for_path(mParent->fTextOutFile->Text(), &ref);
	if ((err != B_OK) && (err != B_ENTRY_NOT_FOUND))
	{
		printf("Export_MediaKit::StartEncode() Problem with get_ref_for_path() %s\n", strerror(err));
		BAlert *alert = new BAlert("Export_MediaKit::Start Encode()", "Cannot open file (1)", "OK");
		alert->Go();
		return err;
	}

	//	BMediaFormat
	media_file_format mfi;
	media_format format;
	media_format audio_format;
	media_codec_info mci;
	BMediaTrack *video_track = nullptr, *audio_track = nullptr;
	float video_frame_rate;

	int32 cookie = 0;
	int32 selected_cookie = instance->fFileFormatCookies[mParent->fOptionFileFormat->SelectedOption()];
	while (get_next_file_format(&cookie, &mfi) == B_OK)
	{
		if (cookie == selected_cookie)
			break;
	}
	assert(selected_cookie == cookie);

	//	Out BMediFile
	BMediaFile *out = new BMediaFile(&ref, &mfi, B_MEDIA_FILE_BIG_BUFFERS);
	err = out->InitCheck();
	if (err != B_OK)
	{
		printf("failed to properly init the output file... (%s)\n", strerror(err));
		delete out;
		BAlert *alert = new BAlert("Start Encode", "Cannot open file (2)", "OK");
		alert->Go();
		return err;
	}

	//	Video Encoder
	if (mParent->fEnableVideo->Value() > 0)
	{
		const uint32 width = mParent->GetSelectedVideoWidth();
		const uint32 height = mParent->GetSelectedVideoHeight();
		video_frame_rate = mParent->GetSelectedVideoFrameRate();
		memset(&format, 0, sizeof(media_format));
		media_raw_video_format *rvf = &format.u.raw_video;
		format.type = B_MEDIA_RAW_VIDEO;
		rvf->first_active = 0;
		rvf->last_active = height - 1;
		rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
		rvf->interlace = 0;
		rvf->field_rate = video_frame_rate;
		rvf->pixel_width_aspect = 1;
		rvf->pixel_height_aspect = 1;
		rvf->display.format = B_RGB32;
		rvf->display.line_width = width;
		rvf->display.line_count = height;
		rvf->display.bytes_per_row = 4 * width;

		//	Out codec
		cookie = 0;
		media_format outfmt;
		const char *option_video_codec;
		int32 selected_video_codec_idx = mParent->fOptionVideoCodec->SelectedOption(&option_video_codec);
		assert(selected_video_codec_idx >= 0);
		bool video_codec_found = false;
		while (get_next_encoder(&cookie, &mfi, &format, &outfmt, &mci) == B_OK)
		{
			if (strcmp(option_video_codec, mci.pretty_name) == 0)
			{
				video_codec_found = true;
				break;
			}
		}
		assert(video_codec_found);

		//	BMediaTrack
		video_track = out->CreateTrack(&format, &mci);
		if (video_track)
		{
			status_t err = video_track->InitCheck();
			if (err != B_OK)
				printf("BMediaTrack::InitCheck(video) returned (%d, %s)\n", err, strerror(err));
		}

		if (video_track == nullptr)
		{
			BAlert *alert = new BAlert("Start Encode", "Failed to create Video Track", "OK");
			alert->Go();
			return err;
		}
	}

	//	Audio Encoder
	if (mParent->fEnableAudio->Value() > 0)
	{
		memset(&audio_format, 0, sizeof(media_format));
		audio_format.type = B_MEDIA_RAW_AUDIO;

		//	Out codec
		cookie = 0;
		media_format outfmt;
		const char *option_audio_codec;
		int32 selected_audio_codec_idx = mParent->fOptionAudioCodec->SelectedOption(&option_audio_codec);
		assert(selected_audio_codec_idx >= 0);
		bool audio_codec_found = false;
		while (get_next_encoder(&cookie, &mfi, &audio_format, &outfmt, &mci) == B_OK)
		{
			if (strcmp(option_audio_codec, mci.pretty_name) == 0)
			{
				audio_codec_found = true;
				break;
			}
		}
		assert(audio_codec_found);

		uint32 sample_rate = mParent->GetSelectedAudioSampleRate();
#if 1
		media_raw_audio_format *raf = &audio_format.u.raw_audio;
		raf->format = media_raw_audio_format::B_AUDIO_FLOAT;
		raf->frame_rate = sample_rate;
		raf->channel_count = mParent->GetSelectedAudioNumberChannels();
		raf->buffer_size = sample_rate*sizeof(float)*raf->channel_count;
#else
		media_encoded_audio_format *eaf = &audio_format.u.encoded_audio;
		eaf->output.format = media_raw_audio_format::B_AUDIO_FLOAT;
		eaf->output.frame_rate = sample_rate;
		eaf->output.channel_count = mParent->GetSelectedAudioNumberChannels();
		eaf->output.buffer_size = sample_rate*sizeof(float)*eaf->output.channel_count;
#endif
		//	BMediaTrack
		audio_track = out->CreateTrack(&audio_format, &mci);
		if (audio_track)
		{
			status_t err = audio_track->InitCheck();
			if (err != B_OK)
				printf("BMediaTrack::InitCheck(audio) returned (%d, %s)\n", err, strerror(err));
		}

		if (audio_track == nullptr)
		{
			BAlert *alert = new BAlert("Start Encode", "Failed to create Audio Track", "OK");
			alert->Go();
			return err;
		}
	}

	// Add the copyright and commit the header
	out->AddCopyright("Copyright 2021 Medo");
	out->CommitHeader();

	//	Fill video track
	double previous_progress = 0.0;
	if ((mParent->fEnableVideo->Value() > 0) && video_track)
	{
		bigtime_t timeline = 0;
		//while (timeline < gProject->mTotalDuration)
		while ((timeline < gProject->mTotalDuration) && instance->fKeepAlive)
		{
			BBitmap *frame;
			gRenderActor->Async(&RenderActor::AsyncPrepareExportFrame, gRenderActor, timeline, instance->fRenderSemaphore, &frame);
			status_t err;
			while ((err = acquire_sem(instance->fRenderSemaphore)) == B_INTERRUPTED) ;
			if (err == B_OK)
			{
				int attempt = 0;
				do
				{
					err = video_track->WriteFrames(frame->Bits(), 1);
					++attempt;
				} while ((err != B_OK) && (attempt < 3));
				if (err != B_OK)
					printf("Error writing video track (frame_index=%ld) error=%d (%s)\n", timeline, err, strerror(err));
			}
			else
				printf("Export_MediaKit::WorkThread() - cannot acquire fRenderSemaphore\n");
			timeline += kFramesSecond/video_frame_rate;

			double progress = 100.0*(double)timeline / (double)gProject->mTotalDuration;
			if ((progress - previous_progress > 0.1) && (progress < 100.0))
			{
				mParent->fMsgExportEngine->ReplaceFloat("progress", (float)progress);
				mParent->PostMessage(mParent->fMsgExportEngine);
				previous_progress = progress;
			}
		}
		video_track->Flush();
	}

	//	Audio track
	previous_progress = 0.0;
	if ((mParent->fEnableAudio->Value() > 0) && audio_track)
	{
		uint32 sample_rate = mParent->GetSelectedAudioSampleRate();
		media_raw_audio_format *raf = &audio_format.u.raw_audio;
		printf("audio_format.u.raw_audio.buffer_size = %ld\n", raf->buffer_size);
		unsigned char *audio_buffer = new unsigned char [raf->buffer_size];
		raf->format = media_raw_audio_format::B_AUDIO_FLOAT;

		bigtime_t timeline = 0;
		while ((timeline < gProject->mTotalDuration) && instance->fKeepAlive)
		{
			bigtime_t new_timeline = gAudioManager->GetOutputBuffer(timeline, gProject->mTotalDuration,
													  audio_buffer, audio_format.u.raw_audio.buffer_size,
													  *raf);
			const double kTargetConversionRate = (double)kFramesSecond/(double)sample_rate;
			int32 done_frames = int32(double(new_timeline - timeline)/kTargetConversionRate);
			printf("done_frames = %d (%f)\n", done_frames, kTargetConversionRate);
			int attempt = 0;
			do
			{
				err = audio_track->WriteFrames(audio_buffer, done_frames);
				++attempt;
			} while ((err != B_OK) && (attempt < 3));
			if (err != B_OK)
				printf("Error exporting audio (frame_index=%ld) error=%d (%s)\n", timeline, err, strerror(err));

			timeline = new_timeline;

			double progress = (double)timeline / (double)gProject->mTotalDuration;
			if ((progress - previous_progress > 0.1) && (progress < 100.0))
			{
				mParent->fMsgExportEngine->ReplaceFloat("progress", (float)progress);
				mParent->PostMessage(mParent->fMsgExportEngine);
				previous_progress = progress;
			}
		}
		audio_track->Flush();
	}

	if (video_track)
		out->ReleaseTrack(video_track);
	if (audio_track)
		out->ReleaseTrack(audio_track);
	out->CloseFile();
	delete out;

	mParent->fMsgExportEngine->ReplaceFloat("progress", 100.0f);
	mParent->PostMessage(mParent->fMsgExportEngine);
	instance->fThread = 0;
	return B_OK;
}
