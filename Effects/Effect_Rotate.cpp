/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Rotate
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

#include "Effect_Rotate.h"

const char * Effect_Rotate :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Rotate :: GetEffectName() const	{return "Rotate";}

enum kRotateMessages
{
	kMsgDirection		= 'erot',
	kMsgMirror,
};

enum ROTATE_DIRECTION
{
	ROTATE_0_CLOCKWISE,
	ROTATE_90_CLOCKWISE,
	ROTATE_180_CLOCKWISE,
	ROTATE_270_CLOCKWISE,
};
struct ROTATE_PARAMETERS
{
	ROTATE_DIRECTION	direction;
	LANGUAGE_TEXT		description;
};
static const ROTATE_PARAMETERS kRotateParameters[] =
{
	{ROTATE_0_CLOCKWISE,		TXT_EFFECTS_ROTATE_0_CLOCKWISE},
	{ROTATE_90_CLOCKWISE,		TXT_EFFECTS_ROTATE_90_CLOCKWISE},
	{ROTATE_180_CLOCKWISE,		TXT_EFFECTS_ROTATE_180_CLOCKWISE},
	{ROTATE_270_CLOCKWISE,		TXT_EFFECTS_ROTATE_270_CLOCKWISE},
};

//	Rotate Data
struct EffectRotationData
{
	ROTATE_DIRECTION		direction;
	bool					mirror;
};

using namespace yrender;


/*	FUNCTION:		Effect_Rotate :: Effect_Rotate
	ARGS:			frame
					filename
					is_window
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Rotate :: Effect_Rotate(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	
	//	Popup direction
	BOptionPopUp *popup_direction = new BOptionPopUp(BRect(20, 20, 600, 70), "direction", GetText(TXT_EFFECTS_ROTATE_DIRECTION), new BMessage(kMsgDirection));
	for (int i=0; i < sizeof(kRotateParameters)/sizeof(ROTATE_PARAMETERS); i++)
	{
		popup_direction->AddOption(GetText(kRotateParameters[i].description), i);
	}
	mEffectView->AddChild(popup_direction);

	BCheckBox *mirror = new BCheckBox(BRect(20, 100, 400, 140), "mirror", GetText(TXT_EFFECTS_ROTATE_MIRROR), new BMessage(kMsgMirror));
	mEffectView->AddChild(mirror);
}

/*	FUNCTION:		Effect_Rotate :: ~Effect_Rotate
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Rotate :: ~Effect_Rotate()
{ }

/*	FUNCTION:		Effect_Rotate :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_Rotate :: AttachedToWindow()
{
	dynamic_cast<BOptionPopUp *>(FindView("direction"))->SetTarget(this, Window());
	dynamic_cast<BCheckBox *>(FindView("mirror"))->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Rotate :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Rotate :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Rotate.png");
	return icon;	
}

/*	FUNCTION:		Effect_Rotate :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Rotate :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_ROTATE);
}

/*	FUNCTION:		Effect_Rotate :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Rotate :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_ROTATE_TEXT_A);
}

/*	FUNCTION:		Effect_Rotate :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Rotate :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_ROTATE_TEXT_B);
}

/*	FUNCTION:		Effect_Rotate :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Rotate :: CreateMediaEffect()
{
	const float width = gProject->mResolution.width;
	const float height = gProject->mResolution.height;

	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	media_effect->mEffectData = new EffectRotationData;
	memset(media_effect->mEffectData, 0, sizeof(EffectRotationData));
	return media_effect;	
}

/*	FUNCTION:		Effect_Rotate :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Rotate :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;
	
	//	Update GUI
	EffectRotationData *data = (EffectRotationData *) effect->mEffectData;
	dynamic_cast<BOptionPopUp *>(FindView("direction"))->SetValue(data->direction);
	dynamic_cast<BCheckBox *>(FindView("mirror"))->SetValue(data->mirror ? 1 : 0);
}

/*	FUNCTION:		Effect_Rotate :: ChainedSpatialTransform
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Rotate :: ChainedSpatialTransform(MediaEffect *data, int64 frame_idx)
{
	EffectRotationData *move = (EffectRotationData *) data->mEffectData;
	if (move)
	{
		YSpatial spatial;
		switch (move->direction)
		{
			case ROTATE_0_CLOCKWISE:			spatial.SetRotation(ymath::YVector3(0, (move->mirror ? 180 : 0), 0));		break;
			case ROTATE_90_CLOCKWISE:			spatial.SetRotation(ymath::YVector3(0, (move->mirror ? 180 : 0), 90));		break;
			case ROTATE_180_CLOCKWISE:			spatial.SetRotation(ymath::YVector3(0, (move->mirror ? 180 : 0), 180));		break;
			case ROTATE_270_CLOCKWISE:			spatial.SetRotation(ymath::YVector3(0, (move->mirror ? 180 : 0), 270));		break;
			default:							assert(0);
		}
		spatial.SetPosition(ymath::YVector3(0.5f*gProject->mResolution.width, 0.5f*gProject->mResolution.height, 0));
		spatial.Transform();
	}
}

/*	FUNCTION:		Effect_Rotate :: RenderEffect
	ARGS:			source
					data
					frame_idx
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Rotate :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	unsigned int w = source->Bounds().IntegerWidth() + 1;
	unsigned int h = source->Bounds().IntegerHeight() + 1;
	yrender::YPicture *picture = gRenderActor->GetPicture(w, h, source);

	yMatrixStack.Push();
	ChainedSpatialTransform(data, frame_idx);
	picture->Render(0.0f);
	yMatrixStack.Pop();
}

/*	FUNCTION:		Effect_Rotate :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Rotate :: MessageReceived(BMessage *msg)
{
	EffectRotationData *data = nullptr;
	if (GetCurrentMediaEffect())
		data = (EffectRotationData *) GetCurrentMediaEffect()->mEffectData;
	
	switch (msg->what)
	{
		case kMsgDirection:
			if (data)
			{
				data->direction = ROTATE_DIRECTION(dynamic_cast<BOptionPopUp *>(FindView("direction"))->Value());
				InvalidatePreview();
			}
			break;

		case kMsgMirror:
			if (data)
			{
				data->mirror = dynamic_cast<BCheckBox *>(FindView("mirror"))->Value() > 0 ? true : false;
				InvalidatePreview();
			}
				
		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_Rotate :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Rotate :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = true;	
	EffectRotationData *data = (EffectRotationData *) media_effect->mEffectData;
	memset(data, 0, sizeof(EffectRotationData));
	
	//	direction
	if (v.HasMember("direction") && v["direction"].IsUint())
	{
		data->direction = ROTATE_DIRECTION(v["direction"].GetUint());
	}

	//	mirror
	if (v.HasMember("mirror") && v["mirror"].IsBool())
	{
		data->mirror = v["mirror"].GetBool();
	}
	
	return valid;
}

/*	FUNCTION:		Effect_Rotate :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Save project data
*/
bool Effect_Rotate :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectRotationData *d = (EffectRotationData *) media_effect->mEffectData;
	if (d)
	{
		char buffer[0x80];	//	128 bytes
		sprintf(buffer, "\t\t\t\t\"direction\": %u,\n", d->direction);
		fwrite(buffer, strlen(buffer), 1, file);	
		sprintf(buffer, "\t\t\t\t\"mirror\": %s\n", d->mirror ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);
	}
	return true;	
}
