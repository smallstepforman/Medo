/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Wipe (left/cross/circle)
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
#include "Editor/Project.h"
#include "Editor/RenderActor.h"

#include "Effect_Wipe.h"

enum WIPE_LANGUAGE_TEXT
{
	TXT_WIPE_NAME,
	TXT_WIPE_TEXT_A,
	TXT_WIPE_TEXT_B,
	TXT_WIPE_LEFT_RIGHT,
	TXT_WIPE_RIGHT_LEFT,
	TXT_WIPE_MIDDLE_OUT,
	TXT_WIPE_CROSS,
	TXT_WIPE_CIRCLE,
	NUMBER_WIPE_LANGUAGE_TEXT
};
static const char *kWipeLanguages[][NUMBER_WIPE_LANGUAGE_TEXT] =
{
{"Wipe",		"Wipe",				"Wipe Transition between 2 tracks",		"Left -> Right",		"Right -> Left",		"Middle -> Out",		"Cross",			"Circle"},				//	"English (Britian)",
{"Wipe",		"Wipe",				"Wipe Transition between 2 tracks",		"Left -> Right",		"Right -> Left",		"Middle -> Out",		"Cross",			"Circle"},				//	"English (USA)",
{"Wischen",		"Wischen",			"Übergang verwischen zwischen 2 Tracks","Links -> Rechtst",		"Rechts -> Links",		"Mitte -> Aus",			"Kreuz",			"Kreis"},				//	"Deutsch",
{"Effacer",		"Effacer",			"Effacer la transition entre 2 pistes", "Gauche -> Droite",		"Droite -> Gauche",		"Milieu -> Sortie",		"Croix",			"Cercle"},				//	"Français",
{"Pulisci",		"Pulisci",			"Pulisci Transizione fra 2 tracce",		"Sinistra -> Destra",	"Destra -> Sinistra",	"Al centro -> Fuori",	"Incrociare",		"Cerchio"},				//	"Italiano",
{"Вытеснение",	"Вытеснение",		"Плавный переход между 2 треками",		"Слева -> направо",		"Справа -> налево",		"Средний -> Из",		"Пересечение",		"Круговое вытеснение"},	//	"Русский",
{"Прелаз",		"Гладак прелаз",	"Гладак прелаз између 2 траке",			"Лево -> Десно",		"Десно -> Лево",		"Средина -> Страна",	"Укрштени прелаз",	"Кружни прелаз"},		//	"Српски",
{"Limpiar",		"Limpiar",			"Limpiar transiciones entre dos pistas","Izquierda -> Derecha",	"Derecha -> Izquierda",	"Medio -> Fuera",		"Cruz",				"Círculo"},				//	"Español",
{"Wissen",		"Wissen",			"Wis overgang tussen 2 tracks",			"Links -> Rechts",		"Rechts -> Links",		"Midden -> Out",		"Kruis",			"Cirkel",},				//	"Dutch",
{"Menghapus",	"Menghapus",		"Hapus Transisi antara 2 trek",			"Kiri -> Kanan",		"Kanan -> Kiri",		"Tengah -> Keluar",		"Menyeberang",		"Lingkaran"},			//	"Indonesian",
};

Effect_Wipe *instantiate_effect(BRect frame)
{
	return new Effect_Wipe(frame, nullptr);
}

const char * Effect_Wipe :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Wipe :: GetEffectName() const	{return "Wipe";}

enum kWipeMessages
{
	kMsgWipeLeftRight	= 'ewd0',
	kMsgWipeRightLeft,
	kMsgWipeMiddleOut,
	kMsgWipeCross,
	kMsgWipeCircle,
	kNumberWipeMessages = kMsgWipeCircle - kMsgWipeLeftRight + 1,
};

struct RadioButton
{
	BRect				rect;
	const char			*name;
	kWipeMessages		msg;
	WIPE_LANGUAGE_TEXT	text;
};
static const RadioButton kRadioButtons[] =
{
	{BRect(40, 40, 300, 70),	"left_right",	kMsgWipeLeftRight,	TXT_WIPE_LEFT_RIGHT},
	{BRect(40, 80, 300, 110),	"right_left",	kMsgWipeRightLeft,	TXT_WIPE_RIGHT_LEFT},
	{BRect(40, 120, 300, 150),	"middle_out",	kMsgWipeMiddleOut,	TXT_WIPE_MIDDLE_OUT},
	{BRect(40, 160, 300, 190),	"cross",		kMsgWipeCross,		TXT_WIPE_CROSS},
	{BRect(40, 200, 300, 230),	"circle",		kMsgWipeCircle,		TXT_WIPE_CIRCLE},
};

struct EffectWipeData
{
	uint32		direction;
	int			swap;
};

using namespace yrender;

static const YGeometry_P3T2 kWipeGeometry[] =
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
	
static const char *kFragmentShaderLeftRight = "\
	uniform sampler2D	uTextureUnit0;\
	uniform float		uTime;\
	uniform int			uSwap;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour =texture(uTextureUnit0, vTexCoord0);\
		float width = 0.2;\
		float correction = mix(width, -width, uTime);\
		float choose = smoothstep(uTime - width, uTime + width, vTexCoord0.x + correction);\
		if (uSwap > 0)\
			fFragColour.a = mix(0.0, 1.0, choose);\
		else\
			fFragColour.a = mix(1.0, 0.0, choose);\
	}";
static const char *kFragmentShaderRightLeft = "\
	uniform sampler2D	uTextureUnit0;\
	uniform float		uTime;\
	uniform int			uSwap;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour =texture(uTextureUnit0, vTexCoord0);\
		float width = 0.2;\
		float t = 1.0 - uTime;\
		float correction = mix(width, -width, t);\
		float choose = smoothstep(t - width, t + width, vTexCoord0.x + correction);\
		if (uSwap > 0)\
			fFragColour.a = mix(1.0, 0.0, choose);\
		else\
			fFragColour.a = mix(0.0, 1.0, choose);\
	}";
static const char *kFragmentShaderMiddleOut = "\
	uniform sampler2D	uTextureUnit0;\
	uniform float		uTime;\
	uniform int			uSwap;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour =texture(uTextureUnit0, vTexCoord0);\
		float width = 0.05;\
		float correction = mix(width, -width, uTime);\
		float t = uTime*0.5;\
		float d = abs(vTexCoord0.x - 0.5);\
		float choose = smoothstep(t - width, t + width, d + correction);\
		if (uSwap > 0)\
			fFragColour.a = mix(0.0, 1.0, choose);\
		else\
			fFragColour.a = mix(1.0, 0.0, choose);\
	}";
static const char *kFragmentShaderCross = "\
	uniform sampler2D	uTextureUnit0;\
	uniform float		uTime;\
	uniform int			uSwap;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour =texture(uTextureUnit0, vTexCoord0);\
		float width = 0.05;\
		float correction = mix(width, -width, uTime);\
		float t = uTime*0.5;\
		float d = min(abs(vTexCoord0.x - 0.5), abs(vTexCoord0.y - 0.5));\
		float choose = smoothstep(t - width, t + width, d + correction);\
		if (uSwap > 0)\
			fFragColour.a = mix(0.0, 1.0, choose);\
		else\
			fFragColour.a = mix(1.0, 0.0, choose);\
	}";
static const char *kFragmentShaderCircle = "\
	uniform sampler2D	uTextureUnit0;\
	uniform float		uTime;\
	uniform vec2		uResolution;\
	uniform int			uSwap;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour =texture(uTextureUnit0, vTexCoord0);\
		float width = 0.05;\
		float correction = mix(width, -width, uTime);\
		float maxEdge = max(uResolution.x, uResolution.y);\
		float r = length((uResolution.y/uResolution.x)*vec2(uResolution.x, uResolution.y))/maxEdge;\
		float t = uTime * r;\
		float d = distance(vTexCoord0, vec2(0.5, 0.5));\
		float choose = smoothstep(t - width, t + width, d + correction);\
		if (uSwap > 0)\
			fFragColour.a = mix(0.0, 1.0, choose);\
		else\
			fFragColour.a = mix(1.0, 0.0, choose);\
	}";

class WipeShader : public yrender::YShaderNode
{
private:
	struct ShaderData
	{
		yrender::YShader	*fShader;
		GLint				fLocation_uTransform;
		GLint				fLocation_uTextureUnit0;
		GLint				fLocation_uTime;
		GLint				fLocation_uResolution;
		GLint				fLocation_uSwap;
	};
	ShaderData		fShaderLeftRight;
	ShaderData		fShaderRightLeft;
	ShaderData		fShaderMiddleOut;
	ShaderData		fShaderCross;
	ShaderData		fShaderCircle;
	float			fTime;
	
public:
	enum Shader {eLeftRight, eRightLeft, eMiddleOut, eCross, eCircle};
	int		mShaderType;
	int		mSwap;

	WipeShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");

		fShaderLeftRight.fShader = new YShader(&attributes, kVertexShader, kFragmentShaderLeftRight);
		fShaderLeftRight.fLocation_uTransform = fShaderLeftRight.fShader->GetUniformLocation("uTransform");
		fShaderLeftRight.fLocation_uTextureUnit0 = fShaderLeftRight.fShader->GetUniformLocation("uTextureUnit0");
		fShaderLeftRight.fLocation_uTime = fShaderLeftRight.fShader->GetUniformLocation("uTime");
		fShaderLeftRight.fLocation_uSwap = fShaderLeftRight.fShader->GetUniformLocation("uSwap");

		fShaderRightLeft.fShader = new YShader(&attributes, kVertexShader, kFragmentShaderRightLeft);
		fShaderRightLeft.fLocation_uTransform = fShaderRightLeft.fShader->GetUniformLocation("uTransform");
		fShaderRightLeft.fLocation_uTextureUnit0 = fShaderRightLeft.fShader->GetUniformLocation("uTextureUnit0");
		fShaderRightLeft.fLocation_uTime = fShaderRightLeft.fShader->GetUniformLocation("uTime");
		fShaderRightLeft.fLocation_uSwap = fShaderRightLeft.fShader->GetUniformLocation("uSwap");

		fShaderMiddleOut.fShader = new YShader(&attributes, kVertexShader, kFragmentShaderMiddleOut);
		fShaderMiddleOut.fLocation_uTransform = fShaderMiddleOut.fShader->GetUniformLocation("uTransform");
		fShaderMiddleOut.fLocation_uTextureUnit0 = fShaderMiddleOut.fShader->GetUniformLocation("uTextureUnit0");
		fShaderMiddleOut.fLocation_uTime = fShaderMiddleOut.fShader->GetUniformLocation("uTime");
		fShaderMiddleOut.fLocation_uSwap = fShaderMiddleOut.fShader->GetUniformLocation("uSwap");

		fShaderCross.fShader = new YShader(&attributes, kVertexShader, kFragmentShaderCross);
		fShaderCross.fLocation_uTransform = fShaderCross.fShader->GetUniformLocation("uTransform");
		fShaderCross.fLocation_uTextureUnit0 = fShaderCross.fShader->GetUniformLocation("uTextureUnit0");
		fShaderCross.fLocation_uTime = fShaderCross.fShader->GetUniformLocation("uTime");
		fShaderCross.fLocation_uSwap = fShaderCross.fShader->GetUniformLocation("uSwap");

		fShaderCircle.fShader = new YShader(&attributes, kVertexShader, kFragmentShaderCircle);
		fShaderCircle.fLocation_uTransform = fShaderCircle.fShader->GetUniformLocation("uTransform");
		fShaderCircle.fLocation_uTextureUnit0 = fShaderCircle.fShader->GetUniformLocation("uTextureUnit0");
		fShaderCircle.fLocation_uTime = fShaderCircle.fShader->GetUniformLocation("uTime");
		fShaderCircle.fLocation_uResolution = fShaderCircle.fShader->GetUniformLocation("uResolution");
		fShaderCircle.fLocation_uSwap = fShaderCircle.fShader->GetUniformLocation("uSwap");

		fTime = 0.0f;
		mShaderType = Shader::eLeftRight;
		mSwap = 0;
	}
	~WipeShader()
	{
		delete fShaderLeftRight.fShader;
		delete fShaderRightLeft.fShader;
		delete fShaderMiddleOut.fShader;
		delete fShaderCross.fShader;
		delete fShaderCircle.fShader;
	}
	void SetTime(float t) {fTime = t;}
	void SetShader(int type) {mShaderType = type;}
	void SetSwap(int swap) {mSwap = swap;}
	void Render(float delta_time)
	{
		switch (mShaderType)
		{
			case Shader::eLeftRight:
				fShaderLeftRight.fShader->EnableProgram();
				glUniformMatrix4fv(fShaderLeftRight.fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
				glUniform1i(fShaderLeftRight.fLocation_uTextureUnit0, 0);
				glUniform1f(fShaderLeftRight.fLocation_uTime, fTime);
				glUniform1i(fShaderLeftRight.fLocation_uSwap, mSwap);
				break;
			case Shader::eRightLeft:
				fShaderRightLeft.fShader->EnableProgram();
				glUniformMatrix4fv(fShaderRightLeft.fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
				glUniform1i(fShaderRightLeft.fLocation_uTextureUnit0, 0);
				glUniform1f(fShaderRightLeft.fLocation_uTime, fTime);
				glUniform1i(fShaderRightLeft.fLocation_uSwap, mSwap);
				break;
			case Shader::eMiddleOut:
				fShaderMiddleOut.fShader->EnableProgram();
				glUniformMatrix4fv(fShaderMiddleOut.fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
				glUniform1i(fShaderMiddleOut.fLocation_uTextureUnit0, 0);
				glUniform1f(fShaderMiddleOut.fLocation_uTime, fTime);
				glUniform1i(fShaderMiddleOut.fLocation_uSwap, mSwap);
				break;
			case Shader::eCross:
				fShaderCross.fShader->EnableProgram();
				glUniformMatrix4fv(fShaderCross.fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
				glUniform1i(fShaderCross.fLocation_uTextureUnit0, 0);
				glUniform1f(fShaderCross.fLocation_uTime, fTime);
				glUniform1i(fShaderCross.fLocation_uSwap, mSwap);
				break;
			case Shader::eCircle:
			{
				fShaderCircle.fShader->EnableProgram();
				glUniformMatrix4fv(fShaderCircle.fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
				glUniform1i(fShaderCircle.fLocation_uTextureUnit0, 0);
				glUniform1f(fShaderCircle.fLocation_uTime, fTime);
				ymath::YVector2 resolution(gProject->mResolution.width, gProject->mResolution.height);
				glUniform2fv(fShaderCircle.fLocation_uResolution, 1, resolution.v);
				glUniform1i(fShaderCircle.fLocation_uSwap, mSwap);
				break;
			}
			default:
				assert(0);
		}
	}	
};

/*	FUNCTION:		Effect_Wipe :: Effect_Wipe
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Wipe :: Effect_Wipe(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	assert(sizeof(kWipeLanguages)/(NUMBER_WIPE_LANGUAGE_TEXT*sizeof(char *)) == GetAvailableLanguages().size());

	InitSwapTexturesCheckbox();

	fRenderNode = nullptr;

	for (std::size_t i=0; i < sizeof(kRadioButtons)/sizeof(RadioButton); i++)
	{
		BRadioButton *button = new BRadioButton(kRadioButtons[i].rect, kRadioButtons[i].name, kWipeLanguages[GetLanguage()][kRadioButtons[i].text], new BMessage(kRadioButtons[i].msg));
		fGuiButtons.push_back(button);
		mEffectView->AddChild(button);
	}
	fGuiButtons[0]->SetValue(1);
}

/*	FUNCTION:		Effect_Wipe :: ~Effect_Wipe
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Wipe :: ~Effect_Wipe()
{ }

/*	FUNCTION:		Effect_Fade :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Wipe :: AttachedToWindow()
{
	for (auto &i : fGuiButtons)
		i->SetTarget(this, Window());

	mSwapTexturesCheckbox->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Wipe :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Wipe :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new WipeShader;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kWipeGeometry, 4);
	//fRenderNode->mTexture = new YTexture(width, height);
}

/*	FUNCTION:		Effect_Wipe :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Wipe :: DestroyRenderObjects()
{
	delete fRenderNode;
}

/*	FUNCTION:		Effect_Wipe :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Wipe :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("AddOns/Wipe/icon_wipe.png");
	return icon;	
}

/*	FUNCTION:		Effect_Wipe :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Wipe :: GetTextEffectName(const uint32 language_idx)
{
	return kWipeLanguages[GetLanguage()][TXT_WIPE_NAME];
}

/*	FUNCTION:		Effect_Wipe :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Wipe :: GetTextA(const uint32 language_idx)
{
	return kWipeLanguages[GetLanguage()][TXT_WIPE_TEXT_A];
}

/*	FUNCTION:		Effect_Wipe :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Wipe :: GetTextB(const uint32 language_idx)
{
	return kWipeLanguages[GetLanguage()][TXT_WIPE_TEXT_B];
}

/*	FUNCTION:		Effect_Wipe :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Wipe :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectWipeData *data = new EffectWipeData;
	data->direction = 0;
	data->swap = mSwapTexturesCheckbox->Value();
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_Wipe :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Wipe :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	EffectWipeData *data = (EffectWipeData *)effect->mEffectData;

	//	Update GUI
	for (std::size_t i=0; i < fGuiButtons.size(); i++)
		fGuiButtons[i]->SetValue(data->direction == i ? 1 : 0);

	mSwapTexturesCheckbox->SetValue(data->swap);

}

/*	FUNCTION:		Effect_Wipe :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Wipe :: RenderEffect(BBitmap *source, MediaEffect *effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	if (!effect)
		return;
	EffectWipeData *data = (EffectWipeData *)effect->mEffectData;

	float t = float(frame_idx - effect->mTimelineFrameStart)/float(effect->Duration());
	if (t > 1.0f)
		t = 1.0f;
	WipeShader *shader = (WipeShader *)fRenderNode->mShaderNode;
	shader->SetTime(t);
	shader->SetShader(data->direction);
	shader->SetSwap(data->swap);
	if (source)
		fRenderNode->mTexture = gRenderActor->GetPicture((unsigned int)source->Bounds().Width() + 1, (unsigned int)source->Bounds().Height() + 1, source)->mTexture;
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_Wipe :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Wipe :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgSwapTextureUnits:
			if (GetCurrentMediaEffect())
			{
				((EffectWipeData *)GetCurrentMediaEffect()->mEffectData)->swap = mSwapTexturesCheckbox->Value();
				gProject->InvalidatePreview();
			}
			break;

		default:
			if ((msg->what >= kMsgWipeLeftRight) && (msg->what <= kMsgWipeLeftRight + kNumberWipeMessages))
			{
				if (GetCurrentMediaEffect())
				{
					((EffectWipeData *)GetCurrentMediaEffect()->mEffectData)->direction = msg->what - kMsgWipeLeftRight;
					gProject->InvalidatePreview();
				}
			}
			else
				EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_Wipe :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Wipe :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	EffectWipeData *data = (EffectWipeData *) media_effect->mEffectData;

	//	direction
	if (v.HasMember("direction") && v["direction"].IsUint())
	{
		data->direction = v["direction"].GetUint();
		if (data->direction >= fGuiButtons.size())
			data->direction = 0;
	}
	else
	{
		printf("[Effect_Wipe::LoadParameters()] - invalid parameter \"direction\"\n");
		return false;
	}

	//	swap
	if (v.HasMember("swap") && v["swap"].IsBool())
		data->swap = v["swap"].GetBool() ? 1 : 0;
	else
	{
		printf("[Effect_Wipe::LoadParameters()] - invalid parameter \"swap\"\n");
		return false;
	}
	
	return true;
}

/*	FUNCTION:		Effect_Wipe :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Wipe :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectWipeData *data = (EffectWipeData *) media_effect->mEffectData;

	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"direction\": %u,\n", data->direction);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"swap\": %s\n", data->swap > 0 ? "true" : "false");
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
