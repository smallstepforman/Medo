/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Chroma Key
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
#include "Gui/AlphaColourControl.h"
#include "Gui/BitmapCheckbox.h"
#include "Gui/Magnify.h"
#include "Gui/Spinner.h"
#include "Gui/ValueSlider.h"

#include "Effect_ChromaKey.h"

enum CHROMA_KEY_LANGUAGE_TEXT
{
	TXT_CHROMA_KEY_TEXT_A,
	TXT_CHROMA_KEY_TEXT_B,
	TXT_CHROMA_KEY_ENABLED,
	TXT_CHROMA_KEY_THRESHOLD,
	TXT_CHROMA_KEY_SMOOTHING,
	TXT_CHROMA_KEY_COLOUR,
	NUMBER_CHROMA_KEY_LANGUAGE_TEXT
};

Effect_ChromaKey *instantiate_effect(BRect frame)
{
    return new Effect_ChromaKey(frame, nullptr);
}

const char * Effect_ChromaKey :: GetVendorName() const	{return "ZenYes";}
const char * Effect_ChromaKey :: GetEffectName() const	{return "ChromaKey";}

enum kColourMessages
{
	eMsgSliderThreshold0 = 'eck0', eMsgSliderThreshold1, eMsgSliderThreshold2, eMsgSliderThreshold3,
	eMsgSliderSmoothing0, eMsgSliderSmoothing1, eMsgSliderSmoothing2, eMsgSliderSmoothing3,
	eMsgColorControl0, eMsgColorControl1, eMsgColorControl2, eMsgColorControl3,
	eMsgColourPicker0, eMsgColourPicker1, eMsgColourPicker2, eMsgColourPicker3,
	eMsgEnabled0, eMsgEnabled1, eMsgEnabled2, eMsgEnabled3,
	eMsgColourPickerRes
};

struct EffectChromaKeyData
{
	float				thresholds[Effect_ChromaKey::kNumberChromaColours];
	float				smoothings[Effect_ChromaKey::kNumberChromaColours];
	int					enabled[Effect_ChromaKey::kNumberChromaColours];
	ymath::YVector4		chroma_colours[Effect_ChromaKey::kNumberChromaColours];
};

static const rgb_color kDefaultChromaColors[] = 
{
	{0, 255, 0, 255},
	{255, 0, 0, 255},
	{0, 255, 0, 255},
	{0, 0, 255, 255},
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
	Chroma Key Shader
*************************/
static const char *kChromaKeyVertexShader = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture0;\
	out vec2		vTexCoord0;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);vTexCoord0 = aTexture0;\
	}";

static const char *kChromaKeyFragmentShader = "\
	uniform sampler2D		uTexture0;\
	uniform float			uThreshold0;\
	uniform float			uThreshold1;\
	uniform float			uThreshold2;\
	uniform float			uThreshold3;\
	uniform float			uSmoothing0;\
	uniform float			uSmoothing1;\
	uniform float			uSmoothing2;\
	uniform float			uSmoothing3;\
	uniform int				uEnabled0;\
	uniform int				uEnabled1;\
	uniform int				uEnabled2;\
	uniform int				uEnabled3;\
	uniform vec4			uChromaColour0;\
	uniform vec4			uChromaColour1;\
	uniform vec4			uChromaColour2;\
	uniform vec4			uChromaColour3;\
	in vec2					vTexCoord0;\
	out vec4				fFragColour;\
	\
	float CalculateBlend(vec4 chroma, float Cr, float Cb, float threshold, float smoothing)\
	{\
		float maskY = 0.2989 * chroma.r + 0.5866 * chroma.g + 0.1145 * chroma.b;\
		float maskCr = 0.7132 * (chroma.r - maskY);\
		float maskCb = 0.5647 * (chroma.b - maskY);\
		\
		return smoothstep(threshold, threshold + smoothing, distance(vec2(Cr, Cb), vec2(maskCr, maskCb)));\
	}\
	\
	void main()\
	{\
		vec4 textureColor = texture(uTexture0, vTexCoord0);\
		float Y = 0.2989 * textureColor.r + 0.5866 * textureColor.g + 0.1145 * textureColor.b;\
		float Cr = 0.7132 * (textureColor.r - Y);\
		float Cb = 0.5647 * (textureColor.b - Y);\
		\
		float blend0 = (uEnabled0 > 0) ? CalculateBlend(uChromaColour0, Cr, Cb, uThreshold0, uSmoothing0) : 1.0;\
		float blend1 = (uEnabled1 > 0) ? CalculateBlend(uChromaColour1, Cr, Cb, uThreshold1, uSmoothing1) : 1.0;\
		float blend2 = (uEnabled2 > 0) ? CalculateBlend(uChromaColour2, Cr, Cb, uThreshold2, uSmoothing2) : 1.0;\
		float blend3 = (uEnabled3 > 0) ? CalculateBlend(uChromaColour3, Cr, Cb, uThreshold2, uSmoothing3) : 1.0;\
		\
		fFragColour = vec4(textureColor.rgb, textureColor.a * blend0*blend1*blend2*blend3);\
	}";


class ChromaKeyShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;
	GLint				fLocation_uThresholds[Effect_ChromaKey::kNumberChromaColours];
	GLint				fLocation_uSmoothings[Effect_ChromaKey::kNumberChromaColours];
	GLint				fLocation_uChromaColours[Effect_ChromaKey::kNumberChromaColours];
	GLint				fLocation_uEnabled[Effect_ChromaKey::kNumberChromaColours];

	float				fThresholds[Effect_ChromaKey::kNumberChromaColours];
	float				fSmoothings[Effect_ChromaKey::kNumberChromaColours];
	int					fEnabled[Effect_ChromaKey::kNumberChromaColours];
	ymath::YVector4		fChromaColours[Effect_ChromaKey::kNumberChromaColours];

public:
	ChromaKeyShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kChromaKeyVertexShader, kChromaKeyFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");

		for (int i=0; i < Effect_ChromaKey::kNumberChromaColours; i++)
		{
			BString threshold_str("uThreshold");
			BString smoothing_str("uSmoothing");
			BString colour_str("uChromaColour");
			BString enabled_str("uEnabled");
			threshold_str.Append('0'+i, 1);
			smoothing_str.Append('0'+i, 1);
			colour_str.Append('0'+i, 1);
			enabled_str.Append('0'+i, 1);
			fLocation_uThresholds[i] = fShader->GetUniformLocation(threshold_str.String());
			fLocation_uSmoothings[i] = fShader->GetUniformLocation(smoothing_str.String());
			fLocation_uEnabled[i] = fShader->GetUniformLocation(enabled_str.String());
			fLocation_uChromaColours[i] = fShader->GetUniformLocation(colour_str.String());
		}
		fShader->PrintToStream();
	}
	~ChromaKeyShader()
	{
		delete fShader;
	}
	void SetThreshold(const int idx, const float threshold)		{fThresholds[idx] = threshold;}
	void SetSmoothing(const int idx, const float smoothing)		{fSmoothings[idx] = smoothing;}
	void SetEnabled(const int idx, const int enabled)			{fEnabled[idx] = enabled;}
	void SetChromaColour(const int idx, const ymath::YVector4 &colour) {fChromaColours[idx] = colour;}
	void Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0, 0);
		for (int i=0; i < Effect_ChromaKey::kNumberChromaColours; i++)
		{
			glUniform1f(fLocation_uThresholds[i], fThresholds[i]);
			glUniform1f(fLocation_uSmoothings[i], fSmoothings[i]);
			glUniform1i(fLocation_uEnabled[i], fEnabled[i]);
			glUniform4fv(fLocation_uChromaColours[i], 1, fChromaColours[i].v);
		}
	}
};

/*	FUNCTION:		Effect_ChromaKey :: Effect_ChromaKey
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_ChromaKey :: Effect_ChromaKey(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fLanguage = new LanguageJson("AddOns/ChromaKey/Languages.json");
	if (fLanguage->GetTextCount() == 0)
	{
		printf("Effect_ChromaKey() Error - cannot load \"Languages.json\"\n");
		return;
	}
	
	const float kFontFactor = be_plain_font->Size()/20.0f;

	fRenderNodeTexture = nullptr;
	fColourPickerWindow = nullptr;
	fColourPickerMessage = nullptr;

	for (int i=0; i < kNumberChromaColours; i++)
	{
		float ypos = 6 + 164*i;

		//	Enabled
		fGuiCheckboxEnabled[i] = new BCheckBox(BRect(10, ypos+10, 120, ypos+50), "checkbox", fLanguage->GetText(TXT_CHROMA_KEY_ENABLED), new BMessage(eMsgEnabled0 + i));
		fGuiCheckboxEnabled[i]->SetValue(i==0);
		mEffectView->AddChild(fGuiCheckboxEnabled[i]);

		//	Colour picker
		fColourPickerButtons[i] = new BitmapCheckbox(BRect(140, ypos, 180, ypos+40), "colour_picker",
												 BTranslationUtils::GetBitmap("Resources/icon_colour_picker_idle.png"),
												 BTranslationUtils::GetBitmap("Resources/icon_colour_picker_active.png"),
												 new BMessage(eMsgColourPicker0 + i));
		fColourPickerButtons[i]->SetState(false);
		mEffectView->AddChild(fColourPickerButtons[i]);

		//	Sample colour
		fGuiSampleColours[i] = new BView(BRect(200, ypos, 260, ypos+40), nullptr, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);
		fGuiSampleColours[i]->SetViewColor(kDefaultChromaColors[i]);
		mEffectView->AddChild(fGuiSampleColours[i]);

		//	Title
		BString aString(fLanguage->GetText(TXT_CHROMA_KEY_COLOUR));
		aString.Append('1'+i, 1);
		BStringView *title = new BStringView(BRect(280, ypos, 420, ypos+40), nullptr, aString.String());
		title->SetFont(be_bold_font);
		mEffectView->AddChild(title);

		//	Colour control
		fGuiColourControls[i] = new BColorControl(BPoint(10, ypos+50), B_CELLS_32x8, 6.0f, aString.String(), new BMessage(eMsgColorControl0 + i), true);
		fGuiColourControls[i]->SetValue(kDefaultChromaColors[i]);
		mEffectView->AddChild(fGuiColourControls[i]);

		//	Threshold
		fGuiSliderThresholds[i] = new ValueSlider(BRect(450, ypos+i*2, 640, ypos+70+i*2), "slider_threshold", fLanguage->GetText(TXT_CHROMA_KEY_THRESHOLD), nullptr, 0, 1000);
		fGuiSliderThresholds[i]->SetModificationMessage(new BMessage(eMsgSliderThreshold0 + i));
		fGuiSliderThresholds[i]->SetValue(100);
		fGuiSliderThresholds[i]->SetHashMarks(B_HASH_MARKS_BOTH);
		fGuiSliderThresholds[i]->SetHashMarkCount(11);
		fGuiSliderThresholds[i]->SetLimitLabels("0.0", "0.1");
		fGuiSliderThresholds[i]->UpdateTextValue(100.0f/1000);
		fGuiSliderThresholds[i]->SetStyle(B_BLOCK_THUMB);
		fGuiSliderThresholds[i]->SetFloatingPointPrecision(3);
		fGuiSliderThresholds[i]->SetBarColor({255, 255, 0, 255});
		fGuiSliderThresholds[i]->UseFillColor(true);
		mEffectView->AddChild(fGuiSliderThresholds[i]);

		//	Smoothing
		fGuiSliderSmoothings[i] = new ValueSlider(BRect(450, ypos+80+i*2, 640, ypos+140+i*2), "slider_smoothing", fLanguage->GetText(TXT_CHROMA_KEY_SMOOTHING), nullptr, 0, 1000);
		fGuiSliderSmoothings[i]->SetModificationMessage(new BMessage(eMsgSliderSmoothing0 + i));
		fGuiSliderSmoothings[i]->SetValue(100);
		fGuiSliderSmoothings[i]->SetHashMarks(B_HASH_MARKS_BOTH);
		fGuiSliderSmoothings[i]->SetHashMarkCount(11);
		fGuiSliderSmoothings[i]->SetLimitLabels("0.0", "0.2");
		fGuiSliderSmoothings[i]->UpdateTextValue(100.0f/1000);
		fGuiSliderSmoothings[i]->SetStyle(B_BLOCK_THUMB);
		fGuiSliderSmoothings[i]->SetFloatingPointPrecision(3);
		fGuiSliderSmoothings[i]->SetBarColor({255, 255, 0, 255});
		fGuiSliderSmoothings[i]->UseFillColor(true);
		mEffectView->AddChild(fGuiSliderSmoothings[i]);
	}
}

/*	FUNCTION:		Effect_ChromaKey :: ~Effect_ChromaKey
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_ChromaKey :: ~Effect_ChromaKey()
{
	if (fColourPickerWindow)
	{
		fColourPickerWindow->Terminate();
		fColourPickerWindow = nullptr;
	}
}

/*	FUNCTION:		Effect_ChromaKey :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_ChromaKey :: AttachedToWindow()
{
	for (int i=0; i < kNumberChromaColours; i++)
	{
		fGuiSliderThresholds[i]->SetTarget(this, Window());
		fGuiSliderSmoothings[i]->SetTarget(this, Window());
		fGuiColourControls[i]->SetTarget(this, Window());
		fColourPickerButtons[i]->SetTarget(this, Window());
		fGuiCheckboxEnabled[i]->SetTarget(this, Window());
	}
}

/*	FUNCTION:		Effect_ChromaKey :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ChromaKey :: InitRenderObjects()
{
	assert(fRenderNodeTexture == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNodeTexture = new yrender::YRenderNode;
	fRenderNodeTexture->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNodeTexture->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNodeTexture->mShaderNode = new ChromaKeyShader;
	fRenderNodeTexture->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kFadeGeometry, 4);
}

/*	FUNCTION:		Effect_ChromaKey :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ChromaKey :: DestroyRenderObjects()
{
	delete fRenderNodeTexture;		fRenderNodeTexture = nullptr;
}

/*	FUNCTION:		Effect_ChromaKey :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_ChromaKey :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("AddOns/ChromaKey/Effect_ChromaKey.png");
	return icon;	
}

/*	FUNCTION:		Effect_ChromaKey :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_ChromaKey :: GetTextEffectName(const uint32 language_idx)
{
	return fLanguage->GetText(TXT_CHROMA_KEY_TEXT_A);
}

/*	FUNCTION:		Effect_ChromaKey :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_ChromaKey :: GetTextA(const uint32 language_idx)
{
	return fLanguage->GetText(TXT_CHROMA_KEY_TEXT_A);
}

/*	FUNCTION:		Effect_ChromaKey :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_ChromaKey :: GetTextB(const uint32 language_idx)
{
	return fLanguage->GetText(TXT_CHROMA_KEY_TEXT_B);
}

/*	FUNCTION:		Effect_ChromaKey :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_ChromaKey :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;

	EffectChromaKeyData *data = new EffectChromaKeyData;
	for  (int i=0; i < kNumberChromaColours; i++)
	{
		data->thresholds[i] = 0.1f*fGuiSliderThresholds[i]->Value()/1000.0f;
		data->smoothings[i] = 0.2f*fGuiSliderSmoothings[i]->Value()/1000.0f;
		data->enabled[i] = fGuiCheckboxEnabled[i]->Value();

		rgb_color c = fGuiColourControls[i]->ValueAsColor();
		data->chroma_colours[i].Set((float)c.red/255.0f, (float)c.green/255.0f, (float)c.blue/255.0f, 1.0f);
	}
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_ChromaKey :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_ChromaKey :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	EffectChromaKeyData *data = (EffectChromaKeyData *)effect->mEffectData;

	//	Update GUI
	for (int i=0; i < kNumberChromaColours; i++)
	{
		fGuiCheckboxEnabled[i]->SetValue(data->enabled[i]);

		fGuiSliderThresholds[i]->SetValue(data->thresholds[i]*1000.0f*(1.0f/0.1f));
		fGuiSliderThresholds[i]->UpdateTextValue(data->thresholds[i]);

		fGuiSliderSmoothings[i]->SetValue(data->smoothings[i]*1000.0f*(1.0f/0.2f));
		fGuiSliderSmoothings[i]->UpdateTextValue(data->smoothings[i]);

		rgb_color c;
		c.red = data->chroma_colours[i][0]*255;
		c.green = data->chroma_colours[i][1]*255;
		c.blue = data->chroma_colours[i][2]*255;
		c.alpha = 255;
		fGuiColourControls[i]->SetValue(c);
		fGuiSampleColours[i]->SetViewColor(c);
	}
}

/*	FUNCTION:		Effect_ChromaKey :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_ChromaKey :: RenderEffect(BBitmap *source, MediaEffect *effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	EffectChromaKeyData *data = (EffectChromaKeyData *)effect->mEffectData;
	ChromaKeyShader *shader = (ChromaKeyShader *)fRenderNodeTexture->mShaderNode;
	for (int i=0; i < kNumberChromaColours; i++)
	{
		shader->SetThreshold(i, data->thresholds[i]);
		shader->SetSmoothing(i, data->smoothings[i]);
		shader->SetEnabled(i, data->enabled[i]);

		ymath::YVector4 colour(data->chroma_colours[i][2], data->chroma_colours[i][1], data->chroma_colours[i][0], data->chroma_colours[i][3]);
		shader->SetChromaColour(i, colour);
	}
	if (source != gRenderActor->GetBackgroundBitmap())
	{
		fRenderNodeTexture->mTexture = gRenderActor->GetPicture((unsigned int)source->Bounds().Width() + 1, (unsigned int)source->Bounds().Height() + 1, source)->mTexture;
		fRenderNodeTexture->Render(0.0f);
	}
}

/*	FUNCTION:		Effect_ChromaKey :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_ChromaKey :: MessageReceived(BMessage *msg)
{
	EffectChromaKeyData *data = nullptr;
	if (GetCurrentMediaEffect())
		data = (EffectChromaKeyData *)GetCurrentMediaEffect()->mEffectData;

	switch (msg->what)
	{
		case eMsgSliderThreshold0:
		case eMsgSliderThreshold1:
		case eMsgSliderThreshold2:
		case eMsgSliderThreshold3:
		{
			int idx = msg->what - eMsgSliderThreshold0;
			float threshold = 0.1f*fGuiSliderThresholds[idx]->Value()/1000.0f;
			fGuiSliderThresholds[idx]->UpdateTextValue(threshold);
			if (data)
			{
				data->thresholds[idx] = threshold;
				InvalidatePreview();
			}
			break;
		}

		case eMsgSliderSmoothing0:
		case eMsgSliderSmoothing1:
		case eMsgSliderSmoothing2:
		case eMsgSliderSmoothing3:
		{
			int idx = msg->what - eMsgSliderSmoothing0;
			float smoothing = 0.2f*fGuiSliderSmoothings[idx]->Value()/1000.0f;
			fGuiSliderSmoothings[idx]->UpdateTextValue(smoothing);
			if (data)
			{
				data->smoothings[idx] = smoothing;
				InvalidatePreview();
			}
			break;
		}

		case eMsgEnabled0:
		case eMsgEnabled1:
		case eMsgEnabled2:
		case eMsgEnabled3:
		{
			int idx = msg->what - eMsgEnabled0;
			int enabled = fGuiCheckboxEnabled[idx]->Value();
			fGuiSliderThresholds[idx]->SetEnabled(enabled);
			fGuiSliderSmoothings[idx]->SetEnabled(enabled);
			if (data)
			{
				data->enabled[idx] = enabled;
				InvalidatePreview();
			}
			break;
		}
				
		case eMsgColorControl0:
		case eMsgColorControl1:
		case eMsgColorControl2:
		case eMsgColorControl3:
		{
			int idx = msg->what - eMsgColorControl0;
			fGuiSampleColours[idx]->SetViewColor(fGuiColourControls[idx]->ValueAsColor());
			fGuiSampleColours[idx]->Invalidate();
			if (data)
			{
				rgb_color c = fGuiColourControls[idx]->ValueAsColor();
				data->chroma_colours[idx].Set((float)c.red/255.0f, (float)c.green/255.0f, (float)c.blue/255.0f, (float)c.alpha/255.0f);
				InvalidatePreview();
			}
			break;
		}

		case eMsgColourPicker0:
		case eMsgColourPicker1:
		case eMsgColourPicker2:
		case eMsgColourPicker3:
		{
			int idx = msg->what - eMsgColourPicker0;
			MessageColourPickerSelected(idx);
			break;
		}

		case eMsgColourPickerRes:
		{
			rgb_color c;
			bool active;
			int32 index;
			if ((msg->FindColor("colour", &c) == B_OK) &&
				(msg->FindBool("active", &active) == B_OK) &&
				(msg->FindInt32("index", &index) == B_OK))
			{
				if (active)
				{
					BColorControl *colour_control = fGuiColourControls[index];
					fGuiSampleColours[index]->SetViewColor(c);
					fGuiSampleColours[index]->Invalidate();
					colour_control->SetValue(c);
					if (data)
					{
						data->chroma_colours[index].Set((float)c.red/255.0f, (float)c.green/255.0f, (float)c.blue/255.0f, (float)c.alpha/255.0f);
						InvalidatePreview();
					}
				}
				else
				{
					fColourPickerWindow->Hide();
					fColourPickerButtons[index]->SetState(false);
				}
			}
			break;
		}

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_ChromaKey :: MessageColourPickerSelected
	ARGS:			index
	RETURN:			none
	DESCRIPTION:	Hook function when colour picker selected
*/

void Effect_ChromaKey :: MessageColourPickerSelected(int32 index)
{
	if (fColourPickerWindow == nullptr)
	{
		fColourPickerMessage = new BMessage(eMsgColourPickerRes);
		fColourPickerMessage->AddColor("colour", {0, 0, 0, 255});
		fColourPickerMessage->AddBool("active", true);
		fColourPickerMessage->AddInt32("index", 0);
		fColourPickerWindow = new Magnify::TWindow(this, fColourPickerMessage);
		fColourPickerWindow->SetTitle("Chroma Colour");
	}
	fColourPickerWindow->GetNotificationMessage()->ReplaceInt32("index", index);

	if (fColourPickerButtons[index]->Value())
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
}

/*	FUNCTION:		Effect_ChromaKey :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_ChromaKey :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	EffectChromaKeyData *data = (EffectChromaKeyData *) media_effect->mEffectData;

#define ERROR_EXIT(a) {printf("[Effect_ChromaKey] Error - %s\n", a); return false;}

	for (int i=0; i < kNumberChromaColours; i++)
	{
		BString aString;

		//	enabled
		aString.SetTo("enabled_");
		aString.Append('0' + i, 1);
		if (v.HasMember(aString.String()) && v[aString.String()].IsBool())
			data->enabled[i] = v[aString.String()].GetBool();
		else
			ERROR_EXIT("missing enabled");

		//	threshold
		aString.SetTo("threshold_");
		aString.Append('0' + i, 1);

		if (v.HasMember(aString.String()) && v[aString.String()].IsFloat())
		{
			data->thresholds[i] = v[aString.String()].GetFloat();
			if ((data->thresholds[i] < 0.0f) || (data->thresholds[i] > 1.0f))
				ERROR_EXIT("threshold invalid");
		}
		else
			ERROR_EXIT("missing threshold");

		//	smoothing
		aString.SetTo("smoothing_");
		aString.Append('0' + i, 1);
		if (v.HasMember(aString.String()) && v[aString.String()].IsFloat())
		{
			data->smoothings[i] = v[aString.String()].GetFloat();
			if ((data->smoothings[i] < 0.0f) || (data->smoothings[i] > 1.0f))
				ERROR_EXIT("smoothing invalid");
		}
		else
			ERROR_EXIT("missing smoothing");

		//	colour
		aString.SetTo("colour_");
		aString.Append('0' + i, 1);
		if (v.HasMember(aString.String()) && v[aString.String()].IsArray())
		{
			const rapidjson::Value &array = v[aString.String()];
			if (array.Size() != 4)
				ERROR_EXIT("colour invalid");
			uint32_t colour[4];
			for (rapidjson::SizeType e=0; e < 4; e++)
			{
				if (!array[e].IsInt())
					ERROR_EXIT("colour invalid");
				colour[e] = array[e].GetInt();
				if ((colour[e] < 0) || (colour[e] > 255))
					ERROR_EXIT("colour invalid");
			}
			data->chroma_colours[i].Set((float)colour[0]/255.0f, (float)colour[1]/255.0f, (float)colour[2]/255.0f, (float)colour[3]/255.0f);
		}
		else
			ERROR_EXIT("missing colour");
	}

	return true;
}

/*	FUNCTION:		Effect_ChromaKey :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_ChromaKey :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectChromaKeyData *data = (EffectChromaKeyData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes

	for (int i=0; i < kNumberChromaColours; i++)
	{
		sprintf(buffer, "\t\t\t\t\"enabled_%d\": %s,\n", i, data->enabled[i] ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"threshold_%d\": %f,\n", i, data->thresholds[i]);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"smoothing_%d\": %f,\n", i, data->smoothings[i]);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\t\"colour_%d\": [%d, %d, %d, %d]%s\n", i,
				int(data->chroma_colours[i][0]*255), int(data->chroma_colours[i][1]*255), int(data->chroma_colours[i][2]*255), int(data->chroma_colours[i][3]*255),
				i < kNumberChromaColours - 1 ? "," : "");
		fwrite(buffer, strlen(buffer), 1, file);
	}


	return true;
}
