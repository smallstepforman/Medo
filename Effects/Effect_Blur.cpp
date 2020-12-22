/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Blur
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>
#include <interface/OptionPopUp.h>

#include "Yarra/Render/Texture.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"

#include "Gui/ValueSlider.h"
#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/Project.h"
#include "Editor/RenderActor.h"

#include "Effect_Blur.h"

const char * Effect_Blur :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Blur :: GetEffectName() const	{return "Blur";}

enum
{
	kMsgBlurSlider0		= 'ebl0',
	kMsgBlurSlider1,
	kMsgBlurAlgorithm,
	kMsgBlurInterpolate,
};

struct EffectBlurData
{
	float			factor[2];
	uint32			method;
	bool			interpolate;
};
static const float kDefaultBlur = 6.0f;

using namespace yrender;

static const YGeometry_P3T2 kBlurGeometry[] =
{
	{-1, -1, 0,		0, 0},
	{1, -1, 0,		1, 0},
	{-1, 1, 0,		0, 1},
	{1, 1, 0,		1, 1},
};

/************************
	Blur Shaders
*************************/
static const char *kVertexShader = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture0;\
	out vec2		vTexCoord0;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);\
		vTexCoord0 = aTexture0;\
	}";

static const char *kFragmentShader_BoxBlur = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec2		uDirection;\
	uniform vec2		uResolution;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		vec4 col0 = texture(uTextureUnit0, vTexCoord0 + vec2(-uDirection.s, -uDirection.t)/uResolution);\
		vec4 col1 = texture(uTextureUnit0, vTexCoord0 + vec2(         0.0,	-uDirection.t)/uResolution);\
		vec4 col2 = texture(uTextureUnit0, vTexCoord0 + vec2(uDirection.s,	-uDirection.t)/uResolution);\
		vec4 col3 = texture(uTextureUnit0, vTexCoord0 + vec2(-uDirection.s,		     0.0)/uResolution);\
		vec4 col4 = texture(uTextureUnit0, vTexCoord0 + vec2(         0.0,          0.0)/uResolution);\
		vec4 col5 = texture(uTextureUnit0, vTexCoord0 + vec2(uDirection.s,          0.0)/uResolution);\
		vec4 col6 = texture(uTextureUnit0, vTexCoord0 + vec2(-uDirection.s, uDirection.t)/uResolution);\
		vec4 col7 = texture(uTextureUnit0, vTexCoord0 + vec2(         0.0, uDirection.t)/uResolution);\
		vec4 col8 = texture(uTextureUnit0, vTexCoord0 + vec2(uDirection.s, uDirection.t)/uResolution);\
		\
		vec4 sum = (1.0*col0 + 2.0*col1 + 1.0*col2 + 2.0*col3 + 4.0*col4 + 2.0*col5 + 1.0*col6 + 2.0*col7 + 1.0*col8) / 16.0;\
		fFragColour = vec4(sum.rgb, 1.0);\
	}";

// Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
static const char *kFragmentShader_GaussianBlur = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec2		uDirection;\
	uniform vec2		uResolution;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	const float sigma = 4.0;\
	const float pi = 3.14159265f;\
	const int numBlurPixelsPerSide = 3;\
	void main(void) {\
		vec3 incrementalGaussian;\
		incrementalGaussian.x = 1.0f / (sqrt(2.0f * pi) * sigma);\
		incrementalGaussian.y = exp(-0.5f / (sigma * sigma));\
		incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;\
		vec4 avgValue = vec4(0.0f, 0.0f, 0.0f, 0.0f);\
		float coefficientSum = 0.0f;\
		avgValue += texture(uTextureUnit0, vTexCoord0) * incrementalGaussian.x;\
		coefficientSum += incrementalGaussian.x;\
		incrementalGaussian.xy *= incrementalGaussian.yz;\
		for (int i = 1; i <= numBlurPixelsPerSide; i++)\
		{\
			avgValue += texture(uTextureUnit0, vTexCoord0 - i*uDirection/uResolution) * incrementalGaussian.x;\
			avgValue += texture(uTextureUnit0, vTexCoord0 + i*uDirection/uResolution) * incrementalGaussian.x;\
			coefficientSum += 2.0 * incrementalGaussian.x;\
			incrementalGaussian.xy *= incrementalGaussian.yz;\
		}\
		fFragColour = avgValue / coefficientSum;\
	}";

static const char *kFragmentShader_blur5 = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec2		uDirection;\
	uniform vec2		uResolution;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	vec4 blur5() {\
		vec4 colour = vec4(0.0);\
		vec2 offset = vec2(1.3333333333333333) * uDirection;\
		colour += texture(uTextureUnit0, vTexCoord0) * 0.29411764705882354;\
		colour += texture(uTextureUnit0, vTexCoord0 + (offset/uResolution)) * 0.35294117647058826;\
		colour += texture(uTextureUnit0, vTexCoord0 - (offset/uResolution)) * 0.35294117647058826;\
		return colour;\
	}\
	void main(void) {\
		fFragColour = blur5();\
	}";

static const char *kFragmentShader_blur9 = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec2		uDirection;\
	uniform vec2		uResolution;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	vec4 blur9() {\
		vec4 colour = vec4(0.0);\
		vec2 off1 = vec2(1.3846153846) * uDirection;\
		vec2 off2 = vec2(3.2307692308) * uDirection;\
		colour += texture2D(uTextureUnit0, vTexCoord0) * 0.2270270270;\
		colour += texture2D(uTextureUnit0, vTexCoord0 + (off1 / uResolution)) * 0.3162162162;\
		colour += texture2D(uTextureUnit0, vTexCoord0 - (off1 / uResolution)) * 0.3162162162;\
		colour += texture2D(uTextureUnit0, vTexCoord0 + (off2 / uResolution)) * 0.0702702703;\
		colour += texture2D(uTextureUnit0, vTexCoord0 - (off2 / uResolution)) * 0.0702702703;\
		return colour;\
	}\
	void main(void) {\
		fFragColour = blur9();\
	}";

static const char *kFragmentShader_blur13 = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec2		uDirection;\
	uniform vec2		uResolution;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	vec4 blur13() {\
		vec4 colour = vec4(0.0);\
		vec2 off1 = vec2(1.411764705882353) * uDirection;\
		vec2 off2 = vec2(3.2941176470588234) * uDirection;\
		vec2 off3 = vec2(5.176470588235294) * uDirection;\
		colour += texture2D(uTextureUnit0, vTexCoord0) * 0.1964825501511404;\
		colour += texture2D(uTextureUnit0, vTexCoord0 + (off1 / uResolution)) * 0.2969069646728344;\
		colour += texture2D(uTextureUnit0, vTexCoord0 - (off1 / uResolution)) * 0.2969069646728344;\
		colour += texture2D(uTextureUnit0, vTexCoord0 + (off2 / uResolution)) * 0.09447039785044732;\
		colour += texture2D(uTextureUnit0, vTexCoord0 - (off2 / uResolution)) * 0.09447039785044732;\
		colour += texture2D(uTextureUnit0, vTexCoord0 + (off3 / uResolution)) * 0.010381362401148057;\
		colour += texture2D(uTextureUnit0, vTexCoord0 - (off3 / uResolution)) * 0.010381362401148057;\
		return colour;\
	}\
	void main(void) {\
		fFragColour = blur13();\
	}";

class BlurShader : public yrender::YShaderNode
{
public:
	enum
	{
		BLUR_BOX,
		BLUR_SHADER_5,
		BLUR_SHADER_9,
		BLUR_SHADER_13,
		BLUR_GAUSSIAN,
		NUMBER_BLUR_SHADERS
	};
private:
	yrender::YShader	*fShader[NUMBER_BLUR_SHADERS];
	GLint				fLocation_uTransform[NUMBER_BLUR_SHADERS];
	GLint				fLocation_uTextureUnit0[NUMBER_BLUR_SHADERS];
	GLint				fLocation_uDirection[NUMBER_BLUR_SHADERS];
	GLint				fLocation_uResolution[NUMBER_BLUR_SHADERS];

	int					fShaderIndex;
	ymath::YVector2		fDirection;
	ymath::YVector2		fResolution;
	
public:
	BlurShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader[BLUR_BOX] = new YShader(&attributes, kVertexShader, kFragmentShader_BoxBlur);
		fShader[BLUR_SHADER_5] = new YShader(&attributes, kVertexShader, kFragmentShader_blur5);
		fShader[BLUR_SHADER_9] = new YShader(&attributes, kVertexShader, kFragmentShader_blur9);
		fShader[BLUR_SHADER_13] = new YShader(&attributes, kVertexShader, kFragmentShader_blur13);
		fShader[BLUR_GAUSSIAN] = new YShader(&attributes, kVertexShader, kFragmentShader_GaussianBlur);
		for (int i=0; i < NUMBER_BLUR_SHADERS; i++)
		{
			fLocation_uTransform[i] = fShader[i]->GetUniformLocation("uTransform");
			fLocation_uTextureUnit0[i] = fShader[i]->GetUniformLocation("uTextureUnit0");
			fLocation_uDirection[i] = fShader[i]->GetUniformLocation("uDirection");
			fLocation_uResolution[i] = fShader[i]->GetUniformLocation("uResolution");
		}

		fShaderIndex = BLUR_GAUSSIAN;
		fDirection.Set(kDefaultBlur, kDefaultBlur);
		fResolution.Set(gProject->mResolution.width, gProject->mResolution.height);
	}
	~BlurShader()
	{
		for (int i=0; i < NUMBER_BLUR_SHADERS; i++)
			delete fShader[i];
	}
	void	SetShaderIndex(int index)
	{
		assert((index >= 0) && (index < NUMBER_BLUR_SHADERS));
		fShaderIndex = index;
	}
	void	SetDirection(const float x, const float y) {fDirection.Set(x, y);}
	void	SetResolution(const float width, const float height) {fResolution.Set(width, height);}
	void	Render(float delta_time)
	{
		fShader[fShaderIndex]->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform[fShaderIndex], 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0[fShaderIndex], 0);
		glUniform2fv(fLocation_uResolution[fShaderIndex], 1, fResolution.v);
		glUniform2fv(fLocation_uDirection[fShaderIndex], 1, fDirection.v);
		//printf("BlurShader[%d] ", fShaderIndex);	fDirection.PrintToStream();
	}	
};

/**********************************
	Effect_Blur
***********************************/

/*	FUNCTION:		Effect_Blur :: Effect_Blur
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Blur :: Effect_Blur(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fRenderNode = nullptr;

	const float kFontFactor = be_plain_font->Size()/20.0f;

	//	Algorithm
	fMethodPopup = new BOptionPopUp(BRect(20*kFontFactor, 20, 340*kFontFactor, 60), "method", GetText(TXT_EFFECTS_IMAGE_BLUR_METHOD), new BMessage(kMsgBlurAlgorithm));
	fMethodPopup->AddOption("Box Blur", BlurShader::BLUR_BOX);
	fMethodPopup->AddOption("Blur 5", BlurShader::BLUR_SHADER_5);
	fMethodPopup->AddOption("Blur 9", BlurShader::BLUR_SHADER_9);
	fMethodPopup->AddOption("Blur 13", BlurShader::BLUR_SHADER_13);
	fMethodPopup->AddOption("Gaussian Blur", BlurShader::BLUR_GAUSSIAN);
	mEffectView->AddChild(fMethodPopup);
	fMethodPopup->SetValue(BlurShader::BLUR_GAUSSIAN);

	fCheckboxInterpolate = new BCheckBox(BRect(20*kFontFactor, 100, 200*kFontFactor, 140), "interpolate", GetText(TXT_EFFECTS_COMMON_INTERPOLATE), new BMessage(kMsgBlurInterpolate));
	fCheckboxInterpolate->SetValue(0);
	mEffectView->AddChild(fCheckboxInterpolate);

	fBlurSliders[0] = new ValueSlider(BRect(20*kFontFactor, 160, 480*kFontFactor, 200), "blur_slider_0", GetText(TXT_EFFECTS_IMAGE_BLUR_START), nullptr, 0, 200);
	fBlurSliders[0]->SetModificationMessage(new BMessage(kMsgBlurSlider0));
	fBlurSliders[0]->SetValue(kDefaultBlur*10);
	fBlurSliders[0]->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fBlurSliders[0]->SetHashMarkCount(20);
	fBlurSliders[0]->SetLimitLabels("0.0", "20.0");
	fBlurSliders[0]->UpdateTextValue(kDefaultBlur);
	mEffectView->AddChild(fBlurSliders[0]);

	fBlurSliders[1] = new ValueSlider(BRect(20*kFontFactor, 220, 480*kFontFactor, 260), "blur_slider_1", GetText(TXT_EFFECTS_IMAGE_BLUR_END), nullptr, 0, 200);
	fBlurSliders[1]->SetModificationMessage(new BMessage(kMsgBlurSlider1));
	fBlurSliders[1]->SetValue(kDefaultBlur*10);
	fBlurSliders[1]->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fBlurSliders[1]->SetHashMarkCount(20);
	fBlurSliders[1]->SetLimitLabels("0.0", "20.0");
	fBlurSliders[1]->UpdateTextValue(kDefaultBlur);
	mEffectView->AddChild(fBlurSliders[1]);
	fBlurSliders[1]->SetEnabled(false);
}

/*	FUNCTION:		Effect_Blur :: ~Effect_Blur
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Blur :: ~Effect_Blur()
{ }

/*	FUNCTION:		Effect_Blur :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Blur :: AttachedToWindow()
{
	fMethodPopup->SetTarget(this, Window());
	fCheckboxInterpolate->SetTarget(this, Window());
	fBlurSliders[0]->SetTarget(this, Window());
	fBlurSliders[1]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Blur :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Blur :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new BlurShader;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kBlurGeometry, 4);
	fRenderNode->mTexture = new YTexture(width, height, YTexture::YTF_MIRRORED_REPEAT);
}

/*	FUNCTION:		Effect_Blur :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Blur :: DestroyRenderObjects()
{
	delete fRenderNode;
}

/*	FUNCTION:		Effect_Blur :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Blur :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Blur.png");
	return icon;	
}

/*	FUNCTION:		Effect_Blur :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Blur :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_BLUR);
}

/*	FUNCTION:		Effect_Blur :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Blur :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_BLUR_TEXT_A);
}

/*	FUNCTION:		Effect_Blur :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Blur :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_BLUR_TEXT_B);
}

/*	FUNCTION:		Effect_Blur :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Blur :: CreateMediaEffect()
{
	const float width = gProject->mResolution.width;
	const float height = gProject->mResolution.height;

	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectBlurData *data = new EffectBlurData;
	data->factor[0] = fBlurSliders[0]->Value()/10.0f;
	data->factor[1] = fBlurSliders[1]->Value()/10.0f;
	data->method = BlurShader::BLUR_GAUSSIAN;
	data->interpolate = fCheckboxInterpolate->Value() > 0 ? true : false;
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_Blur :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Blur :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	EffectBlurData *data = (EffectBlurData *)effect->mEffectData;

	//	Update GUI
	fBlurSliders[0]->SetValue(data->factor[0] * 10);
	fBlurSliders[1]->SetValue(data->factor[1] * 10);
	fMethodPopup->SetValue(data->method);
	fCheckboxInterpolate->SetValue(data->interpolate ? 1 : 0);
	fBlurSliders[1]->SetEnabled(data->interpolate);
}

/*	FUNCTION:		Effect_Blur :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Blur :: RenderEffect(BBitmap *source, MediaEffect *effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	if ((effect == nullptr) || (effect->mEffectData == nullptr))
		return;

	EffectBlurData *blur_data = (EffectBlurData *)effect->mEffectData;
	BlurShader *blur_shader = (BlurShader *)fRenderNode->mShaderNode;

	blur_shader->SetShaderIndex(blur_data->method);
	blur_shader->SetResolution(source->Bounds().Width(), source->Bounds().Height());

	float blur_factor;
	if (blur_data->interpolate)
	{
		float t = float(frame_idx - effect->mTimelineFrameStart)/float(effect->Duration());
		if (t > 1.0f)
			t = 1.0f;
		blur_factor = blur_data->factor[0] + t*(blur_data->factor[1] - blur_data->factor[0]);
	}
	else
		blur_factor = blur_data->factor[0];

	blur_shader->SetDirection(blur_factor, 0.0f);
	fRenderNode->mTexture->Upload(source);
	gRenderActor->ActivateSecondaryRenderBuffer(true);
	fRenderNode->Render(0.0f);
	gRenderActor->DeactivateSecondaryRenderBuffer();
	fRenderNode->mTexture->Upload(gRenderActor->GetSecondaryFrameBufferTexture());
	blur_shader->SetDirection(0.0f, blur_factor);
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_Blur :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Blur :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgBlurInterpolate:
			fBlurSliders[1]->SetEnabled(fCheckboxInterpolate->Value() > 0);
			if (GetCurrentMediaEffect())
			{
				EffectBlurData *data = (EffectBlurData *)GetCurrentMediaEffect()->mEffectData;
				data->interpolate = fCheckboxInterpolate->Value() > 0;
				InvalidatePreview();
			}
			break;

		case kMsgBlurSlider0:
			if (fCheckboxInterpolate->Value() == 0)
				fBlurSliders[1]->SetValue(fBlurSliders[0]->Value());

			fBlurSliders[0]->UpdateTextValue(fBlurSliders[0]->Value()/10.0f);
			fBlurSliders[1]->UpdateTextValue(fBlurSliders[1]->Value()/10.0f);
			if (GetCurrentMediaEffect())
			{
				EffectBlurData *data = (EffectBlurData *)GetCurrentMediaEffect()->mEffectData;
				data->factor[0] = fBlurSliders[0]->Value() / 10.0f;
				data->factor[1] = fBlurSliders[1]->Value() / 10.0f;
				InvalidatePreview();
			}
			break;

		case kMsgBlurSlider1:
			assert(fCheckboxInterpolate->Value() == 1);
			fBlurSliders[1]->UpdateTextValue(fBlurSliders[1]->Value()/10.0f);
			if (GetCurrentMediaEffect())
			{
				EffectBlurData *data = (EffectBlurData *)GetCurrentMediaEffect()->mEffectData;
				data->factor[1] = fBlurSliders[1]->Value() / 10.0f;
				InvalidatePreview();
			}
			break;

		case kMsgBlurAlgorithm:
			if (GetCurrentMediaEffect())
			{
				EffectBlurData *data = (EffectBlurData *)GetCurrentMediaEffect()->mEffectData;
				data->method = fMethodPopup->Value();
				InvalidatePreview();
			}
			break;
				
		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_Blur :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Blur :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	#define ERROR_EXIT(a) {printf("[Effect_Blur] Error - %s\n", a); return false;}

	EffectBlurData *data = (EffectBlurData *) media_effect->mEffectData;

	//	interpolate
	if (v.HasMember("interpolate") && v["interpolate"].IsBool())
		data->interpolate = v["interpolate"].GetBool();
	else
		ERROR_EXIT("missing interpolate");

	//	start
	if (v.HasMember("start") && v["start"].IsFloat())
	{
		data->factor[0] = v["start"].GetFloat();
	}
	else
		ERROR_EXIT("missing start");

	//	end
	if (v.HasMember("end") && v["end"].IsFloat())
	{
		data->factor[1] = v["end"].GetFloat();
	}
	else
		ERROR_EXIT("missing end");

	//	method
	if (v.HasMember("method") && v["method"].IsUint())
	{
		data->method = v["method"].GetUint();
		if (data->method >= BlurShader::NUMBER_BLUR_SHADERS)
			ERROR_EXIT("corrupt method");
	}
	else
		ERROR_EXIT("missing method");

	return true;
}

/*	FUNCTION:		Effect_Blur :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Blur :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectBlurData *data = (EffectBlurData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"interpolate\": %s,\n", data->interpolate ? "true" : "false");
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"start\": %0.2f,\n", data->factor[0]);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"end\": %0.2f,\n", data->factor[1]);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"method\": %u\n", data->method);
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
