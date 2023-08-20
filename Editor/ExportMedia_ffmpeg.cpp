/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Export Media using ffmpeg
 */

#include <cstdio>
#include <cassert>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>

#include "Project.h"
#include "AudioManager.h"
#include "Language.h"
#include "ExportMediaWindow.h"
#include "ExportMedia_ffmpeg.h"
#include "Actor/Actor.h"

#include "RenderActor.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

struct FFMPEG_CODEC_COMPLIANCE
{
	LANGUAGE_TEXT	text;
	int			value;
};
static const FFMPEG_CODEC_COMPLIANCE kCompliance[] =
{
	{TXT_EXPORT_COMPLIANCE_VERY_STRICT,		FF_COMPLIANCE_VERY_STRICT},
	{TXT_EXPORT_COMPLIANCE_STRICT,			FF_COMPLIANCE_STRICT},
	{TXT_EXPORT_COMPLIANCE_NORMAL,			FF_COMPLIANCE_NORMAL},
	{TXT_EXPORT_COMPLIANCE_UNOFFICIAL,		FF_COMPLIANCE_UNOFFICIAL},
	{TXT_EXPORT_COMPLIANCE_EXPERIMENTAL,	FF_COMPLIANCE_EXPERIMENTAL},
};

enum kFfmpegGuiMessages
{
	kMsgVideoComplianceSelected	= 'exff',
	kMsgAudioComplianceSelected,
};

/******************************************/
class Ffmpeg_Actor : public yarra::Actor
{
	Export_ffmpeg		*fParent;
public:
			Ffmpeg_Actor(Export_ffmpeg *parent);
	void	Async_Start(int i);
	void	Async_Stop(const bool complete);
	void	Async_VideoFrameAvailable(BBitmap *frame);
	void	Async_AudioFrameAvailable(void *);
private:
	void	PrepareNextFrame();
};

/*	FUNCTION:		Export_ffmpeg :: Export_ffmpeg
	ARGS:			parent
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Export_ffmpeg :: Export_ffmpeg(ExportMediaWindow *parent)
	: ExportEngine(parent)
{
	fWorkActor = nullptr;
	fThread = 0;
	if ((fRenderSemaphore = create_sem(0, "ExportFfmpeg Semaphore")) < B_OK)
	{
		printf("Export_ffmpeg() Cannot create fRenderSemaphore\n");
		exit(1);
	}
}

/*	FUNCTION:		Export_ffmpeg :: ~Export_ffmpeg
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Export_ffmpeg :: ~Export_ffmpeg()
{
	if (fRenderSemaphore >= B_OK)
		delete_sem(fRenderSemaphore);
}

/*	FUNCTION:		Export_ffmpeg :: AddCustomVideoGui
	ARGS:			start_y
	RETURN:			Updated start_y
	DESCRIPTION:	Add Compliance popup to video codec
*/
float Export_ffmpeg :: AddCustomVideoGui(float start_y)
{
	if (mParent->fHasVideo)
	{
		start_y -= 152;
		fOptionVideoCodecCompliance = new BOptionPopUp(BRect(480, start_y, 640, start_y+44), "compliance", nullptr, new BMessage(kMsgVideoComplianceSelected));
		mParent->fBackgroundView->AddChild(fOptionVideoCodecCompliance);

		for (int32 i=0; i < sizeof(kCompliance)/sizeof(FFMPEG_CODEC_COMPLIANCE); i++)
			fOptionVideoCodecCompliance->AddOption(GetText(kCompliance[i].text), i);
		start_y += 152;
	}
	return start_y;
}

/*	FUNCTION:		Export_ffmpeg :: AddCustomAudioGui
	ARGS:			start_y
	RETURN:			Updated start_y
	DESCRIPTION:	Add Compliance popup to audio codec
*/
float Export_ffmpeg :: AddCustomAudioGui(float start_y)
{
	if (mParent->fHasAudio)
	{
		start_y -= 152;
		fOptionAudioCodecCompliance = new BOptionPopUp(BRect(480, start_y, 640, start_y+44), "compliance", nullptr, new BMessage(kMsgAudioComplianceSelected));
		mParent->fBackgroundView->AddChild(fOptionAudioCodecCompliance);

		for (int32 i=0; i < sizeof(kCompliance)/sizeof(FFMPEG_CODEC_COMPLIANCE); i++)
			fOptionAudioCodecCompliance->AddOption(GetText(kCompliance[i].text), i);

		start_y += 152;
	}
	return start_y;
}

/*	FUNCTION:		Export_ffmpeg :: MessageRedirect
	ARGS:			msg
	RETURN:			true if processed
	DESCRIPTION:	Trap custom GUI messages
*/
bool Export_ffmpeg :: MessageRedirect(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgVideoComplianceSelected:
			if (mParent->fHasVideo)
				BuildVideoCodecOptions();
			return true;

		case kMsgAudioComplianceSelected:
			if (mParent->fHasAudio)
				BuildAudioCodecOptions();
			return true;

		default:
			break;
	}
	return false;
}

/*	FUNCTION:		Export_ffmpeg :: BuildFileFormatOptions
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Build file format popup
*/
void Export_ffmpeg :: BuildFileFormatOptions()
{
	while (mParent->fOptionFileFormat->CountOptions() > 0)
		mParent->fOptionFileFormat->RemoveOptionAt(0);
	fFileFormatCookies.clear();

	bool has_video = mParent->fEnableVideo->Value() > 0;
	bool has_audio = mParent->fEnableAudio->Value() > 0;

	int add_option_index = 0;
	int default_format_index = -1;
	const AVOutputFormat *format;
	void *format_opaque = NULL;
	while((format = av_muxer_iterate(&format_opaque)) != nullptr)
	{
		//	Remove test formats (like crc, framecrc, hash, md5 etc)
		if (format->extensions == nullptr)
			continue;

		//	Filter formats with invalid codecs
		if (has_video && (format->video_codec == AV_CODEC_ID_NONE))
			continue;
		if (has_audio && (format->audio_codec == AV_CODEC_ID_NONE))
			continue;

		// Check whether formats which report support for video/audio actually have available codec
		bool valid_video = false;
		bool valid_audio = false;
		const AVCodec *codec;
		void *codac_opaque = nullptr;
		int video_compliance_option = (has_video ? kCompliance[fOptionVideoCodecCompliance->SelectedOption()].value : 0);
		int audio_compliance_option = (has_audio ? kCompliance[fOptionAudioCodecCompliance->SelectedOption()].value : 0);
		while((codec = av_codec_iterate(&codac_opaque)) != nullptr)
		{
			if ((codec->type == AVMEDIA_TYPE_VIDEO) && (avformat_query_codec(format, codec->id, video_compliance_option) == 1))
				valid_video = true;

			if ((codec->type == AVMEDIA_TYPE_AUDIO) && (avformat_query_codec(format, codec->id, audio_compliance_option) == 1))
				valid_audio = true;

			if (has_video && has_audio)
			{
				if (valid_audio && valid_video)
					break;
			}
			else if (has_video && valid_video)
				break;
			else if (has_audio && valid_audio)
				break;
		}

		//	Check if format supports current user select (ie. for audio+video formats, make sure both audio+video codecs exists
		bool valid = false;
		if (has_video && has_audio)
		{
			if (valid_video && valid_audio)
				valid = true;
		}
		else if (has_video)
		{
			if (valid_video)
				valid = true;
		}
		else if (has_audio)
		{
			if (valid_audio)
				valid = true;
		}

		//	Filter [asf_stream] since it appears twice
		if (valid && (strcmp(format->name, "asf_stream") == 0))
			valid = false;

		//	Add the format
		if (valid)
		{
			printf("[%s] %s <%s>\n", format->name, format->long_name, format->extensions);
			mParent->fOptionFileFormat->AddOption(format->long_name, add_option_index);
			if ((default_format_index < 0) && (strcmp(format->name, "mp4") == 0))
				default_format_index = add_option_index;
			add_option_index++;
			fFileFormatCookies.push_back(format);
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

/*	FUNCTION:		Export_ffmpeg :: FileFormatSelectionChanged
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Rebuild video/audio codecs
*/
void Export_ffmpeg :: FileFormatSelectionChanged()
{
	int selected_option = mParent->fOptionFileFormat->SelectedOption();
	if (selected_option >= 0)
	{
		const AVOutputFormat *format = fFileFormatCookies[mParent->fOptionFileFormat->SelectedOption()];
		BString aString("/boot/home/video.");
		const char *p = format->extensions;
		while (p && *p && (*p != ','))
			aString.Append(*p++, 1);
		//sprintf(outfile, "/boot/home/video.%s", format->extensions);
		mParent->fTextOutFile->SetText(aString);
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

/*	FUNCTION:		Export_ffmpeg :: BuildVideoCodecOptions
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Build video codec popup
*/
void Export_ffmpeg :: BuildVideoCodecOptions()
{
	assert(mParent->fHasVideo);

	while (mParent->fOptionVideoCodec->CountOptions() > 0)
		mParent->fOptionVideoCodec->RemoveOptionAt(0);
	fVideoCodecCookies.clear();
	if (fFileFormatCookies.empty() || (mParent->fEnableVideo->Value() == 0))
	{
		//	Workaround for Haiku bug, when popup empty it will still display previous entry until clicked
		mParent->fOptionVideoCodec->AddOption(GetText(TXT_EXPORT_FILE_FORMAT_NONE), 0);
		mParent->fOptionVideoCodec->RemoveOptionAt(0);
		return;
	}

	const AVOutputFormat *format = fFileFormatCookies[mParent->fOptionFileFormat->SelectedOption()];
	int compliance_option = kCompliance[fOptionVideoCodecCompliance->SelectedOption()].value;
	printf("BuildVideoCodecOptions(%d) [%s] %s <%s>\n", compliance_option, format->name, format->long_name, format->extensions);

	int add_option_index = 0;
	int default_codec_index = -1;
	const AVCodec *codec;
	void *codac_opaque = nullptr;
	std::vector<AVCodecID> unique_codecs;
	while((codec = av_codec_iterate(&codac_opaque)) != nullptr)
	{
		// only add the codec if it can be used with this container
		if (avformat_query_codec(format, codec->id, compliance_option) == 1)
		{
			if (codec->type == AVMEDIA_TYPE_VIDEO)
			{
				if (std::find(unique_codecs.begin(), unique_codecs.end(), codec->id) == unique_codecs.end())
				{
					printf("   [%s] Video Codec: %s (%x)\n", codec->name, codec->long_name, codec->id);
					mParent->fOptionVideoCodec->AddOption(codec->long_name, add_option_index);
					if ((default_codec_index < 0) && (format->video_codec == codec->id))
						default_codec_index = add_option_index;
					add_option_index++;
					unique_codecs.push_back(codec->id);
					fVideoCodecCookies.push_back(codec->id);
				}
			}
		}
	}

	//	Set Default Option
	if (mParent->fOptionVideoCodec->CountOptions() > 0)
	{
		if (default_codec_index >=0)
			mParent->fOptionVideoCodec->SelectOptionFor(default_codec_index);
	}
	else
	{
		//	Workaround for Haiku bug, when popup empty it will still display previous entry until clicked
		mParent->fOptionVideoCodec->AddOption(GetText(TXT_EXPORT_FILE_FORMAT_NONE), 0);
		mParent->fOptionVideoCodec->RemoveOptionAt(0);
	}

}

/*	FUNCTION:		Export_ffmpeg :: BuildAudioCodecOptions
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Build audio codec popup
*/
void Export_ffmpeg :: BuildAudioCodecOptions()
{
	assert(mParent->fHasAudio);

	while (mParent->fOptionAudioCodec->CountOptions() > 0)
		mParent->fOptionAudioCodec->RemoveOptionAt(0);
	fAudioCodecCookies.clear();
	if (fFileFormatCookies.empty() || (mParent->fEnableAudio->Value() == 0))
	{
		//	Workaround for Haiku bug, when popup empty it will still display previous entry until clicked
		mParent->fOptionAudioCodec->AddOption(GetText(TXT_EXPORT_FILE_FORMAT_NONE), 0);
		mParent->fOptionAudioCodec->RemoveOptionAt(0);
		return;
	}

	const AVOutputFormat *format = fFileFormatCookies[mParent->fOptionFileFormat->SelectedOption()];
	int compliance_option = kCompliance[fOptionAudioCodecCompliance->SelectedOption()].value;
	printf("BuildAudioCodecOptions(%d) [%s] %s <%s>\n", compliance_option, format->name, format->long_name, format->extensions);

	int add_option_index = 0;
	int default_codec_index = -1;
	const AVCodec *codec;
	void *codec_opaque = nullptr;
	std::vector<AVCodecID> unique_codecs;
	while((codec = av_codec_iterate(&codec_opaque)) != nullptr)
	{
		// only add the codec if it can be used with this container
		if (avformat_query_codec(format, codec->id, compliance_option) == 1)
		{
			if (codec->type == AVMEDIA_TYPE_AUDIO)
			{
				if (std::find(unique_codecs.begin(), unique_codecs.end(), codec->id) == unique_codecs.end())
				{
					printf("   [%s] Audio Codec: %s (%x)\n", codec->name, codec->long_name, codec->id);
					mParent->fOptionAudioCodec->AddOption(codec->long_name, add_option_index);
					if ((default_codec_index < 0) && (format->audio_codec == codec->id))
						default_codec_index = add_option_index;

					//	ffmpeg aac encoder has corruption, should default to ac3 if available
					if (strcmp(codec->name, "aac") == 0)
						default_codec_index = add_option_index;

					add_option_index++;
					unique_codecs.push_back(codec->id);
					fAudioCodecCookies.push_back(codec->id);
				}
			}
		}
	}

	//	Set Default Option
	if (mParent->fOptionAudioCodec->CountOptions() > 0)
	{
		if (default_codec_index >= 0)
			mParent->fOptionAudioCodec->SelectOptionFor(default_codec_index);
	}
	else
	{
		//	Workaround for Haiku bug, when popup empty it will still display previous entry until clicked
		mParent->fOptionAudioCodec->AddOption(GetText(TXT_EXPORT_FILE_FORMAT_NONE), 0);
		mParent->fOptionAudioCodec->RemoveOptionAt(0);
	}
}






/**************************************************************************
	This is the ffmpeg encoder, C API
	Should be replaced with modern C++ API since the C code is terrible
	The Export_ffmpeg::WorkThread() needs to be replaced with Actors
	Lots of static variables to allow abrupt thread termination
****************************************************************************/


extern "C" {
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
}

#define SCALE_FLAGS			SWS_BICUBIC

static AVFormatContext		*sAVFormatContext = nullptr;
static const size_t sErrorBufferSize = 0x100;
static char sErrorBuffer[sErrorBufferSize];

static void AlertFfmpegExit(int ret, const char *title)
{
	av_strerror(ret, sErrorBuffer, sErrorBufferSize);
	printf("AlertFfmpegExit(%s) %s\n", title, sErrorBuffer);
	BAlert *alert = new BAlert("ffmpeg alert", sErrorBuffer, "Dismiss");
	alert->Go();

	if (sAVFormatContext)
		avformat_free_context(sAVFormatContext);
	exit(1);
}

/// a wrapper around a single output AVStream
struct OutputStream
{
	AVStream *st;
	AVCodecContext *enc;

	/* pts of the next frame that will be generated */
	int64_t next_pts;

	AVFrame *frame;
	AVFrame *tmp_frame;

	struct SwsContext *sws_ctx;
};

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;

	/* Write the compressed frame to the media file. */
	return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
void Export_ffmpeg :: add_stream(OutputStream *ost, AVFormatContext *oc, const AVCodec **codec, int codec_id)
{
	AVCodecContext *c;
	int i;

	/* find the encoder */
	*codec = avcodec_find_encoder((AVCodecID)codec_id);
	if (!(*codec))
	{
		fprintf(stderr, "Could not find encoder for '%s'\n",
		avcodec_get_name((AVCodecID)codec_id));
		exit(1);
	}

	ost->st = avformat_new_stream(oc, NULL);
	if (!ost->st) {
		fprintf(stderr, "Could not allocate stream\n");
		exit(1);
	}
	ost->st->id = oc->nb_streams-1;
	c = avcodec_alloc_context3(*codec);
	if (!c) {
		fprintf(stderr, "Could not alloc an encoding context\n");
		exit(1);
	}
	ost->enc = c;

	switch ((*codec)->type)
	{
		case AVMEDIA_TYPE_AUDIO:
			c->bit_rate = mParent->GetSelectedAudioBitrate();
			c->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
			c->sample_rate = (int)mParent->GetSelectedAudioSampleRate();
			c->channels = (int) mParent->GetSelectedAudioNumberChannels();
			c->channel_layout = (c->channels == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
			ost->st->time_base = (AVRational){ 1, c->sample_rate };
			c->time_base = ost->st->time_base;
			break;

		case AVMEDIA_TYPE_VIDEO:
			c->codec_id = (AVCodecID)codec_id;
			c->bit_rate = mParent->GetSelectedVideoBitrate();
			/* Resolution must be a multiple of two. */
			c->width = (int) mParent->GetSelectedVideoWidth();
			c->height = (int) mParent->GetSelectedVideoHeight();
			/* timebase: This is the fundamental unit of time (in seconds) in terms
			* of which frame timestamps are represented. For fixed-fps content,
			* timebase should be 1/framerate and timestamp increments should be
			* identical to 1. */
			ost->st->time_base = (AVRational){ 100, (int)(100.0f*mParent->GetSelectedVideoFrameRate()) };
			c->time_base = ost->st->time_base;
			c->gop_size = 12; /* emit one intra frame every twelve frames at most */
			c->pix_fmt = AV_PIX_FMT_YUV420P;
			if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
			{
				/* just for testing, we also add B-frames */
				c->max_b_frames = 2;
			}
			if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
			{
				/* Needed to avoid using macroblocks in which some coeffs overflow.
				* This does not happen with normal video, it just happens here as
				* the motion of the chroma plane does not match the luma plane. */
				c->mb_decision = 2;
			}
			break;

		default:
			break;
	}

	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

/**************************************************************/
/* audio output */

AVFrame * Export_ffmpeg :: alloc_audio_frame(int sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
{
	AVFrame *frame = av_frame_alloc();
	int ret;

	if (!frame) {
		fprintf(stderr, "Error allocating an audio frame\n");
		exit(1);
	}

	frame->format = (AVSampleFormat) sample_fmt;
	frame->channel_layout = channel_layout;
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;

	if (nb_samples) {
		ret = av_frame_get_buffer(frame, 0);
		if (ret < 0)
			AlertFfmpegExit(ret, "av_frame_get_buffer (audio)");
	}

	return frame;
}

void Export_ffmpeg :: open_audio(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
	AVCodecContext *c;
	int nb_samples;
	int ret;
	AVDictionary *opt = NULL;

	c = ost->enc;

	/* open it */
	av_dict_copy(&opt, opt_arg, 0);
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0)
		AlertFfmpegExit(ret, "Could not open audio codec");

	if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = c->frame_size;

	ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples);
	ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_FLT, c->channel_layout, c->sample_rate, nb_samples);

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0)
		AlertFfmpegExit(ret, "Could not copy the stream parameters");
}

/* get audio frame of 'frame_size' samples and
* 'nb_channels' channels. */
AVFrame * Export_ffmpeg :: get_audio_frame(OutputStream *ost)
{
	AVFrame *frame = ost->frame;

	AVCodecContext *c = ost->enc;
	int64 start_frame = ost->next_pts * kFramesSecond*c->time_base.num/c->time_base.den;
	int64 end_frame = gProject->mTotalDuration;
	if (start_frame >= end_frame)
		return nullptr;

	media_raw_audio_format format;
	format.format = media_raw_audio_format::B_AUDIO_FLOAT;
	format.frame_rate = c->sample_rate;
	format.channel_count = c->channels;
	format.buffer_size = frame->linesize[0];
	assert(format.channel_count == ost->enc->channels);

	//	codec may be planar instead of interleaved, convert it
	int64 next_start_frame, samples_done;
	if ((frame->format == AV_SAMPLE_FMT_FLTP) && (format.channel_count == 2) && ost->frame->data[1])
	{
		//	Convert packet to planar format
		next_start_frame = gAudioManager->GetOutputBuffer(start_frame, end_frame, ost->tmp_frame->data[0], ost->tmp_frame->linesize[0], format);
		float *src = (float *)ost->tmp_frame->data[0];
		float *dst0 = (float*)ost->frame->data[0];
		float *dst1 = (float*)ost->frame->data[1];
		samples_done = frame->nb_samples;	//(next_start_frame - start_frame)/kFramesSecond * c->time_base.den;
		for (int i=0; i < samples_done; i++)
		{
			*dst0++ = *src++;
			*dst1++ = *src++;
		}
	}
	else
	{
		next_start_frame = gAudioManager->GetOutputBuffer(start_frame, end_frame, frame->data[0], frame->linesize[0], format);
		samples_done = frame->nb_samples;	//(next_start_frame - start_frame)/kFramesSecond * c->time_base.den;

	}

	DEBUG("[Export_ffmpeg] Start=%ld, Next=%ld, frame_pts=%ld, next_pts=%ld (Done=%ld)\n", start_frame, next_start_frame, ost->next_pts, ost->next_pts + samples_done, samples_done);

	//	presentation time stamp (in time_base units)
	frame->pts = ost->next_pts;	
	ost->next_pts += samples_done;
	return frame;
}

/*
* encode one audio frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
int Export_ffmpeg :: write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
	AVCodecContext *c;
	AVPacket *pkt = av_packet_alloc();
	AVFrame *frame;
	int ret;
	int got_packet = 0;

	c = ost->enc;

	frame = get_audio_frame(ost);

	/* when we pass a frame to the encoder, it may keep a reference to it
	* internally;
	* make sure we do not overwrite it here
	*/
	if (frame)
	{
		ret = av_frame_make_writable(ost->frame);
		if (ret < 0)
			AlertFfmpegExit(ret, "av_frame_make_writable (audio)");
	}
	ret = avcodec_send_frame(c, frame);
	if (ret < 0)
	{
		av_strerror(ret, sErrorBuffer, sErrorBufferSize);
		printf("avcodec_send_frame (audio) returned %d (%s)\n", ret, sErrorBuffer);
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(c, pkt);
		if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF))
		{
			return ((ret==AVERROR(EAGAIN)) && frame) ? 0:1;
		}
		else if (ret < 0)
			AlertFfmpegExit(ret, "avcodec_receive_packet (audio)");

		ret = write_frame(oc, &c->time_base, ost->st, pkt);
		if (ret < 0)
			AlertFfmpegExit(ret, "write_frame (audio)");
		av_packet_unref(pkt);
	}

	av_packet_free(&pkt);

	return (frame) ? 0 : 1;
}

/**************************************************************/
/* video output */

static AVFrame * alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	int ret;

	picture = av_frame_alloc();
	if (!picture)
		return NULL;

	picture->format = pix_fmt;
	picture->width = width;
	picture->height = height;

	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 32);
	if (ret < 0)
		AlertFfmpegExit(ret, "could not allocate frame data (video)");
	return picture;
}

void Export_ffmpeg :: open_video(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
	int ret;
	AVCodecContext *c = ost->enc;
	AVDictionary *opt = NULL;

	av_dict_copy(&opt, opt_arg, 0);

	/* open the codec */
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0)
		AlertFfmpegExit(ret, "could not open codec (video)");

	/* allocate and init a re-usable frame */
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
	if (!ost->frame)
		AlertFfmpegExit(ret, "could not allocate frame data (video)");

	/* If the output format is not YUV420P, then a temporary YUV420P
	* picture is needed too. It is then converted to the required
	* output format. */
	ost->tmp_frame = NULL;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P)
	{
		ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
		if (!ost->tmp_frame)
			AlertFfmpegExit(ret, "could not allocate tmp_frame data (video)");
	}

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0)
		AlertFfmpegExit(ret, "could not copy the stream parameters (video)");
}

AVFrame * Export_ffmpeg :: get_video_frame(OutputStream *ost)
{
	AVCodecContext *c = ost->enc;

	/* check if we want to generate more frames */
	bigtime_t frame_idx = ost->next_pts*kFramesSecond*c->time_base.num/c->time_base.den;
	if (frame_idx >= gProject->mTotalDuration)
		return nullptr;

	/* when we pass a frame to the encoder, it may keep a reference to it
	* internally; make sure we do not overwrite it here */
	if (av_frame_make_writable(ost->frame) < 0)
		exit(1);

	if (1)//c->pix_fmt != AV_PIX_FMT_YUV420P)
	{
		/* as we only generate a YUV420P picture, we must convert it
		* to the codec pixel format if needed */
		if (!ost->sws_ctx)
		{
			const bool resize = (gProject->mResolution.width != c->width) || (gProject->mResolution.height != c->height);
			ost->sws_ctx = sws_getContext(gProject->mResolution.width, gProject->mResolution.height, AV_PIX_FMT_BGRA,
										  c->width, c->height, AV_PIX_FMT_YUV420P,
										  resize ? SCALE_FLAGS : 0, nullptr, nullptr, nullptr);
			if (!ost->sws_ctx) {
				fprintf(stderr, "Could not initialize the conversion context\n");
				exit(1);
			}
		}

		BBitmap *output = nullptr;
		gRenderActor->Async(&RenderActor::AsyncPrepareExportFrame, gRenderActor, frame_idx, fRenderSemaphore, &output);
		status_t err;
		while ((err = acquire_sem(fRenderSemaphore)) == B_INTERRUPTED) ;
		if (err == B_OK)
		{
			// colour conversion
			if (output)
			{
				uint8_t *inData[1]     = { (uint8_t *)output->Bits()};
				int      inLinesize[1] = { 4 * (int)gProject->mResolution.width };
				sws_scale(ost->sws_ctx, inData, inLinesize, 0, gProject->mResolution.height, ost->frame->data, ost->frame->linesize);
			}
			else
				printf("Export_ffmpeg::get_video_frame(), warning output = nullptr\n");
		}
		else
		{
			fprintf(stderr, "ffmpeg - error acquiring fRenderSemaphore(%d)\n", err);
			exit(1);
		}

	} else {
		;//fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
	}

	ost->frame->pts = ost->next_pts;
	++ost->next_pts;
	DEBUG("[Export_ffmpeg] ***Video*** =%ld\n", ost->frame->pts);

	return ost->frame;
}

/*
* encode one video frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
int Export_ffmpeg :: write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
	int ret;
	AVCodecContext *c;
	AVFrame *frame;
	int got_packet = 0;
	AVPacket *pkt = av_packet_alloc();

	c = ost->enc;

	frame = get_video_frame(ost);

	/* encode the image */
	ret = avcodec_send_frame(c, frame);
	if (ret < 0)
	{
		av_strerror(ret, sErrorBuffer, sErrorBufferSize);
		printf("avcodec_send_frame (video) returned %d (%s)\n", ret, sErrorBuffer);
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(c, pkt);
		if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF))
		{
			return (ret==AVERROR(EAGAIN)) ? 0:1;
		}
		else if (ret < 0)
			AlertFfmpegExit(ret, "avcodec_receive_packet (video)");

		ret = write_frame(oc, &c->time_base, ost->st, pkt);
		if (ret < 0)
			AlertFfmpegExit(ret, "write_frame (video)");
		av_packet_unref(pkt);
	}

	av_packet_free(&pkt);

	return (frame) ? 0 : 1;
}

void Export_ffmpeg :: close_stream(AVFormatContext *oc, OutputStream *ost)
{
	avcodec_free_context(&ost->enc);
	av_frame_free(&ost->frame);
	av_frame_free(&ost->tmp_frame);
	sws_freeContext(ost->sws_ctx);
}

/**************************************************************
	TODO - need to refactor from using encoding thread to actors
	Also check if ffmpeg doesn't corrupt when reading from other threads
	Occasionally I've noticed aac encoder complaining about NaN and will then corrupt its pts
****************************************************************/

Ffmpeg_Actor :: Ffmpeg_Actor(Export_ffmpeg *parent)
{
	fParent = parent;
	DEBUG("Ffmpeg_Actor() constructor\n");
}
void Ffmpeg_Actor :: Async_Start(int i)
{
	printf("Ffmpeg_Actor::AsyncStart(%d)\n", i);
	while (fParent->fThread > 0)
		snooze(1000);

	fParent->fKeepAlive = true;
	fParent->fThread = spawn_thread((status_t (*)(void *))Export_ffmpeg::WorkThread, "Export_ffmpeg::WorkThread", B_NORMAL_PRIORITY, fParent);
	resume_thread(fParent->fThread);
}
void Ffmpeg_Actor :: Async_Stop(const bool complete)
{
	printf("Ffmpeg_Actor::AsyncStop(%d)\n", complete);
	if (!complete)
	{
		fParent->fKeepAlive = false;
		int attempt = 0;
		while ((fParent->fThread > 0) && (attempt < 2000))
		{
			snooze(1000);
			attempt++;
		}
		if (fParent->fThread > 0)
		{
			if (kill_thread(fParent->fThread) != B_OK)
			{
				printf("Cannot kill thread(%d)\n", fParent->fThread);
			}
			fParent->fThread = 0;
		}
	}
	DEBUG("Ffmpeg_Actor::AsyncStop()2\n");
}

/**************************************************************/
/* media file output */
void Export_ffmpeg :: StartEncode()
{
	if (!fWorkActor)
		fWorkActor = new Ffmpeg_Actor(this);
	fWorkActor->Async(&Ffmpeg_Actor::Async_Start, fWorkActor, 99);
}

void Export_ffmpeg :: StopEncode(const bool complete)
{
	DEBUG("Export_ffmpeg::StopEncode(%d)\n", complete);
	fWorkActor->Async(&Ffmpeg_Actor::Async_Stop, fWorkActor, complete);
}

/**************************************************************/
/* WorkThread */
status_t Export_ffmpeg :: WorkThread(void *arg)
{
	assert(arg != nullptr);
	Export_ffmpeg *instance = (Export_ffmpeg *) arg;
	ExportMediaWindow *mParent = instance->mParent;

	printf("Export_ffmpeg::WorkThread(%s)\n", mParent->fTextOutFile->Text());
	sAVFormatContext = nullptr;

	OutputStream video_st = { 0 }, audio_st = { 0 };
	const char *filename;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	const AVCodec *audio_codec, *video_codec;
	int ret;
	bool have_video = (mParent->fHasVideo && (mParent->fEnableVideo->Value() > 0));
	bool have_audio = (mParent->fHasAudio && (mParent->fEnableAudio->Value() > 0));
	int encode_video = 0, encode_audio = 0;
	AVDictionary *opt = NULL;
	int i;

	filename = mParent->fTextOutFile->Text();

	/* allocate the output media context */
	avformat_alloc_output_context2(&oc, NULL, NULL, filename);
	if (!oc) {
		printf("Could not deduce output format from file extension: using MPEG.\n");
		avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
	}
	sAVFormatContext = oc;
	if (!oc)
	{
		BAlert *alert = new BAlert("Export_ffmpeg::WorkThread", "Cannot initialise ffmpeg context", "Dismiss");
		alert->Go();
		exit(1);
	}

	fmt = (AVOutputFormat*) oc->oformat;

	//	Video stream + codec
	int selected_codec_idx = have_video ? mParent->fOptionVideoCodec->SelectedOption() : -1;
	if (selected_codec_idx >= 0)
	{
		assert(selected_codec_idx < instance->fVideoCodecCookies.size());
		fmt->video_codec = (AVCodecID) instance->fVideoCodecCookies[selected_codec_idx];
		instance->add_stream(&video_st, oc, &video_codec, fmt->video_codec);
		encode_video = 1;
	}
	else
		fmt->video_codec = AV_CODEC_ID_NONE;

	//	Audio stream + codec
	selected_codec_idx = have_audio ? mParent->fOptionAudioCodec->SelectedOption() : -1;
	if (selected_codec_idx >= 0)
	{
		assert(selected_codec_idx < instance->fAudioCodecCookies.size());
		fmt->audio_codec = (AVCodecID) instance->fAudioCodecCookies[selected_codec_idx];
		instance->add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
		encode_audio = 1;
	}
	else
		fmt->audio_codec = AV_CODEC_ID_NONE;

	/* Now that all the parameters are set, we can open the audio and
	* video codecs and allocate the necessary encode buffers. */
	if (have_video)
		instance->open_video(oc, video_codec, &video_st, opt);

	if (have_audio)
		instance->open_audio(oc, audio_codec, &audio_st, opt);

	av_dump_format(oc, 0, filename, 1);

	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
		if (ret < 0)
			AlertFfmpegExit(ret, "Cannot open file");
	}

	/* Write the stream header, if any. */
	ret = avformat_write_header(oc, &opt);
	if (ret < 0)
		AlertFfmpegExit(ret, "Cannot open file");

	float previous_progress = 0.0f;
	while ((encode_video || encode_audio) && instance->fKeepAlive)
	{
		/* select the stream to encode */
		if (encode_video && (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base, audio_st.next_pts, audio_st.enc->time_base) <= 0))
		{
			encode_video = !instance->write_video_frame(oc, &video_st);
		} else if (have_audio)
		{
			encode_audio = !instance->write_audio_frame(oc, &audio_st);
		}

		//	Update progress bar
		double progress;
		if (have_video)
		{
			double frame_duration = (double)video_st.enc->time_base.num/(double)video_st.enc->time_base.den;
			int64 next_frame = kFramesSecond * frame_duration * video_st.next_pts;
			progress = 100.0 * (double)next_frame / (double)gProject->mTotalDuration;
		}
		else
		{
			double frame_duration = (double)audio_st.enc->time_base.num/(double)audio_st.enc->time_base.den;
			int64 next_frame = kFramesSecond * frame_duration * audio_st.next_pts;
			progress = 100.0 * (double)next_frame / (double)gProject->mTotalDuration;
		}
		if ((progress - previous_progress > 0.01) && (progress < 100.0))
		{
			mParent->fMsgExportEngine->ReplaceFloat("progress", (float)progress);
			mParent->PostMessage(mParent->fMsgExportEngine);
			previous_progress = progress;
		}
	}

	printf("[Export_ffmpeg] Duration = %ld\n", gProject->mTotalDuration);
	if (have_video)
		printf("[Export_ffmpeg] Final video->next_pts=%ld (timeline=%ld)\n", video_st.next_pts, video_st.next_pts * kFramesSecond*video_st.enc->time_base.num/video_st.enc->time_base.den);
	if (have_audio)
		printf("[Export_ffmpeg] Final audio->next_pts=%ld (timeline=%ld)\n", audio_st.next_pts, audio_st.next_pts * kFramesSecond*audio_st.enc->time_base.num/audio_st.enc->time_base.den);

	/* Write the trailer, if any. The trailer must be written before you
	* close the CodecContexts open when you wrote the header; otherwise
	* av_write_trailer() may try to use memory that was freed on
	* av_codec_close(). */
	av_write_trailer(oc);

	/* Close each codec. */
	if (have_video)
		instance->close_stream(oc, &video_st);
	if (have_audio)
		instance->close_stream(oc, &audio_st);

	if (!(fmt->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_closep(&oc->pb);

	/* free the stream */
	avformat_free_context(oc);

	mParent->fMsgExportEngine->ReplaceFloat("progress", 100.0f);
	mParent->PostMessage(mParent->fMsgExportEngine);

	instance->fThread = 0;

	printf("Export_ffmpeg::ExitWorkThread\n");
	return B_OK;
}


