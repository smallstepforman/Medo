/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Move
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/Picture.h"
#include "Yarra/Render/MatrixStack.h"
#include "Yarra/Render/Texture.h"
#include "Yarra/Math/Interpolation.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/Project.h"
#include "Editor/RenderActor.h"
#include "Gui/BitmapButton.h"

#include "Effect_Move.h"

const char * Effect_Move :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Move :: GetEffectName() const	{return "Move";}

enum kMoveMessages
{
	kMsgDirection		= 'emv0',
	kMsgInterpolation,
};

enum MOVE_DIRECTION
{
	MOVE_DIR_CENTER_ABOVE,
	MOVE_DIR_CENTER_BELOW,
	MOVE_DIR_CENTER_LEFT,
	MOVE_DIR_CENTER_RIGHT,
	MOVE_DIR_CENTER_ABOVE_LEFT,
	MOVE_DIR_CENTER_ABOVE_RIGHT,
	MOVE_DIR_CENTER_BELOW_LEFT,
	MOVE_DIR_CENTER_BELOW_RIGHT,
	MOVE_DIR_ABOVE_CENTER,
	MOVE_DIR_BELOW_CENTER,
	MOVE_DIR_LEFFT_CENTER,
	MOVE_DIR_RIGHT_CENTER,
	MOVE_DIR_ABOVE_LEFT_CENTER,
	MOVE_DIR_ABOVE_RIGHT_CENTER,
	MOVE_DIR_BELOW_LEFT_CENTER,
	MOVE_DIR_BELOW_RIGHT_CENTER,
};
struct MOVE_PARAMETERS
{
	MOVE_DIRECTION		direction;
	ymath::YVector3		start_offset;
	ymath::YVector3		end_offset;
	ymath::YVector3		start_scale;
	ymath::YVector3		end_scale;
	LANGUAGE_TEXT		description;
};
static const MOVE_PARAMETERS kMoveParameters[] = 
{
	{MOVE_DIR_CENTER_ABOVE,			ymath::YVector3(0, 0, 0),			ymath::YVector3(0, -2, 0),			ymath::YVector3(1, 1, 1),			ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_CENTRE_ABOVE},
	{MOVE_DIR_CENTER_BELOW,			ymath::YVector3(0, 0, 0),			ymath::YVector3(0, 2, 0),			ymath::YVector3(1, 1, 1),			ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_CENTRE_BELOW},
	{MOVE_DIR_CENTER_LEFT,			ymath::YVector3(0, 0, 0),			ymath::YVector3(-2, 0, 0),			ymath::YVector3(1, 1, 1),			ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_CENTRE_LEFT},
	{MOVE_DIR_CENTER_RIGHT,			ymath::YVector3(0, 0, 0),			ymath::YVector3(2, 0, 0),			ymath::YVector3(1, 1, 1),			ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_CENTRE_RIGHT},
	{MOVE_DIR_CENTER_ABOVE_LEFT,	ymath::YVector3(0, 0, 0),			ymath::YVector3(-1.2f, -1.2f, 0),	ymath::YVector3(1, 1, 1),			ymath::YVector3(0.2f, 0.2f, 0.2f),	TXT_EFFECTS_MOVE_DIRECTION_CENTRE_ABOVE_LEFT},
	{MOVE_DIR_CENTER_ABOVE_RIGHT,	ymath::YVector3(0, 0, 0),			ymath::YVector3(1.2f, -1.2f, 0),	ymath::YVector3(1, 1, 1),			ymath::YVector3(0.2f, 0.2f, 0.2f),	TXT_EFFECTS_MOVE_DIRECTION_CENTRE_ABOVE_RIGHT},
	{MOVE_DIR_CENTER_BELOW_LEFT,	ymath::YVector3(0, 0, 0),			ymath::YVector3(-1.2f, 1.2f, 0),	ymath::YVector3(1, 1, 1),			ymath::YVector3(0.2f, 0.2f, 0.2f),	TXT_EFFECTS_MOVE_DIRECTION_CENTRE_BELOW_LEFT},
	{MOVE_DIR_CENTER_BELOW_RIGHT,	ymath::YVector3(0, 0, 0),			ymath::YVector3(1.2f, 1.2f, 0),		ymath::YVector3(1, 1, 1),			ymath::YVector3(0.2f, 0.2f, 0.2f),	TXT_EFFECTS_MOVE_DIRECTION_CENTRE_BELOW_RIGHT},
	{MOVE_DIR_ABOVE_CENTER,			ymath::YVector3(0, -2, 0),			ymath::YVector3(0, 0, 0),			ymath::YVector3(1, 1, 1),			ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_ABOVE_CENTRE},
	{MOVE_DIR_BELOW_CENTER,			ymath::YVector3(0, 2, 0),			ymath::YVector3(0, 0, 0),			ymath::YVector3(1, 1, 1),			ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_BELOW_CENTRE},
	{MOVE_DIR_LEFFT_CENTER,			ymath::YVector3(-2, 0, 0),			ymath::YVector3(0, 0, 0),			ymath::YVector3(1, 1, 1),			ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_LEFT_CENTRE},
	{MOVE_DIR_RIGHT_CENTER,			ymath::YVector3(2, 0, 0),			ymath::YVector3(0, 0, 0),			ymath::YVector3(1, 1, 1),			ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_RIGHT_CENTRE},
	{MOVE_DIR_ABOVE_LEFT_CENTER,	ymath::YVector3(-1.2f, -1.2f, 0),	ymath::YVector3(0, 0, 0),			ymath::YVector3(0.2f, 0.2f, 0.2f),	ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_ABOVE_LEFT_CENTER},
	{MOVE_DIR_ABOVE_RIGHT_CENTER,	ymath::YVector3(1.2f, -1.2f, 0),	ymath::YVector3(0, 0, 0),			ymath::YVector3(0.2f, 0.2f, 0.2f),	ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_ABOVE_RIGHT_CENTER},
	{MOVE_DIR_BELOW_LEFT_CENTER,	ymath::YVector3(-1.2f, 1.2f, 0),	ymath::YVector3(0, 0, 0),			ymath::YVector3(0.2f, 0.2f, 0.2f),	ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_BELOW_LEFT_CENTER},
	{MOVE_DIR_BELOW_RIGHT_CENTER,	ymath::YVector3(1.2f, 1.2f, 0),		ymath::YVector3(0, 0, 0),			ymath::YVector3(0.2f, 0.2f, 0.2f),	ymath::YVector3(1, 1, 1),			TXT_EFFECTS_MOVE_DIRECTION_BELOW_RIGHT_CENTER},
};

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

//	Move Data
struct EffectMoveData
{
	int		direction;
	int		interpolation;
};

using namespace yrender;


/*	FUNCTION:		Effect_Move :: Effect_Move
	ARGS:			frame
					filename
					is_window
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Move :: Effect_Move(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	
	//	Popup direction
	BOptionPopUp *popup_direction = new BOptionPopUp(BRect(20, 20, 600, 70), "direction", GetText(TXT_EFFECTS_MOVE_DIRECTION), new BMessage(kMsgDirection));
	for (int i=0; i < sizeof(kMoveParameters)/sizeof(MOVE_PARAMETERS); i++)
	{
		popup_direction->AddOption(GetText(kMoveParameters[i].description), i);
	}
	mEffectView->AddChild(popup_direction);
		
	//	Interpolation
	BOptionPopUp *popup_interpolation = new BOptionPopUp(BRect(20, 90, 600, 140), "interpolation", GetText(TXT_EFFECTS_COMMON_INTERPOLATION_TYPE), new BMessage(kMsgInterpolation));
	for (int i=0; i < sizeof(kInterpolationType)/sizeof(INTERPOLATION_TYPE); i++)
	{
		popup_interpolation->AddOption(GetText(kInterpolationType[i].translated_text), i);
	}
	popup_interpolation->SelectOptionFor(1);
	mEffectView->AddChild(popup_interpolation);
}

/*	FUNCTION:		Effect_Move :: ~Effect_Move
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Move :: ~Effect_Move()
{ }

/*	FUNCTION:		Effect_Move :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_Move :: AttachedToWindow()
{
	dynamic_cast<BOptionPopUp *>(FindView("direction"))->SetTarget(this, Window());
	dynamic_cast<BOptionPopUp *>(FindView("interpolation"))->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Move :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Move :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Move.png");
	return icon;	
}

/*	FUNCTION:		Effect_Move :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Move :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_MOVE);
}

/*	FUNCTION:		Effect_Transform :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Move :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_MOVE_TEXT_A);
}

/*	FUNCTION:		Effect_Transform :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Move :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_MOVE_TEXT_B);
}

/*	FUNCTION:		Effect_Move :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Move :: CreateMediaEffect()
{
	const float width = gProject->mResolution.width;
	const float height = gProject->mResolution.height;

	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	media_effect->mEffectData = new EffectMoveData;
	memset(media_effect->mEffectData, 0, sizeof(EffectMoveData));
	return media_effect;	
}

/*	FUNCTION:		Effect_Move :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Move :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;
	
	//	Update GUI
	EffectMoveData *data = (EffectMoveData *) effect->mEffectData;
	dynamic_cast<BOptionPopUp *>(FindView("direction"))->SetValue(data->direction);
	dynamic_cast<BOptionPopUp *>(FindView("interpolation"))->SetValue(data->interpolation);
}

/*	FUNCTION:		Effect_Move :: ChainedSpatialTransform
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Move :: ChainedSpatialTransform(MediaEffect *data, int64 frame_idx)
{
	EffectMoveData *move = (EffectMoveData *) data->mEffectData;

	float t = float(frame_idx - data->mTimelineFrameStart)/float(data->Duration());
	if (t > 1.0f)
		t = 1.0f;
	
	//	Position
	const YVector3 center(0.5f*gProject->mResolution.width, 0.5f*gProject->mResolution.height, 0);
	YVector3 start_position(center);
	YVector3 end_position(center);
	YVector3 start_scale;
	YVector3 end_scale;
	
	if (move)
	{
		start_position += YVector3(center)*kMoveParameters[move->direction].start_offset;
		end_position += YVector3(center)*kMoveParameters[move->direction].end_offset;
		start_scale = kMoveParameters[move->direction].start_scale;
		end_scale = kMoveParameters[move->direction].end_scale;

		const ymath::YVector3 (*fn)(const ymath::YVector3 &start, const ymath::YVector3 &end, const float t);
		switch (move->interpolation)
		{
			case kInterpolationLinear:			fn = ymath::YInterpolationLinear;			break;
			case kInterpolationCosine:			fn = ymath::YInterpolationCosine;			break;
			case kInterpolationAcceleration:	fn = ymath::YInterpolationAcceleration;		break;
			case kInterpolationDeceleration:	fn = ymath::YInterpolationDeceleration;		break;
			default:							assert(0);
		}

		YSpatial spatial;
		spatial.SetPosition(fn(start_position, end_position, t));
		spatial.SetScale(fn(start_scale, end_scale, t));
		spatial.Transform();
	}
}

/*	FUNCTION:		Effect_Move :: RenderEffect
	ARGS:			source
					data
					frame_idx
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Move :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	unsigned int w = source->Bounds().IntegerWidth() + 1;
	unsigned int h = source->Bounds().IntegerHeight() + 1;
	yrender::YPicture *picture = gRenderActor->GetPicture(w, h, source);

	yMatrixStack.Push();
	ChainedSpatialTransform(data, frame_idx);
	picture->Render(0.0f);
	yMatrixStack.Pop();
}

/*	FUNCTION:		Effect_Move :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Move :: MessageReceived(BMessage *msg)
{
	EffectMoveData *data = nullptr;
	if (GetCurrentMediaEffect())
		data = (EffectMoveData *) GetCurrentMediaEffect()->mEffectData;
	
	switch (msg->what)
	{
		case kMsgDirection:
			if (data)
			{
				data->direction = dynamic_cast<BOptionPopUp *>(FindView("direction"))->Value();
				InvalidatePreview();
			}
			break;
				
		case kMsgInterpolation:	
			if (data)
			{
				data->interpolation = dynamic_cast<BOptionPopUp *>(FindView("interpolation"))->Value();
				InvalidatePreview();
			}
			break;
		
		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_Move :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Move :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = true;	
	EffectMoveData *data = (EffectMoveData *) media_effect->mEffectData;
	memset(data, 0, sizeof(EffectMoveData));
	
	//	direction
	if (v.HasMember("direction") && v["direction"].IsUint())
	{
		data->direction = v["direction"].GetUint();
		if (data->direction >= sizeof(kMoveParameters)/sizeof(MOVE_PARAMETERS))
			data->direction = 0;
	}
	
	//	interpolation
	if (v.HasMember("interpolation") && v["interpolation"].IsString())
	{
		const char *interpolation = v["interpolation"].GetString();
		for (int i=0; i < sizeof(kInterpolationType)/sizeof(INTERPOLATION_TYPE); i++)
		{
			if (strcmp(interpolation, kInterpolationType[i].text) == 0)
			{
				data->interpolation = i;
				break;
			}
		}
	}
	
	return valid;
}

/*	FUNCTION:		Effect_Move :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Save project data
*/
bool Effect_Move :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectMoveData *d = (EffectMoveData *) media_effect->mEffectData;
	if (d)
	{
		char buffer[0x80];	//	128 bytes
		sprintf(buffer, "\t\t\t\t\"direction\": %u,\n", d->direction);
		fwrite(buffer, strlen(buffer), 1, file);	
		sprintf(buffer, "\t\t\t\t\"interpolation\": \"%s\"\n", kInterpolationType[d->interpolation].text);
		fwrite(buffer, strlen(buffer), 1, file);	
	}
	return true;	
}
