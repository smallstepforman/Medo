/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Transform
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <translation/TranslationUtils.h>
#include <LayoutBuilder.h>

#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Picture.h"
#include "Yarra/Render/MatrixStack.h"
#include "Yarra/Math/Interpolation.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/Project.h"
#include "Editor/RenderActor.h"

#include "Gui/BitmapButton.h"
#include "Gui/Spinner.h"

#include "Effect_Transform.h"

const char * Effect_Transform :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Transform :: GetEffectName() const	{return "Transform";}

enum kTransformMessages
{
	kMsgStartPosX		= 'edt0',
	kMsgStartPosY,
	kMsgStartPosZ,
	kMsgEndPosX,
	kMsgEndPosY,
	kMsgEndPosZ,

	kMsgStartRotX,
	kMsgStartRotY,
	kMsgStartRotZ,
	kMsgEndRotX,
	kMsgEndRotY,
	kMsgEndRotZ,

	kMsgStartScaleX,
	kMsgStartScaleY,
	kMsgStartScaleZ,
	kMsgEndScaleX,
	kMsgEndScaleY,
	kMsgEndScaleZ,

	kMsgInterpolate,
	kMsgIncrement,
	kMsgInterpolationType,
};

struct SPINNER_LAYOUT
{
	const BRect		rect;
	const char		*id;
	LANGUAGE_TEXT	text;
	const char		*label;
	const float		min_value;
	const float		max_value;
	const float		default_value;
	const uint32	message;
};
enum SPINNERS {
	SPINNER_POSITION_X, SPINNER_POSITION_Y, SPINNER_POSITION_Z,
	SPINNER_ROTATION_X, SPINNER_ROTATION_Y, SPINNER_ROTATION_Z,
	SPINNER_SCALE_X, SPINNER_SCALE_Y, SPINNER_SCALE_Z,
	NUMBER_SPINNERS,
};
static const SPINNER_LAYOUT kStartSpinnerLayouts[] = 
{
{BRect(10, 30, 200, 60),		"spos_x",		TXT_EFFECTS_COMMON_POSITION,	" X",	-10000,		10000,	0.5f,	kMsgStartPosX},
{BRect(10, 70, 200, 100),		"spos_y",		TXT_EFFECTS_COMMON_POSITION,	" Y",	-10000,		10000,	0.5f,	kMsgStartPosY},
{BRect(10, 110, 200, 140),		"spos_z",		TXT_EFFECTS_COMMON_POSITION,	" Z",	-10000,		10000,	0.5f,	kMsgStartPosZ},
{BRect(230, 30, 410, 60),		"srot_x",		TXT_EFFECTS_COMMON_ROTATION,	" X",	-10000,		10000,	0.0f,	kMsgStartRotX},
{BRect(230, 70, 410, 100),		"srot_y",		TXT_EFFECTS_COMMON_ROTATION,	" Y",	-10000,		10000,	0.0f,	kMsgStartRotY},
{BRect(230, 110, 410, 140),		"srot_z",		TXT_EFFECTS_COMMON_ROTATION,	" Z",	-10000,		10000,	0.0f,	kMsgStartRotZ},
{BRect(440, 30, 620, 60),		"sscale_x",		TXT_EFFECTS_COMMON_SCALE,		" X",		-10000,		10000,	1.0f,	kMsgStartScaleX},
{BRect(440, 70, 620, 100),		"sscale_y",		TXT_EFFECTS_COMMON_SCALE,		" Y",		-10000,		10000,	1.0f,	kMsgStartScaleY},
{BRect(440, 110, 620, 140),		"sscale_z",		TXT_EFFECTS_COMMON_SCALE,		" Z",		-10000,		10000,	1.0f,	kMsgStartScaleZ},
};
static const SPINNER_LAYOUT kEndSpinnerLayouts[] = 	
{			
{BRect(10, 30, 200, 60),		"epos_x",		TXT_EFFECTS_COMMON_POSITION,	" X",	-10000,		10000,	0.5f,	kMsgEndPosX},
{BRect(10, 70, 200, 100),		"epos_y",		TXT_EFFECTS_COMMON_POSITION,	" Y",	-10000,		10000,	0.5f,	kMsgEndPosY},
{BRect(10, 110, 200, 140),		"epos_z",		TXT_EFFECTS_COMMON_POSITION,	" Z",	-10000,		10000,	0.5f,	kMsgEndPosZ},
{BRect(230, 30, 410, 60),		"erot_x",		TXT_EFFECTS_COMMON_ROTATION,	" X",	-10000,		10000,	0.0f,	kMsgEndRotX},
{BRect(230, 70, 410, 100),		"erot_y",		TXT_EFFECTS_COMMON_ROTATION,	" Y",	-10000,		10000,	0.0f,	kMsgEndRotY},
{BRect(230, 110, 410, 140),		"erot_z",		TXT_EFFECTS_COMMON_ROTATION,	" Z",	-10000,		10000,	0.0f,	kMsgEndRotZ},
{BRect(440, 30, 620, 60),		"escale_x",		TXT_EFFECTS_COMMON_SCALE,		" X",		-10000,		10000,	1.0f,	kMsgEndScaleX},
{BRect(440, 70, 620, 100),		"escale_y",		TXT_EFFECTS_COMMON_SCALE,		" Y",		-10000,		10000,	1.0f,	kMsgEndScaleY},
{BRect(440, 110, 620, 140),		"escale_z",		TXT_EFFECTS_COMMON_SCALE,		" Z",		-10000,		10000,	1.0f,	kMsgEndScaleZ},
};
static_assert(sizeof(kStartSpinnerLayouts)/sizeof(SPINNER_LAYOUT) == NUMBER_SPINNERS, "sizeof(kStartSpinnerLayouts) != NUMBER_SPINNERS");
static_assert(sizeof(kEndSpinnerLayouts)/sizeof(SPINNER_LAYOUT) == NUMBER_SPINNERS, "sizeof(kEndSpinnerLayouts) != NUMBER_SPINNERS");

static float kIncrementPopupValues[] = 
{
	0.001, 0.01f, 0.1f, 1.0f, 10.0f
};

static ymath::YVector3	kDefaultPosition(0.5f, 0.5f, 0);
static ymath::YVector3	kDefaultRotation(0, 0, 0);
static ymath::YVector3	kDefaultScale(1,1,1);

enum INTERPOLATION {kInterpolationLinear, kInterpolationCosine, kInterpolationAcceleration, kInterpolationDeceleration};
struct INTERPOLATION_TYPE
{
	INTERPOLATION		interpolation;
	const char			*text;
	LANGUAGE_TEXT		translated_text;
};
static const INTERPOLATION_TYPE kInterpolationType[] =
{
	{kInterpolationLinear,			"Linear",			TXT_EFFECTS_COMMON_INTERPOLATION_LINEAR},
	{kInterpolationCosine,			"Cosine",			TXT_EFFECTS_COMMON_INTERPOLATION_COSINE},
	{kInterpolationAcceleration,	"Acceleration",		TXT_EFFECTS_COMMON_INTERPOLATION_ACCELERATION},
	{kInterpolationDeceleration,	"Deceleration",		TXT_EFFECTS_COMMON_INTERPOLATION_DECELERATION},
};

//	Transform Data
struct EffectTransformData
{
	ymath::YVector3		start_position;
	ymath::YVector3		start_rotation;
	ymath::YVector3		start_scale;
	ymath::YVector3		end_position;
	ymath::YVector3		end_rotation;
	ymath::YVector3		end_scale;
	bool				interpolate;
	int					interpolation_type;
	
	void PrintToStream()
	{
		printf("start_position:");	start_position.PrintToStream();
		printf("start_rotation:");	start_rotation.PrintToStream();	
		printf("start_scale:");		start_scale.PrintToStream();
		printf("end_position:");	end_position.PrintToStream();	
		printf("end_rotation:");	end_rotation.PrintToStream();	
		printf("end_scale:");		end_scale.PrintToStream();
		printf("interpolate: %s\n",	interpolate ? "true" : "false");
		printf("interpolation type: %s\n", kInterpolationType[interpolation_type]);
	}			
};

using namespace yrender;


/*	FUNCTION:		Effect_Transform :: Effect_Transform
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Transform :: Effect_Transform(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	//	Increment Popup
	BOptionPopUp *increment_popup = new BOptionPopUp(BRect(440, 200, 680, 240), "increment", GetText(TXT_EFFECTS_COMMON_INCREMENT), new BMessage(kMsgIncrement));
	for (int i=0; i < sizeof(kIncrementPopupValues)/sizeof(float); i++)
	{
		char buffer[20];
		sprintf(buffer, "%2f", kIncrementPopupValues[i]);
		increment_popup->AddOption(buffer, i);
	}
	const int kIncrementPopupDefaultIndex = 2;
	increment_popup->SelectOptionFor(kIncrementPopupDefaultIndex);
	mEffectView->AddChild(increment_popup);

	//	Start transform
	BBox *start_box = new BBox(BRect(10, 10, 640, 170), "box_start");
	start_box->SetLabel(GetText(TXT_EFFECTS_COMMON_START));
	mEffectView->AddChild(start_box);
	for (int i=0; i < sizeof(kStartSpinnerLayouts)/sizeof(SPINNER_LAYOUT); i++)
	{
		char buffer[32];
		sprintf(buffer, "%s%s", GetText(kStartSpinnerLayouts[i].text), kStartSpinnerLayouts[i].label);
		Spinner *spinner = new Spinner(kStartSpinnerLayouts[i].rect, kStartSpinnerLayouts[i].id, buffer,
										new BMessage(kStartSpinnerLayouts[i].message));
		spinner->SetRange(kStartSpinnerLayouts[i].min_value, kStartSpinnerLayouts[i].max_value);
		spinner->SetValue(kStartSpinnerLayouts[i].default_value);
		spinner->SetSteps(kIncrementPopupValues[kIncrementPopupDefaultIndex]);
		start_box->AddChild(spinner);
	}
	//	Interpolate
	fCheckboxInterpolate = new BCheckBox(BRect(10, 250, 400, 290), "interpolate", GetText(TXT_EFFECTS_COMMON_INTERPOLATE), new BMessage(kMsgInterpolate));
	mEffectView->AddChild(fCheckboxInterpolate);

	//	End transform
	BBox *end_box = new BBox(BRect(10, 300, 640, 460), "box_end");
	end_box->SetLabel(GetText(TXT_EFFECTS_COMMON_END));
	mEffectView->AddChild(end_box);
	for (int i=0; i < sizeof(kEndSpinnerLayouts)/sizeof(SPINNER_LAYOUT); i++)
	{
		char buffer[32];
		sprintf(buffer, "%s%s", GetText(kEndSpinnerLayouts[i].text), kEndSpinnerLayouts[i].label);
		Spinner *spinner = new Spinner(kEndSpinnerLayouts[i].rect, kEndSpinnerLayouts[i].id, buffer,
										new BMessage(kEndSpinnerLayouts[i].message));
		spinner->SetRange(kEndSpinnerLayouts[i].min_value, kEndSpinnerLayouts[i].max_value);
		spinner->SetValue(kEndSpinnerLayouts[i].default_value);
		spinner->SetSteps(kIncrementPopupValues[kIncrementPopupDefaultIndex]);
		spinner->SetEnabled(false);
		end_box->AddChild(spinner);
	}
	
	//	Interpolation type
	BOptionPopUp *interpolation = new BOptionPopUp(BRect(10, 480, 400, 540), "interpolation_type", GetText(TXT_EFFECTS_COMMON_INTERPOLATION_TYPE), new BMessage(kMsgInterpolationType));
	for (int i=0; i < sizeof(kInterpolationType)/sizeof(INTERPOLATION_TYPE); i++)
	{
		interpolation->AddOption(GetText(kInterpolationType[i].translated_text), i);
	}
	interpolation->SetEnabled(false);
	mEffectView->AddChild(interpolation);
}

/*	FUNCTION:		Effect_Transform :: ~Effect_Transform
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Transform :: ~Effect_Transform()
{
}

/*	FUNCTION:		Effect_Transform :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_Transform :: AttachedToWindow()
{
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_Z].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_Z].id))->SetTarget(this, Window());

	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Z].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Z].id))->SetTarget(this, Window());

	dynamic_cast<BOptionPopUp *>(FindView("interpolation_type"))->SetTarget(this, Window());
	dynamic_cast<BOptionPopUp *>(FindView("increment"))->SetTarget(this, Window());
	fCheckboxInterpolate->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Transform :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Transform :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Transform.png");
	return icon;	
}

/*	FUNCTION:		Effect_Transform :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Transform :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TRANSFORM);
}

/*	FUNCTION:		Effect_Transform :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Transform :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TRANSFORM_TEXT_A);
}

/*	FUNCTION:		Effect_Transform :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Transform :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TRANSFORM_TEXT_B);
}

/*	FUNCTION:		Effect_Transform :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Transform :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;

	EffectTransformData *data = new EffectTransformData;
	media_effect->mEffectData = data;

	YVector3 start_position(dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_X].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_Y].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_Z].id))->Value());
	data->start_position = start_position;

	YVector3 start_rotation(dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_X].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_Y].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_Z].id))->Value());
	data->start_rotation = start_rotation;

	YVector3 start_scale(dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_X].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_Y].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_Z].id))->Value());
	data->start_scale = start_scale;

	YVector3 end_position(dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_X].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Y].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Z].id))->Value());
	data->end_position = end_position;

	YVector3 end_rotation(dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_X].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Y].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Z].id))->Value());
	data->end_rotation = end_rotation;

	YVector3 end_scale(dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_X].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Y].id))->Value(),
						dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Z].id))->Value());
	data->end_scale = end_scale;

	data->interpolation_type = dynamic_cast<BOptionPopUp *>(FindView("interpolation_type"))->Value();
	data->interpolate = fCheckboxInterpolate->Value() > 0;

	return media_effect;
}

/*	FUNCTION:		Effect_Transform :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Transform :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;
	
	//	Update GUI
	EffectTransformData *data = (EffectTransformData *) effect->mEffectData;
	
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_X].id))->SetValue(data->start_position.x);
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_Y].id))->SetValue(data->start_position.y);
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_Z].id))->SetValue(data->start_position.z);
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_X].id))->SetValue(data->start_rotation.x);
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetValue(data->start_rotation.y);
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetValue(data->start_rotation.z);
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_X].id))->SetValue(data->start_scale.x);
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_Y].id))->SetValue(data->start_scale.y);
	dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_Z].id))->SetValue(data->start_scale.z);
	
	
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_X].id))->SetValue(data->end_position.x);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Y].id))->SetValue(data->end_position.y);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Z].id))->SetValue(data->end_position.z);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_X].id))->SetValue(data->end_rotation.x);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetValue(data->end_rotation.y);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetValue(data->end_rotation.z);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_X].id))->SetValue(data->end_scale.x);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Y].id))->SetValue(data->end_scale.y);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Z].id))->SetValue(data->end_scale.z);

	printf("data->interpolation_type=%d\n", data->interpolation_type);
	dynamic_cast<BOptionPopUp *>(FindView("interpolation_type"))->SetValue(data->interpolation_type);
	fCheckboxInterpolate->SetValue(data->interpolate ? 1 : 0);

	//	Enable/Disable based on interpolation
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_X].id))->SetEnabled(data->interpolate);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Y].id))->SetEnabled(data->interpolate);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Z].id))->SetEnabled(data->interpolate);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_X].id))->SetEnabled(data->interpolate);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetEnabled(data->interpolate);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetEnabled(data->interpolate);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_X].id))->SetEnabled(data->interpolate);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Y].id))->SetEnabled(data->interpolate);
	dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Z].id))->SetEnabled(data->interpolate);
	dynamic_cast<BOptionPopUp *>(FindView("interpolation_type"))->SetEnabled(data->interpolate);
}

/*	FUNCTION:		Effect_Transform :: OutputViewMouseDown
	ARGS:			media_effect
					point
	RETURN:			n/a
	DESCRIPTION:	Hook function when OutputView mouse down
*/
void Effect_Transform :: OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)
{
	if (!media_effect)
		return;

	fOutputViewMouseDown = point;
	EffectTransformData *data = (EffectTransformData *)media_effect->mEffectData;
	fOutputViewTrackPosition = data->start_position;
}

/*	FUNCTION:		Effect_Transform :: OutputViewMouseMoved
	ARGS:			media_effect
					point
	RETURN:			n/a
	DESCRIPTION:	Hook function when OutputView mouse moved (with button down)
*/
void Effect_Transform :: OutputViewMouseMoved(MediaEffect *media_effect, const BPoint &point)\
{
	if (!media_effect)
		return;

	EffectTransformData *data = (EffectTransformData *)media_effect->mEffectData;
	data->start_position = fOutputViewTrackPosition + ymath::YVector3((point.x - fOutputViewMouseDown.x)/gProject->mResolution.width, (point.y - fOutputViewMouseDown.y)/gProject->mResolution.height, 0);

	dynamic_cast<Spinner *>(this->FindView(kStartSpinnerLayouts[SPINNER_POSITION_X].id))->SetValue(data->start_position.x);
	dynamic_cast<Spinner *>(this->FindView(kStartSpinnerLayouts[SPINNER_POSITION_Y].id))->SetValue(data->start_position.y);
	dynamic_cast<Spinner *>(this->FindView(kStartSpinnerLayouts[SPINNER_POSITION_Z].id))->SetValue(data->start_position.z);

	InvalidatePreview();
}

/*	FUNCTION:		Effect_Transform :: ChainedSpatialTransform
	ARGS:			data
					frame_idx
	RETURN:			n/a
	DESCRIPTION:	Chained spatial transform
*/
void Effect_Transform :: ChainedSpatialTransform(MediaEffect *effect, int64 frame_idx)
{
	EffectTransformData *transform = (EffectTransformData *) effect->mEffectData;

	float t = 0.0f;
	if (transform->interpolate)
	{
		t = float(frame_idx - effect->mTimelineFrameStart)/float(effect->Duration());
		if (t > 1.0f)
			t = 1.0f;
	}

	const ymath::YVector3 (*fn)(const ymath::YVector3 &start, const ymath::YVector3 &end, const float t);
	switch (transform->interpolation_type)
	{
		case kInterpolationLinear:			fn = ymath::YInterpolationLinear;		break;
		case kInterpolationCosine:			fn = ymath::YInterpolationCosine;		break;
		case kInterpolationAcceleration:	fn = ymath::YInterpolationAcceleration;		break;
		case kInterpolationDeceleration:	fn = ymath::YInterpolationDeceleration;		break;
		default:							assert(0);
	}

	YSpatial spatial;
	spatial.SetPosition(fn(transform->start_position, transform->end_position, t) * ymath::YVector3(gProject->mResolution.width, gProject->mResolution.height, 1));
	spatial.SetRotation(fn(transform->start_rotation, transform->end_rotation, t));
	spatial.SetScale(fn(transform->start_scale, transform->end_scale, t));

	spatial.Transform();
}

/*	FUNCTION:		Effect_Transform :: RenderEffect
	ARGS:			source
					data
					frame_idx
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Transform :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	unsigned int w = source->Bounds().IntegerWidth() + 1;
	unsigned int h = source->Bounds().IntegerHeight() + 1;
	yrender::YPicture *picture = gRenderActor->GetPicture(w, h, source);

	yMatrixStack.Push();
	ChainedSpatialTransform(data, frame_idx);
	picture->Render(0.0f);
	yMatrixStack.Pop();
}


/*	FUNCTION:		Effect_Transform :: MessageReceived
	ARGS:			msg
	RETURN:			none
	DESCRIPTION:	Process view messages
*/
void Effect_Transform :: MessageReceived(BMessage *msg)
{
	uint32 modify_mask = 0;
	switch (msg->what)
	{
		case kMsgStartPosX:		modify_mask |= (1 << 0);		break;
		case kMsgStartPosY:		modify_mask |= (1 << 1);		break;
		case kMsgStartPosZ:		modify_mask |= (1 << 2);		break;
		case kMsgStartRotX:		modify_mask |= (1 << 3);		break;
		case kMsgStartRotY:		modify_mask |= (1 << 4);		break;
		case kMsgStartRotZ:		modify_mask |= (1 << 5);		break;
		case kMsgStartScaleX:	modify_mask |= (1 << 6);		break;
		case kMsgStartScaleY:	modify_mask |= (1 << 7);		break;
		case kMsgStartScaleZ:	modify_mask |= (1 << 8);		break;
		
		case kMsgEndPosX:		modify_mask |= (1 << 9);		break;
		case kMsgEndPosY:		modify_mask |= (1 << 10);		break;
		case kMsgEndPosZ:		modify_mask |= (1 << 11);		break;
		case kMsgEndRotX:		modify_mask |= (1 << 12);		break;
		case kMsgEndRotY:		modify_mask |= (1 << 13);		break;
		case kMsgEndRotZ:		modify_mask |= (1 << 14);		break;
		case kMsgEndScaleX:		modify_mask |= (1 << 15);		break;
		case kMsgEndScaleY:		modify_mask |= (1 << 16);		break;
		case kMsgEndScaleZ:		modify_mask |= (1 << 17);		break;
		
		case kMsgIncrement:	
		{
			int32 value = 0;
			if (msg->FindInt32("be:value", &value) == B_OK)
			{
				for (int i=0; i < sizeof(kStartSpinnerLayouts)/sizeof(SPINNER_LAYOUT); i++)
				{
					BView *view = FindView(kStartSpinnerLayouts[i].id);
					Spinner *spinner = dynamic_cast<Spinner *>(view);
					if (spinner)
						spinner->SetSteps(kIncrementPopupValues[value]);
				}
				for (int i=0; i < sizeof(kEndSpinnerLayouts)/sizeof(SPINNER_LAYOUT); i++)
				{
					BView *view = FindView(kEndSpinnerLayouts[i].id);
					Spinner *spinner = dynamic_cast<Spinner *>(view);
					if (spinner)
						spinner->SetSteps(kIncrementPopupValues[value]);
				}
			}
			break;
		}
		case kMsgInterpolationType:
			modify_mask |= 0b1000'0000'0000'0000'0000;
			break;

		case kMsgInterpolate:
		{
			bool enabled = fCheckboxInterpolate->Value() > 0;
			dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_X].id))->SetEnabled(enabled);
			dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Y].id))->SetEnabled(enabled);
			dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Z].id))->SetEnabled(enabled);
			dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_X].id))->SetEnabled(enabled);
			dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetEnabled(enabled);
			dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetEnabled(enabled);
			dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_X].id))->SetEnabled(enabled);
			dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Y].id))->SetEnabled(enabled);
			dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Z].id))->SetEnabled(enabled);
			dynamic_cast<BOptionPopUp *>(FindView("interpolation_type"))->SetEnabled(enabled);
			modify_mask |= 0b0011'1111'1110'0000'0000;
			break;
		}
		
		default:
			EffectNode::MessageReceived(msg);
			break;
	}

	if (!GetCurrentMediaEffect())
		return;

	if (modify_mask == 0)
		return;

	EffectTransformData *transform = (EffectTransformData *) GetCurrentMediaEffect()->mEffectData;
	assert(transform != nullptr);

	//	Start position
	if (modify_mask & 0b0000'0000'0000'0000'0111)
	{
		transform->start_position.Set(dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_POSITION_Z].id))->Value());
	}
	//	Start rotation
	if (modify_mask & 0b0000'0000'0000'0011'1000)
	{
		transform->start_rotation.Set(dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_ROTATION_Z].id))->Value());
	}
	//	Start scale
	if (modify_mask & 0b0000'0000'0001'1100'0000)
	{
		transform->start_scale.Set(dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kStartSpinnerLayouts[SPINNER_SCALE_Z].id))->Value());
	}
	//	End position
	if (modify_mask & 0b0000'0000'1110'0000'0000)
	{
		transform->end_position.Set(dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_POSITION_Z].id))->Value());
	}
	//	End rotation
	if (modify_mask & 0b0000'0111'0000'0000'0000)
	{
		transform->end_rotation.Set(dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_ROTATION_Z].id))->Value());
	}
	//	End scale
	if (modify_mask & 0b0011'1000'0000'0000'0000)
	{
		transform->end_scale.Set(dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kEndSpinnerLayouts[SPINNER_SCALE_Z].id))->Value());
	}

	transform->interpolation_type = dynamic_cast<BOptionPopUp *>(FindView("interpolation_type"))->Value();
	transform->interpolate = fCheckboxInterpolate->Value() > 0;

	InvalidatePreview();
}	

/*	FUNCTION:		Effect_Transform :: AutoScale
	ARGS:			effect
					source
	RETURN:			none
	DESCRIPTION:	Adjust media scale to match resolution
*/
void Effect_Transform :: AutoScale(MediaEffect *effect, MediaSource *source)
{
	if (!effect || !source)
		return;
	assert(effect->mEffectNode == this);
	assert((source->GetMediaType() == MediaSource::MEDIA_VIDEO) ||
		   (source->GetMediaType() == MediaSource::MEDIA_VIDEO_AND_AUDIO) ||
		   (source->GetMediaType() == MediaSource::MEDIA_PICTURE));

	EffectTransformData *data = (EffectTransformData *)effect->mEffectData;
	float ratio_x = (float)gProject->mResolution.width / (float)source->GetVideoWidth();
	float ratio_y = (float)gProject->mResolution.height / (float)source->GetVideoHeight();
	//printf("Effect_Transform::AutoScale() ratio_x=%f, ratio_y=%f\n", ratio_x, ratio_y);
	float r = (ratio_x < ratio_y) ? ratio_x : ratio_y;
	data->start_scale.Set(r, r, 1);
	data->interpolate = false;
}

/*	FUNCTION:		Effect_Transform :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Transform :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = true;	
	EffectTransformData *data = (EffectTransformData *) media_effect->mEffectData;
	memset(media_effect->mEffectData, 0, sizeof(EffectTransformData));
	
	//	parse data function
	auto parse = [&v, &valid](const char *p, ymath::YVector3 &vec)
	{
		if (v.HasMember(p) && v[p].IsArray())
		{
			const rapidjson::Value &arr = v[p];
			rapidjson::SizeType num_elems = arr.GetArray().Size();
			if (num_elems == 3)
			{
				vec.x = arr.GetArray()[0].GetFloat();
				vec.y = arr.GetArray()[1].GetFloat();
				vec.z = arr.GetArray()[2].GetFloat();
			}
			else
				vec.Set(0.0f, 0.0f, 0.0f);
		}
		else
		{
			printf("Effect_Transform[ZenYes::Transform] Missing parameter %s\n", p);
			valid = false;
		}
	};
	parse("start position",	data->start_position);
	parse("start rotation",	data->start_rotation);
	parse("start scale",	data->start_scale);
	parse("end position",	data->end_position);
	parse("end rotation",	data->end_rotation);
	parse("end scale",		data->end_scale);
	
	//	interpolate
	if (v.HasMember("interpolate") && v["interpolate"].IsBool())
	{
		data->interpolate = v["interpolate"].GetBool();
	}

	//	interpolation_type
	if (v.HasMember("interpolation") && v["interpolation"].IsString())
	{
		const char *interpolation = v["interpolation"].GetString();
		for (int i=0; i < sizeof(kInterpolationType)/sizeof(INTERPOLATION_TYPE); i++)
		{
			if (strcmp(interpolation, kInterpolationType[i].text) == 0)
			{
				data->interpolation_type = i;
				break;
			}
		}
	}
	
	return valid;
}

/*	FUNCTION:		Effect_Transform :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Save project data
*/
bool Effect_Transform :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectTransformData *d = (EffectTransformData *) media_effect->mEffectData;
	if (d)
	{
		char buffer[0x80];	//	128 bytes
		sprintf(buffer, "\t\t\t\t\"start position\": [%f, %f, %f],\n", d->start_position.x, d->start_position.y, d->start_position.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"start rotation\": [%f, %f, %f],\n", d->start_rotation.x, d->start_rotation.y, d->start_rotation.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"start scale\": [%f, %f, %f],\n", d->start_scale.x, d->start_scale.y, d->start_scale.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"end position\": [%f, %f, %f],\n", d->end_position.x, d->end_position.y, d->end_position.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"end rotation\": [%f, %f, %f],\n", d->end_rotation.x, d->end_rotation.y, d->end_rotation.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"end scale\": [%f, %f, %f],\n", d->end_scale.x, d->end_scale.y, d->end_scale.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"interpolate\": %s,\n", d->interpolate ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"interpolation\": \"%s\"\n", kInterpolationType[d->interpolation_type].text);
		fwrite(buffer, strlen(buffer), 1, file);	
	}
	return true;	
}
