/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Export Engine - ffmpeg
 */

#ifndef _EXPORT_FFMEG_H_
#define _EXPORT_FFMEG_H_

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _EXPORT_MEDIA_WINDOW_H_
#include "ExportMediaWindow.h"
#endif

class BOptionPopUp;

struct AVOutputFormat;
struct AVFormatContext;
struct AVCodec;
struct AVCodecContext;
struct AVStream;
struct AVFrame;

struct SwsContext;
struct OutputStream;
struct AVDictionary;

class Ffmpeg_Actor;

class Export_ffmpeg : public ExportEngine
{
public:
				Export_ffmpeg(ExportMediaWindow *parent);
				~Export_ffmpeg() override;
	void		BuildFileFormatOptions() override;
	void		BuildVideoCodecOptions() override;
	void		BuildAudioCodecOptions() override;
	void		FileFormatSelectionChanged() override;
	void		StartEncode() override;
	void		StopEncode(const bool complete) override;

	float		AddCustomVideoGui(float start_y) override ;
	float		AddCustomAudioGui(float start_y) override;
	bool		MessageRedirect(BMessage *msg) override;

private:
	BOptionPopUp		*fOptionVideoCodecCompliance;
	BOptionPopUp		*fOptionAudioCodecCompliance;
	std::vector<const AVOutputFormat *>	fFileFormatCookies;
	std::vector<int>	fVideoCodecCookies;
	std::vector<int>	fAudioCodecCookies;

	static status_t		WorkThread(void *arg);
	int32				fThread;
	bool				fKeepAlive;		// to be removed when transition to actor model
	sem_id				fRenderSemaphore;
	friend class Ffmpeg_Actor;
	Ffmpeg_Actor		*fWorkActor;


	void		add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, int codec_id);
	AVFrame		*alloc_audio_frame(int sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples);
	void		open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
	AVFrame		*get_audio_frame(OutputStream *ost);
	int			write_audio_frame(AVFormatContext *oc, OutputStream *ost);
	void		open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
	AVFrame		*get_video_frame(OutputStream *ost);
	int			write_video_frame(AVFormatContext *oc, OutputStream *ost);
	void		close_stream(AVFormatContext *oc, OutputStream *ost);
};

#endif	//#ifndef _EXPORT_FFMEG_H_
