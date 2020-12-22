/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Text
 */

#include <cstdio>

#include <InterfaceKit.h>
#include <interface/ChannelSlider.h>
#include <translation/TranslationUtils.h>
#include <support/StringList.h>

#include "Yarra/Render/Font.h"
#include "Yarra/Render/Picture.h"
#include "Yarra/Render/Texture.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/Project.h"

#include "Gui/Spinner.h"
#include "Gui/FontPanel.h"

#include "Effect_Text.h"
#include "Effect_Text_Counter.h"

enum kGuiMessages
{
	eMsgControlStart	= 'emtc',
	eMsgControlEnd,
	eMsgRadioCurrency,
	eMsgRadioNumber,
	eMsgRadioTimeMinSec,
	eMsgRadioTimeHourMinSec,
	eMsgRadioDate,
	eMsgThresholdLeft,
	eMsgThresholdRight,
};
struct RADIO_BUTTON
{
	BRect			position;
	LANGUAGE_TEXT	text;
	kGuiMessages	message;
	const char		*format;
	const char		*tooltip;
};
static const RADIO_BUTTON kRadioButtons[] =
{
	{BRect(420, 10, 620, 40),		TXT_EFFECTS_TEXT_COUNTER_CURRENCY,				eMsgRadioCurrency,			"$%d.%02d",			"$%d.%0.2dc will display $1.23c"},
	{BRect(420, 40, 620, 40),		TXT_EFFECTS_TEXT_COUNTER_NUMBER,				eMsgRadioNumber,			"%d",				"%d will display 100000"},
	{BRect(420, 70, 620, 100),		TXT_EFFECTS_TEXT_COUNTER_TIME_MIN_SEC,			eMsgRadioTimeMinSec,		"%dm:%02ds",		"%dm:%02dpm will display 10:00pm"},
	{BRect(420, 100, 620, 130),		TXT_EFFECTS_TEXT_COUNTER_TIME_HOUR_MIN_SEC,		eMsgRadioTimeHourMinSec,	"%dh:%02dm:%02ds",	"%dh:%02dm:%02ds will display 10:00:00"},
	{BRect(420, 130, 620, 160),		TXT_EFFECTS_TEXT_COUNTER_DATE,					eMsgRadioDate,				"%d/%M/%y",			"%d/%M/%y will display 28/Feb/2000\n%M-%d-%Y will display 02-28-00"},
};

const char * Effect_TextCounter :: GetVendorName() const	{return "ZenYes";}
const char * Effect_TextCounter :: GetEffectName() const	{return "Counter";}

struct EffectTextCounterData
{
	int64_t			start_amount;
	int64_t			end_amount;
	int				counter_type;
	int				left_delay;
	int				right_delay;
	BString			format;
};

//	TextCounterMediaEffect allows customising destructor
class TextCounterMediaEffect : public ImageMediaEffect
{
public:
	TextCounterMediaEffect()  : ImageMediaEffect() { }
	~TextCounterMediaEffect()
	{
		assert(mEffectNode);
		assert(mEffectData);

		Effect_TextCounter *effect_node = (Effect_TextCounter *)mEffectNode;
		Effect_Text::EffectTextData *data = (Effect_Text::EffectTextData *)mEffectData;
		delete (EffectTextCounterData *) data->derived_data;
		delete data;
	}
};

using namespace yrender;

/*	FUNCTION:		Effect_TextCounter :: Effect_TextCounter
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_TextCounter :: Effect_TextCounter(BRect frame, const char *filename)
	: Effect_Text(frame, filename)
{
	mEffectView->RemoveChild(fTextView);

	BStringView *title = new BStringView(BRect(20, 10, 200, 40), "title", GetText(TXT_EFFECTS_TEXT_COUNTER_RANGE));
	title->SetFont(be_bold_font);
	mEffectView->AddChild(title);

	fTextRange[0] = new BTextControl(BRect(20, 50, 340, 80), "start", GetText(TXT_EFFECTS_TEXT_COUNTER_START_VALUE), "1", new BMessage(eMsgControlStart));
	mEffectView->AddChild(fTextRange[0]);
	fTextRange[1] = new BTextControl(BRect(20, 90, 340, 120), "end", GetText(TXT_EFFECTS_TEXT_COUNTER_END_VALUE), "100", new BMessage(eMsgControlEnd));
	mEffectView->AddChild(fTextRange[1]);

	static_assert(sizeof(kRadioButtons)/sizeof(RADIO_BUTTON) == kNumberCounters, "sizeof(kRadioButtons) != kNumberCounters");
	for (int i=0; i < kNumberCounters; i++)
	{
		fRadioControl[i] = new BRadioButton(kRadioButtons[i].position, nullptr, GetText(kRadioButtons[i].text), new BMessage(kRadioButtons[i].message));
		mEffectView->AddChild(fRadioControl[i]);
	}
	//	Disable date for now
	fRadioControl[kCounterDate]->SetEnabled(false);

	//	Thresholds
	fSliderThreshold[0] = new BChannelSlider(BRect(20, 140, 360, 180), "threshold", GetText(TXT_EFFECTS_TEXT_COUNTER_LEFT_DELAY), new BMessage(eMsgThresholdLeft));
	fSliderThreshold[0]->SetValue(10);
	mEffectView->AddChild(fSliderThreshold[0]);
	fSliderThreshold[1] = new BChannelSlider(BRect(20, 180, 360, 220), "threshold", GetText(TXT_EFFECTS_TEXT_COUNTER_RIGHT_DELAY), new BMessage(eMsgThresholdRight));
	fSliderThreshold[1]->SetValue(90);
	mEffectView->AddChild(fSliderThreshold[1]);

	fTextFormat = new BTextControl(BRect(380, 170, 620, 200), "format", GetText(TXT_EFFECTS_TEXT_COUNTER_FORMAT), kRadioButtons[0].format, new BMessage(eMsgControlStart));
	//printf("fTextFormat->Divider() = %f\n", fTextFormat->Divider());
	fTextFormat->SetDivider(1.25f*be_plain_font->StringWidth(GetText(TXT_EFFECTS_TEXT_COUNTER_FORMAT)));
	mEffectView->AddChild(fTextFormat);

	//	Default value
	fCounterType = kCounterCurrency;
	fTextFormat->SetText(kRadioButtons[fCounterType].format);
	fTextFormat->SetToolTip(kRadioButtons[fCounterType].tooltip);
	fRadioControl[fCounterType]->SetValue(1);
}

/*	FUNCTION:		Effect_TextCounter :: ~Effect_TextCounter
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_TextCounter :: ~Effect_TextCounter()
{
	delete fTextView;
}

/*	FUNCTION:		Effect_TextCounter :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_TextCounter :: AttachedToWindow()
{
	Effect_Text::AttachedToWindow();
	fTextRange[0]->SetTarget(this, Window());
	fTextRange[1]->SetTarget(this, Window());

	for (int i=0; i < kNumberCounters; i++)
	{
		fRadioControl[i]->SetTarget(this, Window());
	}
	fTextFormat->SetTarget(this, Window());
	fSliderThreshold[0]->SetTarget(this, Window());
	fSliderThreshold[1]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_TextCounter :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_TextCounter :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_TextCounter.png");
	return icon;	
}

/*	FUNCTION:		Effect_TextCounter :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_TextCounter :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_COUNTER);
}

/*	FUNCTION:		Effect_TextCounter :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_TextCounter :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_COUNTER_TEXT_A);
}

/*	FUNCTION:		Effect_TextCounter :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_TextCounter :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_COUNTER_TEXT_B);
}

/*	FUNCTION:		Effect_TextCounter :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_TextCounter :: CreateMediaEffect()
{
	TextCounterMediaEffect *effect = new TextCounterMediaEffect;
	InitMediaEffect(effect);

	EffectTextCounterData *counter_data = new EffectTextCounterData;
	counter_data->start_amount = atol(fTextRange[0]->Text());
	counter_data->end_amount = atol(fTextRange[1]->Text());
	counter_data->counter_type = fCounterType;
	counter_data->left_delay = fSliderThreshold[0]->Value();
	counter_data->right_delay = fSliderThreshold[1]->Value();
	counter_data->format.SetTo(fTextFormat->Text());

	((EffectTextData *)effect->mEffectData)->derived_data = counter_data;

	return effect;
}

/*	FUNCTION:		Effect_TextCounter :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_TextCounter :: RenderEffect(BBitmap *source, MediaEffect *media_effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	EffectTextData *effect_data = (EffectTextData *) media_effect->mEffectData;
	EffectTextCounterData *counter_data = (EffectTextCounterData *)effect_data->derived_data;

	const float t1 = counter_data->left_delay/100.0f;
	const float t2 = counter_data->right_delay/100.0f;
	float t = float(frame_idx - media_effect->mTimelineFrameStart)/float(media_effect->Duration());
	if (t < t1)
		t = 0.0f;
	else if (t > t2)
		t = 1.0f;
	else
	{
		t = float(frame_idx - (media_effect->mTimelineFrameStart + t1*media_effect->Duration())) / float((t2-t1)*media_effect->Duration());
	}
	switch (counter_data->counter_type)
	{
		case kCounterCurrency:			GenerateText_Currency(t, effect_data);			break;
		case kCounterNumber:			GenerateText_Number(t, effect_data);			break;
		case kCounterTimeMinSec:		GenerateText_TimeMinSec(t, effect_data);		break;
		case kCounterTimeHourMinSec:	GenerateText_TimeHourMinSec(t, effect_data);	break;
		case kCounterDate:				GenerateText_Date(t, effect_data);				break;
		default:	assert(0);
	}

	Effect_Text::RenderEffect(source, media_effect, frame_idx, chained_effects);
}

/*	FUNCTION:		Effect_TextCounter :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_TextCounter :: MediaEffectSelected(MediaEffect *effect)
{
	Effect_Text::MediaEffectSelected(effect);

	EffectTextData *effect_data = (EffectTextData *) effect->mEffectData;
	EffectTextCounterData *counter_data = (EffectTextCounterData *)effect_data->derived_data;

	char buffer[0x40];
	sprintf(buffer, "%ld", counter_data->start_amount);	fTextRange[0]->SetText(buffer);
	sprintf(buffer, "%ld", counter_data->end_amount);	fTextRange[1]->SetText(buffer);
	fSliderThreshold[0]->SetValue(counter_data->left_delay);
	fSliderThreshold[1]->SetValue(counter_data->right_delay);

	fCounterType = (kCounterType) counter_data->counter_type;
	fRadioControl[fCounterType]->SetValue(1);
	fTextFormat->SetText(kRadioButtons[fCounterType].format);
	fTextFormat->SetToolTip(kRadioButtons[fCounterType].tooltip);
}

/*	FUNCTION:		Effect_TextCounter :: MessageReceived
	ARGS:			msg
	RETURN:			true if processed
	DESCRIPTION:	Process view messages
*/
void Effect_TextCounter :: MessageReceived(BMessage *msg)
{
	EffectTextData *text_data = nullptr;
	EffectTextCounterData *counter_data = nullptr;
	if (GetCurrentMediaEffect())
	{
		text_data = (EffectTextData *)GetCurrentMediaEffect()->mEffectData;
		if (text_data)
			counter_data = (EffectTextCounterData *) text_data->derived_data;
	}

	bool update = false;
	switch (msg->what)
	{
		case eMsgControlStart:
			if (counter_data)
				counter_data->start_amount = atol(fTextRange[0]->Text());
			InvalidatePreview();
			break;
		case eMsgControlEnd:
			if (counter_data)
				counter_data->end_amount = atol(fTextRange[1]->Text());
			InvalidatePreview();
			break;
		case eMsgThresholdLeft:
			if (counter_data)
				counter_data->left_delay = fSliderThreshold[0]->Value();
			InvalidatePreview();
			break;
		case eMsgThresholdRight:
			if (counter_data)
				counter_data->right_delay = fSliderThreshold[1]->Value();
			InvalidatePreview();
			break;

		case eMsgRadioCurrency:			fCounterType = kCounterCurrency;		update = true;		break;
		case eMsgRadioNumber:			fCounterType = kCounterNumber;			update = true;		break;
		case eMsgRadioTimeMinSec:		fCounterType = kCounterTimeMinSec;		update = true;		break;
		case eMsgRadioTimeHourMinSec:	fCounterType = kCounterTimeHourMinSec;	update = true;		break;
		case eMsgRadioDate:				fCounterType = kCounterDate;			update = true;		break;

		default:
			Effect_Text::MessageReceived(msg);
			break;
	}

	if (update)
	{
		fTextFormat->SetText(kRadioButtons[fCounterType].format);
		fTextFormat->SetToolTip(kRadioButtons[fCounterType].tooltip);

		if (counter_data)
		{
			counter_data->counter_type = fCounterType;
			counter_data->format.SetTo(fTextFormat->Text());
		}
		InvalidatePreview();
	}
}

/*	FUNCTION:		Effect_TextCounter :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_TextCounter :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = Effect_Text::LoadParameters(v, media_effect);

	EffectTextData *effect_data = (EffectTextData *) media_effect->mEffectData;
	EffectTextCounterData *counter_data = (EffectTextCounterData *)effect_data->derived_data;

	//	start_amount
	if (v.HasMember("start_amount") && v["start_amount"].IsInt())
	{
		counter_data->start_amount = v["start_amount"].GetInt();
	}
	else
	{
		printf("Effect_TextCounter :: LoadParameters() - missing parameter \"start_amount\"\n");
		valid = false;
	}

	//	end_amount
	if (v.HasMember("end_amount") && v["end_amount"].IsInt())
	{
		counter_data->end_amount = v["end_amount"].GetInt();
	}
	else
	{
		printf("Effect_TextCounter :: LoadParameters() - missing parameter \"end_amount\"\n");
		valid = false;
	}

	//	left_delay
	if (v.HasMember("left_delay") && v["left_delay"].IsUint())
	{
		counter_data->left_delay = v["left_delay"].GetUint();
		if (counter_data->left_delay < 0)
			counter_data->left_delay = 0;
		if (counter_data->right_delay > 100)
			counter_data->right_delay = 100;
	}
	else
	{
		printf("Effect_TextCounter :: LoadParameters() - missing parameter \"left_delay\"\n");
		valid = false;
	}

	//	right_delay
	if (v.HasMember("right_delay") && v["right_delay"].IsUint())
	{
		counter_data->right_delay = v["right_delay"].GetUint();
		if (counter_data->right_delay < 0)
			counter_data->right_delay = 0;
		if (counter_data->right_delay > 100)
			counter_data->right_delay = 100;
	}
	else
	{
		printf("Effect_TextCounter :: LoadParameters() - missing parameter \"right_delay\"\n");
		valid = false;
	}

	//	counter_type
	if (v.HasMember("counter_type") && v["counter_type"].IsUint())
	{
		counter_data->counter_type = v["counter_type"].GetUint();
		if (counter_data->counter_type < 0)
			counter_data->counter_type = 0;
		if (counter_data->counter_type >= kNumberCounters)
			counter_data->counter_type = kNumberCounters - 1;
	}
	else
	{
		printf("Effect_TextCounter :: LoadParameters() - missing parameter \"counter_type\"\n");
		valid = false;
	}

	//	format
	if (v.HasMember("format") && v["format"].IsString())
	{
		std::string aString;
		aString.assign(v["format"].GetString());
		printf("v[format]=%s\n", aString.c_str());
		counter_data->format.SetTo(aString.c_str());
	}
	else
	{
		printf("Effect_TextCounter :: LoadParameters() - missing parameter \"format\"\n");
		valid = false;
	}

	return valid;
}

/*	FUNCTION:		Effect_TextCounter :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_TextCounter :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	bool valid = Effect_Text::SaveParametersBase(file, media_effect, true);

	EffectTextData *effect_data = (EffectTextData *) media_effect->mEffectData;
	EffectTextCounterData *counter_data = (EffectTextCounterData *)effect_data->derived_data;
	if (counter_data)
	{
		char buffer[0x200];	//	512 bytes
		sprintf(buffer, "\t\t\t\t\"start_amount\": %ld,\n", counter_data->start_amount);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"end_amount\": %ld,\n", counter_data->end_amount);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"left_delay\": %u,\n", counter_data->left_delay);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"right_delay\": %u,\n", counter_data->right_delay);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"counter_type\": %u,\n", counter_data->counter_type);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"format\": \"%s\"\n", counter_data->format.String());
		fwrite(buffer, strlen(buffer), 1, file);
	}
	return valid;
}

/*	FUNCTION:		Effect_TextCounter :: GenerateText_Currency
	ARGS:			t
	RETURN:			n/a
	DESCRIPTION:	Generate currency text
*/
void Effect_TextCounter :: GenerateText_Currency(const float t, Effect_Text::EffectTextData *data)
{
	assert(data);
	EffectTextCounterData *counter_data = (EffectTextCounterData *)data->derived_data;
	int64 value = counter_data->start_amount + t*(counter_data->end_amount - counter_data->start_amount);

	char buffer[0x40];
	sprintf(buffer, counter_data->format, value/100, value%100);
	data->text.SetTo(buffer);
}

/*	FUNCTION:		Effect_TextCounter :: GenerateText_Number
	ARGS:			t
	RETURN:			n/a
	DESCRIPTION:	Generate number text
*/
void Effect_TextCounter :: GenerateText_Number(const float t, Effect_Text::EffectTextData *data)
{
	assert(data);
	EffectTextCounterData *counter_data = (EffectTextCounterData *)data->derived_data;
	int64 value = counter_data->start_amount + t*(counter_data->end_amount - counter_data->start_amount);

	char buffer[0x40];
	sprintf(buffer, counter_data->format, value);
	data->text.SetTo(buffer);
}

/*	FUNCTION:		Effect_TextCounter :: GenerateText_TimeMinSec
	ARGS:			t
	RETURN:			n/a
	DESCRIPTION:	Generate time text
*/
void Effect_TextCounter :: GenerateText_TimeMinSec(const float t, Effect_Text::EffectTextData *data)
{
	assert(data);
	EffectTextCounterData *counter_data = (EffectTextCounterData *)data->derived_data;
	int64 value = counter_data->start_amount + t*(counter_data->end_amount - counter_data->start_amount);

	char buffer[0x40];
	sprintf(buffer, counter_data->format, value/60, value%60);
	data->text.SetTo(buffer);
}

/*	FUNCTION:		Effect_TextCounter :: GenerateText_TimeHourMinSec
	ARGS:			t
	RETURN:			n/a
	DESCRIPTION:	Generate time text
*/
void Effect_TextCounter :: GenerateText_TimeHourMinSec(const float t, Effect_Text::EffectTextData *data)
{
	assert(data);
	EffectTextCounterData *counter_data = (EffectTextCounterData *)data->derived_data;
	int64 value = counter_data->start_amount + t*(counter_data->end_amount - counter_data->start_amount);

	char buffer[0x40];
	sprintf(buffer, counter_data->format, value/3600, (value - 3600*(value/3600))/60, value%60);
	data->text.SetTo(buffer);
}

/*	FUNCTION:		Effect_TextCounter :: GenerateText_Date
	ARGS:			t
	RETURN:			n/a
	DESCRIPTION:	Generate date text
*/
void Effect_TextCounter :: GenerateText_Date(const float t, Effect_Text::EffectTextData *data)
{
	printf("Effect_TextCounter :: GenerateText_Date() Not implemented\n");
}
