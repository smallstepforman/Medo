/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Audio Effect / IIR Filter
 *					Designing Audio Effects Plugins in C++, Will C. Pirkle, 2nd Edition
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

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

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
	{"fc  (Log)",	BRect(20, 20, 200,	640),	20,		20480,		1000,		"[20, 20,480] Hz",		false},
	{"Q",			BRect(240, 20, 340,	300),	0.707f,	20,			0.707f,		"[0.707, 20]",			true},
	{"Boost/Cut",	BRect(360, 20, 460, 300),	-20,	20,			0,			"(-20, 20) dB",			true},
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

//	Filter Data
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
	FilterCache anItem;
	anItem.audio_filter = new AudioFilter;
	anItem.audio_end_frame = -1;
	anItem.sample_rate = 192'000;
	AudioFilterParameters params = anItem.audio_filter->getParameters();
	params.algorithm = filterAlgorithm::kLPF1;
	params.fc = kFilterSliders[(int)eFilterSlider::eFc].start;
	params.Q = kFilterSliders[(int)eFilterSlider::eQ].start;
	params.boostCut_dB = kFilterSliders[(int)eFilterSlider::eBoost].start;
	anItem.audio_filter->setParameters(params);
	anItem.audio_filter->setSampleRate(anItem.sample_rate);
	fFilterCache.push_back(anItem);

	DEBUG("AudioFilterParamters:\n");
	DEBUG("Q=%f, boost_cutoff=%f, fc=%f, algorithm=%s, sample_rate=%f\n", params.Q, params.boostCut_dB, params.fc, kFilterAlgorithms[(int)params.algorithm].name, anItem.sample_rate);
	DEBUG("sizeof(AudioFiler) = %lu\n", sizeof(AudioFilter));

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
		data->filters.push_back(ConvertSliderToLog(fSliders[i]->Value(), 1000, kFilterSliders[i].min, kFilterSliders[i].max));
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
				if (i == (int)eFilterSlider::eFc)
					v = ConvertSliderToLog(fSliders[i]->Value(), 1000, kFilterSliders[i].min, kFilterSliders[i].max);
				else
				{
					float slider_value = (float)fSliders[i]->Value()/1000.0f;
					v = kFilterSliders[i].min + slider_value*(kFilterSliders[i].max - kFilterSliders[i].min);
				}
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
			break;
		}
		case kMsgReset:
		{
			AudioFilterParameters params;
			params.algorithm = kFilterAlgorithms[(int)filterAlgorithm::kLPF1].algorithm;
			params.fc = kFilterSliders[int(eFilterSlider::eFc)].start;
			params.Q = kFilterSliders[int(eFilterSlider::eQ)].start;
			params.boostCut_dB = kFilterSliders[int(eFilterSlider::eBoost)].start;

			fOptionAlgorithm->SetValue((int)params.algorithm);
			EffectIIRFilterData *data = GetCurrentMediaEffect() ? (EffectIIRFilterData *)GetCurrentMediaEffect()->mEffectData : nullptr;
			if (data)
			{
				data->algorithm = (int)params.algorithm;
				for (int i=0; i < kNumberSliders; i++)
				{
					data->filters[i] = kFilterSliders[i].start;
					SetSliderValue(i, data->filters[i]);
				}
			}
			else
			{
				for (int i=0; i < kNumberSliders; i++)
					SetSliderValue(i, kFilterSliders[i].start);
			}
			break;
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

	const float kSampleRate = count_samples * kFramesSecond / (end_frame-start_frame);

	//	Find cached version
	std::size_t cache_idx = 0;
	for (auto &i : fFilterCache)
	{
		DEBUG("Assess %d.  audio_start=%lu, candidate=%lu\n", cache_idx, audio_start, i.audio_end_frame);
		if ((audio_start == i.audio_end_frame) || (audio_start == i.audio_end_frame + 1))
		{
			AudioFilterParameters params = i.audio_filter->getParameters();
			DEBUG("params.algorithm=%d, fc=%f, Q=%f, boost=%f, sample_rate=%f\n", params.algorithm, params.fc, params.Q, params.boostCut_dB, i.sample_rate);
			DEBUG("data.algorithm=%d, fc=%f, Q=%f, boost=%f, sample_rate=%f\n", data->algorithm, data->filters[(int)eFilterSlider::eFc], data->filters[(int)eFilterSlider::eQ], data->filters[(int)eFilterSlider::eBoost], kSampleRate);
			if ((params.algorithm == (filterAlgorithm)data->algorithm) &&
				(params.fc == data->filters[(int)eFilterSlider::eFc]) &&
				(params.Q == data->filters[(int)eFilterSlider::eQ]) &&
				(params.boostCut_dB == data->filters[(int)eFilterSlider::eBoost]) &&
				(i.sample_rate == kSampleRate))
			{
				//	Item found, move to beginning of fFilterCache
				if (cache_idx != 0)
				{
					FilterCache item = i;
					fFilterCache.erase(fFilterCache.begin() + cache_idx);
					fFilterCache.insert(fFilterCache.begin(), item);
					cache_idx = 0;
					DEBUG("Item found at %d, move to 0\n", cache_idx);
				}
				else
					DEBUG("Item found at 0\n");
				break;
			}
		}
		cache_idx++;
	}
	//	If item not found, create new entry
	if (cache_idx == fFilterCache.size())
	{
		//	Max Queue Size
		if (cache_idx >= 16)
		{
			delete fFilterCache[fFilterCache.size() - 1].audio_filter;
			fFilterCache.pop_back();
			DEBUG("Item not found - delete last entry\n");
		}
		else
			DEBUG("Item not found - Adding to cache\n");

		FilterCache item;
		item.audio_filter  = new AudioFilter;
		AudioFilterParameters params = item.audio_filter->getParameters();
		params.algorithm = (filterAlgorithm)data->algorithm;
		params.fc = data->filters[(int)eFilterSlider::eFc];
		params.Q = data->filters[(int)eFilterSlider::eQ];
		params.boostCut_dB = data->filters[(int)eFilterSlider::eBoost];
		item.audio_filter->setParameters(params);
		item.audio_filter->setSampleRate(kSampleRate);
		item.sample_rate = kSampleRate;
		fFilterCache.insert(fFilterCache.begin(), item);
		cache_idx = 0;
	}

	AudioFilter *audio_filter = fFilterCache[0].audio_filter;
	fFilterCache[0].audio_end_frame = audio_end;

#if 0
	DEBUG("Status of cache:\n");
	for (int i=0; i < fFilterCache.size(); i++)
	{
		AudioFilterParameters params = fFilterCache[i].audio_filter->getParameters();
		DEBUG("[%d] algorithm=%d, fc=%f, Q=%f, boost=%f, sample_rate=%f, end=%lu\n", i, params.algorithm, params.fc, params.Q, params.boostCut_dB, fFilterCache[i].sample_rate, fFilterCache[i].audio_end_frame);
	}
#endif

	//	Apply filter
	float *d = (float *)destination;
	float *s = (float *)source;
	for (int i=0; i < count_samples; i++)
	{
		for (int ch=0; ch < count_channels; ch++)
		{
			double in = (double) *s++;
			double out = audio_filter->processAudioSample(in);
			*d++ = (float) out;
		}
	}
	return count_channels;
}

