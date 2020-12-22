/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Portrait Blur
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <translation/TranslationUtils.h>
#include <LayoutBuilder.h>

#include "Yarra/Render/Texture.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"
#include "Yarra/Render/Picture.h"

#include "Editor/EffectNode.h"
#include "Editor/ImageUtility.h"
#include "Editor/Language.h"
#include "Editor/Project.h"
#include "Editor/RenderActor.h"

#include "Gui/BitmapButton.h"
#include "Gui/Spinner.h"
#include "Gui/ValueSlider.h"

#include "Effect_PortraitBlur.h"
#include "Effect_Blur.h"

const char * Effect_PortraitBlur :: GetVendorName() const	{return "ZenYes";}
const char * Effect_PortraitBlur :: GetEffectName() const	{return "Portrait Blur";}

static const uint32 kMsgPortraitPosX		= 'epsx';
static const uint32 kMsgPortraitPosY		= 'epsy';
static const uint32 kMsgPortraitPosZ		= 'epsz';
static const uint32 kMsgBlurPosX			= 'epex';
static const uint32 kMsgBlurPosY			= 'epey';
static const uint32 kMsgBlurPosZ			= 'epez';

static const uint32 kMsgPortraitRotX		= 'ersx';
static const uint32 kMsgPortraitRotY		= 'ersy';
static const uint32 kMsgPortraitRotZ		= 'ersz';
static const uint32 kMsgBlurRotX			= 'erex';
static const uint32 kMsgBlurRotY			= 'erey';
static const uint32 kMsgBlurRotZ			= 'erez';

static const uint32 kMsgPortraitScaleX		= 'essx';
static const uint32 kMsgPortraitScaleY		= 'essy';
static const uint32 kMsgPortraitScaleZ		= 'essz';
static const uint32 kMsgBlurScaleX			= 'esex';
static const uint32 kMsgBlurScaleY			= 'esey';
static const uint32 kMsgBlurScaleZ			= 'esez';

static const uint32 kMsgIncrement			= 'einc';
static const uint32 kMsgBlurSlider			= 'eblr';

struct SPINNER_LAYOUT
{
	const BRect		rect;
	const char		*id;
	LANGUAGE_TEXT	text;
	const char		*label;
	const float		min_value;
	const float		max_value;
	const bool		enabled;
	const uint32	message;
};
enum SPINNERS {
	SPINNER_POSITION_X, SPINNER_POSITION_Y, SPINNER_POSITION_Z,
	SPINNER_ROTATION_X, SPINNER_ROTATION_Y, SPINNER_ROTATION_Z,
	SPINNER_SCALE_X, SPINNER_SCALE_Y, SPINNER_SCALE_Z,
	NUMBER_SPINNERS,
};
static const SPINNER_LAYOUT kPortraitSpinnerLayouts[] =
{
	{BRect(10, 30, 200, 60),		"spos_x",		TXT_EFFECTS_COMMON_POSITION,	" X",	-10000,		10000,	true,	kMsgPortraitPosX},
	{BRect(10, 70, 200, 100),		"spos_y",		TXT_EFFECTS_COMMON_POSITION,	" Y",	-10000,		10000,	true,	kMsgPortraitPosY},
	{BRect(10, 110, 200, 140),		"spos_z",		TXT_EFFECTS_COMMON_POSITION,	" Z",	-10000,		10000,	false,	kMsgPortraitPosZ},
	{BRect(230, 30, 410, 60),		"srot_x",		TXT_EFFECTS_COMMON_ROTATION,	" X",	-10000,		10000,	true,	kMsgPortraitRotX},
	{BRect(230, 70, 410, 100),		"srot_y",		TXT_EFFECTS_COMMON_ROTATION,	" Y",	-10000,		10000,	true,	kMsgPortraitRotY},
	{BRect(230, 110, 410, 140),		"srot_z",		TXT_EFFECTS_COMMON_ROTATION,	" Z",	-10000,		10000,	true,	kMsgPortraitRotZ},
	{BRect(440, 30, 620, 60),		"sscale_x",		TXT_EFFECTS_COMMON_SCALE,		" X",	-10000,		10000,	true,	kMsgPortraitScaleX},
	{BRect(440, 70, 620, 100),		"sscale_y",		TXT_EFFECTS_COMMON_SCALE,		" Y",	-10000,		10000,	true,	kMsgPortraitScaleY},
	{BRect(440, 110, 620, 140),		"sscale_z",		TXT_EFFECTS_COMMON_SCALE,		" Z",	-10000,		10000,	false,	kMsgPortraitScaleZ},
};
static const SPINNER_LAYOUT kBlurSpinnerLayouts[] =
{			
	{BRect(10, 30, 200, 60),		"epos_x",		TXT_EFFECTS_COMMON_POSITION,	" X",	-10000,		10000,	true,	kMsgBlurPosX},
	{BRect(10, 70, 200, 100),		"epos_y",		TXT_EFFECTS_COMMON_POSITION,	" Y",	-10000,		10000,	true,	kMsgBlurPosY},
	{BRect(10, 110, 200, 140),		"epos_z",		TXT_EFFECTS_COMMON_POSITION,	" Z",	-10000,		10000,	false,	kMsgBlurPosZ},
	{BRect(230, 30, 410, 60),		"erot_x",		TXT_EFFECTS_COMMON_ROTATION,	" X",	-10000,		10000,	true,	kMsgBlurRotX},
	{BRect(230, 70, 410, 100),		"erot_y",		TXT_EFFECTS_COMMON_ROTATION,	" Y",	-10000,		10000,	true,	kMsgBlurRotY},
	{BRect(230, 110, 410, 140),		"erot_z",		TXT_EFFECTS_COMMON_ROTATION,	" Z",	-10000,		10000,	true,	kMsgBlurRotZ},
	{BRect(440, 30, 620, 60),		"escale_x",		TXT_EFFECTS_COMMON_SCALE,		" X",	-10000,		10000,	true,	kMsgBlurScaleX},
	{BRect(440, 70, 620, 100),		"escale_y",		TXT_EFFECTS_COMMON_SCALE,		" Y",	-10000,		10000,	true,	kMsgBlurScaleY},
	{BRect(440, 110, 620, 140),		"escale_z",		TXT_EFFECTS_COMMON_SCALE,		" Z",	-10000,		10000,	false,	kMsgBlurScaleZ},
};
static_assert(sizeof(kPortraitSpinnerLayouts)/sizeof(SPINNER_LAYOUT) == NUMBER_SPINNERS, "sizeof(kStartSpinnerLayouts) != NUMBER_SPINNERS");
static_assert(sizeof(kBlurSpinnerLayouts)/sizeof(SPINNER_LAYOUT) == NUMBER_SPINNERS, "sizeof(kBlurSpinnerLayouts) != NUMBER_SPINNERS");

static float kIncrementPopupValues[] = 
{
	0.01f, 0.1f, 1.0f, 10.0f
};

static const char *kInterpolation[] = 
{
	"Linear",
	"Cosine",
	"Acceleration",
	"Deceleration",
};

static const BRect kIdealWindowSize(0, 0, 682 + B_V_SCROLL_BAR_WIDTH, 440 + B_H_SCROLL_BAR_HEIGHT);

static const YGeometry_P3T2 kBlurGeometry[] =
{
	{-1, -1, 0,		0, 0},
	{1, -1, 0,		1, 0},
	{-1, 1, 0,		0, 1},
	{1, 1, 0,		1, 1},
};
static const float kDefaultBlurDirection = 4.5f;

/************************
	Blur Shader
*************************/
static const char *kVertexShader_Blur = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture0;\
	out vec2		vTexCoord0;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);\
		vTexCoord0 = aTexture0;\
	}";

static const char *kFragmentShader_Blur = "\
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

class PortraitBlurShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;
	GLint				fLocation_uDirection;
	GLint				fLocation_uResolution;

	ymath::YVector2		fDirection;
	ymath::YVector2		fResolution;

public:
	PortraitBlurShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new yrender::YShader(&attributes, kVertexShader_Blur, kFragmentShader_Blur);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
		fLocation_uDirection = fShader->GetUniformLocation("uDirection");
		fLocation_uResolution = fShader->GetUniformLocation("uResolution");

		fDirection = ymath::YVector2(kDefaultBlurDirection, kDefaultBlurDirection);
		fResolution.Set(gProject->mResolution.width, gProject->mResolution.height);
	}
	~PortraitBlurShader()
	{
		delete fShader;
	}
	void	SetDirection(const float x, const float y) {fDirection.Set(x, y);}
	void	SetResolution(const float width, const float height) {fResolution.Set(width, height);}
	void	Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0, 0);
		glUniform2fv(fLocation_uResolution, 1, fResolution.v);
		glUniform2fv(fLocation_uDirection, 1, fDirection.v);
	}
};


//	Transform Data
struct EffectPortraitBlurData
{
	ymath::YVector3		portrait_position;
	ymath::YVector3		portrait_rotation;
	ymath::YVector3		portrait_scale;
	ymath::YVector3		blur_position;
	ymath::YVector3		blur_rotation;
	ymath::YVector3		blur_scale;
	int					interpolation;
	float				blur_direction;
	
	void Debug() 
	{
		printf("portrait_position:");	portrait_position.PrintToStream();
		printf("portrait_rotation:");	portrait_rotation.PrintToStream();
		printf("portrait_scale:");		portrait_scale.PrintToStream();
		printf("blur_position:");		blur_position.PrintToStream();
		printf("blur_rotation:");		blur_rotation.PrintToStream();
		printf("blur_scale:");			blur_scale.PrintToStream();
		printf("blur direction: = %f\n", blur_direction);
	}
};

using namespace yrender;

/************************
	Effect_PortraitBlur
*************************/

/*	FUNCTION:		Effect_PortraitBlur :: Effect_PortraitBlur
	ARGS:			frame
					filename
					is_window
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_PortraitBlur :: Effect_PortraitBlur(BRect frame, const char *filename)
	: EffectNode(frame, filename), fBlurRenderNode(nullptr)
{
	//	Increment Popup
	BOptionPopUp *increment_popup = new BOptionPopUp(BRect(10, 380, 240, 440), "increment", GetText(TXT_EFFECTS_COMMON_INCREMENT), new BMessage(kMsgIncrement));
	for (int i=0; i < sizeof(kIncrementPopupValues)/sizeof(float); i++)
	{
		char buffer[20];
		sprintf(buffer, "%0.2f", kIncrementPopupValues[i]);
		increment_popup->AddOption(buffer, i);	
	}
	const int kIncrementPopupDefaultIndex = 3;
	increment_popup->SelectOptionFor(kIncrementPopupDefaultIndex);
	mEffectView->AddChild(increment_popup);

	//	BlurSlider
	fBlurSlider = new ValueSlider(BRect(10, 440, 480, 500), "blur_slider", GetText(TXT_EFFECTS_IMAGE_PORTRAIT_BLUR_AMOUNT), nullptr, 0, 200);
	fBlurSlider->SetModificationMessage(new BMessage(kMsgBlurSlider));
	fBlurSlider->SetValue(kDefaultBlurDirection*10);
	fBlurSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fBlurSlider->SetHashMarkCount(20);
	fBlurSlider->SetLimitLabels("0.0", "20.0");
	fBlurSlider->UpdateTextValue(kDefaultBlurDirection);
	mEffectView->AddChild(fBlurSlider);
	
	//	Start transform
	BBox *start_box = new BBox(BRect(10, 10, 640, 170), "box_start");
	start_box->SetLabel(GetText(TXT_EFFECTS_IMAGE_PORTRAIT_BLUR_TRANSFORM));
	mEffectView->AddChild(start_box);
	for (int i=0; i < sizeof(kPortraitSpinnerLayouts)/sizeof(SPINNER_LAYOUT); i++)
	{
		char buffer[32];
		sprintf(buffer, "%s%s", GetText(kPortraitSpinnerLayouts[i].text), kPortraitSpinnerLayouts[i].label);
		Spinner *spinner = new Spinner(kPortraitSpinnerLayouts[i].rect, kPortraitSpinnerLayouts[i].id, buffer,
										new BMessage(kPortraitSpinnerLayouts[i].message));
		spinner->SetRange(kPortraitSpinnerLayouts[i].min_value, kPortraitSpinnerLayouts[i].max_value);
		spinner->SetValue(0);
		spinner->SetSteps(kIncrementPopupValues[kIncrementPopupDefaultIndex]);
		start_box->AddChild(spinner);
		spinner->SetEnabled(kPortraitSpinnerLayouts[i].enabled);
	}
	//	Blur transform
	BBox *blur_box = new BBox(BRect(10, 200, 640, 360), "box_blur");
	blur_box->SetLabel(GetText(TXT_EFFECTS_IMAGE_PORTRAIT_BLUR_BACKGROUND_TRANSFORM));
	mEffectView->AddChild(blur_box);
	for (int i=0; i < sizeof(kBlurSpinnerLayouts)/sizeof(SPINNER_LAYOUT); i++)
	{
		char buffer[32];
		sprintf(buffer, "%s%s", GetText(kBlurSpinnerLayouts[i].text), kBlurSpinnerLayouts[i].label);
		Spinner *spinner = new Spinner(kBlurSpinnerLayouts[i].rect, kBlurSpinnerLayouts[i].id, buffer,
										new BMessage(kBlurSpinnerLayouts[i].message));
		spinner->SetRange(kBlurSpinnerLayouts[i].min_value, kBlurSpinnerLayouts[i].max_value);
		spinner->SetValue(0);
		spinner->SetSteps(kIncrementPopupValues[kIncrementPopupDefaultIndex]);
		blur_box->AddChild(spinner);
		spinner->SetEnabled(kBlurSpinnerLayouts[i].enabled);
	}
}

/*	FUNCTION:		Effect_PortraitBlur :: ~Effect_PortraitBlur
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_PortraitBlur :: ~Effect_PortraitBlur()
{
}

/*	FUNCTION:		Effect_PortraitBlur :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_PortraitBlur :: AttachedToWindow()
{
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_POSITION_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_POSITION_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_POSITION_Z].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_ROTATION_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_SCALE_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_SCALE_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_SCALE_Z].id))->SetTarget(this, Window());

	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_POSITION_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_POSITION_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_POSITION_Z].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_ROTATION_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_SCALE_X].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_SCALE_Y].id))->SetTarget(this, Window());
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_SCALE_Z].id))->SetTarget(this, Window());

	dynamic_cast<BOptionPopUp *>(FindView("increment"))->SetTarget(this, Window());
	fBlurSlider->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_PortraitBlur :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_PortraitBlur :: InitRenderObjects()
{
	assert(fBlurRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fBlurRenderNode = new yrender::YRenderNode;
	fBlurRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fBlurRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.75f*height, 0));
	fBlurRenderNode->mShaderNode = new PortraitBlurShader;
	fBlurRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kBlurGeometry, 4);
	fBlurRenderNode->mTexture = new YTexture(width, height);//, YTexture::YTF_REPEAT);
}

/*	FUNCTION:		Effect_PortraitBlur :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_PortraitBlur :: DestroyRenderObjects()
{
	delete fBlurRenderNode;
}

/*	FUNCTION:		Effect_PortraitBlur :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_PortraitBlur :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_PortraitBlur.png");
	return icon;	
}

/*	FUNCTION:		Effect_PortraitBlur :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_PortraitBlur :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_PORTRAIT_BLUR);
}

/*	FUNCTION:		Effect_PortraitBlur :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_PortraitBlur :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_PORTRAIT_BLUR_TEXT_A);
}

/*	FUNCTION:		Effect_PortraitBlur :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_PortraitBlur :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_PORTRAIT_BLUR_TEXT_B);
}

/*	FUNCTION:		Effect_PortraitBlur :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_PortraitBlur :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectPortraitBlurData *data = new EffectPortraitBlurData;

	const float scale_x = 1.5f;
	const float scale_y = (float)gProject->mResolution.width / (float)gProject->mResolution.height;
	data->portrait_position.Set(0.5f*gProject->mResolution.width, (0.5f+(0.125f*scale_x))*gProject->mResolution.height, 0);
	data->portrait_rotation.Set(0, 0, 90);
	data->portrait_scale.Set(scale_x, scale_y, 1);

	data->blur_position.Set(0.5f*gProject->mResolution.width, (0.5f+(0.125f*scale_x))*gProject->mResolution.height, 0);
	data->blur_rotation.Set(0, 0, 90);
	data->blur_scale.Set(scale_x, scale_x, 1);	//	Maintain proportion

	data->blur_direction = kDefaultBlurDirection;

	media_effect->mEffectData = data;
	return media_effect;	
}

/*	FUNCTION:		Effect_PortraitBlur :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_PortraitBlur :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;
	
	//	Update GUI
	EffectPortraitBlurData *data = (EffectPortraitBlurData *) effect->mEffectData;
	
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_POSITION_X].id))->SetValue(data->portrait_position.x);
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_POSITION_Y].id))->SetValue(data->portrait_position.y);
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_POSITION_Z].id))->SetValue(data->portrait_position.z);
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_ROTATION_X].id))->SetValue(data->portrait_rotation.x);
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetValue(data->portrait_rotation.y);
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetValue(data->portrait_rotation.z);
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_SCALE_X].id))->SetValue(data->portrait_scale.x);
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_SCALE_Y].id))->SetValue(data->portrait_scale.y);
	dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_SCALE_Z].id))->SetValue(data->portrait_scale.z);
	
	
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_POSITION_X].id))->SetValue(data->blur_position.x);
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_POSITION_Y].id))->SetValue(data->blur_position.y);
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_POSITION_Z].id))->SetValue(data->blur_position.z);
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_ROTATION_X].id))->SetValue(data->blur_rotation.x);
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_ROTATION_Y].id))->SetValue(data->blur_rotation.y);
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_ROTATION_Z].id))->SetValue(data->blur_rotation.z);
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_SCALE_X].id))->SetValue(data->blur_scale.x);
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_SCALE_Y].id))->SetValue(data->blur_scale.y);
	dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_SCALE_Z].id))->SetValue(data->blur_scale.z);

	fBlurSlider->SetValue(data->blur_direction * 10);
}

/*	FUNCTION:		Effect_PortraitBlur :: ChainedSpatialTransform
	ARGS:			data
					frame_idx
	RETURN:			n/a
	DESCRIPTION:	Chained spatial transform
*/
void Effect_PortraitBlur :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	EffectPortraitBlurData *portrait_data = (EffectPortraitBlurData *) data->mEffectData;

	unsigned int w = source->Bounds().IntegerWidth() + 1;
	unsigned int h = source->Bounds().IntegerHeight() + 1;
	yrender::YPicture *picture = gRenderActor->GetPicture(w, h, source);
	float aspect = (float)gProject->mResolution.width / (float)gProject->mResolution.height;

	fBlurRenderNode->mSpatial.SetPosition(YVector3(0.5f*gProject->mResolution.width, 0.5f*gProject->mResolution.height, 0));
	fBlurRenderNode->mSpatial.SetRotation(YVector3(0.0f, 0.0f, 0.0f));
	fBlurRenderNode->mSpatial.SetScale(YVector3(0.5f*gProject->mResolution.width, 0.5f*gProject->mResolution.height, 0));

	PortraitBlurShader *blur_shader = (PortraitBlurShader *) fBlurRenderNode->mShaderNode;
	blur_shader->SetResolution(gProject->mResolution.width, gProject->mResolution.height);

	//	Horizontal blur
	blur_shader->SetDirection(portrait_data->blur_direction, 0.0f);
	fBlurRenderNode->mTexture->Upload(source);
	gRenderActor->ActivateSecondaryRenderBuffer();
	fBlurRenderNode->Render(0.0f);
	gRenderActor->DeactivateSecondaryRenderBuffer();
	fBlurRenderNode->mTexture->Upload(gRenderActor->GetSecondaryFrameBufferTexture());

	//	Repeat the horizontal blur (looks nicer)
	gRenderActor->ActivateSecondaryRenderBuffer();
	fBlurRenderNode->Render(0.0f);
	gRenderActor->DeactivateSecondaryRenderBuffer();
	fBlurRenderNode->mTexture->Upload(gRenderActor->GetSecondaryFrameBufferTexture());

	//	Rotation blur
	blur_shader->SetDirection(0.0f, portrait_data->blur_direction);
	fBlurRenderNode->mSpatial.SetPosition(portrait_data->blur_position);
	fBlurRenderNode->mSpatial.SetRotation(portrait_data->blur_rotation);
	fBlurRenderNode->mSpatial.SetScale(YVector3(0.5f*portrait_data->blur_scale.x*gProject->mResolution.height,
												0.5f*portrait_data->blur_scale.y*gProject->mResolution.height,
												1));
	//blur_shader->SetDirection(0.0f, portrait_data->blur_direction);
	fBlurRenderNode->Render(0.0f);

	picture->mSpatial.SetPosition(portrait_data->portrait_position);
	picture->mSpatial.SetRotation(portrait_data->portrait_rotation);
	picture->mSpatial.SetScale(YVector3(0.5f*portrait_data->portrait_scale.x*gProject->mResolution.height,
										0.5f*portrait_data->portrait_scale.y*gProject->mResolution.height/aspect,
										1));
	picture->Render(0.0f);
}


/*	FUNCTION:		Effect_PortraitBlur :: MessageReceived
	ARGS:			msg
	RETURN:			none
	DESCRIPTION:	Process view messages
*/
void Effect_PortraitBlur :: MessageReceived(BMessage *msg)
{
	uint32 modify_mask = 0;
	switch (msg->what)
	{
		case kMsgPortraitPosX:		modify_mask |= (1 << 0);		break;
		case kMsgPortraitPosY:		modify_mask |= (1 << 1);		break;
		case kMsgPortraitPosZ:		modify_mask |= (1 << 2);		break;
		case kMsgPortraitRotX:		modify_mask |= (1 << 3);		break;
		case kMsgPortraitRotY:		modify_mask |= (1 << 4);		break;
		case kMsgPortraitRotZ:		modify_mask |= (1 << 5);		break;
		case kMsgPortraitScaleX:	modify_mask |= (1 << 6);		break;
		case kMsgPortraitScaleY:	modify_mask |= (1 << 7);		break;
		case kMsgPortraitScaleZ:	modify_mask |= (1 << 8);		break;
		
		case kMsgBlurPosX:			modify_mask |= (1 << 9);		break;
		case kMsgBlurPosY:			modify_mask |= (1 << 10);		break;
		case kMsgBlurPosZ:			modify_mask |= (1 << 11);		break;
		case kMsgBlurRotX:			modify_mask |= (1 << 12);		break;
		case kMsgBlurRotY:			modify_mask |= (1 << 13);		break;
		case kMsgBlurRotZ:			modify_mask |= (1 << 14);		break;
		case kMsgBlurScaleX:		modify_mask |= (1 << 15);		break;
		case kMsgBlurScaleY:		modify_mask |= (1 << 16);		break;
		case kMsgBlurScaleZ:		modify_mask |= (1 << 17);		break;
		
		case kMsgIncrement:	
		{
			int32 value = 0;
			if (msg->FindInt32("be:value", &value) == B_OK)
			{
				for (int i=0; i < sizeof(kPortraitSpinnerLayouts)/sizeof(SPINNER_LAYOUT); i++)
				{
					BView *view = FindView(kPortraitSpinnerLayouts[i].id);
					Spinner *spinner = dynamic_cast<Spinner *>(view);
					if (spinner)
						spinner->SetSteps(kIncrementPopupValues[value]);
				}
				for (int i=0; i < sizeof(kBlurSpinnerLayouts)/sizeof(SPINNER_LAYOUT); i++)
				{
					BView *view = FindView(kBlurSpinnerLayouts[i].id);
					Spinner *spinner = dynamic_cast<Spinner *>(view);
					if (spinner)
						spinner->SetSteps(kIncrementPopupValues[value]);
				}
			}
			break;
		}

		case kMsgBlurSlider:
		{
			fBlurSlider->UpdateTextValue(fBlurSlider->Value()/10.0f);
			if (GetCurrentMediaEffect())
			{
				EffectPortraitBlurData *data = (EffectPortraitBlurData *) GetCurrentMediaEffect()->mEffectData;
				data->blur_direction = fBlurSlider->Value() / 10.0f;
				InvalidatePreview();
			}
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

	EffectPortraitBlurData *transform = (EffectPortraitBlurData *) GetCurrentMediaEffect()->mEffectData;
	assert(transform != nullptr);

	//	Start position
	if (modify_mask & 0b0000'0000'0000'0000'0111)
	{
		transform->portrait_position.Set(dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_POSITION_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_POSITION_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_POSITION_Z].id))->Value());
	}
	//	Start rotation
	if (modify_mask & 0b0000'0000'0000'0011'1000)
	{
		transform->portrait_rotation.Set(dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_ROTATION_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_ROTATION_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_ROTATION_Z].id))->Value());
	}
	//	Start scale
	if (modify_mask & 0b0000'0000'0001'1100'0000)
	{
		transform->portrait_scale.Set(dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_SCALE_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_SCALE_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kPortraitSpinnerLayouts[SPINNER_SCALE_Z].id))->Value());
	}
	//	End position
	if (modify_mask & 0b0000'0000'1110'0000'0000)
	{
		transform->blur_position.Set(dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_POSITION_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_POSITION_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_POSITION_Z].id))->Value());
	}
	//	End rotation
	if (modify_mask & 0b0000'0111'0000'0000'0000)
	{
		transform->blur_rotation.Set(dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_ROTATION_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_ROTATION_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_ROTATION_Z].id))->Value());
	}
	//	End scale
	if (modify_mask & 0b0011'1000'0000'0000'0000)
	{
		transform->blur_scale.Set(dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_SCALE_X].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_SCALE_Y].id))->Value(),
								dynamic_cast<Spinner *>(FindView(kBlurSpinnerLayouts[SPINNER_SCALE_Z].id))->Value());
	}

	InvalidatePreview();
}	

/*	FUNCTION:		Effect_PortraitBlur :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_PortraitBlur :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = true;	
	EffectPortraitBlurData *data = (EffectPortraitBlurData *) media_effect->mEffectData;
	memset(media_effect->mEffectData, 0, sizeof(EffectPortraitBlurData));
	
	//	parse data function
	auto parse = [&v, &valid](const char *p, ymath::YVector3 &vec)
	{
		if (v.HasMember(p) && v[p].IsArray())
		{
			const rapidjson::Value &arr = v[p];
			rapidjson::SizeType num_elems = arr.GetArray().Size();
			vec.x = arr.GetArray()[0].GetFloat();
			vec.y = arr.GetArray()[1].GetFloat();
			vec.z = arr.GetArray()[2].GetFloat();
		}
		else
		{
			printf("Effect_PortraitBlur[ZenYes::PortraitBlur] Missing parameter %s\n", p);
			valid = false;
		}
	};
	parse("start position",	data->portrait_position);
	parse("start rotation",	data->portrait_rotation);
	parse("start scale",	data->portrait_scale);
	parse("blur position",	data->blur_position);
	parse("blur rotation",	data->blur_rotation);
	parse("blur scale",		data->blur_scale);
		
	return valid;
}

/*	FUNCTION:		Effect_PortraitBlur :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Save project data
*/
bool Effect_PortraitBlur :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectPortraitBlurData *d = (EffectPortraitBlurData *) media_effect->mEffectData;
	if (d)
	{
		char buffer[0x80];	//	128 bytes
		sprintf(buffer, "\t\t\t\t\"start position\": [%f, %f, %f],\n", d->portrait_position.x, d->portrait_position.y, d->portrait_position.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"start rotation\": [%f, %f, %f],\n", d->portrait_rotation.x, d->portrait_rotation.y, d->portrait_rotation.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"start scale\": [%f, %f, %f],\n", d->portrait_scale.x, d->portrait_scale.y, d->portrait_scale.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"blur position\": [%f, %f, %f],\n", d->blur_position.x, d->blur_position.y, d->blur_position.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"blur rotation\": [%f, %f, %f],\n", d->blur_rotation.x, d->blur_rotation.y, d->blur_rotation.z);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"blur scale\": [%f, %f, %f]\n", d->blur_scale.x, d->blur_scale.y, d->blur_scale.z);
		fwrite(buffer, strlen(buffer), 1, file);
	}
	return true;	
}
