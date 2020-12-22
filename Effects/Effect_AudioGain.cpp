/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effects Node template
 */

#include <cstdio>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/Project.h"
#include "Gui/DualSlider.h"

#include "Effect_AudioGain.h"


enum GainMessages
{
	eMsgSliderStart = 'eagn',
	eMsgSliderEnd,
	eMsgInterpolate,
};

static const float kMaxGain = 4.0f;

static float Clamp(float v, float min, float max)
{
	if (v < min)
		v = min;
	else if (v > max)
		v = max;
	return v;
}

//	Move Data
struct EffectGainData
{
	float	start_gain[2];
	float	end_gain[2];
	bool	interpolate;
};

const char *Effect_AudioGain :: GetVendorName() const	{return "ZenYes";}
const char *Effect_AudioGain :: GetEffectName() const	{return "Gain";}

/*	FUNCTION:		Effect_AudioGain :: Effect_AudioGain
	ARGS:			filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_AudioGain :: Effect_AudioGain(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fSliderStart = new DualSlider(BRect(20, 60, 120, 600), "start", GetText(TXT_EFFECTS_AUDIO_GAIN_START), new BMessage(eMsgSliderStart), 0, kMaxGain*100,
								  GetText(TXT_EFFECTS_COMMON_L), GetText(TXT_EFFECTS_COMMON_R));
	fSliderStart->SetValue(0, 100);
	fSliderStart->SetValue(1, 100);
	mEffectView->AddChild(fSliderStart);

	fCheckboxInterpolate = new BCheckBox(BRect(300, 20, 500, 50), "interpolate", GetText(TXT_EFFECTS_COMMON_INTERPOLATE), new BMessage(eMsgInterpolate));
	mEffectView->AddChild(fCheckboxInterpolate);

	fSliderEnd = new DualSlider(BRect(300, 60, 400, 600), "end", GetText(TXT_EFFECTS_AUDIO_GAIN_END), new BMessage(eMsgSliderEnd), 0, kMaxGain*100,
								GetText(TXT_EFFECTS_COMMON_L), GetText(TXT_EFFECTS_COMMON_R));
	fSliderEnd->SetValue(0, 100);
	fSliderEnd->SetValue(1, 100);
	fSliderEnd->SetEnabled(false);
	mEffectView->AddChild(fSliderEnd);
}

/*	FUNCTION:		Effect_AudioGain :: ~Effect_AudioGain
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_AudioGain :: ~Effect_AudioGain()
{
}

/*	FUNCTION:		Effect_AudioGain :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_AudioGain :: AttachedToWindow()
{
	fSliderStart->SetTarget(this, Window());
	fSliderEnd->SetTarget(this, Window());
	fCheckboxInterpolate->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_AudioGain :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_AudioGain :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_AudioGain.png");
	return icon;
}

/*	FUNCTION:		Effect_AudioGain :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_AudioGain :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_AUDIO_GAIN);
}

/*	FUNCTION:		Effect_AudioGain :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_AudioGain :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_AUDIO_GAIN_TEXT_A);
}

/*	FUNCTION:		Effect_AudioGain :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_AudioGain :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_AUDIO_GAIN_TEXT_B);
}

/*	FUNCTION:		Effect_AudioGain :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_AudioGain :: CreateMediaEffect()
{
	AudioMediaEffect *media_effect = new AudioMediaEffect;
	media_effect->mEffectNode = this;

	EffectGainData *data = new EffectGainData;
	data->start_gain[0] = fSliderStart->GetValue(0)/100.0f;
	data->start_gain[1] = fSliderStart->GetValue(1)/100.0f;
	data->end_gain[0] = fSliderEnd->GetValue(0)/100.0f;
	data->end_gain[1] = fSliderEnd->GetValue(1)/100.0f;
	data->interpolate = fCheckboxInterpolate->Value();
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_AudioGain :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_AudioGain :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
	EffectGainData *data = ((EffectGainData *)effect->mEffectData);
	fSliderStart->SetValue(0, data->start_gain[0]*100);
	fSliderStart->SetValue(1, data->start_gain[1]*100);
	fSliderEnd->SetValue(0, data->end_gain[0]*100);
	fSliderEnd->SetValue(1, data->end_gain[1]*100);
	fSliderEnd->SetEnabled(data->interpolate);
	fCheckboxInterpolate->SetValue(data->interpolate);
}

/*	FUNCTION:		Effect_AudioGain :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_AudioGain :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case eMsgSliderStart:
		{
			bool interpolate = fCheckboxInterpolate->Value() > 0;
			if (!interpolate)
			{
				fSliderEnd->SetValue(0, fSliderStart->GetValue(0));
				fSliderEnd->SetValue(1, fSliderStart->GetValue(1));
			}
			if (GetCurrentMediaEffect())
			{
				EffectGainData *data = (EffectGainData *)GetCurrentMediaEffect()->mEffectData;
				data->start_gain[0] = fSliderStart->GetValue(0)/100.0f;
				data->start_gain[1] = fSliderStart->GetValue(1)/100.0f;
				if (!interpolate)
				{
					data->end_gain[0] = data->start_gain[0];
					data->end_gain[1] = data->start_gain[1];
				}
			}
			break;
		}
		case eMsgSliderEnd:
		{
			if (GetCurrentMediaEffect())
			{
				EffectGainData *data = (EffectGainData *)GetCurrentMediaEffect()->mEffectData;
				data->end_gain[0] = fSliderEnd->GetValue(0)/100.0f;
				data->end_gain[1] = fSliderEnd->GetValue(1)/100.0f;
			}
			break;
		}
		case eMsgInterpolate:
		{
			bool interpolate = fCheckboxInterpolate->Value() > 0;
			fSliderEnd->SetEnabled(interpolate);
			if (!interpolate)
			{
				fSliderEnd->SetValue(0, fSliderStart->GetValue(0));
				fSliderEnd->SetValue(1, fSliderStart->GetValue(1));
			}
			if (GetCurrentMediaEffect())
			{
				EffectGainData *data = (EffectGainData *)GetCurrentMediaEffect()->mEffectData;
				data->interpolate = interpolate;
				if (!interpolate)
				{
					data->end_gain[0] = data->start_gain[0];
					data->end_gain[1] = data->start_gain[1];
				}
			}
			break;
		}

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		Effect_AudioGain :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_AudioGain :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = true;
	EffectGainData *data = (EffectGainData *) media_effect->mEffectData;
	memset(data, 0, sizeof(EffectGainData));

#define ERROR_EXIT(a) {printf("[Effect_Colour] Error - %s\n", a); return false;}

	//	start
	if (v.HasMember("start") && v["start"].IsArray())
	{
		const rapidjson::Value &array = v["start"];
		if (array.Size() != 2)
			ERROR_EXIT("start invalid");
		for (rapidjson::SizeType e=0; e < 2; e++)
		{
			if (!array[e].IsFloat())
				ERROR_EXIT("start invalid");
			data->start_gain[e] = array[e].GetFloat();
			if ((data->start_gain[e] < 0.0f) || (data->start_gain[e] > kMaxGain))
				ERROR_EXIT("start invalid");
		}
	}
	else
		ERROR_EXIT("missing start");

	//	end
	if (v.HasMember("end") && v["end"].IsArray())
	{
		const rapidjson::Value &array = v["end"];
		if (array.Size() != 2)
			ERROR_EXIT("end invalid");
		for (rapidjson::SizeType e=0; e < 2; e++)
		{
			if (!array[e].IsFloat())
				ERROR_EXIT("end invalid");
			data->end_gain[e] = array[e].GetFloat();
			if ((data->end_gain[e] < 0.0f) || (data->end_gain[e] > kMaxGain))
				ERROR_EXIT("end invalid");
		}
	}
	else
		ERROR_EXIT("missing end");

	//	interpolate
	if (v.HasMember("interpolate") && v["interpolate"].IsBool())
	{
		data->interpolate = v["interpolate"].GetBool();
	}

	return valid;
}

/*	FUNCTION:		Effect_AudioGain :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Save project data
*/
bool Effect_AudioGain :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectGainData *data = (EffectGainData *) media_effect->mEffectData;
	if (data)
	{
		char buffer[0x80];	//	128 bytes
		sprintf(buffer, "\t\t\t\t\"start\": [%f, %f],\n", data->start_gain[0], data->start_gain[1]);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"end\": [%f, %f],\n", data->end_gain[0], data->end_gain[1]);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"interpolate\": %s\n", data->interpolate ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);
	}
	return true;
}


/*	FUNCTION:		Effect_AudioGain :: AudioEffect
	ARGS:			effect
					destination, source
					start_frame, end_frame
					count_channels
					sample_size
					count_samples
	RETURN:			New number channels
	DESCRIPTION:	Perform audio effect on buffer
*/
int Effect_AudioGain :: AudioEffect(MediaEffect *effect, uint8 *destination, uint8 *source,
										const int64 start_frame, const int64 end_frame,
										const int64 audio_start, const int64 audio_end,
										const int count_channels, const size_t sample_size, const size_t count_samples)
{
	assert(destination != nullptr);
	assert(source != nullptr);
	assert((count_channels > 0) && (count_channels <= 2));

	float *d = (float *)destination;
	float *s = (float *)source;

	EffectGainData *data = (EffectGainData *) effect->mEffectData;
	assert(data);
	assert(sample_size == sizeof(float));

	float t0 = Clamp(float(start_frame - effect->mTimelineFrameStart)/effect->Duration(), 0.0f, 1.0f);
	float t1 = Clamp(float(end_frame - effect->mTimelineFrameStart)/effect->Duration(), 0.0f, 1.0f);
	float g0[2];
	float g1[2];
	for (int c=0; c < 2; c++)
	{
		g0[c] = data->start_gain[c] + t0*(data->end_gain[c] - data->start_gain[c]);
		g1[c] = data->start_gain[c] + t1*(data->end_gain[c] - data->start_gain[c]);
	}

#if 0
	memcpy(destination, source, sample_size * count_samples * count_channels);
#else
	if (count_samples <= 2)
	{
		for (int i=0; i < count_samples; i++)
		{
			float t = i/count_samples;
			for (int c=0; c < count_channels; c++)
			{
				*d++ = *s++ * (g0[c] + t*(g1[c]-g0[c]));
			}
		}
	}
	else
	{
		//	when dealing with more than 2 audio channels, use left channel volume for remaineder
		for (int i=0; i < count_samples; i++)
		{
			float t = i/count_samples;
			for (int c=0; c < 2; c++)
			{
				*d++ = *s++ * (g0[c] + t*(g1[c]-g0[c]));
			}
			for (int c=2; c < count_channels; c++)
			{
				*d++ = *s++ * (g0[0] + t*(g1[0]-g0[0]));
			}
		}
	}
#endif

	return count_channels;
}


