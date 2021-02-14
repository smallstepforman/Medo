/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Fade to Black/Alpha
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/Texture.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"
#include "Yarra/Render/Picture.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/LanguageJson.h"
#include "Editor/Project.h"
#include "Editor/RenderActor.h"

#include "Effect_Fade.h"

enum FADE_LANGUAGE_TEXT
{
	TXT_FADE_NAME,
	TXT_FADE_TEXT_A,
	TXT_FADE_TEXT_B,
	TXT_FADE_FROM_BLACK,
	TXT_FADE_TO_BLACK,
	TXT_FADE_ALPHA_IN,
	TXT_FADE_ALPHA_OUT,
	NUMBER_FADE_LANGUAGE_TEXT
};

Effect_Fade *instantiate_effect(BRect frame)
{
    return new Effect_Fade(frame, nullptr);
}

const char * Effect_Fade :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Fade :: GetEffectName() const	{return "Fade";}

enum kFadeMessages
{
	kMsgFadeFromBlack		= 'efd0',
	kMsgFadeToBlack,
	kMsgFadeAlphaIn,
	kMsgFadeAlphaOut,
};

struct EffectFadeData
{
	uint32		direction;
};

static const ymath::YVector4 sFadeColours[4][2] =
{
	{ymath::YVector4(0,0,0,1),	ymath::YVector4(1,1,1,1)},		//	From Black
	{ymath::YVector4(1,1,1,1),	ymath::YVector4(0,0,0,1)},		//	To Black
	{ymath::YVector4(1,1,1,0),	ymath::YVector4(1,1,1,1)},		//	Alpha In
	{ymath::YVector4(1,1,1,1),	ymath::YVector4(1,1,1,0)},		//	Alpha Out
};

using namespace yrender;

static const YGeometry_P3T2 kFadeGeometry[] =
{
	{-1, -1, 0,		0, 0},
	{1, -1, 0,		1, 0},
	{-1, 1, 0,		0, 1},
	{1, 1, 0,		1, 1},
};

/************************
	Fade Shader
*************************/
static const char *kVertexShader = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture0;\
	out vec2		vTexCoord0;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);vTexCoord0 = aTexture0;\
	}";
	
static const char *kFragmentShader = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec4		uColour;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour = texture(uTextureUnit0, vTexCoord0) * uColour;\
	}";


class FadeShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;
	GLint				fLocation_uColour;
	ymath::YVector4		fColour;
	
public:
	FadeShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kVertexShader, kFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
		fLocation_uColour = fShader->GetUniformLocation("uColour");	
	}
	~FadeShader()
	{
		delete fShader;
	}
	void SetColour(const ymath::YVector4 &colour) {fColour = colour;}
	void Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0, 0);
		glUniform4fv(fLocation_uColour, 1, fColour.v);	
	}	
};

/*	FUNCTION:		Effect_Fade :: Effect_Fade
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Fade :: Effect_Fade(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fRenderNode = nullptr;

	fLanguage = new LanguageJson("AddOns/Fade/Languages.json");
	if (fLanguage->GetTextCount() == 0)
	{
		printf("Effect_Fade() Error - cannot load \"Languages.json\"\n");
		return;
	}
	
	fGuiButtons[0]= new BRadioButton(BRect(40, 40, 300, 70), "fade_1", fLanguage->GetText(TXT_FADE_FROM_BLACK), new BMessage(kMsgFadeFromBlack));
	fGuiButtons[1] = new BRadioButton(BRect(40, 80, 300, 110), "fade_2", fLanguage->GetText(TXT_FADE_TO_BLACK), new BMessage(kMsgFadeToBlack));
	fGuiButtons[2] = new BRadioButton(BRect(40, 120, 300, 150), "fade_3", fLanguage->GetText(TXT_FADE_ALPHA_IN), new BMessage(kMsgFadeAlphaIn));
	fGuiButtons[3] = new BRadioButton(BRect(40, 160, 300, 190), "fade_4", fLanguage->GetText(TXT_FADE_ALPHA_OUT), new BMessage(kMsgFadeAlphaOut));
	fGuiButtons[0]->SetValue(1);
	for (int i=0; i < 4; i++)
		mEffectView->AddChild(fGuiButtons[i]);
}

/*	FUNCTION:		Effect_Fade :: ~Effect_Fade
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Fade :: ~Effect_Fade()
{
	delete fLanguage;
}

/*	FUNCTION:		Effect_Fade :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Fade :: AttachedToWindow()
{
	for (int i=0; i < 4; i++)
		fGuiButtons[i]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Fade :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Fade :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new FadeShader;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kFadeGeometry, 4);
	//fRenderNode->mTexture = new YTexture(width, height);
}

/*	FUNCTION:		Effect_Fade :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Fade :: DestroyRenderObjects()
{
	delete fRenderNode;
	fRenderNode = nullptr;
}

/*	FUNCTION:		Effect_Fade :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Fade :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("AddOns/Fade/icon_fade.png");
	return icon;	
}

/*	FUNCTION:		Effect_Fade :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Fade :: GetTextEffectName(const uint32 language_idx)
{
	return fLanguage->GetText(TXT_FADE_NAME);
}

/*	FUNCTION:		Effect_Fade :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Fade :: GetTextA(const uint32 language_idx)
{
	return fLanguage->GetText(TXT_FADE_TEXT_A);
}

/*	FUNCTION:		Effect_Fade :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Fade :: GetTextB(const uint32 language_idx)
{
	return fLanguage->GetText(TXT_FADE_TEXT_B);
}

/*	FUNCTION:		Effect_Fade :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Fade :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectFadeData *data = new EffectFadeData;
	for (int i=0; i < 4; i++)
	{
		if (fGuiButtons[i]->Value() > 0)
			data->direction = i;
	}
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_Fade :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Fade :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	EffectFadeData *data = (EffectFadeData *)effect->mEffectData;

	//	Update GUI
	for (int i=0; i < 4; i++)
		fGuiButtons[i]->SetValue(data->direction == i ? 1 : 0);
}

/*	FUNCTION:		Effect_Fade :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Fade :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	float t = float(frame_idx - data->mTimelineFrameStart)/float(data->Duration());
	if (t > 1.0f)
		t = 1.0f;
	int index = ((EffectFadeData *)data->mEffectData)->direction;
	const ymath::YVector4 fade_colour = sFadeColours[index][0] + (sFadeColours[index][1] - sFadeColours[index][0])*t;
	((FadeShader *)fRenderNode->mShaderNode)->SetColour(fade_colour);

	if (source)
		fRenderNode->mTexture = gRenderActor->GetPicture((unsigned int)source->Bounds().Width() + 1, (unsigned int)source->Bounds().Height() + 1, source)->mTexture;
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_Fade :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Fade :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgFadeFromBlack:
			if (GetCurrentMediaEffect())
				((EffectFadeData *)GetCurrentMediaEffect()->mEffectData)->direction = 0;
			break;
				
		case kMsgFadeToBlack:
			if (GetCurrentMediaEffect())
				((EffectFadeData *)GetCurrentMediaEffect()->mEffectData)->direction = 1;
			break;

		case kMsgFadeAlphaIn:
			if (GetCurrentMediaEffect())
				((EffectFadeData *)GetCurrentMediaEffect()->mEffectData)->direction = 2;
			break;

		case kMsgFadeAlphaOut:
			if (GetCurrentMediaEffect())
				((EffectFadeData *)GetCurrentMediaEffect()->mEffectData)->direction = 3;
			break;
		
		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_Fade :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Fade :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	//	direction
	if (v.HasMember("direction") && v["direction"].IsUint())
	{
		EffectFadeData *data = (EffectFadeData *) media_effect->mEffectData;
		data->direction = v["direction"].GetUint();
		if (data->direction > 3)
			data->direction = 0;
	}
	else
		printf("Effect_Fade::LoadParamaters() corrupt direction\n");
	
	return true;
}

/*	FUNCTION:		Effect_Fade :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Fade :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectFadeData *data = (EffectFadeData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"direction\": %u\n", data->direction);
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
