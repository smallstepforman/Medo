/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Text 3D
 */

#include <cstdio>

#include <InterfaceKit.h>
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

#include "Gui/AlphaColourControl.h"
#include "Gui/FontPanel.h"
#include "Gui/Spinner.h"
#include "Gui/ValueSlider.h"

#include "Effect_Text.h"
#include "Effect_Text_3D.h"

enum kGuiMessages
{
	eMsgDepth,
};

struct EffectText3dData
{
	int				depth;
};

static const int	kMinDepth = 1;
static const int	kMaxDepth = 200;
static const int	kDefaultDepth = 64;

//	Text3dMediaEffect allows customising destructor
class Text3dMediaEffect : public ImageMediaEffect
{
public:
	Text3dMediaEffect()  : ImageMediaEffect() { }
	~Text3dMediaEffect()
	{
		assert(mEffectNode);
		assert(mEffectData);

		Effect_Text3D *effect_node = (Effect_Text3D *)mEffectNode;
		Effect_Text::EffectTextData *data = (Effect_Text::EffectTextData *)mEffectData;
		delete (EffectText3dData *) data->derived_data;
		delete data;
	}
};

const char * Effect_Text3D :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Text3D :: GetEffectName() const	{return "Text 3D";}

using namespace yrender;

/*	FUNCTION:		Effect_Text3D :: Effect_Text3D
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Text3D :: Effect_Text3D(BRect frame, const char *filename)
	: Effect_Text(frame, filename)
{
	fIs3dFont = true;

	fTextView->ResizeTo(frame.Width()-20, 100);

	fSliderDepth = new ValueSlider(BRect(20, 140, 360, 180), "depth", GetText(TXT_EFFECTS_TEXT_3D_DEPTH), nullptr, kMinDepth, kMaxDepth);
	fSliderDepth->SetModificationMessage(new BMessage(eMsgDepth));
	fSliderDepth->SetValue(kDefaultDepth);
	fSliderDepth->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderDepth->SetHashMarkCount(5);
	char min_buffer[8], max_buffer[8], mid_buffer[8];
	sprintf(min_buffer, "%d", kMinDepth);
	sprintf(max_buffer, "%d", kMaxDepth);
	sprintf(mid_buffer, "%d", kMaxDepth/2);
	fSliderDepth->SetLimitLabels(min_buffer, max_buffer);
	fSliderDepth->UpdateTextValue(kDefaultDepth);
	fSliderDepth->SetStyle(B_BLOCK_THUMB);
	fSliderDepth->SetMidpointLabel(mid_buffer);
	fSliderDepth->SetFloatingPointPrecision(0);
	fSliderDepth->SetBarColor({0, 255, 0, 255});
	fSliderDepth->UseFillColor(true);
	mEffectView->AddChild(fSliderDepth);

	fBackgroundTitle->Hide();
	fBackgroundCheckBox->Hide();
	fBackgroundColourControl->Hide();
	fBackgroundOffset->Hide();
	fShadowCheckBox->Hide();
	fShadowSpinners[0]->Hide();
	fShadowSpinners[1]->Hide();
}

/*	FUNCTION:		Effect_Text3D :: ~Effect_Text3D
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Text3D :: ~Effect_Text3D()
{
}

/*	FUNCTION:		Effect_Text3D :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_Text3D :: AttachedToWindow()
{
	Effect_Text::AttachedToWindow();
	fSliderDepth->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Text3D :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Text3D :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Text3D.png");
	return icon;	
}

/*	FUNCTION:		Effect_Text3D :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Text3D :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_3D);
}

/*	FUNCTION:		Effect_Text3D :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Text3D :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_3D_TEXT_A);
}

/*	FUNCTION:		Effect_Text3D :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Text3D :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_3D_TEXT_B);
}

/*	FUNCTION:		Effect_Text3D :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Text3D :: CreateMediaEffect()
{
	Text3dMediaEffect *effect = new Text3dMediaEffect;
	InitMediaEffect(effect);

	EffectText3dData *data = new EffectText3dData;
	data->depth = fSliderDepth->Value();

	((EffectTextData *)effect->mEffectData)->derived_data = data;

	return effect;
}

/*	FUNCTION:		Effect_Text :: CreateOpenGLObjects
	ARGS:			data
	RETURN:			n/a
	DESCRIPTION:	Recreate YTextScene (called when font changed)
*/
void Effect_Text3D :: CreateOpenGLObjects(EffectTextData *data)
{
	assert(data != nullptr);
	if (fTextScene)
		delete fTextScene;

	EffectText3dData *text_3d_data = (EffectText3dData *)data->derived_data;

	fTextScene = new yrender::YTextScene(new yrender::YFontFreetype(data->font_size, data->font_path.String(), (float)text_3d_data->depth), true);
	fTextScene->SetText(data->text.String());
	fTextScene->SetColour(YVector4(data->font_colour.red/255.0f, data->font_colour.green/255.0f, data->font_colour.blue/255.0f, data->font_colour.alpha/255.0f));
	//fTextScene->mSpatial.SetPosition(data->position);
	fOpenGLPendingUpdate = false;
}

/*	FUNCTION:		Effect_Text3D :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Text3D :: RenderEffect(BBitmap *source, MediaEffect *media_effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	if (!media_effect || !media_effect->mEffectData)
		return;


	if ((fTextScene == nullptr) || fOpenGLPendingUpdate)
	{
		EffectTextData *data = (EffectTextData *) media_effect->mEffectData;
		CreateOpenGLObjects(data);
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glClear(GL_DEPTH_BUFFER_BIT);

	Effect_Text::RenderEffect(source, media_effect, frame_idx, chained_effects);

	//glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
}

/*	FUNCTION:		Effect_Text3D :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Text3D :: MediaEffectSelected(MediaEffect *effect)
{
	Effect_Text::MediaEffectSelected(effect);
	EffectTextData *effect_data = (EffectTextData *) effect->mEffectData;
	EffectText3dData *text_3d_data = (EffectText3dData *)effect_data->derived_data;
	fSliderDepth->SetValue(text_3d_data->depth);
	fSliderDepth->UpdateTextValue(text_3d_data->depth);
}

/*	FUNCTION:		Effect_Text3D :: MessageReceived
	ARGS:			msg
	RETURN:			true if processed
	DESCRIPTION:	Process view messages
*/
void Effect_Text3D :: MessageReceived(BMessage *msg)
{
	EffectTextData *text_data = nullptr;
	EffectText3dData *text_3d_data = nullptr;
	if (GetCurrentMediaEffect())
	{
		text_data = (EffectTextData *)GetCurrentMediaEffect()->mEffectData;
		if (text_data)
			text_3d_data = (EffectText3dData *) text_data->derived_data;
	}

	switch (msg->what)
	{
		case eMsgDepth:
			if (text_3d_data)
				text_3d_data->depth = fSliderDepth->Value();
			fSliderDepth->UpdateTextValue(text_3d_data->depth);
			fOpenGLPendingUpdate = true;
			InvalidatePreview();
			break;

		default:
			Effect_Text::MessageReceived(msg);
			return;
	}
}

/*	FUNCTION:		Effect_Text3D :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Text3D :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = Effect_Text::LoadParameters(v, media_effect);

	EffectTextData *effect_data = (EffectTextData *) media_effect->mEffectData;
	EffectText3dData *text_3d_data = (EffectText3dData *)effect_data->derived_data;

	//	depth
	if (v.HasMember("depth") && v["depth"].IsUint())
	{
		text_3d_data->depth = v["depth"].GetUint();
		if (text_3d_data->depth < kMinDepth)
			text_3d_data->depth = kMinDepth;
		if (text_3d_data->depth > kMaxDepth)
			text_3d_data->depth = kMaxDepth;
	}
	else
	{
		printf("Effect_Text3D :: LoadParameters() - missing parameter \"depth\"\n");
		valid = false;
	}

	return valid;
}

/*	FUNCTION:		Effect_Text3D :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Text3D :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectTextData *effect_data = (EffectTextData *) media_effect->mEffectData;
	EffectText3dData *text_3d_data = (EffectText3dData *)effect_data->derived_data;

	bool valid = Effect_Text::SaveParametersBase(file, media_effect, true);
	if (text_3d_data)
	{
		char buffer[0x200];	//	512 bytes
		sprintf(buffer, "\t\t\t\t\"depth\": %u\n", text_3d_data->depth);
		fwrite(buffer, strlen(buffer), 1, file);
	}
	return valid;
}


