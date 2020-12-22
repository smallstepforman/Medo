/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Saturation
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/Texture.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"

#include "Gui/ValueSlider.h"
#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/Project.h"

#include "Effect_ColourGrading.h"

const char * Effect_ColourGrading :: GetVendorName() const	{return "ZenYes";}
const char * Effect_ColourGrading :: GetEffectName() const	{return "Colour Grading";}

enum kMsgSaturation
{
	kMsgValueChanged		= 'ecs0',
	kMsgReset,
};

static const float		kDefaultSaturation		= 1.0f;
static const float		kDefaultBrightness		= 1.0f;
static const float		kDefaultContrast		= 1.0f;
static const float		kDefaultGamma			= 1.0f;
static const float		kDefaultExposure		= 0.0f;
static const float		kDefaultTemperature		= 0.0f;
static const float		kDefaultTint			= 0.0f;

struct EffectColourGradingData
{
	float				saturation;
	float				brightness;
	float				contrast;
	float				gamma;
	float				exposure;
	float				temperature;
	float				tint;
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
	ColourGrading Shader
	Algorithm from Chapter 19.5.3 of OpenGL Shading Language (Orange) book, and TGM Shader Pack (Irrlicht forum post 21057))

	Temperature + Tint Shader
	https://gitlab.bestminr.com/bestminr/FrontShaders/blob/master/shaders/temperature.glsl
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

static const char *kFragmentShader = "\
	uniform sampler2D	uTextureUnit0;\
	uniform float		uSaturation;\
	uniform float		uBrightness;\
	uniform float		uExposure;\
	uniform float		uContrast;\
	uniform float		uGamma;\
	uniform float		uTemperature;\
	uniform float		uTint;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	\n\
	mat3 matRGBtoXYZ = mat3(\n\
		0.4124564390896922, 0.21267285140562253, 0.0193338955823293,\n\
		0.357576077643909, 0.715152155287818, 0.11919202588130297,\n\
		0.18043748326639894, 0.07217499330655958, 0.9503040785363679\n\
	);\n\
	\n\
	mat3 matXYZtoRGB = mat3(\n\
		3.2404541621141045, -0.9692660305051868, 0.055643430959114726,\n\
		-1.5371385127977166, 1.8760108454466942, -0.2040259135167538,\n\
		-0.498531409556016, 0.041556017530349834, 1.0572251882231791\n\
	);\n\
	\n\
	mat3 matAdapt = mat3(\n\
		0.8951, -0.7502, 0.0389,\n\
		0.2664, 1.7135, -0.0685,\n\
		-0.1614, 0.0367, 1.0296\n\
	);\n\
	\n\
	mat3 matAdaptInv = mat3(\n\
		0.9869929054667123, 0.43230526972339456, -0.008528664575177328,\n\
		-0.14705425642099013, 0.5183602715367776, 0.04004282165408487,\n\
		0.15996265166373125, 0.0492912282128556, 0.9684866957875502\n\
	);\n\
	\n\
	vec3 refWhite, refWhiteRGB;\n\
	vec3 d, s;\n\
	vec3 RGBtoXYZ(vec3 rgb){\n\
		vec3 xyz, XYZ;\n\
		xyz = matRGBtoXYZ * rgb;\n\
		// adaption\n\
		XYZ = matAdapt * xyz;\n\
		XYZ *= d/s;\n\
		xyz = matAdaptInv * XYZ;\n\
		return xyz;\n\
	}\n\
	\n\
	vec3 XYZtoRGB(vec3 xyz){\n\
		vec3 rgb, RGB;\n\
		// adaption\n\
		RGB = matAdapt * xyz;\n\
		rgb *= s/d;\n\
		xyz = matAdaptInv * RGB;\n\
		rgb = matXYZtoRGB * xyz;\n\
		return rgb;\n\
	}\n\
	\n\
	float Lum(vec3 c){\n\
		return 0.299*c.r + 0.587*c.g + 0.114*c.b;\n\
	}\n\
	\n\
	vec3 ClipColor(vec3 c){\n\
		float l = Lum(c);\n\
		float n = min(min(c.r, c.g), c.b);\n\
		float x = max(max(c.r, c.g), c.b);\n\
		if (n < 0.0) c = (c-l)*l / (l-n) + l;\n\
		if (x > 1.0) c = (c-l) * (1.0-l) / (x-l) + l;\n\
		return c;\n\
	}\n\
	\n\
	vec3 SetLum(vec3 c, float l){\n\
		float d = l - Lum(c);\n\
		c.r = c.r + d;\n\
		c.g = c.g + d;\n\
		c.b = c.b + d;\n\
		return ClipColor(c);\n\
	}\n\
	\n\
	// illuminants\n\
	//vec3 A = vec3(1.09850, 1.0, 0.35585);\n\
	vec3 D50 = vec3(0.96422, 1.0, 0.82521);\n\
	vec3 D65 = vec3(0.95047, 1.0, 1.08883);\n\
	//vec3 D75 = vec3(0.94972, 1.0, 1.22638);\n\
	//vec3 D50 = vec3(0.981443, 1.0, 0.863177);\n\
	//vec3 D65 = vec3(0.968774, 1.0, 1.121774);\n\
	vec3 CCT2K = vec3(1.274335, 1.0, 0.145233);\n\
	vec3 CCT4K = vec3(1.009802, 1.0, 0.644496);\n\
	vec3 CCT20K = vec3(0.995451, 1.0, 1.886109);\n\
	\n\
	void main(void) {\
		const vec3 AvgLum = vec3(0.5, 0.5, 0.5);\
		const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);\
		vec4 tx_colour = texture(uTextureUnit0, vTexCoord0.st).rgba;\
		vec3 brtColor = tx_colour.rgb * uBrightness;\
		vec3 intensity = vec3(dot(brtColor, LumCoeff));\
		vec3 satColor = mix(intensity, brtColor, uSaturation);\
		vec3 conColor = mix(AvgLum, satColor, uContrast);\
		vec3 exposure = conColor * pow(2.0, uExposure);\
		vec3 gammaColor = vec3(pow(exposure.r, uGamma), pow(exposure.g, uGamma), pow(exposure.b, uGamma));\
		\
		vec4 col = vec4(gammaColor, tx_colour.a);\n\
		vec3 to, from;\n\
		if (uTemperature < 0.0) {\n\
			to = CCT20K;\n\
			from = D65;\n\
		} else {\n\
			to = CCT4K;\n\
			from = D65;\n\
		}\n\
		\n\
		vec3 base = col.rgb;\n\
		float lum = Lum(base);\n\
		// mask by luminance\n\
		float temp = abs(uTemperature) * (1.0 - pow(lum, 2.72));\n\
		// from\n\
		refWhiteRGB = from;\n\
		// to\n\
		refWhite = vec3(mix(from.x, to.x, temp), mix(1.0, 0.9, uTint), mix(from.z, to.z, temp));\n\
		// mix based on alpha for local adjustments\n\
		refWhite = mix(refWhiteRGB, refWhite, col.a);\n\
		d = matAdapt * refWhite;\n\
		s = matAdapt * refWhiteRGB;\n\
		vec3 xyz = RGBtoXYZ(base);\n\
		vec3 rgb = XYZtoRGB(xyz);\n\
		// brightness compensation\n\
		vec3 res = rgb * (1.0 + (temp + uTint) / 10.0);\n\
		// preserve luminance\n\
		//vec3 res = SetLum(rgb, lum);\n\
		fFragColour = vec4(mix(base, res, col.a), col.a);\n\
	}";
class ColourGradingShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;
	GLint				fLocation_uSaturation;
	GLint				fLocation_uBrightness;
	GLint				fLocation_uContrast;
	GLint				fLocation_uGamma;
	GLint				fLocation_uExposure;
	GLint				fLocation_uTemperature;
	GLint				fLocation_uTint;

	float				fSaturation;
	float				fBrightness;
	float				fContrast;
	float				fGamma;
	float				fExposure;
	float				fTemperature;
	float				fTint;

public:
	ColourGradingShader(const float saturation = kDefaultSaturation)
		: fSaturation(saturation)
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kVertexShader, kFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
		fLocation_uSaturation = fShader->GetUniformLocation("uSaturation");
		fLocation_uBrightness = fShader->GetUniformLocation("uBrightness");
		fLocation_uContrast = fShader->GetUniformLocation("uContrast");
		fLocation_uGamma = fShader->GetUniformLocation("uGamma");
		fLocation_uExposure = fShader->GetUniformLocation("uExposure");
		fLocation_uTemperature = fShader->GetUniformLocation("uTemperature");
		fLocation_uTint = fShader->GetUniformLocation("uTint");
	}
	~ColourGradingShader()
	{
		delete fShader;
	}
	void SetSaturation(const float saturation)		{fSaturation = saturation;}
	void SetBrightness(const float brightness)		{fBrightness = brightness;}
	void SetContrast(const float contrast)			{fContrast = contrast;}
	void SetGamma(const float gamma)				{fGamma = gamma;}
	void SetExposure(const float exposure)			{fExposure = exposure;}
	void SetTemperature(const float temperature)	{fTemperature = temperature;}
	void SetTint(const float tint)					{fTint = tint;}

	void Render(float)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0, 0);
		glUniform1f(fLocation_uSaturation, fSaturation);
		glUniform1f(fLocation_uBrightness, fBrightness);
		glUniform1f(fLocation_uContrast, fContrast);
		glUniform1f(fLocation_uGamma, fGamma);
		glUniform1f(fLocation_uExposure, fExposure);
		glUniform1f(fLocation_uTemperature, fTemperature);
		glUniform1f(fLocation_uTint, fTint);
	}
};

/*	FUNCTION:		Effect_ColourGrading :: Effect_ColourGrading
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_ColourGrading :: Effect_ColourGrading(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fRenderNode = nullptr;

	const float kFontFactor = be_plain_font->Size()/20.0f;
	const float kScrollBarScale = be_plain_font->Size()/12.0f;
	const float kFrameRight = frame.right - 10 - kScrollBarScale*B_V_SCROLL_BAR_WIDTH;

	fSliderSaturation = new ValueSlider(BRect(10, 10, kFrameRight, 80), "saturation_slider", GetText(TXT_EFFECTS_COLOUR_GRADING_SATURATION), nullptr, 0, 200);
	fSliderSaturation->SetModificationMessage(new BMessage(kMsgValueChanged));
	fSliderSaturation->SetValue(kDefaultSaturation*100);
	fSliderSaturation->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderSaturation->SetHashMarkCount(9);
	fSliderSaturation->SetLimitLabels("0.0", "2.0");
	fSliderSaturation->UpdateTextValue(kDefaultSaturation);
	fSliderSaturation->SetStyle(B_BLOCK_THUMB);
	fSliderSaturation->SetMidpointLabel("1.0");
	fSliderSaturation->SetFloatingPointPrecision(2);
	fSliderSaturation->SetBarColor({255, 0, 0, 255});
	fSliderSaturation->UseFillColor(true);
	mEffectView->AddChild(fSliderSaturation);

	fSliderBrightness = new ValueSlider(BRect(10, 100, kFrameRight, 170), "brightness_slider", GetText(TXT_EFFECTS_COLOUR_GRADING_BRIGHTNESS), nullptr, 0, 200);
	fSliderBrightness->SetModificationMessage(new BMessage(kMsgValueChanged));
	fSliderBrightness->SetValue(kDefaultBrightness*100);
	fSliderBrightness->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderBrightness->SetHashMarkCount(5);
	fSliderBrightness->SetLimitLabels("0.0", "2.0");
	fSliderBrightness->UpdateTextValue(kDefaultBrightness);
	fSliderBrightness->SetStyle(B_BLOCK_THUMB);
	fSliderBrightness->SetMidpointLabel("1.0");
	fSliderBrightness->SetFloatingPointPrecision(2);
	fSliderBrightness->SetBarColor({255, 255, 0, 255});
	fSliderBrightness->UseFillColor(true);
	mEffectView->AddChild(fSliderBrightness);

	fSliderContrast = new ValueSlider(BRect(10, 190, kFrameRight, 260), "contrast_slider", GetText(TXT_EFFECTS_COLOUR_GRADING_CONTRAST), nullptr, 0, 200);
	fSliderContrast->SetModificationMessage(new BMessage(kMsgValueChanged));
	fSliderContrast->SetValue(kDefaultContrast*100);
	fSliderContrast->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderContrast->SetHashMarkCount(5);
	fSliderContrast->SetLimitLabels("0.0", "2.0");
	fSliderContrast->UpdateTextValue(kDefaultContrast);
	fSliderContrast->SetStyle(B_BLOCK_THUMB);
	fSliderContrast->SetMidpointLabel("1.0");
	fSliderContrast->SetFloatingPointPrecision(2);
	fSliderContrast->SetBarColor({0, 255, 0, 255});
	fSliderContrast->UseFillColor(true);
	mEffectView->AddChild(fSliderContrast);

	fSliderGamma = new ValueSlider(BRect(10, 280, kFrameRight, 350), "gamma_slider", GetText(TXT_EFFECTS_COLOUR_GRADING_GAMMA), nullptr, 0, 200);
	fSliderGamma->SetModificationMessage(new BMessage(kMsgValueChanged));
	fSliderGamma->SetValue(kDefaultGamma*100);
	fSliderGamma->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderGamma->SetHashMarkCount(9);
	fSliderGamma->SetLimitLabels("0.0", "2.0");
	fSliderGamma->UpdateTextValue(kDefaultGamma);
	fSliderGamma->SetStyle(B_BLOCK_THUMB);
	fSliderGamma->SetMidpointLabel("1.0");
	fSliderGamma->SetFloatingPointPrecision(2);
	fSliderGamma->SetBarColor({0, 0, 255, 255});
	fSliderGamma->UseFillColor(true);
	mEffectView->AddChild(fSliderGamma);

	fSliderExposure = new ValueSlider(BRect(10, 370, kFrameRight, 440), "exposure_slider", GetText(TXT_EFFECTS_COLOUR_GRADING_EXPOSURE), nullptr, -200, 200);
	fSliderExposure->SetModificationMessage(new BMessage(kMsgValueChanged));
	fSliderExposure->SetValue(kDefaultExposure*100);
	fSliderExposure->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderExposure->SetHashMarkCount(9);
	fSliderExposure->SetLimitLabels("-2.0", "2.0");
	fSliderExposure->UpdateTextValue(kDefaultExposure);
	fSliderExposure->SetStyle(B_BLOCK_THUMB);
	fSliderExposure->SetMidpointLabel("0.0");
	fSliderExposure->SetFloatingPointPrecision(2);
	fSliderExposure->SetBarColor({255, 255, 255, 255});
	fSliderExposure->UseFillColor(true);
	mEffectView->AddChild(fSliderExposure);

	fSliderTemperature = new ValueSlider(BRect(10, 460, kFrameRight, 530), "temperature_slider", GetText(TXT_EFFECTS_COLOUR_GRADING_TEMPERATURE), nullptr, 0, 200);
	fSliderTemperature->SetModificationMessage(new BMessage(kMsgValueChanged));
	fSliderTemperature->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderTemperature->SetHashMarkCount(11);
	fSliderTemperature->SetLimitLabels(GetText(TXT_EFFECTS_COLOUR_GRADING_TEMPERATURE_WARMER), GetText(TXT_EFFECTS_COLOUR_GRADING_TEMPERATURE_COOLER));
	fSliderTemperature->SetStyle(B_BLOCK_THUMB);
	fSliderTemperature->SetMidpointLabel("CCT 6.5K");
	fSliderTemperature->SetFloatingPointPrecision(2);
	fSliderTemperature->UpdateTextValue(0);
	fSliderTemperature->SetValue(100);
	fSliderTemperature->SetBarColor({255, 128, 0, 255});
	fSliderTemperature->UseFillColor(true);
	mEffectView->AddChild(fSliderTemperature);

	fSliderTint = new ValueSlider(BRect(10, 550, kFrameRight, 620), "tint_slider", GetText(TXT_EFFECTS_COLOUR_GRADING_TINT), nullptr, 0, 200);
	fSliderTint->SetModificationMessage(new BMessage(kMsgValueChanged));
	fSliderTint->SetValue(100);
	fSliderTint->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderTint->SetHashMarkCount(11);
	fSliderTint->SetLimitLabels(GetText(TXT_EFFECTS_COLOUR_GRADING_GREEN), GetText(TXT_EFFECTS_COLOUR_GRADING_PINK));
	fSliderTint->UpdateTextValue(0.0f);
	fSliderTint->SetStyle(B_BLOCK_THUMB);
	fSliderTint->SetMidpointLabel("0.0");
	fSliderTint->SetFloatingPointPrecision(2);
	fSliderTint->SetBarColor({0, 255, 255, 255});
	fSliderTint->UseFillColor(true);
	mEffectView->AddChild(fSliderTint);

	fButtonReset = new BButton(BRect(430*kFontFactor, 650, kFrameRight, 690), "button_reset", GetText(TXT_EFFECTS_COMMON_RESET), new BMessage(kMsgReset));
	mEffectView->AddChild(fButtonReset);

	SetViewIdealSize(frame.Width() + 40, 740);
}

/*	FUNCTION:		Effect_ColourGrading :: ~Effect_ColourGrading
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_ColourGrading :: ~Effect_ColourGrading()
{ }

/*	FUNCTION:		Effect_ColourGrading :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_ColourGrading :: AttachedToWindow()
{
	fSliderSaturation->SetTarget(this, Window());
	fSliderBrightness->SetTarget(this, Window());
	fSliderContrast->SetTarget(this, Window());
	fSliderGamma->SetTarget(this, Window());
	fSliderExposure->SetTarget(this, Window());
	fSliderTemperature->SetTarget(this, Window());
	fSliderTint->SetTarget(this, Window());

	fButtonReset->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Saturation :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ColourGrading :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new ColourGradingShader;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kFadeGeometry, 4);
	fRenderNode->mTexture = new YTexture(width, height);
}

/*	FUNCTION:		Effect_ColourGrading :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ColourGrading :: DestroyRenderObjects()
{
	delete fRenderNode;
	fRenderNode = nullptr;
}

/*	FUNCTION:		Effect_ColourGrading :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_ColourGrading :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_ColourGrading.png");
	return icon;	
}

/*	FUNCTION:		Effect_ColourGrading :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_ColourGrading :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_GRADING);
}

/*	FUNCTION:		Effect_ColourGrading :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_ColourGrading :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_GRADING_TEXT_A);
}

/*	FUNCTION:		Effect_ColourGrading :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_ColourGrading :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_GRADING_TEXT_B);
}

/*	FUNCTION:		Effect_ColourGrading :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_ColourGrading :: CreateMediaEffect()
{
	const float width = gProject->mResolution.width;
	const float height = gProject->mResolution.height;

	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectColourGradingData *data = new EffectColourGradingData;
	data->saturation = kDefaultSaturation;
	data->brightness = kDefaultBrightness;
	data->contrast = kDefaultContrast;
	data->gamma = kDefaultGamma;
	data->exposure = kDefaultExposure;
	data->temperature = kDefaultTemperature;
	data->tint = kDefaultTint;
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_ColourGrading :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_ColourGrading :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
	EffectColourGradingData *data = (EffectColourGradingData *)effect->mEffectData;

	fSliderSaturation->SetValue(data->saturation*100);
	fSliderSaturation->UpdateTextValue(data->saturation);

	fSliderBrightness->SetValue(data->brightness*100);
	fSliderBrightness->UpdateTextValue(data->brightness);

	fSliderContrast->SetValue(data->contrast*100);
	fSliderContrast->UpdateTextValue(data->contrast);

	fSliderGamma->SetValue(data->gamma*100);
	fSliderGamma->UpdateTextValue(data->gamma);

	fSliderExposure->SetValue(data->exposure*100);
	fSliderExposure->UpdateTextValue(data->exposure);

	fSliderTemperature->SetValue(100 + 100.0f*data->temperature);
	fSliderTemperature->UpdateTextValue(data->temperature);

	fSliderTint->SetValue(100 + 100.0f*data->tint);
	fSliderTint->UpdateTextValue(data->tint);
}

/*	FUNCTION:		Effect_ColourGrading :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_ColourGrading :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	float t = float(frame_idx - data->mTimelineFrameStart)/float(data->Duration());
	if (t > 1.0f)
		t = 1.0f;
	EffectColourGradingData *effect_data = (EffectColourGradingData *)data->mEffectData;
	ColourGradingShader *shader = (ColourGradingShader *) fRenderNode->mShaderNode;
	shader->SetSaturation(effect_data->saturation);
	shader->SetBrightness(effect_data->brightness);
	shader->SetContrast(effect_data->contrast);
	shader->SetGamma(effect_data->gamma);
	shader->SetExposure(effect_data->exposure);
	shader->SetTemperature(effect_data->temperature);
	shader->SetTint(effect_data->tint);

	fRenderNode->mTexture->Upload(source);
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_ColourGrading :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_ColourGrading :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgReset:
			fSliderSaturation->SetValue(kDefaultSaturation*100);
			fSliderBrightness->SetValue(kDefaultBrightness*100);
			fSliderContrast->SetValue(kDefaultContrast*100);
			fSliderGamma->SetValue(kDefaultGamma*100);
			fSliderExposure->SetValue(kDefaultExposure*100);
			fSliderTemperature->SetValue(100 + 100.0f*kDefaultTemperature);
			fSliderTint->SetValue(100 + 100.0f*kDefaultTint);
			//	fall through

		case kMsgValueChanged:
			fSliderSaturation->UpdateTextValue(fSliderSaturation->Value()/100.0f);
			fSliderBrightness->UpdateTextValue(fSliderBrightness->Value()/100.0f);
			fSliderContrast->UpdateTextValue(fSliderContrast->Value()/100.0f);
			fSliderGamma->UpdateTextValue(fSliderGamma->Value()/100.0f);
			fSliderExposure->UpdateTextValue(fSliderExposure->Value()/100.0f);
			fSliderTemperature->UpdateTextValue((-100.0f + fSliderTemperature->Value())/100.0f);
			fSliderTint->UpdateTextValue((-100.0f + fSliderTint->Value())/100.0f);
			if (GetCurrentMediaEffect())
			{
				EffectColourGradingData *data = (EffectColourGradingData *)GetCurrentMediaEffect()->mEffectData;
				assert(data);
				data->saturation = fSliderSaturation->Value()/100.0f;
				data->brightness = fSliderBrightness->Value()/100.0f;
				data->contrast = fSliderContrast->Value()/100.0f;
				data->gamma = fSliderGamma->Value()/100.0f;
				data->exposure = fSliderExposure->Value()/100.0f;
				data->temperature = (-100.0f + fSliderTemperature->Value())/100.0f;
				data->tint = (-100.0f + fSliderTint->Value())/100.0f;
				InvalidatePreview();
			}
			else
				printf("Effect_ColourGrading::MessageReceived(kMsgValueChanged) - no selected effect\n");
			break;

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_ColourGrading :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_ColourGrading :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	EffectColourGradingData *data = (EffectColourGradingData *) media_effect->mEffectData;

	//	saturation
	if (v.HasMember("saturation") && v["saturation"].IsFloat())
	{
		data->saturation = v["saturation"].GetFloat();
		if (data->saturation < 0.0f)
			data->saturation = 0.0f;
		if (data->saturation > 2.0f)
			data->saturation = 2.0f;
	}

	//	brightness
	if (v.HasMember("brightness") && v["brightness"].IsFloat())
	{
		data->brightness = v["brightness"].GetFloat();
		if (data->brightness < 0.0f)
			data->brightness = 0.0f;
		if (data->brightness > 2.0f)
			data->brightness = 2.0f;
	}

	//	contrast
	if (v.HasMember("contrast") && v["contrast"].IsFloat())
	{
		data->contrast = v["contrast"].GetFloat();
		if (data->contrast < 0.0f)
			data->contrast = 0.0f;
		if (data->contrast > 2.0f)
			data->contrast = 2.0f;
	}

	//	gamma
	if (v.HasMember("gamma") && v["gamma"].IsFloat())
	{
		data->gamma = v["gamma"].GetFloat();
		if (data->gamma < 0.0f)
			data->gamma = 0.0f;
		if (data->gamma > 2.0f)
			data->gamma = 2.0f;
	}

	//	exposure
	if (v.HasMember("exposure") && v["exposure"].IsFloat())
	{
		data->exposure = v["exposure"].GetFloat();
		if (data->exposure < -5.0f)
			data->exposure = -5.0f;
		if (data->exposure > 5.0f)
			data->exposure = 5.0f;
	}

	//	temperature
	if (v.HasMember("temperature") && v["temperature"].IsFloat())
	{
		data->temperature = v["temperature"].GetFloat();
		if (data->temperature < -1.0f)
			data->temperature = -1.0f;
		else if (data->temperature > 1.0f)
			data->temperature = 1.0f;
	}

	//	tint
	if (v.HasMember("tint") && v["tint"].IsFloat())
	{
		data->tint = v["tint"].GetFloat();
		if (data->tint < -1.0f)
			data->tint = -1.0f;
		else if (data->tint > 1.0f)
			data->tint = 1.0f;
	}

	return true;
}

/*	FUNCTION:		Effect_ColourGrading :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_ColourGrading :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectColourGradingData *data = (EffectColourGradingData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"saturation\": %0.2f,\n", data->saturation);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"brightness\": %0.2f,\n", data->brightness);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"contrast\": %0.2f,\n", data->contrast);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"gamma\": %0.2f,\n", data->gamma);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"exposure\": %0.2f,\n", data->exposure);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"temperature\": \"%f\",\n", data->temperature);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"tint\": \"%f\"\n", data->tint);
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
