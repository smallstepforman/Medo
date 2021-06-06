/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effects Node template
 */

#include <cstdio>
#include <cmath>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <translation/TranslationUtils.h>

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/LanguageJson.h"
#include "Editor/Project.h"
#include "Gui/LinkedSpinners.h"

#include "fxobjects.h"
#include "Effect_IIRFilter.h"

//	TODO need to support mulitple filter contexes.  Proof of concept
static AudioFilter fAudioFilter;
static AudioFilterParameters fAudioFilterParamters;

enum EQUILISER_LANGUAGE_TEXT
{
	TXT_EQUALISER_NAME,
	TXT_EQUALISER_TEXT_A,
	TXT_EQUALISER_TEXT_B,
	TXT_EQUALISER_FILTER,
	NUMBER_EQUALISER_LANGUAGE_TEXT
};

EffectNode_IIRFilter *instantiate_effect(BRect frame)
{
	return new EffectNode_IIRFilter(frame, nullptr);
}

struct FilterAlgorithm
{
	filterAlgorithm		algorithm;
	const char			*name;
};
static FilterAlgorithm kFilterAlgorithms[] =
{
	{filterAlgorithm::kLPF1P,			"LPF 1P"},
	{filterAlgorithm::kLPF1,			"LPF 1"},
	{filterAlgorithm::kHPF1,			"HPF 1"},
	{filterAlgorithm::kLPF2,			"LPF 2"},
	{filterAlgorithm::kHPF2,			"HPF 2"},
	{filterAlgorithm::kBPF2,			"BPF 2"},
	{filterAlgorithm::kBSF2,			"BSF 2"},
	{filterAlgorithm::kButterLPF2,		"Butter LPF 2"},
	{filterAlgorithm::kButterHPF2,		"Butter HPF 2"},
	{filterAlgorithm::kButterBPF2,		"Butter BPF 2"},
	{filterAlgorithm::kButterBSF2,		"Butter BSF 2"},
	{filterAlgorithm::kMMALPF2,			"MMA LPF 2"},
	{filterAlgorithm::kMMALPF2B,		"MMA LPF 2B"},
	{filterAlgorithm::kLowShelf,		"Low Shelf"},
	{filterAlgorithm::kHiShelf,			"Hi Shelf"},
	{filterAlgorithm::kNCQParaEQ,		"NCQ Para EQ"},
	{filterAlgorithm::kCQParaEQ,		"CQ Para EQ"},
	{filterAlgorithm::kLWRLPF2,			"LWR LPF 2"},
	{filterAlgorithm::kLWRHPF2,			"LWR HPF 2"},
	{filterAlgorithm::kAPF1,			"APF 1"},
	{filterAlgorithm::kAPF2,			"APF 2"},
	{filterAlgorithm::kResonA,			"Reson A"},
	{filterAlgorithm::kResonB,			"Reson B"},
	{filterAlgorithm::kMatchLP2A,		"TIGHT fit LPF"},
	{filterAlgorithm::kMatchLP2B,		"LOOSE fit LPF"},
	{filterAlgorithm::kMatchBP2A,		"TIGHT fit BPF"},
	{filterAlgorithm::kMatchBP2B,		"LOOSE fit BPF"},
	{filterAlgorithm::kImpInvLP1,		"Impulse Invariant LP 1"},
	{filterAlgorithm::kImpInvLP2,		"Impulse Invariant LP 2"},
};
struct FilterSlider
{
	const char		*name;
	BRect			position;
	float			min;
	float			max;
	float			start;
	//	range
	const char		*label_range;
	bool			value_float;
};
enum class eFilterSlider {eFc, eQ, eBoost};
static const FilterSlider kFilterSliders[] =
{
	{"fc  (Log)",	BRect(20, 20, 200,	640),	20,		20480,		1000,		"20 - 20,480 Hz",		false},
	{"Q",			BRect(240, 20, 340,	300),	0.707f,	20,			0.707f,		"0.707-20",				true},
	{"Boost/Cut",	BRect(360, 20, 460, 300),	-20,	20,			0,			"-20 - 20 dB",			true},
};
static const int kNumberSliders = sizeof(kFilterSliders)/sizeof(FilterSlider);

enum {
	kMsgFilters			= 'eaif',
	kMsgAlgorithm,
	kMsgReset,
};

static float ConvertSliderToLog(int32 slider_value, int32 slider_range, float min_range, float max_range)
{
	float m1 = log(min_range);
	float m2= log(max_range);
	float s = (m2 - m1)/slider_range;
	return exp(m1 + s*slider_value);
}
static int ConvertLogToSlider(float value, float min_range, float max_range, int32 slider_range)
{
	float m1 = log(min_range);
	float m2= log(max_range);
	float v = log(value);
	float s = (m2 - m1)/slider_range;

	return int((v - m1)/s);
}

//	Move Data
struct EffectIIRFilterData
{
	std::vector<float>		filters;
	int						algorithm;
	EffectIIRFilterData() {filters.reserve(kNumberSliders);}
};

const char *EffectNode_IIRFilter :: GetVendorName() const	{return "ZenYes";}
const char *EffectNode_IIRFilter :: GetEffectName() const	{return "IIR Filter";}

/*	FUNCTION:		EffectNode_IIRFilter :: EffectNode_IIRFilter
	ARGS:			filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
EffectNode_IIRFilter :: EffectNode_IIRFilter(BRect frame, const char *filename)
	: EffectNode(frame, filename, false)
{
	fAudioFilter.setSampleRate(192000);
	fAudioFilterParamters = fAudioFilter.getParameters();
	printf("AudioFilterParamters:\n");
	printf("Q=%f, boost_cutoff=%f, fc=%f, algorithm=%d\n", fAudioFilterParamters.Q, fAudioFilterParamters.boostCut_dB, fAudioFilterParamters.fc, fAudioFilterParamters.algorithm);

	const float kFontFactor = be_plain_font->Size()/20.0f;

	fLanguage = new LanguageJson("AddOns/IIRFilter/Languages.json");
	if (fLanguage->GetTextCount() == 0)
	{
		printf("Cannot open \"AddOns/IIRFilter/Languages.json\"\n");
		return;
	}

	//	Sliders
	for (int i=0; i < kNumberSliders; i++)
	{
		BSlider *slider = new BSlider(kFilterSliders[i].position, nullptr, kFilterSliders[i].name, nullptr, 0, 1000, orientation::B_VERTICAL);
		slider->SetModificationMessage(new BMessage (kMsgFilters));
		slider->SetHashMarks(B_HASH_MARKS_BOTH);
		slider->SetHashMarkCount(9);
		slider->SetBarColor({255, uint8(i*10), 0, 255});
		slider->UseFillColor(true);
		fSliders.push_back(slider);
		AddChild(slider);
	}

	//	Filter type
	fOptionAlgorithm = new BOptionPopUp(BRect(240*kFontFactor, 400, 600*kFontFactor, 440), "filter", fLanguage->GetText(TXT_EQUALISER_FILTER), new BMessage(kMsgAlgorithm));
	for (std::size_t i=0; i < sizeof(kFilterAlgorithms)/sizeof(FilterAlgorithm); i++)
		fOptionAlgorithm->AddOption(kFilterAlgorithms[i].name, (int)kFilterAlgorithms[i].algorithm);
	fOptionAlgorithm->SetValue((int)filterAlgorithm::kLPF1);
	AddChild(fOptionAlgorithm);

	//	Reset
	fButtonReset = new BButton(BRect(240*kFontFactor, 480, 400*kFontFactor, 520), "reset", GetText(TXT_EFFECTS_COMMON_RESET), new BMessage(kMsgReset));
	AddChild(fButtonReset);
}

/*	FUNCTION:		EffectNode_IIRFilter :: ~EffectNode_IIRFilter
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
EffectNode_IIRFilter :: ~EffectNode_IIRFilter()
{
	delete fLanguage;
}

void EffectNode_IIRFilter :: AttachedToWindow()
{
	for (int i=0; i < kNumberSliders; i++)
	{
		fSliders[i]->SetTarget(this, Window());
		SetSliderValue(i, kFilterSliders[i].start);
	}
	fOptionAlgorithm->SetTarget(this, Window());
	fButtonReset->SetTarget(this, Window());
}

/*	FUNCTION:		EffectNode_IIRFilter :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * EffectNode_IIRFilter :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("AddOns/IIRFilter/icon_iirfilter.png");
	return icon;
}

/*	FUNCTION:		EffectNode_IIRFilter :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * EffectNode_IIRFilter :: GetTextEffectName(const uint32 language_idx)
{
	return fLanguage->GetText(TXT_EQUALISER_NAME);
}

/*	FUNCTION:		EffectNode_IIRFilter :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * EffectNode_IIRFilter :: GetTextA(const uint32 language_idx)
{
	return fLanguage->GetText(TXT_EQUALISER_TEXT_A);
}

/*	FUNCTION:		EffectNode_Equaliser :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * EffectNode_IIRFilter :: GetTextB(const uint32 language_idx)
{
	return fLanguage->GetText(TXT_EQUALISER_TEXT_B);
}

/*	FUNCTION:		EffectNode_IIRFilter :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * EffectNode_IIRFilter :: CreateMediaEffect()
{
	AudioMediaEffect *media_effect = new AudioMediaEffect;
	media_effect->mEffectNode = this;

	EffectIIRFilterData *data = new EffectIIRFilterData;
	for (int i=0; i < kNumberSliders; i++)
	{
		float value = (float)fSliders[i]->Value()/1000.0f;
		data->filters.push_back(kFilterSliders[i].min + value*(kFilterSliders[i].max - kFilterSliders[i].min));
	}
	data->algorithm = fOptionAlgorithm->Value();
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		EffectNode_IIRFilter :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void EffectNode_IIRFilter :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
	EffectIIRFilterData *data = ((EffectIIRFilterData *)effect->mEffectData);
	for (int i=0; i < kNumberSliders; i++)
		SetSliderValue(i, data->filters[i]);
	fOptionAlgorithm->SetValue(data->algorithm);
}

/*	FUNCTION:		EffectNode_IIRFilter :: SetSliderValue
	ARGS:			index
					value
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void EffectNode_IIRFilter :: SetSliderValue(int index, float value)
{
	//	Value
	if (index == int(eFilterSlider::eFc))
	{
		int32 iv = ConvertLogToSlider(value, kFilterSliders[index].min, kFilterSliders[index].max, 1000);
		fSliders[index]->SetValue(iv);
	}
	else
	{
		int32 iv = int32(1000.0f * (value - kFilterSliders[index].min) / (kFilterSliders[index].max - kFilterSliders[index].min));
		fSliders[index]->SetValue(iv);
	}

	//	Range label
	char buffer[20];
	if (kFilterSliders[index].value_float)
		sprintf(buffer, "%0.3f", value);
	else
		sprintf(buffer, "%d", (int)value);
	fSliders[index]->SetLimitLabels(buffer, kFilterSliders[index].label_range);
}


/*	FUNCTION:		EffectNode_IIRFilter :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void EffectNode_IIRFilter :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgFilters:
		{
			EffectIIRFilterData *data = GetCurrentMediaEffect() ? (EffectIIRFilterData *)GetCurrentMediaEffect()->mEffectData : nullptr;
			for (int i=0; i < kNumberSliders; i++)
			{
				float v;
				float slider_value = (float)fSliders[i]->Value()/1000.0f;
				if (i == (int)eFilterSlider::eFc)
					v = ConvertSliderToLog(fSliders[i]->Value(), 1000, kFilterSliders[i].min, kFilterSliders[i].max);
				else
					v = kFilterSliders[i].min + slider_value*(kFilterSliders[i].max - kFilterSliders[i].min);
				if (data)
					data->filters[i] = v;
				SetSliderValue(i, v);
			}
			break;
		}
		case kMsgAlgorithm:
		{
			EffectIIRFilterData *data = GetCurrentMediaEffect() ? (EffectIIRFilterData *)GetCurrentMediaEffect()->mEffectData : nullptr;
			if (data)
				data->algorithm = fOptionAlgorithm->Value();

			fAudioFilterParamters = fAudioFilter.getParameters();
			fAudioFilterParamters.algorithm = kFilterAlgorithms[fOptionAlgorithm->Value()].algorithm;
			fAudioFilter.setParameters(fAudioFilterParamters);
			printf("AudioFilterParamters:\n");
			printf("Q=%f, boost_cutoff=%f, fc=%f, algorithm=%d, name=%s\n", fAudioFilterParamters.Q, fAudioFilterParamters.boostCut_dB, fAudioFilterParamters.fc,
				   fAudioFilterParamters.algorithm, kFilterAlgorithms[(int)fAudioFilterParamters.algorithm].name);

			break;
		}
		case kMsgReset:
		{
			fAudioFilterParamters = fAudioFilter.getParameters();
			fAudioFilterParamters.algorithm = kFilterAlgorithms[(int)filterAlgorithm::kLPF1].algorithm;
			fAudioFilterParamters.fc = kFilterSliders[int(eFilterSlider::eFc)].start;
			fAudioFilterParamters.Q = kFilterSliders[int(eFilterSlider::eQ)].start;
			fAudioFilterParamters.boostCut_dB = kFilterSliders[int(eFilterSlider::eBoost)].start;
			fAudioFilter.setParameters(fAudioFilterParamters);
			printf("AudioFilterParamters:\n");
			printf("Q=%f, boost_cutoff=%f, fc=%f, algorithm=%d, name=%s\n", fAudioFilterParamters.Q, fAudioFilterParamters.boostCut_dB, fAudioFilterParamters.fc,
				   fAudioFilterParamters.algorithm, kFilterAlgorithms[(int)fAudioFilterParamters.algorithm].name);

			fOptionAlgorithm->SetValue((int)fAudioFilterParamters.algorithm);
			EffectIIRFilterData *data = GetCurrentMediaEffect() ? (EffectIIRFilterData *)GetCurrentMediaEffect()->mEffectData : nullptr;
			if (data)
			{
				data->algorithm = (int)fAudioFilterParamters.algorithm;
				for (int i=0; i < kNumberSliders; i++)
				{
					data->filters[i] = kFilterSliders[i].start;
					SetSliderValue(i, data->filters[i]);
				}
			}
			else
			{
				for (int i=0; i < kNumberSliders; i++)
					SetSliderValue(i, data->filters[i]);
			}
		}

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		EffectNode_IIRFilter :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool EffectNode_IIRFilter :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
#define ERROR_EXIT(a) {printf("[EffectNode_IIRFilter] Error - %s\n", a); return false;}

	EffectIIRFilterData *data = (EffectIIRFilterData *) media_effect->mEffectData;

	//	gain
	if (v.HasMember("filters") && v["filters"].IsArray())
	{

		const rapidjson::Value &array = v["filters"];
		if (array.Size() != kNumberSliders)
			ERROR_EXIT("filters invalid");
		for (rapidjson::SizeType e=0; e < kNumberSliders; e++)
		{
			if (!array[e].IsFloat())
				ERROR_EXIT("filters invalid");
			data->filters[e] = (double) array[e].GetFloat();
			if ((data->filters[e] < kFilterSliders[e].min) || (data->filters[e] > kFilterSliders[e].max))
				ERROR_EXIT("filters invalid");
		}
	}
	else
		ERROR_EXIT("Missing element filters");

	//	filter
	if (v.HasMember("algorithm") && v["algorithm"].IsUint())
	{
		data->algorithm = v["algorithm"].GetUint();
		if (data->algorithm >= sizeof(kFilterAlgorithms)/sizeof(FilterAlgorithm))
			ERROR_EXIT("algorithm invalid");
	}
	else
		ERROR_EXIT("algorithm invalid");

	return true;
}

/*	FUNCTION:		EffectNode_IIRFilter :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Save project data
*/
bool EffectNode_IIRFilter :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectIIRFilterData *d = (EffectIIRFilterData *) media_effect->mEffectData;
	if (d)
	{
		char buffer[0x80];	//	128 bytes
		sprintf(buffer, "\t\t\t\t\"filters\": [");
		fwrite(buffer, strlen(buffer), 1, file);
		for (int i=0; i < kNumberSliders; i++)
		{
			if (i < kNumberSliders - 1)
				sprintf(buffer, "%f, ", d->filters[i]);
			else
				sprintf(buffer, "%f],\n", d->filters[i]);
			fwrite(buffer, strlen(buffer), 1, file);
		}

		sprintf(buffer, "\t\t\t\t\"algorithm\": %u\n", d->algorithm);
		fwrite(buffer, strlen(buffer), 1, file);
	}
	return true;
}


/*	FUNCTION:		EffectNode_IIRFilter :: AudioEffect
	ARGS:			effect
					destination, source
					start_frame, end_frame
					count_channels
					sample_size
					count_samples
	RETURN:			New number channels
	DESCRIPTION:	Perform audio effect on buffer
*/
int EffectNode_IIRFilter :: AudioEffect(MediaEffect *effect, uint8 *destination, uint8 *source,
											const int64 start_frame, const int64 end_frame,
											const int64 audio_start, const int64 audio_end,
											const int count_channels, const size_t sample_size, const size_t count_samples)
{
	assert(destination != nullptr);
	assert(source != nullptr);
	assert(effect != nullptr);
	assert(effect->mEffectData != nullptr);
	EffectIIRFilterData *data = (EffectIIRFilterData *) effect->mEffectData;

	bool reset = false;
	AudioFilterParameters params = fAudioFilter.getParameters();
	if ((int)params.algorithm != data->algorithm)
	{
		params.algorithm = (filterAlgorithm)data->algorithm;
		reset = true;
	}
	if (params.fc != data->filters[int(eFilterSlider::eFc)])
	{
		params.fc = data->filters[int(eFilterSlider::eFc)];
		reset = true;
	}
	if (params.Q != data->filters[int(eFilterSlider::eQ)])
	{
		params.Q = data->filters[int(eFilterSlider::eQ)];
		reset = true;
	}
	if (params.boostCut_dB != data->filters[int(eFilterSlider::eBoost)])
	{
		params.boostCut_dB = data->filters[int(eFilterSlider::eBoost)];
		reset = true;
	}
	if (reset)
	{
#if 0
		printf("AudioFilterParamters:\n");
		printf("fc=%f, Q=%f, boost_cutoff=%f, algorithm=[%d] %s\n", params.fc, params.Q, params.boostCut_dB,
			   params.algorithm, kFilterAlgorithms[(int)params.algorithm].name);
#endif
		fAudioFilter.setParameters(params);
	}

	float *d = (float *)destination;
	float *s = (float *)source;
	for (int i=0; i < count_samples; i++)
	{
		for (int ch=0; ch < count_channels; ch++)
		{
			double in = (double) *s++;
			double out;
			out = fAudioFilter.processAudioSample(in);
			*d++ = (float) out;
		}
	}
	return count_channels;
}

