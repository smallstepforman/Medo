/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Audio manager (accurate seeking + cache frames)
 */

#include <vector>
#include <cassert>

#include <MediaKit.h>

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include "Yarra/Math/Math.h"
#include "Project.h"
#include "MediaSource.h"
#include "AudioCache.h"
#include "AudioManager.h"
#include "EffectNode.h"
#include "MedoWindow.h"
#include "AudioMixer.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

//	Debug bytes requested vs bytes produced in GetOutputBuffer()
#define DEBUG_TOTAL_OUT		0

/*	FUNCTION:		AudioManager :: PlayPreview
	ARGS:			start_frame, end_frame
					preview_source
	RETURN:			n/a
	DESCRIPTION:	Play sound preview
*/
void AudioManager :: PlayPreview(const int64 start_frame, const int64 end_frame, MediaSource *preview_source)
{
	status_t err;
	while ((err = acquire_sem(fCacheSemaphore)) == B_INTERRUPTED) ;
	if (err == B_OK)
	{
#if 0
		if ((start_frame - fPreviewStartFrame > kFramesSecond / gProject->mResolution.frame_rate) || (start_frame < fPreviewStartFrame))
#endif
			fPreviewStartFrame = start_frame;
		fPreviewEndFrame = end_frame;
		fPreviewSource = preview_source;
		fSoundPlayer->SetHasData(true);
		release_sem(fCacheSemaphore);
	}
	else
		DEBUG("Failed to acquire_sem\n");
}

/*	FUNCTION:		AudioManager :: SoundPlayerCallback
	ARGS:			cookie
					buffer, buffer_size
					format
	RETURN:			n/a
	DESCRIPTION:	Hook function called by BSoundPlayer
*/
void AudioManager :: SoundPlayerCallback(void *cookie, void *buffer, size_t buffer_size, const media_raw_audio_format &format)
{
	AudioManager *amgr = (AudioManager *)cookie;
	if (amgr->fPreviewEndFrame - amgr->fPreviewStartFrame <= 0)
	{
		amgr->fSoundPlayer->SetHasData(false);

		//	Reset visualisation
		AudioMixer *mixer = MedoWindow::GetInstance()->GetAudioMixer();
		if (mixer && !mixer->IsHidden())
		{
			mixer->mMsgVisualiseLevels->ReplaceInt32("track", -1);
			mixer->PostMessage(mixer->mMsgVisualiseLevels);
		}
		return;
	}

	//	BSoundPlayer is used for sound preview and needs complete buffer filled, so adjust preview_end to achieve this
	const int64 kTargetSampleSize = format.channel_count * sizeof(float);
	const int64 kTargetNumberSamples = buffer_size / kTargetSampleSize;
	const double kTargetConversionFactor = double(kFramesSecond) / format.frame_rate;
	const int64 preview_end = amgr->fPreviewStartFrame + kTargetNumberSamples*kTargetConversionFactor;
	amgr->fPreviewStartFrame = amgr->GetOutputBuffer(amgr->fPreviewStartFrame, preview_end, buffer, buffer_size, format);

	//	Set visualisation
	AudioMixer *mixer = MedoWindow::GetInstance()->GetAudioMixer();
	if (mixer && !mixer->IsHidden())
	{
		mixer->mMsgVisualiseLevels->ReplaceInt32("track", gProject->mTimelineTracks.size());
		mixer->PostMessage(mixer->mMsgVisualiseLevels);
	}
}

/*	FUNCTION:		AudioManager :: GetOutputBuffer
	ARGS:			start_frame
					end_frame
					buffer, buffer_size
					format
	RETURN:			end_frame
	DESCRIPTION:	Prepare audio output buffer (resample / mix)
*/
const int64	AudioManager :: GetOutputBuffer(const int64 start_frame, const int64 end_frame,
											void *buffer, size_t buffer_size, const media_raw_audio_format &format)
{
	//DEBUG("AudioManager::GetOutputBuffer(%ld, %ld) amount = %ld, format=%f\n", start_frame, end_frame, end_frame - start_frame, format.frame_rate);

	//	Only support B_AUDIO_FLOAT
	if (format.format != media_raw_audio_format::B_AUDIO_FLOAT)
	{
		printf("AudioManager::GetOutputBuffer(start_frame=%ld, end_frame=%ld), format not B_AUDIO_FLOAT\n", start_frame, end_frame);
		memset(buffer, 0, buffer_size);
		return end_frame;
	}
	const int64 kTargetSampleSize = format.channel_count * sizeof(float);
	const int64 kTargetNumberSamples = buffer_size / kTargetSampleSize;
	const double kTargetConversionFactor = double(kFramesSecond) / (double)format.frame_rate;

	//	crop range to available buffer size
	int64 actual_end_frame = start_frame + kTargetConversionFactor*kTargetNumberSamples;
	if (end_frame < actual_end_frame)
		actual_end_frame = end_frame;

	struct TRACK_CLIP
	{
		TimelineTrack				*track;
		int32						track_idx;
		MediaClip					*clip;
		std::vector<MediaEffect *>	effects;
	};
	std::vector<TRACK_CLIP>		track_clips;

	MediaClip preview_clip;
	if (fPreviewSource)
	{
		preview_clip.mMediaSource = fPreviewSource;
		preview_clip.mMediaSourceType = fPreviewSource->GetMediaType();
		preview_clip.mSourceFrameStart = 0;
		preview_clip.mSourceFrameEnd = fPreviewSource->GetAudioDuration();
		preview_clip.mTimelineFrameStart = 0;

		TRACK_CLIP aClip;
		aClip.clip = &preview_clip;
		track_clips.push_back(aClip);
	}
	else
	{
		int32 track_idx = gProject->mTimelineTracks.size();

		//	Reverse iterate each track, find clips
		for (std::vector<TimelineTrack *>::const_reverse_iterator t = gProject->mTimelineTracks.rbegin(); t < gProject->mTimelineTracks.rend(); ++t)
		{
			assert(--track_idx >= 0);

			if (!(*t)->mAudioEnabled)
				continue;

			//	TODO binary search
			for (auto &clip : (*t)->mClips)
			{
				if (clip.mTimelineFrameStart > actual_end_frame)
					break;

				if ((start_frame < clip.GetTimelineEndFrame()) && (actual_end_frame > clip.mTimelineFrameStart) && clip.mAudioEnabled &&
					((clip.mMediaSourceType == MediaSource::MEDIA_AUDIO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO)))
				{
					TRACK_CLIP c;
					c.track = *t;
					c.track_idx = track_idx;
					c.clip = &clip;

					//	add effects
					for (auto &e : c.track->mEffects)
					{
						if ((start_frame < e->mTimelineFrameEnd) && (actual_end_frame > e->mTimelineFrameStart))
						{
							if (e->Type() == MediaEffect::MEDIA_EFFECT_AUDIO)
								c.effects.push_back(e);
						}
					}
					track_clips.emplace_back(c);
				}
			}
		}
	}

	if (track_clips.empty())
	{
		memset(buffer, 0, buffer_size);
		return actual_end_frame;
	}

	int track_clip_index = 0;
	int processing_buffer = APB_1;

	for (auto c : track_clips)
	{
#if DEBUG_TOTAL_OUT
	size_t total_output = 0;
#endif

		MediaSource *media_source =c.clip->mMediaSource;
		const double kSourceConversionFactor = double(kFramesSecond) / media_source->GetAudioFrameRate();

		uint8 *buffer_start = (uint8 *) buffer;
		uint8 *buffer_end = buffer_start + buffer_size;

		int64 audio_start = round(((start_frame - c.clip->mTimelineFrameStart) + c.clip->mSourceFrameStart) / kSourceConversionFactor);
		if (audio_start >= media_source->GetAudioNumberSamples())
		{
			printf("AudioManager::GetOutputBuffer() - audio_start > media_source->GetAudioNumberSamples()\n");
			audio_start = media_source->GetAudioNumberSamples();
		}
		//	Cater for scenario where clip starts within interval
		if (c.clip->mTimelineFrameStart > start_frame)
		{
			DEBUG("AudioManager::GetOutputBuffer() [1] buffer_start = %p, buffer_end = %p, buffer_size = %ld\n", buffer_start, buffer_end, buffer_size);
			size_t num_zero =  kTargetSampleSize * (c.clip->mTimelineFrameStart - start_frame)/kTargetConversionFactor;
			if (track_clip_index == 0)
				memset(buffer_start, 0, num_zero);
			buffer_start += num_zero;
			audio_start = round(c.clip->mSourceFrameStart / kSourceConversionFactor);
#if DEBUG_TOTAL_OUT
			if (track_clip_index == 0)
				total_output += num_zero;
#endif
			DEBUG("AudioManager::GetOutputBuffer() [2] buffer_start = %p, num_zero = %ld\n", buffer_start, num_zero);
		}

		int64 audio_end = round(((actual_end_frame - c.clip->mTimelineFrameStart) + c.clip->mSourceFrameStart) / kSourceConversionFactor);
		if (audio_end >= media_source->GetAudioNumberSamples())
			audio_end = media_source->GetAudioNumberSamples();

		//	cater for scenario where clip ends within interval
		if (c.clip->GetTimelineEndFrame() < actual_end_frame)
		{
			DEBUG("AudioManager::GetOutputBuffer() [3] audio_end=%ld, buffer_start = %p, buffer_end = %p, buffer_size = %ld\n", audio_end, buffer_start, buffer_end, buffer_size);
			size_t num_zero = kTargetSampleSize * (actual_end_frame - c.clip->GetTimelineEndFrame())/kTargetConversionFactor;
			if (num_zero > buffer_size)
			{
				printf("AudioManager::GetOutputBuffer() num_zero > buffer_size\n");
				num_zero = buffer_size;
			}
			if (track_clip_index == 0)
				memset(buffer_end - num_zero, 0, num_zero);
			buffer_end -= num_zero;
#if DEBUG_TOTAL_OUT
			if (track_clip_index == 0)
				total_output += num_zero;
#endif
			audio_end = c.clip->mSourceFrameEnd / kSourceConversionFactor;
			DEBUG("AudioManager::GetOutputBuffer() [4] audio_end=%ld, buffer_end = %p, num_zero = %ld\n", audio_end, buffer_end, num_zero);
		}
		DEBUG("AudioManager::GetOutputBuffer() [5] start_frame[%ld] end_frame[%ld] audio_start[%ld], audio_end[%ld], num_frames=%ld\n",
			  start_frame, actual_end_frame, audio_start, audio_end, audio_end - audio_start);

		assert(audio_start >= 0);
		assert(audio_end <= media_source->GetAudioNumberSamples());

		if (audio_start == audio_end)
		{
			if (track_clip_index == 0)
				memset(buffer_start, 0, buffer_end - buffer_start);
			continue;
		}

		//	Get audio buffer from cache
		size_t audio_buffer_size = 0;
		uint8 *audio_buffer = nullptr;
		status_t err;
		while ((err = acquire_sem(fCacheSemaphore)) == B_INTERRUPTED) ;
		if (err == B_OK)
		{
			audio_buffer = fAudioCache->GetAudioBufferLocked(media_source, audio_start, audio_end, audio_buffer_size);
		}
		release_sem(fCacheSemaphore);
		if (!audio_buffer || (audio_buffer_size <= 0))
		{
			DEBUG("AudioCache::GetAudioBuffer() - error\n");
			DEBUG("   audio_start=%ld, audio_end=%ld, actual_end_frame=%ld\n", audio_start, audio_end, actual_end_frame);
			DEBUG("   audio_buffer = %p, audio_buffer_size = %ld\n", audio_buffer, audio_buffer_size);
			continue;
		}

		uint32 count_source_channels = media_source->GetAudioNumberChannels();

		//  Resampling
		int64 target_samples_done;
		if (format.frame_rate != media_source->GetAudioFrameRate())
		{
			//	Find existing context
			SwrContext *swr_ctx = nullptr;
			for (auto &i : fResamplerContext)
			{
				if ((i.input_rate == media_source->GetAudioFrameRate()) &&
					(i.output_rate == format.frame_rate) &&
					(i.media_source == media_source))
				{
					swr_ctx = i.context;
					break;
				}
			}
			if (swr_ctx == nullptr)
			{
				swr_ctx = swr_alloc();
				if (!swr_ctx)
				{
					printf("Cannot swr_alloc\n");
					exit(1);
				}
				av_opt_set_int (swr_ctx, "in_channel_count", count_source_channels, 0);
				av_opt_set_int (swr_ctx, "in_sample_rate", media_source->GetAudioFrameRate(), 0);
				av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
				av_opt_set_int (swr_ctx, "out_channel_count", count_source_channels, 0);
				av_opt_set_int (swr_ctx, "out_sample_rate", format.frame_rate, 0);
				av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

				/* initialize the resampling context */
				if (swr_init(swr_ctx) < 0)
				{
					printf("Failed to initialize the resampling context\n");
					exit(1);
				}

				ResamplerContext rc;
				rc.context = swr_ctx;
				rc.input_rate = media_source->GetAudioFrameRate();
				rc.output_rate = format.frame_rate;
				rc.media_source = media_source;
				fResamplerContext.push_back(rc);
				printf("new ResamplerContext: %s (source=%0.2f, target=%0.2f\n", rc.media_source->GetFilename(), rc.input_rate, rc.output_rate);
			}

			target_samples_done = swr_convert(swr_ctx, &fProcessingBuffers[processing_buffer], kTargetNumberSamples, (const uint8 **)&audio_buffer, audio_end - audio_start);
			if (target_samples_done < 0)
			{
				fprintf(stderr, "Error while converting\n");
				exit(1);
			}
			if (target_samples_done < kTargetNumberSamples)
			{
				uint8 *ptr = fProcessingBuffers[processing_buffer] + target_samples_done*sizeof(float)*count_source_channels;
				target_samples_done += swr_convert(swr_ctx, &ptr, kTargetNumberSamples - target_samples_done, nullptr, 0);
				if (target_samples_done != kTargetNumberSamples)
				{
					printf("AudioManager::GetOutputBuffer() - requested=%ld, done=%ld\n", kTargetNumberSamples, target_samples_done);
					memset(fProcessingBuffers[processing_buffer] + target_samples_done*sizeof(float)*count_source_channels, 0, (kTargetNumberSamples - target_samples_done)*sizeof(float)*count_source_channels);
				}
			}

			audio_buffer = fProcessingBuffers[processing_buffer];
			if (++processing_buffer >= APB_MIX)
				processing_buffer = APB_1;
		}
		else
		{
			target_samples_done = (actual_end_frame - start_frame)/kTargetConversionFactor;
		}

		if (count_source_channels == 0)
		{
			printf("AudioManager::GetOutputBuffer() - warning, count_channels = 0\n");
			return actual_end_frame;
		}

		//	audio channels conversion
		if (format.channel_count != count_source_channels)
		{
			ConvertChannels(format.channel_count, fProcessingBuffers[processing_buffer],
							count_source_channels, audio_buffer,
							sizeof(float), target_samples_done);
			audio_buffer = fProcessingBuffers[processing_buffer];
			if (++processing_buffer >= APB_MIX)
				processing_buffer = APB_1;
		}

		//	Effects
		if (c.effects.size() > 0)
		{
			for (auto &e : c.effects)
			{
				count_source_channels = e->mEffectNode->AudioEffect(e, fProcessingBuffers[processing_buffer], audio_buffer,
											start_frame, end_frame,
											audio_start, audio_end,
											count_source_channels, sizeof(float), target_samples_done);

				audio_buffer = fProcessingBuffers[processing_buffer];
				if (++processing_buffer >= APB_MIX)
					processing_buffer = APB_1;
			}
		}

#if DEBUG_TOTAL_OUT
		if (track_clip_index == 0)
			total_output += target_samples_done * (format.channel_count*sizeof(float));
#endif

		//	mix audio
		if (fPreviewSource)
			MixAudio(buffer_start, audio_buffer, nullptr, sizeof(float), format.channel_count, target_samples_done, 0, 1.0f, 1.0f);
		else
		{
			MixAudio(buffer_start,															//	destination
					 audio_buffer,															//	source 1
					 track_clip_index == 0 ? nullptr : buffer_start,						//	source 2
					 sizeof(float), format.channel_count, target_samples_done,				//	size
					 c.track_idx, c.track->mAudioLevels[0], c.track->mAudioLevels[1]);		//	audio_levels
		}

#if DEBUG_TOTAL_OUT
		if ((track_clip_index == 0) && (total_output != buffer_size))
			printf("AudioManager::GetOutputBuffer() Incomplete.  Done=%ld, target=%ld (buffer size=%ld, total_output=%ld, requested=%ld, start=%ld, end=%ld)\n",
				  target_samples_done, kTargetNumberSamples, buffer_size, total_output, audio_end - audio_start, start_frame, end_frame);
#endif
		track_clip_index++;
	}

	return actual_end_frame;
}
