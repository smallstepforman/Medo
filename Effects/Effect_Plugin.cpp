/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Plugin
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/Picture.h"
#include "Yarra/Render/Texture.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"

#include "Gui/BitmapCheckbox.h"
#include "Gui/Magnify.h"
#include "Gui/Spinner.h"
#include "Gui/ValueSlider.h"

#include "Editor/RenderActor.h"
#include "Editor/Language.h"
#include "Editor/LanguageJson.h"
#include "Editor/Project.h"
#include "Editor/MedoWindow.h"
#include "Editor/OutputView.h"

#include "Effect_Plugin.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

enum
{
	eMsgGui = 'epgm',
	eMsgColourPicker,
	eMsgColourPickerRes
};

static const float kSliderRange = 1000.0;

using namespace yrender;

static const YGeometry_P3T2 kPluginGeometry[] =
{
	{-1, -1, 0,		0, 0},
	{1, -1, 0,		1, 0},
	{-1, 1, 0,		0, 1},
	{1, 1, 0,		1, 1},
};

/************************
	Multi Spinner
*************************/
class MultiSpinner : public BControl
{
public:
	std::vector<Spinner *> mSpinners;

	MultiSpinner(const int count, BRect aRect, const char *label, BMessage *msg)
		: BControl(aRect,label,label,msg, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE)
	{
		assert((count > 0) && (count <= 4));
		const char *kIdentifiers[] = {"X", "Y", "Z", "W"};
		for (int i=0; i < count; i++)
		{
			BString aString(label);
			aString.Append(kIdentifiers[i]);
			BRect spinner_rect = aRect;
			float h = aRect.Height() / count;
			spinner_rect.top = aRect.top + h*i;
			spinner_rect.bottom = aRect.top + h*(i+1);
			Spinner *spinner = new Spinner(spinner_rect, aString.String(), aString.String(), msg);
			spinner->SetSteps(0.01f);
			mSpinners.push_back(spinner);
		}
	}
	~MultiSpinner()
	{
		//	mSpinners are autodestructed as children
	}
};

/************************
	Vertex Shader
*************************/
static const char *kVertexShader = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture0;\
	out vec2		vTexCoord0;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);vTexCoord0 = aTexture0;\
	}";

/***************************
	DynamicUniform
****************************/
struct DynamicUniform
{
	union
	{
		float					mFloat;
		int						mInt;
		float					mVec[4];
		GLint					mSampler2D;
	};
	GLint						mLocation;
	PluginUniform::UniformType	mType;
	void	PrintToStream()
	{
		printf("mLocation=%d   ", mLocation);
		switch (mType)
		{
			case PluginUniform::UniformType::eFloat:		printf("eFloat: %f\n", mFloat);												break;
			case PluginUniform::UniformType::eInt:			printf("eInt: %d\n", mInt);													break;
			case PluginUniform::UniformType::eVec2:			printf("eVec2: %f, %f\n", mVec[0], mVec[1]);								break;
			case PluginUniform::UniformType::eVec3:			printf("eVec3: %f, %f, %f\n", mVec[0], mVec[1], mVec[2]);					break;
			case PluginUniform::UniformType::eVec4:			printf("eVec4: %f, %f, %f, %f\n", mVec[0], mVec[1], mVec[2], mVec[3]);		break;
			case PluginUniform::UniformType::eColour:		printf("eColour: %f, %f, %f, %f\n", mVec[0], mVec[1], mVec[2], mVec[3]);	break;
			case PluginUniform::UniformType::eSampler2D:	printf("eSampler2D: %d\n",mSampler2D);										break;
			case PluginUniform::UniformType::eTimestamp:	printf("eTimestamp: %f\n", mFloat);											break;
			case PluginUniform::UniformType::eInterval:		printf("eInterval: %f\n", mFloat);											break;
			case PluginUniform::UniformType::eRsolution:	printf("eRsolution: %f,%f\n", mVec[0], mVec[1]);							break;
			default:	assert(0);
		}
	}
};
struct EffectPluginData
{
	std::vector<DynamicUniform>	uniforms;
	bool		swap_texture_units;
};

/***************************
	PluginFragmentShader
****************************/
class PluginFragmentShader : public yrender::YShaderNode
{
private:
	EffectPlugin		*fPlugin;
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	std::vector<DynamicUniform> fUniforms;
	int					fNumberTextureUnits;
	bool				fSwapTextureUnits;

public:
	PluginFragmentShader(EffectPlugin *plugin)
	{
		fPlugin = plugin;
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kVertexShader, plugin->mFragmentShader.source_text.c_str());
		if (fShader->GetProgram() == 0)
			return;

		fSwapTextureUnits = false;
		fNumberTextureUnits = 0;
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		int shader_idx = 0;
		for (auto &u : plugin->mFragmentShader.uniforms)
		{
			DynamicUniform du;
			du.mType = u.type;
			du.mLocation = fShader->GetUniformLocation(u.name.c_str());
			//	Redundant since will be set by Effect_Plugin::CreateMediaEffect()
			switch (du.mType)
			{
				case PluginUniform::UniformType::eFloat:		du.mFloat = 0.0f;						break;
				case PluginUniform::UniformType::eInt:			du.mInt = 0;							break;
				case PluginUniform::UniformType::eVec2:			memset(du.mVec, 0, 4*sizeof(float));	break;
				case PluginUniform::UniformType::eVec3:			memset(du.mVec, 0, 4*sizeof(float));	break;
				case PluginUniform::UniformType::eVec4:			memset(du.mVec, 0, 4*sizeof(float));	break;
				case PluginUniform::UniformType::eColour:		memset(du.mVec, 0, 4*sizeof(float));	break;
				case PluginUniform::UniformType::eTimestamp:	du.mFloat = 0.0f;						break;
				case PluginUniform::UniformType::eInterval:		du.mFloat = 0.0f;						break;
				case PluginUniform::UniformType::eRsolution:	memset(du.mVec, 0, 4*sizeof(float));	break;

				case PluginUniform::UniformType::eSampler2D:
					du.mSampler2D = shader_idx++;//fShader->GetUniformLocation(u.name.c_str());
					fNumberTextureUnits++;
					break;

				default:	assert(0);
			}
			fUniforms.push_back(du);
		}
	}
	~PluginFragmentShader()
	{
		delete fShader;
	}
	bool	IsValid() {return fShader->GetProgram() > 0;}
	int		GetNumberTextureUnits() {return fNumberTextureUnits;}
	void	SwapTextureUnits(const bool swap) {fSwapTextureUnits = swap;}

	void	SetUniformFloat(int index, float f)
	{
		assert((index >= 0) && (index < fUniforms.size()));
		fUniforms[index].mFloat = f;
	}
	void	SetUniformInt(int index, int i)
	{
		assert((index >= 0) && (index < fUniforms.size()));
		fUniforms[index].mInt = i;
	}
	void	SetUniformVec(int index, float f0 = 0.0f, float f1 = 0.0f, float f2 = 0.0f, float f3 = 0.0f)
	{
		assert((index >= 0) && (index < fUniforms.size()));
		fUniforms[index].mVec[0] = f0;
		fUniforms[index].mVec[1] = f1;
		fUniforms[index].mVec[2] = f2;
		fUniforms[index].mVec[3] = f3;
	}
	void	SetUniformColourBgra(int index, float f0 = 0.0f, float f1 = 0.0f, float f2 = 0.0f, float f3 = 0.0f)
	{
		//	BGRA
		assert((index >= 0) && (index < fUniforms.size()));
		fUniforms[index].mVec[0] = f2;
		fUniforms[index].mVec[1] = f1;
		fUniforms[index].mVec[2] = f0;
		fUniforms[index].mVec[3] = f3;
	}

	void Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		for (auto &u : fUniforms)
		{
			switch (u.mType)
			{
				case PluginUniform::UniformType::eFloat:		glUniform1f(u.mLocation,	u.mFloat);			break;
				case PluginUniform::UniformType::eInt:			glUniform1i(u.mLocation,	u.mInt);			break;
				case PluginUniform::UniformType::eVec2:			glUniform2fv(u.mLocation, 1, u.mVec);			break;
				case PluginUniform::UniformType::eVec3:			glUniform3fv(u.mLocation, 1, u.mVec);			break;
				case PluginUniform::UniformType::eVec4:			glUniform4fv(u.mLocation, 1, u.mVec);			break;
				case PluginUniform::UniformType::eColour:		glUniform4fv(u.mLocation, 1, u.mVec);			break;
				case PluginUniform::UniformType::eTimestamp:	glUniform1f(u.mLocation,	u.mFloat);			break;
				case PluginUniform::UniformType::eInterval:		glUniform1f(u.mLocation,	u.mFloat);			break;
				case PluginUniform::UniformType::eRsolution:	glUniform2fv(u.mLocation, 1, u.mVec);			break;

				case PluginUniform::UniformType::eSampler2D:
					if (fSwapTextureUnits)
						glUniform1i(u.mLocation,	fNumberTextureUnits - u.mSampler2D - 1);
					else
						glUniform1i(u.mLocation,	u.mSampler2D);
					break;

				default:	assert(0);
			}
#if 0
			u.PrintToStream();
#endif
		}
	}
};

/***************************
	Effect_Plugin
****************************/

/*	FUNCTION:		Effect_Plugin :: Effect_Plugin
	ARGS:			plugin
					frame
					view_name
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Plugin :: Effect_Plugin(EffectPlugin *plugin, BRect frame, const char *view_name)
	: EffectNode(frame, view_name), fPlugin(plugin)
{
	const float kFontFactor = be_plain_font->Size()/20.0f;

	if (GetEffectGroup() == EffectNode::EFFECT_TRANSITION)
		InitSwapTexturesCheckbox();

	fRenderNode = nullptr;
	fTextureUnit1 = nullptr;

	fColourPickerButton = nullptr;
	fColourPickerWindow = nullptr;
	fColourPickerMessage = nullptr;

	//	Init Gui objects
	for (auto &w : plugin->mFragmentShader.gui_widgets)
	{
		switch (w.widget_type)
		{
			case PluginGuiWidget::eSlider:
			{
				ValueSlider *slider = new ValueSlider(BRect(w.rect.left*kFontFactor, w.rect.top, w.rect.right*kFontFactor, w.rect.bottom),
													  plugin->mLanguage->GetText(w.txt_label), plugin->mLanguage->GetText(w.txt_label),
													  nullptr, 0, (int)kSliderRange);
				slider->SetModificationMessage(new BMessage(eMsgGui));
				slider->SetHashMarks(B_HASH_MARKS_BOTH);
				slider->SetHashMarkCount(11);
				slider->SetLimitLabels(plugin->mLanguage->GetText(w.txt_slider_min), plugin->mLanguage->GetText(w.txt_slider_max));
				slider->SetStyle(B_BLOCK_THUMB);
				slider->SetFloatingPointPrecision(2);
				float f = (w.default_value[0] - w.range[0]) / (w.range[1] - w.range[0]);
				slider->SetValue(kSliderRange*f);
				slider->UpdateTextValue(w.default_value[0]);
				slider->SetBarColor({255, 0, 0, 255});
				slider->UseFillColor(true);
				if ((w.range[1] - w.range[0]) < 0.1f)
					slider->SetFloatingPointPrecision(3);
				mEffectView->AddChild(slider);
				fGuiWidgets.push_back(slider);
				break;
			}
			case PluginGuiWidget::eCheckbox:
			{
				BCheckBox *button = new BCheckBox(BRect(w.rect.left*kFontFactor, w.rect.top, w.rect.right*kFontFactor, w.rect.bottom),
												  plugin->mLanguage->GetText(w.txt_label), plugin->mLanguage->GetText(w.txt_label), new BMessage(eMsgGui));
				if (w.default_value[0] > 0)
					button->SetValue(1);
				mEffectView->AddChild(button);
				fGuiWidgets.push_back(button);
				break;
			}
			case PluginGuiWidget::eSpinner2:
			case PluginGuiWidget::eSpinner3:
			case PluginGuiWidget::eSpinner4:
			{
				MultiSpinner *multispinner = new MultiSpinner(PluginGuiWidget::kVecCountElements[w.widget_type],
						BRect(w.rect.left*kFontFactor, w.rect.top, w.rect.right*kFontFactor, w.rect.bottom),
						plugin->mLanguage->GetText(w.txt_label), new BMessage(eMsgGui));
				for (unsigned int i=0; i < PluginGuiWidget::kVecCountElements[w.widget_type]; i++)
				{
					multispinner->mSpinners[i]->SetValue(w.default_value[i]);
					multispinner->mSpinners[i]->SetRange(w.range[0], w.range[1]);
					mEffectView->AddChild(multispinner->mSpinners[i]);
				}
				fGuiWidgets.push_back(multispinner);
				break;
			}
			case PluginGuiWidget::eColour:
			{
				BStringView *title = new BStringView(BRect(w.rect.left*kFontFactor, w.rect.top, w.rect.right*kFontFactor, w.rect.top + 40),
													 nullptr, plugin->mLanguage->GetText(w.txt_label));
				title->SetFont(be_bold_font);
				mEffectView->AddChild(title);
				BColorControl *colour_control = new BColorControl(BPoint(w.rect.left*kFontFactor, w.rect.top + 40), B_CELLS_32x8, 6.0f, plugin->mLanguage->GetText(w.txt_label), new BMessage(eMsgGui), true);
				colour_control->SetValue({uint8(255.0f*w.vec4[0]), uint8(255*w.vec4[1]), uint8(255*w.vec4[2]), uint8(255*w.vec4[3])});
				mEffectView->AddChild(colour_control);
				fGuiWidgets.push_back(colour_control);

				//	Colour labels
				BView *view_red = colour_control->FindView("_red");
				BView *view_green = colour_control->FindView("_green");
				BView *view_blue = colour_control->FindView("_blue");
				if (view_red)		((BTextControl *)view_red)->SetLabel(GetText(TXT_EFFECTS_COMMON_RED));
				if (view_green)		((BTextControl *)view_green)->SetLabel(GetText(TXT_EFFECTS_COMMON_GREEN));
				if (view_blue)		((BTextControl *)view_blue)->SetLabel(GetText(TXT_EFFECTS_COMMON_BLUE));

				//	ColourPicker
				assert(fColourPickerButton == nullptr);		//	only support a single ColourPicker for now
				BRect colour_control_bounds = colour_control->Bounds();
				fColourPickerButton = new BitmapCheckbox(BRect((colour_control_bounds.right + 100)*kFontFactor, (w.rect.top+100)*kFontFactor,
															   (colour_control_bounds.right + 140)*kFontFactor, (w.rect.top+140)*kFontFactor), "colour_picker",
														 BTranslationUtils::GetBitmap("Resources/icon_colour_picker_idle.png"),
														 BTranslationUtils::GetBitmap("Resources/icon_colour_picker_active.png"),
														 new BMessage(eMsgColourPicker));
				fColourPickerButton->SetState(false);
				mEffectView->AddChild(fColourPickerButton);
				break;
			}
			case PluginGuiWidget::eText:
			{
				BStringView *title = new BStringView(BRect(w.rect.left*kFontFactor, w.rect.top, w.rect.right*kFontFactor, w.rect.bottom),
													 nullptr, plugin->mLanguage->GetText(w.txt_label));
				if (w.uniform_idx == 1)
					title->SetFont(be_bold_font);
				mEffectView->AddChild(title);
				fGuiWidgets.push_back(title);
				break;
			}
			default:
				assert(0);
		}
	}
	//printf("Effect_Plugin::Constructor::%s (%p)\n", plugin->mHeader.name.c_str(), this);
}

/*	FUNCTION:		Effect_Plugin :: ~Effect_Plugin
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Plugin :: ~Effect_Plugin()
{
	//printf("Effect_Plugin::Destructor(%p)\n", this);
	if (fColourPickerWindow)
	{
		fColourPickerWindow->Terminate();
		fColourPickerWindow = nullptr;
	}
}

/*	FUNCTION:		Effect_Plugin :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Plugin :: AttachedToWindow()
{
	assert(fGuiWidgets.size() == fPlugin->mFragmentShader.gui_widgets.size());
	size_t idx = 0;
	while (idx < fPlugin->mFragmentShader.gui_widgets.size())
	{
		switch (fPlugin->mFragmentShader.gui_widgets[idx].widget_type)
		{
			case PluginGuiWidget::eSlider:			((ValueSlider *)fGuiWidgets[idx])->SetTarget(this, Window());		break;
			case PluginGuiWidget::eCheckbox:		((BCheckBox *)fGuiWidgets[idx])->SetTarget(this, Window());			break;
			case PluginGuiWidget::eText:																				break;

			case PluginGuiWidget::eColour:
				((BColorControl *)fGuiWidgets[idx])->SetTarget(this, Window());
				fColourPickerButton->SetTarget(this, Window());
				break;

			case PluginGuiWidget::eSpinner2:
			case PluginGuiWidget::eSpinner3:
			case PluginGuiWidget::eSpinner4:
			{
				MultiSpinner *multispinner = (MultiSpinner *)fGuiWidgets[idx];
				for (int i=0; i < fPlugin->mFragmentShader.gui_widgets[idx].widget_type; i++)
					multispinner->mSpinners[i]->SetTarget(this, Window());
				break;
			}

			default:
				assert(0);
		}
		idx++;
	}

	if (GetEffectGroup() == EffectNode::EFFECT_TRANSITION)
		mSwapTexturesCheckbox->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Plugin :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Plugin :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	PluginFragmentShader *shader = new PluginFragmentShader(fPlugin);
	if (shader->IsValid())
	{
		fRenderNode->mShaderNode = shader;
		fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kPluginGeometry, 4);
//		fRenderNode->mTexture = new YTexture(width, height);
		if (shader->GetNumberTextureUnits() > 1)
		{
			assert(shader->GetNumberTextureUnits() == 2);
			fTextureUnit1 = new yrender::YTexture(width, height, YTexture::YTF_REPEAT);
			fTextureUnit1->SetTextureUnitIndex(1);
		}
	}
	else
	{
		delete fRenderNode;
		fRenderNode = nullptr;
	}
}

/*	FUNCTION:		Effect_Plugin :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Plugin :: DestroyRenderObjects()
{
	fRenderNode->mTexture = nullptr;
	delete fRenderNode;		fRenderNode = nullptr;
	delete fTextureUnit1;	fTextureUnit1 = nullptr;

}

/*	FUNCTION:		Effect_Plugin :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Plugin :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap(fPlugin->mHeader.icon.c_str());
	return icon;
}

const char * Effect_Plugin :: GetTextEffectName(const uint32 language_idx)	{return fPlugin->mLanguage->GetText(fPlugin->mHeader.txt_labelA);}
const char * Effect_Plugin :: GetTextA(const uint32 language_idx)			{return fPlugin->mLanguage->GetText(fPlugin->mHeader.txt_labelA);}
const char * Effect_Plugin :: GetTextB(const uint32 language_idx)			{return fPlugin->mLanguage->GetText(fPlugin->mHeader.txt_labelB);}


/*	FUNCTION:		Effect_Plugin :: CreateMediaEffect
	ARGS:			none
	RETURN:			media effect
	DESCRIPTION:	Create media effect node
*/
MediaEffect * Effect_Plugin :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;

	EffectPluginData *data = new EffectPluginData;
	data->swap_texture_units = AreTexturesSwapped();

	GLint sampler_idx = 0;
	for (auto &u : fPlugin->mFragmentShader.uniforms)
	{
		DynamicUniform du;
		du.mType = u.type;
		switch (du.mType)
		{
			case PluginUniform::UniformType::eFloat:		du.mFloat = 0.0f;						break;
			case PluginUniform::UniformType::eInt:			du.mInt = 0;							break;
			case PluginUniform::UniformType::eVec2:			memset(du.mVec, 0, 4*sizeof(float));	break;
			case PluginUniform::UniformType::eVec3:			memset(du.mVec, 0, 4*sizeof(float));	break;
			case PluginUniform::UniformType::eVec4:			memset(du.mVec, 0, 4*sizeof(float));	break;
			case PluginUniform::UniformType::eColour:		memset(du.mVec, 0, 4*sizeof(float));	break;
			case PluginUniform::UniformType::eSampler2D:	du.mSampler2D = sampler_idx++;			break;
			case PluginUniform::UniformType::eTimestamp:	du.mFloat = 0.0f;						break;
			case PluginUniform::UniformType::eInterval:		du.mFloat = 0.0f;						break;
			case PluginUniform::UniformType::eRsolution:	memset(du.mVec, 0, 4*sizeof(float));	break;
			default:	assert(0);
		}
		data->uniforms.push_back(du);
	}

	for (auto &w : fPlugin->mFragmentShader.gui_widgets)
	{
		printf("Uniform =%s, idx=%d\n", w.uniform.c_str(), w.uniform_idx);
		switch (w.widget_type)
		{
			case PluginGuiWidget::eSlider:
				data->uniforms[w.uniform_idx].mFloat = w.default_value[0];
				break;
			case PluginGuiWidget::eCheckbox:
				data->uniforms[w.uniform_idx].mInt = w.default_value[0];
				break;
			case PluginGuiWidget::eSpinner2:
			case PluginGuiWidget::eSpinner3:
			case PluginGuiWidget::eSpinner4:
			case PluginGuiWidget::eColour:
				for (unsigned int i=0; i < PluginGuiWidget::kVecCountElements[w.widget_type]; i++)
					data->uniforms[w.uniform_idx].mVec[i] = w.default_value[i];
				break;

			case PluginGuiWidget::eText:
				break;

			default:
				assert(0);
		}
	}

#if 0
	for (auto &u : data->uniforms)
		u.Debug();
#endif

	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_Plugin :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Plugin :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	float t = float(frame_idx - data->mTimelineFrameStart)/float(data->Duration());
	if (t > 1.0f)
		t = 1.0f;

	PluginFragmentShader *fragment_shader = (PluginFragmentShader *)fRenderNode->mShaderNode;
	if (!fragment_shader->IsValid())
	{
		printf("Effect_Plugin(%s) - invalid fragment shader\n", GetEffectName());
		return;
	}

	EffectPluginData *effect_data = (EffectPluginData *)data->mEffectData;
	for (int idx = 0; idx < effect_data->uniforms.size(); idx++)
	{
		switch (effect_data->uniforms[idx].mType)
		{
			case PluginUniform::UniformType::eFloat:
				fragment_shader->SetUniformFloat(idx, effect_data->uniforms[idx].mFloat);
				break;
			case PluginUniform::UniformType::eInt:
				fragment_shader->SetUniformInt(idx, effect_data->uniforms[idx].mInt);
				break;
			case PluginUniform::UniformType::eVec2:
				fragment_shader->SetUniformVec(idx, effect_data->uniforms[idx].mVec[0], effect_data->uniforms[idx].mVec[1]);
				break;
			case PluginUniform::UniformType::eVec3:
				fragment_shader->SetUniformVec(idx, effect_data->uniforms[idx].mVec[0], effect_data->uniforms[idx].mVec[1], effect_data->uniforms[idx].mVec[2]);
				break;
			case PluginUniform::UniformType::eVec4:
				fragment_shader->SetUniformVec(idx, effect_data->uniforms[idx].mVec[0], effect_data->uniforms[idx].mVec[1], effect_data->uniforms[idx].mVec[2], effect_data->uniforms[idx].mVec[3]);
				break;
			case PluginUniform::UniformType::eColour:
				fragment_shader->SetUniformColourBgra(idx, effect_data->uniforms[idx].mVec[0], effect_data->uniforms[idx].mVec[1], effect_data->uniforms[idx].mVec[2], effect_data->uniforms[idx].mVec[3]);
				break;
			case PluginUniform::UniformType::eSampler2D:
				break;
			case PluginUniform::UniformType::eTimestamp:
				fragment_shader->SetUniformFloat(idx, float(double(frame_idx - data->mTimelineFrameStart)/(double)kFramesSecond));
				break;
			case PluginUniform::UniformType::eInterval:
				fragment_shader->SetUniformFloat(idx, float(double(frame_idx - data->mTimelineFrameStart)/(double)data->Duration()));
				break;
			case PluginUniform::UniformType::eRsolution:
#if 0
				fragment_shader->SetUniformVec(idx, (source ? source->Bounds().Width() : gProject->mResolution.width),
											   (source ? source->Bounds().Height() : gProject->mResolution.height));
#else
				fragment_shader->SetUniformVec(idx, gProject->mResolution.width, gProject->mResolution.height);
#endif
				break;

			default:
				assert(0);
		}
	}

	fragment_shader->SwapTextureUnits(effect_data->swap_texture_units);

	if (source)
		fRenderNode->mTexture = gRenderActor->GetPicture((unsigned int)source->Bounds().Width() + 1, (unsigned int)source->Bounds().Height() + 1, source)->mTexture;
	if (fragment_shader->GetNumberTextureUnits() == 2)
	{
		fTextureUnit1->Upload(gRenderActor->GetCurrentFrameBufferTexture());
		fTextureUnit1->Render(0.0f);
	}
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_Plugin :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Plugin :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
	EffectPluginData *effect_data = (EffectPluginData *)effect->mEffectData;

	if (mSwapTexturesCheckbox)
		mSwapTexturesCheckbox->SetValue(effect_data->swap_texture_units ? 1 : 0);

	int gui_idx = 0;
	for (auto &w : fPlugin->mFragmentShader.gui_widgets)
	{
		switch (w.widget_type)
		{
			case PluginGuiWidget::eSlider:
			{
				ValueSlider *slider = (ValueSlider *)fGuiWidgets[gui_idx];
				float f = (effect_data->uniforms[w.uniform_idx].mFloat - w.range[0]) / (w.range[1] - w.range[0]);
				slider->SetValue(kSliderRange*f);
				slider->UpdateTextValue(effect_data->uniforms[w.uniform_idx].mFloat);
				break;
			}
			case PluginGuiWidget::eSpinner2:
			case PluginGuiWidget::eSpinner3:
			case PluginGuiWidget::eSpinner4:
			{
				MultiSpinner *multispinner = (MultiSpinner *)fGuiWidgets[gui_idx];
				for (unsigned int i=0; i < PluginGuiWidget::kVecCountElements[w.widget_type]; i++)
					multispinner->mSpinners[i]->SetValue(effect_data->uniforms[w.uniform_idx].mVec[i]);
				break;
			}
			case PluginGuiWidget::eCheckbox:
			{
				BCheckBox *button = (BCheckBox *)fGuiWidgets[gui_idx];
				button->SetValue(effect_data->uniforms[w.uniform_idx].mInt);
				break;
			}
			case PluginGuiWidget::eColour:
			{
				BColorControl *colour_control = (BColorControl *)fGuiWidgets[gui_idx];
				float *v = effect_data->uniforms[w.uniform_idx].mVec;
				colour_control->SetValue({uint8(255.0f*v[0]), uint8(255*v[1]), uint8(255*v[2]), uint8(255*v[3])});
				break;
			}
			case PluginGuiWidget::eText:
			{
				BStringView *string_view = (BStringView *)fGuiWidgets[gui_idx];
				string_view->SetText(fPlugin->mLanguage->GetText(w.txt_label));
				break;
			}
			default:
				assert(0);
		}
		gui_idx++;
	}
}

/*	FUNCTION:		Effect_Plugin :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function when msg received
*/
void Effect_Plugin :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case eMsgGui:
		{
			MediaEffect *effect = GetCurrentMediaEffect();
			if (!effect)
				break;
			EffectPluginData *effect_data = (EffectPluginData *)effect->mEffectData;
			int gui_idx = 0;
			for (auto &w : fPlugin->mFragmentShader.gui_widgets)
			{
				switch (w.widget_type)
				{
					case PluginGuiWidget::eSlider:
					{
						ValueSlider *slider = (ValueSlider *)fGuiWidgets[gui_idx];
						float f = slider->Value()/kSliderRange;
						effect_data->uniforms[w.uniform_idx].mFloat = w.range[0] + f*(w.range[1] - w.range[0]);
						slider->UpdateTextValue(effect_data->uniforms[w.uniform_idx].mFloat);
						break;
					}
					case PluginGuiWidget::eSpinner2:
					case PluginGuiWidget::eSpinner3:
					case PluginGuiWidget::eSpinner4:
					{
						MultiSpinner *multispinner = (MultiSpinner *)fGuiWidgets[gui_idx];
						for (unsigned int i=0; i < PluginGuiWidget::kVecCountElements[w.widget_type]; i++)
							effect_data->uniforms[w.uniform_idx].mVec[i] = multispinner->mSpinners[i]->Value();
						break;
					}
					case PluginGuiWidget::eCheckbox:
					{
						BCheckBox *button = (BCheckBox *)fGuiWidgets[gui_idx];
						effect_data->uniforms[w.uniform_idx].mInt = button->Value();
						break;
					}
					case PluginGuiWidget::eColour:
					{
						BColorControl *colour_control = (BColorControl *)fGuiWidgets[gui_idx];
						rgb_color c = colour_control->ValueAsColor();
						effect_data->uniforms[w.uniform_idx].mVec[0] = (float)c.red/255.0f;
						effect_data->uniforms[w.uniform_idx].mVec[1] = (float)c.green/255.0f;
						effect_data->uniforms[w.uniform_idx].mVec[2] = (float)c.blue/255.0f;
						effect_data->uniforms[w.uniform_idx].mVec[3] = (float)c.alpha/255.0f;
						break;
					}
					default:
						assert(0);
				}
				gui_idx++;
			}
			InvalidatePreview();
			break;
		}

		case kMsgSwapTextureUnits:
		{
			MediaEffect *effect = GetCurrentMediaEffect();
			if (!effect)
				break;
			EffectPluginData *data = (EffectPluginData *)effect->mEffectData;
			data->swap_texture_units = mSwapTexturesCheckbox->Value() > 0;
			InvalidatePreview();
			break;
		}

		case eMsgColourPicker:
			if (fColourPickerWindow == nullptr)
			{
				fColourPickerMessage = new BMessage(eMsgColourPickerRes);
				fColourPickerMessage->AddColor("colour", {0, 0, 0, 255});
				fColourPickerMessage->AddBool("active", true);
				fColourPickerWindow = new Magnify::TWindow(this, fColourPickerMessage);

				for (auto &w : fPlugin->mFragmentShader.gui_widgets)
				{
					if (w.widget_type == PluginGuiWidget::eColour)
					{
						fColourPickerWindow->SetTitle(fPlugin->mLanguage->GetText(w.txt_label));
					}
				}
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

		case eMsgColourPickerRes:
		{
			rgb_color c;
			bool active;
			if ((msg->FindColor("colour", &c) == B_OK) &&
				(msg->FindBool("active", &active) == B_OK))
			{
				if (active)
				{
					int gui_idx = 0;
					for (auto &w : fPlugin->mFragmentShader.gui_widgets)
					{
						if (w.widget_type == PluginGuiWidget::eColour)
						{
							BColorControl *colour_control = (BColorControl *)fGuiWidgets[gui_idx];
							colour_control->SetValue(c);
							MediaEffect *effect = GetCurrentMediaEffect();
							if (!effect)
								break;
							EffectPluginData *effect_data = (EffectPluginData *)effect->mEffectData;
							effect_data->uniforms[w.uniform_idx].mVec[0] = (float)c.red/255.0f;
							effect_data->uniforms[w.uniform_idx].mVec[1] = (float)c.green/255.0f;
							effect_data->uniforms[w.uniform_idx].mVec[2] = (float)c.blue/255.0f;
							effect_data->uniforms[w.uniform_idx].mVec[3] = (float)c.alpha/255.0f;
							InvalidatePreview();
						}
						gui_idx++;
					}
				}
				else
				{
					fColourPickerWindow->Hide();
					fColourPickerButton->SetState(false);
				}
			}
			break;
		}

		default:
			EffectNode::MessageReceived(msg);
	}
}

/*	FUNCTION:		Effect_Plugin :: OutputViewMouseDown
	ARGS:			media_effect
					point
	RETURN:			n/a
	DESCRIPTION:	Hook function when OutputView mouse down
*/
void Effect_Plugin :: OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)
{
	if (!media_effect)
		return;

	EffectPluginData *effect_data = (EffectPluginData *)media_effect->mEffectData;
	int gui_idx = 0;
	for (auto &w : fPlugin->mFragmentShader.gui_widgets)
	{
		if ((w.widget_type == PluginGuiWidget::eSpinner2) && (w.default_value[3] > 0.0))
		{
			MedoWindow::GetInstance()->LockLooper();
			BRect frame = MedoWindow::GetInstance()->GetOutputView()->Bounds();
			MedoWindow::GetInstance()->UnlockLooper();
			float x = point.x / frame.Width();
			float y = point.y / frame.Height();
			effect_data->uniforms[w.uniform_idx].mVec[0] = x;
			effect_data->uniforms[w.uniform_idx].mVec[1] = y;
			MultiSpinner *multispinner = (MultiSpinner *)fGuiWidgets[gui_idx];
			multispinner->mSpinners[0]->SetValue(x);
			multispinner->mSpinners[1]->SetValue(y);
			InvalidatePreview();
			break;
		}
		gui_idx++;
	}
}

/*	FUNCTION:		Effect_Plugin :: LoadParameters
	ARGS:			parameters
					media_effect
	RETURN:			true if valid
	DESCRIPTION:	Load plugin parameters from .medo file
*/
bool Effect_Plugin :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = true;
	EffectPluginData *effect_data = (EffectPluginData *)media_effect->mEffectData;

	if (v.HasMember("swap_textures") && v["swap_textures"].IsBool())
		effect_data->swap_texture_units = v["swap_textures"].GetBool();
	else
	{
		printf("[Effect_Plugin::LoadParameters(%s)] - invalid parameter \"swap_textures\"\n", fPlugin->mHeader.name.c_str());
		valid = false;
	}

	auto fnReadArray = [effect_data](const rapidjson::Value &array, rapidjson::SizeType size, int uniform_idx) -> bool
	{
		if (array.Size() != size)
			return false;

		for (rapidjson::SizeType e=0; e < size; e++)
		{
			if (!array[e].IsFloat())
				return false;
			effect_data->uniforms[uniform_idx].mVec[e] = array[e].GetFloat();
		}
		return true;
	};

	for (auto &w : fPlugin->mFragmentShader.gui_widgets)
	{
		switch (w.widget_type)
		{
			case PluginGuiWidget::eSlider:
				if (v.HasMember(w.uniform.c_str()) && v[w.uniform.c_str()].IsFloat())
				{
					effect_data->uniforms[w.uniform_idx].mFloat = v[w.uniform.c_str()].GetFloat();
					if (effect_data->uniforms[w.uniform_idx].mFloat < w.range[0])
						effect_data->uniforms[w.uniform_idx].mFloat = w.range[0];
					if (effect_data->uniforms[w.uniform_idx].mFloat > w.range[1])
						effect_data->uniforms[w.uniform_idx].mFloat = w.range[1];
				}
				else
				{
					printf("[Effect_Plugin::LoadParameters(%s)] - invalid eSlider parameter %s\n", fPlugin->mHeader.name.c_str(), w.uniform.c_str());
					valid = false;
				}
				break;

			case PluginGuiWidget::eCheckbox:
				if (v.HasMember(w.uniform.c_str()) && v[w.uniform.c_str()].IsInt())
				{
					effect_data->uniforms[w.uniform_idx].mInt = v[w.uniform.c_str()].GetInt();
					if (effect_data->uniforms[w.uniform_idx].mInt < 0)
						effect_data->uniforms[w.uniform_idx].mInt =0;
					if (effect_data->uniforms[w.uniform_idx].mInt > 1)
						effect_data->uniforms[w.uniform_idx].mInt = 1;
				}
				else
				{
					printf("[Effect_Plugin::LoadParameters(%s)] - invalid eCheckbox parameter %s\n", fPlugin->mHeader.name.c_str(), w.uniform.c_str());
					valid = false;
				}
				break;

			case PluginGuiWidget::eSpinner2:
			case PluginGuiWidget::eSpinner3:
			case PluginGuiWidget::eSpinner4:
			case PluginGuiWidget::eColour:
				if (v.HasMember(w.uniform.c_str()) && v[w.uniform.c_str()].IsArray())
				{
					const rapidjson::Value &array = v[w.uniform.c_str()];
					if (!fnReadArray(array, PluginGuiWidget::kVecCountElements[w.widget_type], w.uniform_idx))
					{
						printf("[Effect_Plugin::LoadParameters(%s)] - invalid eVec parameter %s\n", fPlugin->mHeader.name.c_str(), w.uniform.c_str());
						valid = false;
					}
				}
				else
				{
					printf("[Effect_Plugin::LoadParameters(%s)] - invalid eVec parameter %s\n", fPlugin->mHeader.name.c_str(), w.uniform.c_str());
					valid = false;
				}
				break;

			case PluginGuiWidget::eText:
				break;

			default:
				assert(0);
		}
	}
	return valid;
}

/*	FUNCTION:		Effect_Plugin :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if valid
	DESCRIPTION:	Save plugin parameters
*/
bool Effect_Plugin :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectPluginData *data = (EffectPluginData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes

	sprintf(buffer, "\t\t\t\t\"swap_textures\": %s,\n", data->swap_texture_units ? "true" : "false");
	fwrite(buffer, strlen(buffer), 1, file);

	for (auto &w : fPlugin->mFragmentShader.gui_widgets)
	{
		switch (w.widget_type)
		{
			case PluginGuiWidget::eSlider:
				sprintf(buffer, "\t\t\t\t\"%s\": %f,\n", w.uniform.c_str(), data->uniforms[w.uniform_idx].mFloat);
				fwrite(buffer, strlen(buffer), 1, file);
				break;

			case PluginGuiWidget::eCheckbox:
				sprintf(buffer, "\t\t\t\t\"%s\": %d,\n", w.uniform.c_str(), data->uniforms[w.uniform_idx].mInt);
				fwrite(buffer, strlen(buffer), 1, file);
				break;

			case PluginGuiWidget::eSpinner2:
			case PluginGuiWidget::eSpinner3:
			case PluginGuiWidget::eSpinner4:
			case PluginGuiWidget::eColour:
			{
				sprintf(buffer, "\t\t\t\t\"%s\": [", w.uniform.c_str());
				fwrite(buffer, strlen(buffer), 1, file);
				for (unsigned int idx=0; idx < PluginGuiWidget::kVecCountElements[w.widget_type]; idx++)
				{
					sprintf(buffer, "%f%s", data->uniforms[w.uniform_idx].mVec[idx], idx < PluginGuiWidget::kVecCountElements[w.widget_type] - 1 ? ", " : "");
					fwrite(buffer, strlen(buffer), 1, file);
				}
				sprintf(buffer, "],\n");
				fwrite(buffer, strlen(buffer), 1, file);
				break;
			}

			case PluginGuiWidget::eText:
				break;

			default:
				assert(0);
		}
	}

	return true;
}
