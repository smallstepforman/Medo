/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effects Node template
 */

#include <cstdio>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <translation/TranslationUtils.h>

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/Project.h"
#include "Gui/LinkedSpinners.h"

#include "Effect_Equaliser.h"

enum EQUILISER_LANGUAGE_TEXT
{
	TXT_EQUALISER_NAME,
	TXT_EQUALISER_TEXT_A,
	TXT_EQUALISER_TEXT_B,
	TXT_EQUALISER_FILTER,
	NUMBER_EQUALISER_LANGUAGE_TEXT
};
static const char *kEqualiserLanguages[][NUMBER_EQUALISER_LANGUAGE_TEXT] =
{
{"Equaliser",		"Equaliser",		"20 band Equaliser",		"Filter"},			//	"English (Britian)",
{"Equalizer",		"Equalizer",		"20 band Equalizer",		"Filter"},			//	"English (USA)",
{"Equalizer",		"Equalizer",		"20 band Equalizer",		"Filter"},			//	"Deutsch",
{"Equaliser",		"Equaliser",		"20 band Equaliser",		"Filter"},			//	"Français",
{"Equaliser",		"Equaliser",		"20 band Equaliser",		"Filter"},			//	"Italiano",
{"Equaliser",		"Equaliser",		"20 band Equaliser",		"Filter"},			//	"Русский",
{"Eквилајзер",		"Eквилајзер",		"20-опсежни еквилајзер",	"Филтер"},			//	"Српски",
{"Ecualizador",		"Ecualizador",		"Ecualizador de 20 bandas",	"Filtro"},			//	"Español",
};

EffectNode_Equaliser *instantiate_effect(BRect frame)
{
	return new EffectNode_Equaliser(frame, nullptr);
}

//	Support 5/10/20/30 bands
static const std::size_t kNumberSliders = 20;

enum {
	kMsgGain			= 'eagn',
	kMsgReset,
	kMsgFilter,
};


//	Move Data
struct EffectEqualiserData
{
	std::vector<double>		gains;
	int						filter;
	EffectEqualiserData() {gains.reserve(kNumberSliders);}
};

const char *EffectNode_Equaliser :: GetVendorName() const	{return "ZenYes";}
const char *EffectNode_Equaliser :: GetEffectName() const	{return "Equaliser";}

/*	FUNCTION:		EffectNode_Equaliser :: EffectNode_Equaliser
	ARGS:			filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
EffectNode_Equaliser :: EffectNode_Equaliser(BRect frame, const char *filename)
	: EffectNode(frame, filename, false)
{
	assert(sizeof(kEqualiserLanguages)/(NUMBER_EQUALISER_LANGUAGE_TEXT*sizeof(char *)) == GetAvailableLanguages().size());

	OrfanidisEq::FrequencyGrid fg;
	switch (kNumberSliders)
	{
		case 30:	fFrequencyGrid.set30Bands();		break;
		case 20:	fFrequencyGrid.set20Bands();		break;
		case 10:	fFrequencyGrid.set10Bands();		break;
		case 5:		fFrequencyGrid.set5Bands();			break;
		default:	assert(0);
	}
	//fFrequencyGrid.printFrequencyGrid();
	fEqualiser = new OrfanidisEq::Eq(fFrequencyGrid, OrfanidisEq::filter_type::butterworth);

	fRotatedFont = new BFont(be_plain_font);
	fRotatedFont->SetSize(20);
	fRotatedFont->SetRotation(90);

	char buffer[20];
	for (int i=0; i < kNumberSliders; i++)
	{
		BSlider *slider = new BSlider(BRect(10+i*36, 100, 10+(i+1)*36, 310), nullptr, nullptr, nullptr, 0, 200, orientation::B_VERTICAL);
		slider->SetModificationMessage(new BMessage (kMsgGain));
		slider->SetHashMarks(B_HASH_MARKS_BOTH);
		slider->SetHashMarkCount(5);
		slider->SetValue(100);
		slider->SetBarColor({255, uint8(i*10), 0, 255});
		slider->UseFillColor(true);
		fSliders.push_back(slider);
		AddChild(slider);

		int freq = fFrequencyGrid.getRoundedFreq(i);
		if (freq < 1000)
			sprintf(buffer, "%d Hz", freq);
		else
			sprintf(buffer, "%d.%d KHz", freq/1000, (freq%1000)/100);
		fLabels.push_back(buffer);
	}

	fButtonReset = new BButton(BRect(20, 340, 200, 380), "reset", GetText(TXT_EFFECTS_COMMON_RESET), new BMessage(kMsgReset));
	AddChild(fButtonReset);

	fOptionFilter = new BOptionPopUp(BRect(300, 340, 500, 380), "filter", kEqualiserLanguages[GetLanguage()][TXT_EQUALISER_FILTER], new BMessage(kMsgFilter));
	//fOptionFilter->AddOption("None", OrfanidisEq::filter_type::none);
	fOptionFilter->AddOption("Butterworth", OrfanidisEq::filter_type::butterworth);
	fOptionFilter->AddOption("Chebyshev1", OrfanidisEq::filter_type::chebyshev1);
	fOptionFilter->AddOption("Chebyshev2", OrfanidisEq::filter_type::chebyshev2);
	fOptionFilter->AddOption("Elliptic", OrfanidisEq::filter_type::elliptic);
	fOptionFilter->SetValue(OrfanidisEq::filter_type::elliptic);
	AddChild(fOptionFilter);
}

/*	FUNCTION:		EffectNode_Equaliser :: ~EffectNode_Equaliser
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
EffectNode_Equaliser :: ~EffectNode_Equaliser()
{
	delete fEqualiser;
	delete fRotatedFont;
}

void EffectNode_Equaliser :: AttachedToWindow()
{
	for (int i=0; i < kNumberSliders; i++)
		fSliders[i]->SetTarget(this, Window());
	fButtonReset->SetTarget(this, Window());
	fOptionFilter->SetTarget(this, Window());
}

/*	FUNCTION:		EffectNode_Equaliser :: Draw
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Cannot use BStringView since clips rotated font
*/
void EffectNode_Equaliser :: Draw(BRect frame)
{
	SetFont(fRotatedFont);
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	for (int i=0; i < kNumberSliders; i++)
	{
		MovePenTo(26+i*36, 90);
		DrawString(fLabels[i]);
	}
	SetFont(be_plain_font);
}

/*	FUNCTION:		EffectNode_Equaliser :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * EffectNode_Equaliser :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("AddOns/Equaliser/icon_equaliser.png");
	return icon;
}

/*	FUNCTION:		EffectNode_Equaliser :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * EffectNode_Equaliser :: GetTextEffectName(const uint32 language_idx)
{
	return kEqualiserLanguages[GetLanguage()][TXT_EQUALISER_NAME];
}

/*	FUNCTION:		EffectNode_Equaliser :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * EffectNode_Equaliser :: GetTextA(const uint32 language_idx)
{
	return kEqualiserLanguages[GetLanguage()][TXT_EQUALISER_TEXT_A];
}

/*	FUNCTION:		EffectNode_Equaliser :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * EffectNode_Equaliser :: GetTextB(const uint32 language_idx)
{
	return kEqualiserLanguages[GetLanguage()][TXT_EQUALISER_TEXT_B];
}

/*	FUNCTION:		EffectNode_Equaliser :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * EffectNode_Equaliser :: CreateMediaEffect()
{
	AudioMediaEffect *media_effect = new AudioMediaEffect;
	media_effect->mEffectNode = this;

	EffectEqualiserData *data = new EffectEqualiserData;
	for (int i=0; i < kNumberSliders; i++)
		data->gains.push_back(fSliders[i]->Value()/100.0f);
	data->filter = fOptionFilter->Value();
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		EffectNode_Equaliser :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void EffectNode_Equaliser :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
	EffectEqualiserData *data = ((EffectEqualiserData *)effect->mEffectData);
	for (int i=0; i < kNumberSliders; i++)
		fSliders[i]->SetValue(data->gains[i]*100);
	fOptionFilter->SetValue(data->filter);
}

/*	FUNCTION:		EffectNode_Equaliser :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void EffectNode_Equaliser :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgGain:
		{
			if (GetCurrentMediaEffect())
			{
				EffectEqualiserData *data = (EffectEqualiserData *)GetCurrentMediaEffect()->mEffectData;
				if (data)
				{
					for (std::size_t i=0; i < kNumberSliders; i++)
					{
						data->gains[i] = fSliders[i]->Value()/100.0f;
						if (data->gains[i] < 0.05)
							data->gains[i] = 0.05;
					}
				}
			}
			break;
		}
		case kMsgReset:
		{
			for (std::size_t i=0; i < kNumberSliders; i++)
			{
				fSliders[i]->SetValue(100);
			}
			if (GetCurrentMediaEffect())
			{
				EffectEqualiserData *data = (EffectEqualiserData *)GetCurrentMediaEffect()->mEffectData;
				if (data)
				{
					for (std::size_t i=0; i < kNumberSliders; i++)
						data->gains[i] = 1.0f;
				}
			}
			break;
		}
		case kMsgFilter:
		{
			if (GetCurrentMediaEffect())
			{
				EffectEqualiserData *data = (EffectEqualiserData *)GetCurrentMediaEffect()->mEffectData;
				if (data)
					data->filter = fOptionFilter->Value();
			}
			break;
		}

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		EffectNode_Equaliser :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool EffectNode_Equaliser :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
#define ERROR_EXIT(a) {printf("[Effect_Equaliser] Error - %s\n", a); return false;}

	EffectEqualiserData *data = (EffectEqualiserData *) media_effect->mEffectData;

	//	gain
	if (v.HasMember("gain") && v["gain"].IsArray())
	{

		const rapidjson::Value &array = v["gain"];
		if (array.Size() != kNumberSliders)
			ERROR_EXIT("gain invalid");
		for (rapidjson::SizeType e=0; e < kNumberSliders; e++)
		{
			if (!array[e].IsFloat())
				ERROR_EXIT("gain invalid");
			data->gains[e] = (double) array[e].GetFloat();
			if ((data->gains[e] < 0) || (data->gains[e] > 2.0f))
				ERROR_EXIT("gain invalid");
		}
	}
	else
		ERROR_EXIT("Missing element gain");

	//	filter
	if (v.HasMember("filter") && v["filter"].IsInt())
	{
		data->filter = v["filter"].GetInt();
		if ((data->filter < 0) || (data->filter > 4))
			ERROR_EXIT("filter invalid");
	}
	else
		ERROR_EXIT("filter invalid");

	return true;
}

/*	FUNCTION:		EffectNode_Equaliser :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Save project data
*/
bool EffectNode_Equaliser :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectEqualiserData *d = (EffectEqualiserData *) media_effect->mEffectData;
	if (d)
	{
		char buffer[0x80];	//	128 bytes
		sprintf(buffer, "\t\t\t\t\"gain\": [");
		fwrite(buffer, strlen(buffer), 1, file);
		for (int i=0; i < kNumberSliders; i++)
		{
			if (i < kNumberSliders - 1)
				sprintf(buffer, "%f, ", d->gains[i]);
			else
				sprintf(buffer, "%f],\n", d->gains[i]);
			fwrite(buffer, strlen(buffer), 1, file);
		}

		sprintf(buffer, "\t\t\t\t\"filter\": %d\n", d->filter);
		fwrite(buffer, strlen(buffer), 1, file);
	}
	return true;
}


/*	FUNCTION:		EffectNode_Equaliser :: AudioEffect
	ARGS:			effect
					destination, source
					start_frame, end_frame
					count_channels
					sample_size
					count_samples
	RETURN:			New number channels
	DESCRIPTION:	Perform audio effect on buffer
*/
int EffectNode_Equaliser :: AudioEffect(MediaEffect *effect, uint8 *destination, uint8 *source,
											const int64 start_frame, const int64 end_frame,
											const int64 audio_start, const int64 audio_end,
											const int count_channels, const size_t sample_size, const size_t count_samples)
{
	assert(destination != nullptr);
	assert(source != nullptr);
	assert(effect != nullptr);
	assert(effect->mEffectData != nullptr);
	EffectEqualiserData *data = (EffectEqualiserData *) effect->mEffectData;

	OrfanidisEq::filter_type filter = (OrfanidisEq::filter_type)fOptionFilter->Value();
	if (fEqualiser->getEqType() != filter)
		fEqualiser->setEq(filter);

	assert(fEqualiser->changeGains(data->gains) == 0);
	float *d = (float *)destination;
	float *s = (float *)source;
	for (int i=0; i < count_samples; i++)
	{
		for (int ch=0; ch < count_channels; ch++)
		{
			double in = (double) *s++;
			double out;
			fEqualiser->SBSProcess(&in, &out);
			*d++ = (float) out;
		}
	}
	return count_channels;
}

