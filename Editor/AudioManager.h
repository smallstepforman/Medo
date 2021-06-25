/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Audio manager (accurate seeking + cache frames)
 */

#ifndef _AUDIO_MANAGER_H_
#define _AUDIO_MANAGER_H_

#include <media/SoundPlayer.h>

#ifndef _GLIBCXX_DEQUE
#include <deque>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

class BBitmap;
class MediaSource;
class AudioCache;
class AudioThumbnailActor;
class BSoundPlayer;
struct media_raw_audio_format;
struct SwrContext;

//===================
class AudioManager
{
private:
	AudioCache		*fAudioCache;
	enum AUDIO_PROCESSING_BUFFER {APB_1, APB_2, APB_MIX, NUMBER_AUDIO_PROCESSING_BUFFERS};
	uint8			*fProcessingBuffers[NUMBER_AUDIO_PROCESSING_BUFFERS];

	sem_id					fCacheSemaphore;
	AudioThumbnailActor		*fAudioThumbnailActor;
	friend class			AudioThumbnailActor;

	BBitmap					*GetBitmapAudioFrame(MediaSource *source, const int64 audio_start_frame, const int64 audio_end_frame,
                                                 const float width, const float height);

	static void		SoundPlayerCallback(void *cookie, void *buffer, size_t buffer_size, const media_raw_audio_format &format);
	BSoundPlayer	*fSoundPlayer;
	int64			fPreviewStartFrame;
	int64			fPreviewEndFrame;
	MediaSource		*fPreviewSource;

	struct ResamplerContext
	{
		SwrContext		*context;
		float			input_rate;
		float			output_rate;
		MediaSource		*media_source;
	};
	std::vector<ResamplerContext>	fResamplerContext;

	void			ConvertChannels(const int out_channels, uint8 *destination,
									const int in_channels, uint8_t *source,
									size_t sample_size, size_t count_samples);

	void			MixAudio(uint8 *destination, uint8 *source1, uint8 *source2,
							 const int sample_size, const int count_channels, const int count_samples,
							 const int32 track_idx, float level_left, float level_right);

public:
					AudioManager();
					~AudioManager();

	BBitmap			*GetBitmapAsync(MediaSource *source, const int64 start_frame, const int64 end_frame, const float width, const float height);
	void			ClearPendingThumbnails();

	void			PlayPreview(const int64 start_frame, const int64 end_frame, MediaSource *preview_source = nullptr);
	const int64		GetOutputBuffer(const int64 start_frame, const int64 end_frame,
									void *buffer, size_t buffer_size, const media_raw_audio_format &format);

};

extern AudioManager *gAudioManager;

#endif	//#ifndef _AUDIO_MANAGER_H_

