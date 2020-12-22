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
#include "Effect_Text_Terminal.h"

enum kGuiMessages
{
	eMsgAlignLeft,
	eMsgAlignCenter,
	eMsgAlignRight,
	eMsgThresholdLeft,
	eMsgThresholdRight,
};

struct RADIO_BUTTON
{
	BRect			position;
	LANGUAGE_TEXT	text;
	kGuiMessages	message;
};
static const RADIO_BUTTON kAlignmentButtons[] =
{
	{BRect(520, 130, 620, 160),		TXT_EFFECTS_COMMON_LEFT,		eMsgAlignLeft},
	{BRect(520, 160, 620, 190),		TXT_EFFECTS_COMMON_CENTER,		eMsgAlignCenter},
	{BRect(520, 190, 620, 220),		TXT_EFFECTS_COMMON_RIGHT,		eMsgAlignRight},
};

struct EffectTextTerminalData
{
	int				left_delay;
	int				right_delay;
	int				alignment;
	BString			text;
};

//	TextCounterMediaEffect allows customising destructor
class TextTerminalMediaEffect : public ImageMediaEffect
{
public:
	TextTerminalMediaEffect()  : ImageMediaEffect() { }
	~TextTerminalMediaEffect()
	{
		assert(mEffectNode);
		assert(mEffectData);

		Effect_TextTerminal *effect_node = (Effect_TextTerminal *)mEffectNode;
		Effect_Text::EffectTextData *data = (Effect_Text::EffectTextData *)mEffectData;
		delete (EffectTextTerminalData *) data->derived_data;
		delete data;
	}
};

const char * Effect_TextTerminal :: GetVendorName() const	{return "ZenYes";}
const char * Effect_TextTerminal :: GetEffectName() const	{return "Teletype";}

using namespace yrender;

/*	FUNCTION:		Effect_TextTerminal :: Effect_TextTerminal
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_TextTerminal :: Effect_TextTerminal(BRect frame, const char *filename)
	: Effect_Text(frame, filename)
{
	const float kFontFactor = be_plain_font->Size()/20.0f;

	fTextView->ResizeTo(frame.Width()-20, 100);

	static_assert(sizeof(kAlignmentButtons)/sizeof(RADIO_BUTTON) == kNumberAlignmentButtons, "sizeof(kAlignmentButtons) != kNumberAlignmentButtons");
	for (int i=0; i < kNumberAlignmentButtons; i++)
	{
	BRect button_position(kAlignmentButtons[i].position.left*kFontFactor, kAlignmentButtons[i].position.top,
						  kAlignmentButtons[i].position.right*kFontFactor, kAlignmentButtons[i].position.bottom);
		fAlignmentRadioButtons[i] = new BRadioButton(button_position, nullptr, GetText(kAlignmentButtons[i].text), new BMessage(kAlignmentButtons[i].message));
		mEffectView->AddChild(fAlignmentRadioButtons[i]);
	}
	fAlignment = kAlignmentCenter;
	fAlignmentRadioButtons[fAlignment]->SetValue(1);

	//	Thresholds
	fSliderThreshold[0] = new BChannelSlider(BRect(20*kFontFactor, 140, 360*kFontFactor, 180), "threshold", GetText(TXT_EFFECTS_TEXT_TELETYPE_LEFT_DELAY), new BMessage(eMsgThresholdLeft));
	fSliderThreshold[0]->SetValue(10);
	mEffectView->AddChild(fSliderThreshold[0]);
	fSliderThreshold[1] = new BChannelSlider(BRect(20*kFontFactor, 180, 360*kFontFactor, 220), "threshold", GetText(TXT_EFFECTS_TEXT_TELETYPE_RIGHT_DELAY), new BMessage(eMsgThresholdRight));
	fSliderThreshold[1]->SetValue(90);
	mEffectView->AddChild(fSliderThreshold[1]);
}

/*	FUNCTION:		Effect_TextTerminal :: ~Effect_TextTerminal
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_TextTerminal :: ~Effect_TextTerminal()
{
}

/*	FUNCTION:		Effect_TextTerminal :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_TextTerminal :: AttachedToWindow()
{
	Effect_Text::AttachedToWindow();
	for (int i=0; i < kNumberAlignmentButtons; i++)
		fAlignmentRadioButtons[i]->SetTarget(this, Window());
	fSliderThreshold[0]->SetTarget(this, Window());
	fSliderThreshold[1]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_TextTerminal :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_TextTerminal :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_TextTerminal.png");
	return icon;	
}

/*	FUNCTION:		Effect_TextTerminal :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_TextTerminal :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_TELETYPE);
}

/*	FUNCTION:		Effect_TextTerminal :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_TextTerminal :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_TELETYPE_TEXT_A);
}

/*	FUNCTION:		Effect_TextTerminal :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_TextTerminal :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_TELETYPE_TEXT_B);
}

/*	FUNCTION:		Effect_TextTerminal :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_TextTerminal :: CreateMediaEffect()
{
	TextTerminalMediaEffect *effect = new TextTerminalMediaEffect;
	InitMediaEffect(effect);

	EffectTextTerminalData *terminal_data = new EffectTextTerminalData;
	terminal_data->left_delay = fSliderThreshold[0]->Value();
	terminal_data->right_delay = fSliderThreshold[1]->Value();
	terminal_data->alignment = fAlignment;
	terminal_data->text.SetTo(fTextView->Text());

	((EffectTextData *)effect->mEffectData)->derived_data = terminal_data;

	return effect;
}

/*	FUNCTION:		Effect_TextTerminal :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_TextTerminal :: RenderEffect(BBitmap *source, MediaEffect *media_effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	if (!media_effect || !media_effect->mEffectData)
		return;

	EffectTextData *data = (EffectTextData *) media_effect->mEffectData;
	EffectTextTerminalData *terminal_data = (EffectTextTerminalData *)data->derived_data;

	const float t1 = terminal_data->left_delay/100.0f;
	const float t2 = terminal_data->right_delay/100.0f;
	float t = float(frame_idx - media_effect->mTimelineFrameStart)/float(media_effect->Duration());
	if (t < t1)
		t = 0.0f;
	else if (t > t2)
		t = 1.0f;
	else
	{
		t = float(frame_idx - (media_effect->mTimelineFrameStart + t1*media_effect->Duration())) / float((t2-t1)*media_effect->Duration());
	}

	BString tr = terminal_data->text;
	tr.Truncate(t * tr.Length());
	data->text.SetTo(tr);

	if (fTextScene == nullptr)
		CreateOpenGLObjects(data);
	switch (terminal_data->alignment)
	{
		case kAlignmentLeft:		fTextScene->SetHorizontalAlignment(YTextScene::ALIGN_LEFT);			break;
		case kAlignmentCenter:		fTextScene->SetHorizontalAlignment(YTextScene::ALIGN_HCENTER);		break;
		case kAlignmentRight:		fTextScene->SetHorizontalAlignment(YTextScene::ALIGN_RIGHT);		break;
		default:					assert(0);
	}
	Effect_Text::RenderEffect(source, media_effect, frame_idx, chained_effects);

	//	Restore alignment
	fTextScene->SetHorizontalAlignment(YTextScene::ALIGN_HCENTER);
}

/*	FUNCTION:		Effect_TextTerminal :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_TextTerminal :: MediaEffectSelected(MediaEffect *effect)
{
	Effect_Text::MediaEffectSelected(effect);
	EffectTextData *effect_data = (EffectTextData *) effect->mEffectData;
	EffectTextTerminalData *terminal_data = (EffectTextTerminalData *)effect_data->derived_data;
	char buffer[0x40];
	fSliderThreshold[0]->SetValue(terminal_data->left_delay);
	fSliderThreshold[1]->SetValue(terminal_data->right_delay);
	fTextView->SetText(terminal_data->text.String());

	fAlignment = (kAlignment) terminal_data->alignment;
	fAlignmentRadioButtons[fAlignment]->SetValue(1);
}

/*	FUNCTION:		Effect_TextTerminal :: MessageReceived
	ARGS:			msg
	RETURN:			true if processed
	DESCRIPTION:	Process view messages
*/
void Effect_TextTerminal :: MessageReceived(BMessage *msg)
{
	EffectTextData *text_data = nullptr;
	EffectTextTerminalData *terminal_data = nullptr;
	if (GetCurrentMediaEffect())
	{
		text_data = (EffectTextData *)GetCurrentMediaEffect()->mEffectData;
		if (text_data)
			terminal_data = (EffectTextTerminalData *) text_data->derived_data;
	}

	switch (msg->what)
	{
		case eMsgAlignLeft:			fAlignment = kAlignmentLeft;		break;
		case eMsgAlignCenter:		fAlignment = kAlignmentCenter;		break;
		case eMsgAlignRight:		fAlignment = kAlignmentRight;		break;

		case eMsgThresholdLeft:
			if (terminal_data)
				terminal_data->left_delay = fSliderThreshold[0]->Value();
			break;
		case eMsgThresholdRight:
			if (terminal_data)
				terminal_data->right_delay = fSliderThreshold[1]->Value();
			break;

		default:
			Effect_Text::MessageReceived(msg);
			return;
	}

	if (terminal_data)
		terminal_data->alignment = fAlignment;

	InvalidatePreview();
}

/*	FUNCTION:		Effect_TextTerminal :: TextUpdated
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when fTextView updated
*/
void Effect_TextTerminal :: TextUpdated()
{
	Effect_Text::TextUpdated();

	if (GetCurrentMediaEffect())
	{
		EffectTextData *text_data = (EffectTextData *)GetCurrentMediaEffect()->mEffectData;
		if (text_data)
		{
			EffectTextTerminalData *terminal_data = (EffectTextTerminalData *) text_data->derived_data;
			terminal_data->text.SetTo(fTextView->Text());
		}
	}
}

/*	FUNCTION:		Effect_TextTerminal :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_TextTerminal :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = Effect_Text::LoadParameters(v, media_effect);

	EffectTextData *effect_data = (EffectTextData *) media_effect->mEffectData;
	EffectTextTerminalData *terminal_data = (EffectTextTerminalData *)effect_data->derived_data;

	//	left_delay
	if (v.HasMember("left_delay") && v["left_delay"].IsUint())
	{
		terminal_data->left_delay = v["left_delay"].GetUint();
		if (terminal_data->left_delay < 0)
			terminal_data->left_delay = 0;
		if (terminal_data->right_delay > 100)
			terminal_data->right_delay = 100;
	}
	else
	{
		printf("Effect_TextTerminal :: LoadParameters() - missing parameter \"left_delay\"\n");
		valid = false;
	}

	//	right_delay
	if (v.HasMember("right_delay") && v["right_delay"].IsUint())
	{
		terminal_data->right_delay = v["right_delay"].GetUint();
		if (terminal_data->right_delay < 0)
			terminal_data->right_delay = 0;
		if (terminal_data->right_delay > 100)
			terminal_data->right_delay = 100;
	}
	else
	{
		printf("Effect_TextTerminal :: LoadParameters() - missing parameter \"right_delay\"\n");
		valid = false;
	}

	//	alignment
	if (v.HasMember("alignment") && v["alignment"].IsUint())
	{
		terminal_data->alignment = v["alignment"].GetUint();
		if (terminal_data->alignment < 0)
			terminal_data->alignment = 0;
		if (terminal_data->alignment >= kNumberAlignmentButtons)
			terminal_data->alignment = kNumberAlignmentButtons - 1;
	}
	else
	{
		printf("Effect_TextTerminal :: LoadParameters() - missing parameter \"alignment\"\n");
		valid = false;
	}

	terminal_data->text = effect_data->text;

	return valid;
}

/*	FUNCTION:		Effect_TextTerminal :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_TextTerminal :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectTextData *effect_data = (EffectTextData *) media_effect->mEffectData;
	EffectTextTerminalData *terminal_data = (EffectTextTerminalData *)effect_data->derived_data;
	if (terminal_data)
		effect_data->text = terminal_data->text;

	bool valid = Effect_Text::SaveParametersBase(file, media_effect, true);
	if (terminal_data)
	{
		char buffer[0x200];	//	512 bytes
		sprintf(buffer, "\t\t\t\t\"left_delay\": %u,\n", terminal_data->left_delay);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"right_delay\": %u,\n", terminal_data->right_delay);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"alignment\": %u\n", terminal_data->alignment);
		fwrite(buffer, strlen(buffer), 1, file);
	}
	return valid;
}


