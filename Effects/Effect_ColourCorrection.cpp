/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Colour Correction
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/Texture.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"

#include "Gui/CurvesView.h"
#include "Gui/BitmapCheckbox.h"
#include "Gui/Magnify.h"
#include "Gui/ValueSlider.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/Project.h"

#include "Effect_ColourCorrection.h"

const char * Effect_ColourCorrection :: GetVendorName() const	{return "ZenYes";}
const char * Effect_ColourCorrection :: GetEffectName() const	{return "Colour Correction";}

enum kColourCorrectionMsg
{
	kMsgInterpolation	= 'ecc0',
	kMsgReset,
	kMsgCurvesUpdate,
	kMsgColourRed,
	kMsgColourGreen,
	kMsgColourBlue,
	kMsgColourPicker,
	kMsgColourPickerRes,
	kMsgWhiteBalance,
};

struct EffectColourCorrectionData
{
	int				interpolation;
	float			red[4];
	float			green[4];
	float			blue[4];
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
	ColourCorrection Shader
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

#if 0
static const char *kFragmentShader = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec4		uRed;\
	uniform vec4		uGreen;\
	uniform vec4		uBlue;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	float CatmullRomSpline(in float t, in vec4 v) {\
		float c1 =                v.y;\
		float c2 = -0.5*v.x           + 0.5*v.z;\
		float c3 =      v.x - 2.5*v.y + 2.0*v.z - 0.5*v.a;\
		float c4 = -0.5*v.x + 1.5*v.y - 1.5*v.z + 0.5*v.a;\
		float y = ((c4*t + c3)*t + c2)*t + c1;\
		return y;\
	}\
	void main(void) {\
		vec4 colour = texture(uTextureUnit0, vTexCoord0);\
		float r = CatmullRomSpline(colour.r, uRed);\
		float g = CatmullRomSpline(colour.g, uGreen);\
		float b = CatmullRomSpline(colour.b, uBlue);\
		fFragColour = vec4(r, g, b, colour.a);\
	}";

#else
static const char *kFragmentShader = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec4		uRed;\
	uniform vec4		uGreen;\
	uniform vec4		uBlue;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	float BeizerCurve(in float t, in vec4 v) {\
		float q = 1-t;\
		float y = q*q*q*v.x + 3.0*t*q*q*v.y + 3.0*t*t*q*v.z + t*t*t*v.a;\
		return y;\
	}\
	void main(void) {\
		vec4 colour = texture(uTextureUnit0, vTexCoord0);\
		float r = BeizerCurve(colour.r, uRed);\
		float g = BeizerCurve(colour.g, uGreen);\
		float b = BeizerCurve(colour.b, uBlue);\
		fFragColour = vec4(r, g, b, colour.a);\
	}";

#endif

class ColourCorrectionShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;
	GLint				fLocation_uRed;
	GLint				fLocation_uGreen;
	GLint				fLocation_uBlue;

	ymath::YVector4		fRed;
	ymath::YVector4		fGreen;
	ymath::YVector4		fBlue;

public:
	ColourCorrectionShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kVertexShader, kFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
		fLocation_uRed = fShader->GetUniformLocation("uRed");
		fLocation_uGreen = fShader->GetUniformLocation("uGreen");
		fLocation_uBlue = fShader->GetUniformLocation("uBlue");
	}
	~ColourCorrectionShader()
	{
		delete fShader;
	}
	void SetRed(const float red[4])			{for (int i=0; i < 4; i++) fRed.v[i] = red[i];}
	void SetGreen(const float green[4])		{for (int i=0; i < 4; i++) fGreen.v[i] = green[i];}
	void SetBlue(const float blue[4])		{for (int i=0; i < 4; i++) fBlue.v[i] = blue[i];}
	void Render(float)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0, 0);
		glUniform4fv(fLocation_uRed, 1, fRed.v);
		glUniform4fv(fLocation_uGreen, 1, fGreen.v);
		glUniform4fv(fLocation_uBlue, 1, fBlue.v);
	}
};

/*	FUNCTION:		Effect_ColourCorrection :: Effect_ColourCorrection
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_ColourCorrection :: Effect_ColourCorrection(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fRenderNode = nullptr;

	const float kFontFactor = be_plain_font->Size()/20.0f;
	const float kScrollBarScale = be_plain_font->Size()/12.0f;
	const float kFrameRight = frame.right - 10 - kScrollBarScale*B_V_SCROLL_BAR_WIDTH;

	//	Interpolation
	fOptionInterpolation = new BOptionPopUp(BRect(20*kFontFactor, 20, 20+360*kFontFactor, 60), "interpolation", GetText(TXT_EFFECTS_COMMON_INTERPOLATE), new BMessage(kMsgInterpolation));
	fOptionInterpolation->AddOption("Catmull Rom Spline", 0);
	fOptionInterpolation->AddOption("Beizer Curve", 1);
	mEffectView->AddChild(fOptionInterpolation);

	//	Curves
	fCurvesView = new CurvesView(BRect(10*kFontFactor, 70, 10+480*kFontFactor, (70+480)*kFontFactor), this, new BMessage(kMsgCurvesUpdate));
	mEffectView->AddChild(fCurvesView);

	//	Colour buttons
	assert(NUMBER_COLOUR_BUTTONS == 3);
	fButtonColours[0] = new BRadioButton(BRect(500*kFontFactor, 130, kFrameRight, 160), nullptr, GetText(TXT_EFFECTS_COMMON_RED), new BMessage(kMsgColourRed));
	fButtonColours[0]->SetValue(1);
	mEffectView->AddChild(fButtonColours[0]);
	fButtonColours[1] = new BRadioButton(BRect(500*kFontFactor, 180, kFrameRight, 210), nullptr, GetText(TXT_EFFECTS_COMMON_GREEN), new BMessage(kMsgColourGreen));
	mEffectView->AddChild(fButtonColours[1]);
	fButtonColours[2] = new BRadioButton(BRect(500*kFontFactor, 230, kFrameRight, 260), nullptr, GetText(TXT_EFFECTS_COMMON_BLUE), new BMessage(kMsgColourBlue));
	mEffectView->AddChild(fButtonColours[2]);

	//	Reset
	fButtonReset = new BButton(BRect(500*kFontFactor, 300, kFrameRight, 330), "reset", GetText(TXT_EFFECTS_COMMON_RESET), new BMessage(kMsgReset));
	mEffectView->AddChild(fButtonReset);

	//	White balance sliders
	BStringView *label = new BStringView(BRect(500*kFontFactor, 380, 800*kFontFactor, 420), nullptr, GetText(TXT_EFFECTS_COLOUR_CORRECTION_WHITE_BALANCE));
	label->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	label->SetFont(be_bold_font);
	mEffectView->AddChild(label);

	fWhiteBalanceSliders[0] = new ValueSlider(BRect(500*kFontFactor, 430, 800*kFontFactor, 490), "wb_red", GetText(TXT_EFFECTS_COMMON_RED), nullptr, 0, 255);
	fWhiteBalanceSliders[0]->SetModificationMessage(new BMessage(kMsgWhiteBalance));
	fWhiteBalanceSliders[0]->SetHashMarks(B_HASH_MARKS_BOTH);
	fWhiteBalanceSliders[0]->SetFloatingPointPrecision(0);
	fWhiteBalanceSliders[0]->SetValueUpdateText(255);
	fWhiteBalanceSliders[0]->SetBarColor({255, 0, 0, 255});
	fWhiteBalanceSliders[0]->SetLowColor(255, 0, 0, 255);
	mEffectView->AddChild(fWhiteBalanceSliders[0]);

	fWhiteBalanceSliders[1] = new ValueSlider(BRect(500*kFontFactor, 490, 800*kFontFactor, 550), "wb_green", GetText(TXT_EFFECTS_COMMON_GREEN), nullptr, 0, 255);
	fWhiteBalanceSliders[1]->SetModificationMessage(new BMessage(kMsgWhiteBalance));
	fWhiteBalanceSliders[1]->SetHashMarks(B_HASH_MARKS_BOTH);
	fWhiteBalanceSliders[1]->SetFloatingPointPrecision(0);
	fWhiteBalanceSliders[1]->SetValueUpdateText(255);
	fWhiteBalanceSliders[1]->SetBarColor({0, 255, 0, 255});
	fWhiteBalanceSliders[1]->SetLowColor(0, 255, 0, 255);
	mEffectView->AddChild(fWhiteBalanceSliders[1]);

	fWhiteBalanceSliders[2] = new ValueSlider(BRect(500*kFontFactor, 550, 800*kFontFactor, 610), "wb_blue", GetText(TXT_EFFECTS_COMMON_BLUE), nullptr, 0, 255);
	fWhiteBalanceSliders[2]->SetModificationMessage(new BMessage(kMsgWhiteBalance));
	fWhiteBalanceSliders[2]->SetHashMarks(B_HASH_MARKS_BOTH);
	fWhiteBalanceSliders[2]->SetFloatingPointPrecision(0);
	fWhiteBalanceSliders[2]->SetValueUpdateText(255);
	fWhiteBalanceSliders[2]->SetBarColor({0, 0, 255, 255});
	fWhiteBalanceSliders[2]->SetLowColor(0, 0, 255, 255);
	mEffectView->AddChild(fWhiteBalanceSliders[2]);

	//	ColourPicker
	fColourPickerButton = new BitmapCheckbox(BRect(500*kFontFactor, 630, 540*kFontFactor, 670), "colour_picker",
											 BTranslationUtils::GetBitmap("Resources/icon_colour_picker_idle.png"),
											 BTranslationUtils::GetBitmap("Resources/icon_colour_picker_active.png"),
											 new BMessage(kMsgColourPicker));
	fColourPickerButton->SetState(false);
	mEffectView->AddChild(fColourPickerButton);
	fColourPickerWindow = nullptr;
	fColourPickerMessage = nullptr;

	SetViewIdealSize(840*kFontFactor, 700);
}

/*	FUNCTION:		Effect_ColourCorrection :: ~Effect_ColourCorrection
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_ColourCorrection :: ~Effect_ColourCorrection()
{
	if (fColourPickerWindow)
		fColourPickerWindow->Terminate();
}

/*	FUNCTION:		Effect_ColourCorrection :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_ColourCorrection :: AttachedToWindow()
{
	fOptionInterpolation->SetTarget(this, Window());
	fButtonReset->SetTarget(this, Window());
	fColourPickerButton->SetTarget(this, Window());
	fWhiteBalanceSliders[0]->SetTarget(this, Window());
	fWhiteBalanceSliders[1]->SetTarget(this, Window());
	fWhiteBalanceSliders[2]->SetTarget(this, Window());

	for (int i=0; i < NUMBER_COLOUR_BUTTONS; i++)
		fButtonColours[i]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_ColourCorrection :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ColourCorrection :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new ColourCorrectionShader;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kFadeGeometry, 4);
	fRenderNode->mTexture = new YTexture(width, height);
}

/*	FUNCTION:		Effect_ColourCorrection :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ColourCorrection :: DestroyRenderObjects()
{
	delete fRenderNode;
}

/*	FUNCTION:		Effect_ColourCorrection :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_ColourCorrection :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_ColourCorrection.png");
	return icon;	
}

/*	FUNCTION:		Effect_ColourCorrection :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_ColourCorrection :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_CORRECTION);
}

/*	FUNCTION:		Effect_ColourCorrection :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_ColourCorrection :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_CORRECTION_TEXT_A);
}

/*	FUNCTION:		Effect_ColourCorrection :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_ColourCorrection :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_CORRECTION_TEXT_B);
}

/*	FUNCTION:		Effect_ColourCorrection :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_ColourCorrection :: CreateMediaEffect()
{
	const float width = gProject->mResolution.width;
	const float height = gProject->mResolution.height;

	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectColourCorrectionData *data = new EffectColourCorrectionData;
	data->interpolation = 0;
	const float kCurveValues[4] = {0.0f, 0.33f, 0.66f, 1.0f};
	memcpy(data->red, kCurveValues, 4*sizeof(float));
	memcpy(data->green, kCurveValues, 4*sizeof(float));
	memcpy(data->blue, kCurveValues, 4*sizeof(float));
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_ColourCorrection :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_ColourCorrection :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
	EffectColourCorrectionData *data = (EffectColourCorrectionData *)effect->mEffectData;
	fOptionInterpolation->SetValue(data->interpolation);
	fCurvesView->SetInterpolation((CurvesView::INTERPOLATION)data->interpolation);
	fCurvesView->SetColourValues(CurvesView::CURVE_RED, data->red);
	fCurvesView->SetColourValues(CurvesView::CURVE_GREEN, data->green);
	fCurvesView->SetColourValues(CurvesView::CURVE_BLUE, data->blue);

//	fCurvesView->SetActiveColour(CurvesView::CURVE_RED);
}

/*	FUNCTION:		Effect_ColourCorrection :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_ColourCorrection :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	EffectColourCorrectionData *colour_data = (EffectColourCorrectionData *)data->mEffectData;
	ColourCorrectionShader *shader = (ColourCorrectionShader *) fRenderNode->mShaderNode;
	//	BGRA
	shader->SetRed(colour_data->blue);
	shader->SetGreen(colour_data->green);
	shader->SetBlue(colour_data->red);

	fRenderNode->mTexture->Upload(source);
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_ColourCorrection :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_ColourCorrection :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgInterpolation:
			if (GetCurrentMediaEffect())
			{
				EffectColourCorrectionData *data = (EffectColourCorrectionData *)GetCurrentMediaEffect()->mEffectData;
				assert(data);

				if (fOptionInterpolation->Value() == 0)
					fCurvesView->SetInterpolation(CurvesView::INTERPOLATION_CATMULL_ROM);
				else
					fCurvesView->SetInterpolation(CurvesView::INTERPOLATION_BEIZER);
			}
			break;

		case kMsgColourRed:			fCurvesView->SetActiveColour(CurvesView::CURVE_RED);			break;
		case kMsgColourGreen:		fCurvesView->SetActiveColour(CurvesView::CURVE_GREEN);			break;
		case kMsgColourBlue:		fCurvesView->SetActiveColour(CurvesView::CURVE_BLUE);			break;

		case kMsgReset:
			fCurvesView->Reset();
			fWhiteBalanceSliders[0]->SetValueUpdateText(255);
			fWhiteBalanceSliders[1]->SetValueUpdateText(255);
			fWhiteBalanceSliders[2]->SetValueUpdateText(255);
			//	fall through

		case kMsgCurvesUpdate:
		{
			fWhiteBalanceSliders[0]->SetValueUpdateText(255 * fCurvesView->GetColour(CurvesView::CURVE_RED)[3]);
			fWhiteBalanceSliders[1]->SetValueUpdateText(255 * fCurvesView->GetColour(CurvesView::CURVE_GREEN)[3]);
			fWhiteBalanceSliders[2]->SetValueUpdateText(255 * fCurvesView->GetColour(CurvesView::CURVE_BLUE)[3]);

			if (GetCurrentMediaEffect())
			{
				EffectColourCorrectionData *data = (EffectColourCorrectionData *)GetCurrentMediaEffect()->mEffectData;
				assert(data);
				memcpy(data->red, fCurvesView->GetColour(CurvesView::CURVE_RED), 4*sizeof(float));
				memcpy(data->green, fCurvesView->GetColour(CurvesView::CURVE_GREEN), 4*sizeof(float));
				memcpy(data->blue, fCurvesView->GetColour(CurvesView::CURVE_BLUE), 4*sizeof(float));
				InvalidatePreview();
			}
			break;
		}

		case kMsgColourPicker:
			if (fColourPickerWindow == nullptr)
			{
				fColourPickerMessage = new BMessage(kMsgColourPickerRes);
				fColourPickerMessage->AddColor("colour", {0, 0, 0, 255});
				fColourPickerMessage->AddBool("active", 1);
				fColourPickerWindow = new Magnify::TWindow(this, fColourPickerMessage);
			}

			if (fColourPickerButton->Value())
			{
				int attempt = 0;
				while (attempt < 10 && fColourPickerWindow->IsHidden())
				{
					fColourPickerWindow->Show();
					attempt++;
				}
			}
			else
			{
				fColourPickerWindow->Hide();
			}
			break;

		case kMsgColourPickerRes:
		{
			rgb_color c;
			bool active;
			if ((msg->FindColor("colour", &c) == B_OK) &&
				(msg->FindBool("active", &active) == B_OK))
			{
				if (active)
				{
					fCurvesView->SetWhiteBalance(c, true);
					fWhiteBalanceSliders[0]->SetValueUpdateText(c.red);
					fWhiteBalanceSliders[1]->SetValueUpdateText(c.green);
					fWhiteBalanceSliders[2]->SetValueUpdateText(c.blue);
				}
				else
				{
					fColourPickerWindow->Hide();
					fColourPickerButton->SetState(false);
				}
			}
			break;
		}

		case kMsgWhiteBalance:
		{
			float red = fWhiteBalanceSliders[0]->Value();
			float green = fWhiteBalanceSliders[1]->Value();
			float blue = fWhiteBalanceSliders[2]->Value();
			fWhiteBalanceSliders[0]->UpdateTextValue(red);
			fWhiteBalanceSliders[1]->UpdateTextValue(green);
			fWhiteBalanceSliders[2]->UpdateTextValue(blue);
			rgb_color c;
			c.red = red;
			c.green = green;
			c.blue = blue;
			fCurvesView->SetWhiteBalance(c, false);

			if (GetCurrentMediaEffect())
			{
				EffectColourCorrectionData *data = (EffectColourCorrectionData *)GetCurrentMediaEffect()->mEffectData;
				assert(data);
				memcpy(data->red, fCurvesView->GetColour(CurvesView::CURVE_RED), 4*sizeof(float));
				memcpy(data->green, fCurvesView->GetColour(CurvesView::CURVE_GREEN), 4*sizeof(float));
				memcpy(data->blue, fCurvesView->GetColour(CurvesView::CURVE_BLUE), 4*sizeof(float));
				InvalidatePreview();
			}
			break;
		}
		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		Effect_ColourCorrection :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_ColourCorrection :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	EffectColourCorrectionData *data = (EffectColourCorrectionData *) media_effect->mEffectData;
	memset(media_effect->mEffectData, 0, sizeof(EffectColourCorrectionData));

	//	interpolation
	if (v.HasMember("interpolation") && v["interpolation"].IsInt())
	{
		data->interpolation = v["interpolation"].GetInt();
		if (data->interpolation < 0)
			data->interpolation = 0;
		if (data->interpolation > 1)
			data->interpolation = 1;
	}

	//	parse data function
	bool valid = true;
	auto parse = [&v, &valid](const char *p, float *vec)
	{
		if (v.HasMember(p) && v[p].IsArray())
		{
			const rapidjson::Value &arr = v[p];
			rapidjson::SizeType num_elems = arr.GetArray().Size();
			if (num_elems == 4)
			{
				vec[0] = arr.GetArray()[0].GetFloat();
				vec[1] = arr.GetArray()[1].GetFloat();
				vec[2] = arr.GetArray()[2].GetFloat();
				vec[3] = arr.GetArray()[3].GetFloat();
			}
			else
				valid = false;
		}
		else
		{
			printf("Effect_ColourCorrection[ZenYes::ColourCorrection] Missing parameter %s\n", p);
			valid = false;
		}
	};
	parse("red",	data->red);
	parse("green",	data->green);
	parse("blue",	data->blue);
	
	return valid;
}

/*	FUNCTION:		Effect_ColourCorrection :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_ColourCorrection :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectColourCorrectionData *data = (EffectColourCorrectionData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"interpolation\": \"%d\",\n", data->interpolation);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"red\": [%f, %f, %f, %f],\n", data->red[0], data->red[1], data->red[2], data->red[3]);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"green\": [%f, %f, %f, %f],\n", data->green[0], data->green[1], data->green[2], data->green[3]);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"blue\": [%f, %f, %f, %f]\n", data->blue[0], data->blue[1], data->blue[2], data->blue[3]);
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
