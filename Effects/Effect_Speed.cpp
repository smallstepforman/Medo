/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Speed
 */

#include <cstdio>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Gui/ValueSlider.h"
#include "Editor/Language.h"
#include "Editor/Project.h"

#include "Effect_Speed.h"

const char * Effect_Speed :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Speed :: GetEffectName() const	{return "Speed";}

static constexpr uint32 kMsgSpeedSlider		= 'esd0';
static constexpr float	kSpeedLimits[2] = {-500.0f, 500.0f};

struct EffectSpeedData
{
	float	speed;
};

/*	FUNCTION:		Effect_Speed :: Effect_Speed
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Speed :: Effect_Speed(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fSpeedSlider = new ValueSlider(BRect(10, 20, 510, 80), "speed_slider", GetText(TXT_EFFECTS_SPEED), nullptr, kSpeedLimits[0], kSpeedLimits[1]);
	fSpeedSlider->SetModificationMessage(new BMessage(kMsgSpeedSlider));
	fSpeedSlider->SetValue(100);
	fSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTH);
	fSpeedSlider->SetHashMarkCount(11);
	fSpeedSlider->SetLimitLabels("-5.0", "5.0");
	fSpeedSlider->UpdateTextValue(1.0f);
	fSpeedSlider->SetStyle(B_BLOCK_THUMB);
	fSpeedSlider->SetMidpointLabel("0.0");
	fSpeedSlider->SetFloatingPointPrecision(2);
	mEffectView->AddChild(fSpeedSlider);
}

/*	FUNCTION:		Effect_Speed :: ~Effect_Speed
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Speed :: ~Effect_Speed()
{ }

/*	FUNCTION:		Effect_Speed :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Speed :: AttachedToWindow()
{
	fSpeedSlider->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Speed :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Speed :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Speed.png");
	return icon;	
}

/*	FUNCTION:		Effect_Speed :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Speed :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_SPEED);
}

/*	FUNCTION:		Effect_Speed :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Speed :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_SPEED_TEXT_A);
}

/*	FUNCTION:		Effect_Speed :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Speed :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_SPEED_TEXT_B);
}

/*	FUNCTION:		Effect_Speed :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Speed :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectSpeedData *data = new EffectSpeedData;
	data->speed = 1.0f;
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_Speed :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Speed :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
	EffectSpeedData *data = (EffectSpeedData *) effect->mEffectData;
	fSpeedSlider->SetValue(data->speed*100);
	fSpeedSlider->UpdateTextValue(fSpeedSlider->Value()/100.0f);
}

/*	FUNCTION:		Effect_Speed :: GetSpeedTime
	ARGS:			frame_idx
					effect
	RETURN:			modified timeline
	DESCRIPTION:	Modify frame_idx based on speed
*/
int64 Effect_Speed :: GetSpeedTime(const int64 frame_idx, MediaEffect *effect)
{
	assert(effect && effect->mEffectData);
	EffectSpeedData *data = (EffectSpeedData *)effect->mEffectData;

	int64 delta = frame_idx - effect->mTimelineFrameStart;
	int64 modified_frame_idx = effect->mTimelineFrameStart +data->speed*delta;
	return modified_frame_idx;
}

/*	FUNCTION:		Effect_Speed :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Speed :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgSpeedSlider:
			fSpeedSlider->UpdateTextValue(fSpeedSlider->Value()/100.0f);
			if (GetCurrentMediaEffect())
			{
				EffectSpeedData *data = (EffectSpeedData *)GetCurrentMediaEffect()->mEffectData;
				assert(data);
				data->speed = fSpeedSlider->Value()/100.0f;
				InvalidatePreview();
			}
			else
				printf("Effect_Speed::MessageReceived(kMsgSpeedSlider) - no selected effect\n");
			break;

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_Speed :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Speed :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	//	direction
	if (v.HasMember("speed") && v["speed"].IsFloat())
	{
		EffectSpeedData *data = (EffectSpeedData *) media_effect->mEffectData;
		data->speed = v["speed"].GetFloat();
		if (data->speed < kSpeedLimits[0])
			data->speed = kSpeedLimits[0];
		if (data->speed > kSpeedLimits[1])
			data->speed = kSpeedLimits[1];
	}
	
	return true;
}

/*	FUNCTION:		Effect_Speed :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Speed :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectSpeedData *data = (EffectSpeedData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"speed\": %0.2f\n", data->speed);
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
