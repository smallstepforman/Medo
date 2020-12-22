/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Audio manager utility
 */

#include <cassert>
#include <cstdio>

#include <MediaKit.h>
#include <soxr.h>

#include "AudioManager.h"
#include "AudioMixer.h"
#include "MedoWindow.h"

#if 1
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

/*	FUNCTION:		AudioManager :: ConvertChannels
	ARGS:			channel_count
					destination
					source
					size
	RETURN:			n/a
	DESCRIPTION:	Convert audio from in_channels to out-channels
					Assumption - destination and source are float
*/
void AudioManager :: ConvertChannels(const int out_channels, uint8 *destination,
									 const int in_channels, uint8_t *source,
									 size_t sample_size, size_t count_samples)
{
	assert(destination);
	assert(source);
	assert(out_channels > 0);
	assert(in_channels > 0);
	assert(count_samples > 0);
	assert(sample_size == 4);

	float *d = (float *)destination;
	float *s = (float *)source;

	//	inline conversion
	if ((out_channels == 2) && (in_channels == 1))
	{
		for (size_t i=0; i < count_samples; i++)
		{
			float s1 = *s++;
			*d++ = s1;
			*d++ = s1;
		}
	}
	else if ((out_channels == 1) && (in_channels == 2))
	{
		for (size_t i = 0; i < count_samples; i++)
		{
			float v1 = *s++;
			float v2 = *s++;
			*d++ = 0.5f*(v1 + v2);
		}
	}
	else
		assert(0);
}

/*	FUNCTION:		AudioManager :: clamp_one
	ARGS:			v
	RETURN:			n/a
	DESCRIPTION:	Clamp v to range [-1, 1]
*/
static inline float clamp_one(float v)
{
	if (v < -1.0f)
		v = -1.0f;
	else if (v > 1.0f)
		v = 1.0f;
	return v;
}

/*	FUNCTION:		AudioManager :: MixAudio
	ARGS:			destination
					source1
					source2
					sample_size
					count_channels
					count_samples
					track_idx
					left_level
					right_level
	RETURN:			n/a
	DESCRIPTION:	Mix audio source 1 with source 2.
					Assumption - bitrates and channel count are the same
*/
void AudioManager :: MixAudio(uint8 *destination, uint8 *source1, uint8 *source2,
							  const int sample_size, const int count_channels, const int count_samples,
							  const int32 track_idx, float level_left, float level_right)
{
	assert(sample_size == sizeof(float));
	float max_left = 0.0f;
	float max_right = 0.0f;

	AudioMixer *mixer = MedoWindow::GetInstance()->GetAudioMixer();
	if (source2 == nullptr)
	{
		//	For single source with no mixer, accelerated path
		if (!mixer || mixer->IsHidden())
		{
			if ((level_left == 1.0f) && (level_right == 1.0f))
			{
				memcpy(destination, source1, count_samples * count_channels * sample_size);
				return;
			}
		}

		float *d = (float *)destination;
		float *s1 = (float *)source1;
		switch (count_channels)
		{
			case 1:
				for (int i=0; i < count_samples; i++)
				{
					float v = *s1*level_left;
					if (v > max_left)
					{
						max_left = v;
						max_right = v;
					}
					*d++ = clamp_one(v);
					s1++;
				}
				break;

			case 2:
				for (int i=0; i < count_samples; i++)
				{
					float left = *s1*level_left;
					if (left > max_left)
						max_left = left;
					*d++ = clamp_one(left);	//	channel 1
					s1++;
	\
					float right = *s1*level_right;
					if (right > max_right)
						max_right = right;
					*d++ = clamp_one(right);	//	channel 2
					s1++;
				}
				break;

			default:
				assert(0);
		}
	}
	else
	{
		float *d = (float *)destination;
		float *s1 = (float *)source1;
		float *s2 = (float *)source2;

		switch (count_channels)
		{
			case 1:
				for (int i=0; i < count_samples; i++)
				{
					float v = *s1*level_left;
					if (v > max_left)
					{
						max_left = v;
						max_right = v;
					}
					*d++ = clamp_one(v + *s2);
					s1++, s2++;
				}
				break;

			case 2:
				for (int i=0; i < count_samples; i++)
				{
					float left = *s1*level_left;
					if (left > max_left)
						max_left = left;
					*d++ = clamp_one(left + *s2);		//	channel 1
					s1++, s2++;
\
					float right = *s1*level_right;
					if (right > max_right)
						max_right = right;
					*d++ = clamp_one(right +  *s2);		//	channel 2
					s1++, s2++;
				}
				break;

			default:
				assert(0);
		}
	}

	if (mixer && !mixer->IsHidden())
	{
		mixer->mMsgVisualiseLevels->ReplaceInt32("track", track_idx);
		mixer->mMsgVisualiseLevels->ReplaceFloat("left", max_left);
		mixer->mMsgVisualiseLevels->ReplaceFloat("right", max_right);
		mixer->PostMessage(mixer->mMsgVisualiseLevels);
	}
}
