/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Marker
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <translation/TranslationUtils.h>
#include <interface/ChannelSlider.h>

#include "Yarra/Render/Texture.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"
#include "Yarra/Render/Picture.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/MedoWindow.h"
#include "Editor/OutputView.h"
#include "Editor/Project.h"
#include "Editor/RenderActor.h"

#include "Gui/AlphaColourControl.h"
#include "Gui/Spinner.h"
#include "Gui/ValueSlider.h"

#include "Effect_Marker.h"

const char * Effect_Marker :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Marker :: GetEffectName() const	{return "Marker";}

enum kMarkerMessages
{
	kMsgMarkerColour		= 'efm0',
	kMsgMarkerSpinnerStart,
	kMsgMarkerSpinnerEnd,
	kMsgMarkerWidth,
	kMsgMarkerInterpolate,
	kMsgMarkerDelayStart,
	kMsgMarkerDelayEnd,
	kMsgMarkerBackground,
	kMsgMarkerMaskColour,
	kMsgMarkerMaskType,
	kMsgMarkerMaskFilter,
};

struct EffectMarkerData
{
	ymath::YVector2		start_position;
	ymath::YVector2		end_position;
	rgb_color			colour;
	float				width;

	bool				interpolate;
	float				start_delay;
	float				end_delay;

	bool				background;
	rgb_color			mask_colour;
	int					mask_type;
	float				mask_filter;
};

struct SPINNER_LAYOUT
{
	const BRect		rect;
	const char		*id;
	LANGUAGE_TEXT	text;
	const float		min_value;
	const float		max_value;
	const float		default_value;
	const uint32	message;
};
static const SPINNER_LAYOUT kSpinnerLayouts[] =
{
	{BRect(500, 50, 700, 80),		"start_x",		TXT_EFFECTS_TEXT_MARKER_START_X,	0,		10000,		0.25f,		kMsgMarkerSpinnerStart},
	{BRect(500, 90, 700, 120),		"start_y",		TXT_EFFECTS_TEXT_MARKER_START_Y,	0,		10000,		0.5f,		kMsgMarkerSpinnerStart},
	{BRect(500, 150, 700, 180),		"end_x",		TXT_EFFECTS_TEXT_MARKER_END_X,		0,		10000,		0.75f,		kMsgMarkerSpinnerEnd},
	{BRect(500, 190, 700, 220),		"end_y",		TXT_EFFECTS_TEXT_MARKER_END_Y,		0,		10000,		0.5f,		kMsgMarkerSpinnerEnd},
};

static const float	kDefaultWidth = 0.03f;
static const float	kWidthSliderFactor = 4000;

using namespace yrender;

/************************
	Colour Shader
*************************/
static const char *kVertexShader = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture;\
	out vec2		vTexCoord;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);\
		vTexCoord = aTexture;\
	}";

static const char *kFragmentShader = "\
	uniform vec4		uColour;\
	uniform float		uTime;\
	in vec2				vTexCoord;\
	out vec4			fFragColour;\
	void main(void) {\
		if (vTexCoord.x > uTime) discard;\
		fFragColour = uColour;\
	}";


class MarkerShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uColour;
	GLint				fLocation_uTime;

	ymath::YVector4		fColour;
	float				fTime;

public:
	MarkerShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		fShader = new YShader(&attributes, kVertexShader, kFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uColour = fShader->GetUniformLocation("uColour");
		fLocation_uTime = fShader->GetUniformLocation("uTime");
	}
	~MarkerShader()
	{
		delete fShader;
	}
	void Configure(const ymath::YVector4 &colour, const float time)
	{
		fColour = colour;
		fTime = time;
	}
	void Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform4fv(fLocation_uColour, 1, fColour.v);
		glUniform1f(fLocation_uTime, fTime);
	}
};

/************************
	Background Shader
*************************/
static const char *kVertexShader_Background = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture;\
	out vec2		vTexCoord;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);\
		vTexCoord = aTexture;\
	}";

static const char *kFragmentShader_Background = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec4		uColour;\
	uniform float		uTime;\
	uniform vec4		uMaskColour;\
	uniform int			uMaskType;\
	uniform float		uMaskFilter;\
	in vec2				vTexCoord;\
	out vec4			fFragColour;\
	void main(void) {\
		if (vTexCoord.x > uTime)\
			discard;\
		vec4 colour = texture(uTextureUnit0, vTexCoord);\
		float dist = distance(colour, uMaskColour);\
		if (uMaskType == 0) {\
			if (dist < uMaskFilter)\
				fFragColour = uColour;\
			else\
				discard;\
		}\
		else {\
			if (dist < uMaskFilter)\
				discard;\
			else\
				fFragColour = uColour;\
		}\
	}";

class MarkerBackgroundShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;
	GLint				fLocation_uColour;
	GLint				fLocation_uTime;
	GLint				fLocation_uMaskColour;
	GLint				fLocation_uMaskType;
	GLint				fLocation_uMaskFilter;

	ymath::YVector4		fColour;
	float				fTime;
	ymath::YVector4		fMaskColour;
	int					fMaskType;
	float				fMaskFilter;

public:
	MarkerBackgroundShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		fShader = new YShader(&attributes, kVertexShader_Background, kFragmentShader_Background);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
		fLocation_uColour = fShader->GetUniformLocation("uColour");
		fLocation_uTime = fShader->GetUniformLocation("uTime");
		fLocation_uMaskColour = fShader->GetUniformLocation("uMaskColour");
		fLocation_uMaskType = fShader->GetUniformLocation("uMaskType");
		fLocation_uMaskFilter = fShader->GetUniformLocation("uMaskFilter");
	}
	~MarkerBackgroundShader()
	{
		delete fShader;
	}
	void Configure(const ymath::YVector4 &colour, const float time, const ymath::YVector4 &mask_colour, const bool mask_type, const float filter)
	{
		fColour = colour;
		fTime = time;
		fMaskColour = mask_colour;
		fMaskType = mask_type;
		fMaskFilter = filter;
	}
	void Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0, 0);
		glUniform4fv(fLocation_uColour, 1, fColour.v);
		glUniform1f(fLocation_uTime, fTime);
		glUniform4fv(fLocation_uMaskColour, 1, fMaskColour.v);
		glUniform1i(fLocation_uMaskType, fMaskType);
		glUniform1f(fLocation_uMaskFilter, fMaskFilter);
	}
};

/*	FUNCTION:		Effect_Marker :: Effect_Marker
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Marker :: Effect_Marker(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fRenderNode = nullptr;

	//	Colour
	BStringView *title = new BStringView(BRect(110, 20, 300, 50), nullptr, GetText(TXT_EFFECTS_COMMON_COLOUR));
	title->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	title->SetFont(be_bold_font);
	mEffectView->AddChild(title);
	fGuiSampleColour = new BView(BRect(10, 30, 100, 50), nullptr, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);
	fGuiSampleColour->SetViewColor(255, 255, 0);
	mEffectView->AddChild(fGuiSampleColour);
	fGuiColourControl = new AlphaColourControl(BPoint(10, 70), "BackgroundColourControl0", new BMessage(kMsgMarkerColour));
	fGuiColourControl->SetValue({255, 255, 0, 255});
	mEffectView->AddChild(fGuiColourControl);

	//	Position(s)
	for (int i=0; i < 4; i++)
	{
		fGuiPositionSpinners[i] = new Spinner(kSpinnerLayouts[i].rect, kSpinnerLayouts[i].id, GetText(kSpinnerLayouts[i].text), new BMessage(kSpinnerLayouts[i].message));
		fGuiPositionSpinners[i]->SetRange(kSpinnerLayouts[i].min_value, kSpinnerLayouts[i].max_value);
		fGuiPositionSpinners[i]->SetValue(kSpinnerLayouts[i].default_value);
		fGuiPositionSpinners[i]->SetSteps(0.001f);
		mEffectView->AddChild(fGuiPositionSpinners[i]);
	}

	//	Width
	fGuiSliderWidth = new ValueSlider(BRect(20, 240, 640, 320), "width_slider", GetText(TXT_EFFECTS_TEXT_MARKER_WIDTH), nullptr, 0, 1000);
	fGuiSliderWidth->SetModificationMessage(new BMessage(kMsgMarkerWidth));
	fGuiSliderWidth->SetValue(kDefaultWidth*kWidthSliderFactor);
	fGuiSliderWidth->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGuiSliderWidth->SetHashMarkCount(10);
	fGuiSliderWidth->SetLimitLabels("0.0", "0.25");
	fGuiSliderWidth->UpdateTextValue(kDefaultWidth);
	fGuiSliderWidth->SetFloatingPointPrecision(3);
	mEffectView->AddChild(fGuiSliderWidth);

	//	Interpolate
	BBox *interpolate_box = new BBox(BRect(10, 340, 280, 510), "box_interpolate");
	interpolate_box->SetLabel(GetText(TXT_EFFECTS_COMMON_INTERPOLATE));
	mEffectView->AddChild(interpolate_box);

	fGuiCheckboxInterpolate = new BCheckBox(BRect(10, 30, 260, 60), "interpolate", GetText(TXT_EFFECTS_TEXT_MARKER_USE_INTERPOLATION), new BMessage(kMsgMarkerInterpolate));
	interpolate_box->AddChild(fGuiCheckboxInterpolate);

	//	Delay sliders
	fGuiSlidersDelay[0] = new BChannelSlider(BRect(10, 70, 260, 110), "delay_start", GetText(TXT_EFFECTS_COMMON_START), new BMessage(kMsgMarkerDelayStart));
	fGuiSlidersDelay[0]->SetValue(25);
	fGuiSlidersDelay[0]->SetEnabled(false);
	interpolate_box->AddChild(fGuiSlidersDelay[0]);
	fGuiSlidersDelay[1] = new BChannelSlider(BRect(10, 120, 260, 160), "delay_end", GetText(TXT_EFFECTS_COMMON_END), new BMessage(kMsgMarkerDelayEnd));
	fGuiSlidersDelay[1]->SetValue(75);
	fGuiSlidersDelay[1]->SetEnabled(false);
	interpolate_box->AddChild(fGuiSlidersDelay[1]);

	//	Background
	BBox *background_box = new BBox(BRect(300, 340, 740, 620), "box_background");
	background_box->SetLabel(GetText(TXT_EFFECTS_TEXT_SIMPLE_BACKGROUND));
	mEffectView->AddChild(background_box);

	fGuiCheckboxBackground = new BCheckBox(BRect(10, 30, 200, 60), "background", GetText(TXT_EFFECTS_TEXT_MARKER_ENABLE_BACKGROUND), new BMessage(kMsgMarkerBackground));
	background_box->AddChild(fGuiCheckboxBackground);

	fGuiColourMaskColour = new BColorControl(BPoint(10, 60), B_CELLS_32x8, 6.0f, "mask_colour", new BMessage(kMsgMarkerMaskColour), true);
	background_box->AddChild(fGuiColourMaskColour);

	fGuiOptionMaskType = new BOptionPopUp(BRect(10, 150, 300, 190), "mask_type", GetText(TXT_EFFECTS_TEXT_MARKER_MASK), new BMessage(kMsgMarkerMaskType));
	fGuiOptionMaskType->AddOption(GetText(TXT_EFFECTS_TEXT_MARKER_MASK_BACKGROUND), 0);
	fGuiOptionMaskType->AddOption(GetText(TXT_EFFECTS_TEXT_MARKER_MASK_TEXT), 1);
	background_box->AddChild(fGuiOptionMaskType);

	fGuiSliderMaskFilter = new ValueSlider(BRect(10, 190, 430, 230), "mask_filter", GetText(TXT_EFFECTS_TEXT_MARKER_FILTER), nullptr, 0, 2000);
	fGuiSliderMaskFilter->SetModificationMessage(new BMessage(kMsgMarkerMaskFilter));
	fGuiSliderMaskFilter->SetValue(500);
	fGuiSliderMaskFilter->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGuiSliderMaskFilter->SetHashMarkCount(10);
	fGuiSliderMaskFilter->SetLimitLabels("0.0", "2.0");
	fGuiSliderMaskFilter->SetFloatingPointPrecision(3);
	fGuiSliderMaskFilter->UpdateTextValue(0.100f);
	background_box->AddChild(fGuiSliderMaskFilter);

	//	defaults
	fPreviousStartPosition.Set(0, 0);
	fPreviousEndPosition.Set(0, 0);
	fPreviousWidth = 0.0f;
	fMouseTrackingIndex = -1;

	SetViewIdealSize(780, 740);
}

/*	FUNCTION:		Effect_Marker :: ~Effect_Marker
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Marker :: ~Effect_Marker()
{ }

/*	FUNCTION:		Effect_Marker :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Marker :: AttachedToWindow()
{
	fGuiColourControl->SetTarget(this, Window());
	fGuiSliderWidth->SetTarget(this, Window());

	fGuiCheckboxInterpolate->SetTarget(this, Window());
	fGuiSlidersDelay[0]->SetTarget(this, Window());
	fGuiSlidersDelay[1]->SetTarget(this, Window());

	fGuiCheckboxBackground->SetTarget(this, Window());
	fGuiColourMaskColour->SetTarget(this, Window());
	fGuiOptionMaskType->SetTarget(this, Window());
	fGuiSliderMaskFilter->SetTarget(this, Window());

	for (int i=0; i < 4; i++)
		fGuiPositionSpinners[i]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Marker :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Marker :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0, 0, 0));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(width, height, 0));
	fRenderNode->mTexture = new YTexture(width, height);

	fShaderColour = new MarkerShader;
	fShaderBackground = new MarkerBackgroundShader;
}

/*	FUNCTION:		Effect_Marker :: CreateRenderGeometry
	ARGS:			start, end
	RETURN:			n/a
	DESCRIPTION:	Create render geometry
*/
void Effect_Marker :: CreateRenderGeometry(const ymath::YVector2 &start, const ymath::YVector2 &end, const float width)
{
	YGeometry_P3T2 geom[4];
	float *p = (float *) &geom;

	ymath::YVector2 dir = end - start;
	dir.Normalise();
	ymath::YVector2 perp(-dir.y, dir.x);

	ymath::YVector2 pos;
	pos = start - perp*width;	*p++ = pos.x;		*p++ = pos.y;	*p++ = 0.0f;			*p++ = pos.x;		*p++ = pos.y;
	pos = start + perp*width;	*p++ = pos.x;		*p++ = pos.y;	*p++ = 0.0f;			*p++ = pos.x;		*p++ = pos.y;
	pos = end - perp*width;		*p++ = pos.x;		*p++ = pos.y;	*p++ = 0.0f;			*p++ = pos.x;		*p++ = pos.y;
	pos = end + perp*width;		*p++ = pos.x;		*p++ = pos.y;	*p++ = 0.0f;			*p++ = pos.x;		*p++ = pos.y;

	delete fRenderNode->mGeometryNode;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)&geom, 4);
}

/*	FUNCTION:		Effect_Marker :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Marker :: DestroyRenderObjects()
{
	fRenderNode->mShaderNode = nullptr;
	delete fRenderNode;			fRenderNode = nullptr;

	delete fShaderColour;		fShaderColour = nullptr;
	delete fShaderBackground;	fShaderBackground = nullptr;
}

/*	FUNCTION:		Effect_Marker :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Marker :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Marker.png");
	return icon;	
}

/*	FUNCTION:		Effect_Marker :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Marker :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_MARKER);
}

/*	FUNCTION:		Effect_Marker :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Marker :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_MARKER_TEXT_A);
}

/*	FUNCTION:		Effect_Marker :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Marker :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_MARKER_TEXT_B);
}

/*	FUNCTION:		Effect_Marker :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Marker :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectMarkerData *data = new EffectMarkerData;
	data->start_position.x = fGuiPositionSpinners[0]->Value();
	data->start_position.y = fGuiPositionSpinners[1]->Value();
	data->end_position.x = fGuiPositionSpinners[2]->Value();
	data->end_position.y = fGuiPositionSpinners[3]->Value();
	data->colour = fGuiColourControl->ValueAsColor();
	data->colour.alpha = 255;
	data->width = fGuiSliderWidth->Value()/kWidthSliderFactor;

	data->interpolate = fGuiCheckboxInterpolate->Value();
	data->start_delay = fGuiSlidersDelay[0]->Value()/100.0f;
	data->end_delay = fGuiSlidersDelay[1]->Value()/100.0f;

	data->background = fGuiCheckboxBackground->Value();
	data->mask_colour = fGuiColourMaskColour->ValueAsColor();
	data->mask_type = fGuiOptionMaskType->Value();
	data->mask_filter = fGuiSliderMaskFilter->Value()/1000.0f;
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_Marker :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Marker :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	EffectMarkerData *data = (EffectMarkerData *)effect->mEffectData;

	//	Update GUI
	fGuiPositionSpinners[0]->SetValue(data->start_position.x);
	fGuiPositionSpinners[1]->SetValue(data->start_position.y);
	fGuiPositionSpinners[2]->SetValue(data->end_position.x);
	fGuiPositionSpinners[3]->SetValue(data->end_position.y);
	fGuiColourControl->SetValue(data->colour);
	fGuiSampleColour->SetViewColor(data->colour);
	fGuiSliderWidth->SetValue(data->width*kWidthSliderFactor);

	fGuiCheckboxInterpolate->SetValue(data->interpolate);
	fGuiSlidersDelay[0]->SetValue(data->start_delay*100.0f);
	fGuiSlidersDelay[0]->SetEnabled(data->interpolate);
	fGuiSlidersDelay[1]->SetValue(data->end_delay*100.0f);
	fGuiSlidersDelay[1]->SetEnabled(data->interpolate);
	fGuiSliderWidth->UpdateTextValue(data->width);

	fGuiCheckboxBackground->SetValue(data->background);
	fGuiColourMaskColour->SetValue(data->mask_colour);
	fGuiColourMaskColour->SetEnabled(data->background > 0 ? true : false);
	fGuiOptionMaskType->SetValue(data->mask_type);
	fGuiOptionMaskType->SetEnabled(data->background > 0 ? true : false);
	fGuiSliderMaskFilter->SetValue(data->mask_filter*1000);
	fGuiSliderMaskFilter->UpdateTextValue(data->mask_filter);
	fGuiSliderMaskFilter->SetEnabled(data->background > 0 ? true : false);
}

/*	FUNCTION:		Effect_Marker :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Marker :: RenderEffect(BBitmap *source, MediaEffect *effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	EffectMarkerData *data = (EffectMarkerData *)effect->mEffectData;
	const ymath::YVector4 shader_colour(data->colour.blue/255.0f, data->colour.green/255.0f, data->colour.red/255.0f, data->colour.alpha/255.0f);	//	BGRA

	bool background = fGuiCheckboxBackground->Value() > 0;
	if (background)
	{
		fRenderNode->mTexture = gRenderActor->GetPicture((unsigned int)source->Bounds().Width() + 1, (unsigned int)source->Bounds().Height() + 1, source)->mTexture;
	}

	//	Recreate geometry?
	if ((!fRenderNode->mGeometryNode) || (fPreviousStartPosition != data->start_position) || (fPreviousEndPosition != data->end_position) || (fPreviousWidth != data->width))
	{
		CreateRenderGeometry(data->start_position, data->end_position, data->width);
		fPreviousStartPosition = data->start_position;
		fPreviousEndPosition = data->end_position;
		fPreviousWidth = data->width;
	}

	//	interpolation
	float t = float(frame_idx - effect->mTimelineFrameStart)/float(effect->Duration());
	if (data->interpolate)
	{
		float t1 = data->start_delay;
		float t2 = data->end_delay;
		if (t < t1)
			t = t1;
		else if (t > t2)
			t = 1.0f;
		else
		{
			t = t1 + (1.0f-t1)*float(frame_idx - (effect->mTimelineFrameStart + t1*effect->Duration())) / float((t2-t1)*effect->Duration());
		}

		t = data->start_position.x + t*(data->end_position.x - data->start_position.x);
	}
	else
		t = 1.0f;

	if (background)
	{
		ymath::YVector4 background_colour(data->mask_colour.red/255.0f, data->mask_colour.green/255.0f, data->mask_colour.blue/255.0f, 1.0f);
		((MarkerBackgroundShader *)fShaderBackground)->Configure(shader_colour, t, background_colour, data->mask_type, data->mask_filter);
		fRenderNode->mShaderNode = fShaderBackground;
	}
	else
	{
		((MarkerShader *)fShaderColour)->Configure(shader_colour, t);
		fRenderNode->mShaderNode = fShaderColour;
	}

	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_Marker :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Marker :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgMarkerColour:
			fGuiSampleColour->SetViewColor(fGuiColourControl->ValueAsColor());
			fGuiSampleColour->Invalidate();
			if (GetCurrentMediaEffect())
			{
				((EffectMarkerData *)GetCurrentMediaEffect()->mEffectData)->colour = fGuiColourControl->ValueAsColor();
				InvalidatePreview();
			}
			break;

		case kMsgMarkerSpinnerStart:
			if (GetCurrentMediaEffect())
			{
				EffectMarkerData *data = (EffectMarkerData *)GetCurrentMediaEffect()->mEffectData;
				data->start_position.x = fGuiPositionSpinners[0]->Value();
				data->start_position.y = fGuiPositionSpinners[1]->Value();
				InvalidatePreview();
			}
			break;

		case kMsgMarkerSpinnerEnd:
			if (GetCurrentMediaEffect())
			{
				EffectMarkerData *data = (EffectMarkerData *)GetCurrentMediaEffect()->mEffectData;
				data->end_position.x = fGuiPositionSpinners[2]->Value();
				data->end_position.y = fGuiPositionSpinners[3]->Value();
				InvalidatePreview();
			}
			break;

		case kMsgMarkerWidth:
			if (GetCurrentMediaEffect())
			{
				EffectMarkerData *data = (EffectMarkerData *)GetCurrentMediaEffect()->mEffectData;
				int32 value = fGuiSliderWidth->Value();
				data->width = value/kWidthSliderFactor;
				fGuiSliderWidth->UpdateTextValue(data->width);
				InvalidatePreview();
			}
			break;

		case kMsgMarkerInterpolate:
		{
			int32 interpolate = fGuiCheckboxInterpolate->Value();
			fGuiSlidersDelay[0]->SetEnabled(interpolate > 0);
			fGuiSlidersDelay[1]->SetEnabled(interpolate > 0);
			if (GetCurrentMediaEffect())
			{
				((EffectMarkerData *)GetCurrentMediaEffect()->mEffectData)->interpolate = interpolate;
				InvalidatePreview();
			}
			break;
		}

		case kMsgMarkerDelayStart:
		{
			int32 v0 = fGuiSlidersDelay[0]->Value();
			int32 v1 = fGuiSlidersDelay[1]->Value();
			if (v0 > v1)
			{
				v0 = v1;
				fGuiSlidersDelay[0]->SetValue(v0);
			}
			if (GetCurrentMediaEffect())
			{
				EffectMarkerData *data = (EffectMarkerData *)GetCurrentMediaEffect()->mEffectData;
				data->start_delay = v0/100.0f;
				InvalidatePreview();
			}
			break;
		}

		case kMsgMarkerDelayEnd:
		{
			int32 v0 = fGuiSlidersDelay[0]->Value();
			int32 v1 = fGuiSlidersDelay[1]->Value();
			if (v1 < v0)
			{
				v1 = v0;
				fGuiSlidersDelay[1]->SetValue(v1);
			}
			if (GetCurrentMediaEffect())
			{
				EffectMarkerData *data = (EffectMarkerData *)GetCurrentMediaEffect()->mEffectData;
				data->end_delay = v1/100.0f;
				InvalidatePreview();
			}
			break;
		}

		case kMsgMarkerBackground:
		{
			int32 value = fGuiCheckboxBackground->Value();
			fGuiColourMaskColour->SetEnabled(value > 0 ? true : false);
			fGuiOptionMaskType->SetEnabled(value > 0 ? true : false);
			fGuiSliderMaskFilter->SetEnabled(value > 0 ? true : false);
			if (GetCurrentMediaEffect())
			{
				EffectMarkerData *data = (EffectMarkerData *)GetCurrentMediaEffect()->mEffectData;
				data->background = value;
				InvalidatePreview();
			}
			break;
		}
		case kMsgMarkerMaskColour:
			if (GetCurrentMediaEffect())
			{
				EffectMarkerData *data = (EffectMarkerData *)GetCurrentMediaEffect()->mEffectData;
				data->mask_colour = fGuiColourMaskColour->ValueAsColor();
				InvalidatePreview();
			}
			break;
		case kMsgMarkerMaskType:
			if (GetCurrentMediaEffect())
			{
				EffectMarkerData *data = (EffectMarkerData *)GetCurrentMediaEffect()->mEffectData;
				data->mask_type = fGuiOptionMaskType->Value();
				InvalidatePreview();
			}
			break;
		case kMsgMarkerMaskFilter:
			fGuiSliderMaskFilter->UpdateTextValue(fGuiSliderMaskFilter->Value()/1000.0f);
			if (GetCurrentMediaEffect())
			{
				EffectMarkerData *data = (EffectMarkerData *)GetCurrentMediaEffect()->mEffectData;
				data->mask_filter = fGuiSliderMaskFilter->Value()/1000.0f;
				InvalidatePreview();
			}
			break;

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		Effect_Marker :: OutputViewMouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Output view mouse down
*/
void Effect_Marker :: OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)
{
	EffectMarkerData *data = (EffectMarkerData *) media_effect->mEffectData;

	MedoWindow::GetInstance()->LockLooper();
	BPoint converted = MedoWindow::GetInstance()->GetOutputView()->GetProjectConvertedMouseDown(point);
	MedoWindow::GetInstance()->UnlockLooper();

	fPreviousConvertedMouseDownPosition = converted;
	fMouseTrackingIndex = -1;

	const float kGrace = 0.02f;
	if ((converted.x > data->start_position.x - kGrace) && (converted.x < data->start_position.x+kGrace) && (converted.y > data->start_position.y - kGrace) && (converted.y < data->start_position.y+kGrace))
	{
		//	left reposition
		fMouseTrackingIndex = 0;
	}
	else if ((converted.x > data->end_position.x - kGrace) && (converted.x < data->end_position.x+kGrace) && (converted.y > data->end_position.y - kGrace) && (converted.y < data->end_position.y+kGrace))
	{
		//	right reposition
		fMouseTrackingIndex = 1;
	}
	else
	{
		//	drag/move (TODO check if withing bounds)
		fMouseTrackingIndex = 2;
		fMouseMovedStartPosition = data->start_position;
		fMouseMovedEndPosition = data->end_position;
	}

}

/*	FUNCTION:		Effect_Marker :: OutputViewMouseMoved
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Output view mouse moved
*/
void Effect_Marker :: OutputViewMouseMoved(MediaEffect *media_effect, const BPoint &point)
{
	if (fMouseTrackingIndex < 0)
		return;

	MedoWindow::GetInstance()->LockLooper();
	BPoint converted = MedoWindow::GetInstance()->GetOutputView()->GetProjectConvertedMouseDown(point);
	MedoWindow::GetInstance()->UnlockLooper();

	EffectMarkerData *data = (EffectMarkerData *) media_effect->mEffectData;

	const float kGraceY = 0.01f;	//	vertical alignment (snap)

	switch (fMouseTrackingIndex)
	{
		case 0:		//	left reposition
			if ((converted.y > data->end_position.y - kGraceY) && (converted.y < data->end_position.y + 0.01f))
				converted.y = data->end_position.y;
			data->start_position.Set(converted.x, converted.y);
			fGuiPositionSpinners[0]->SetValue(converted.x);
			fGuiPositionSpinners[1]->SetValue(converted.y);
			break;

		case 1:		//	right reposition
			if ((converted.y > data->start_position.y - kGraceY) && (converted.y < data->start_position.y + 0.01f))
				converted.y = data->start_position.y;
			data->end_position.Set(converted.x, converted.y);
			fGuiPositionSpinners[2]->SetValue(converted.x);
			fGuiPositionSpinners[3]->SetValue(converted.y);

			break;

		case 2:		//	move
		{
			BPoint delta = converted - fPreviousConvertedMouseDownPosition;
			data->start_position = fMouseMovedStartPosition + ymath::YVector2(delta.x, delta.y);
			data->end_position = fMouseMovedEndPosition + ymath::YVector2(delta.x, delta.y);
			fGuiPositionSpinners[0]->SetValue(data->start_position.x);
			fGuiPositionSpinners[1]->SetValue(data->start_position.y);
			fGuiPositionSpinners[2]->SetValue(data->end_position.x);
			fGuiPositionSpinners[3]->SetValue(data->end_position.y);
			break;
		}

		default:
			assert(0);
	}

	InvalidatePreview();
}

/*	FUNCTION:		Effect_Marker :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Marker :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	EffectMarkerData *data = (EffectMarkerData *) media_effect->mEffectData;

#define ERROR_EXIT(a) {printf("[Effect_Colour] Error - %s\n", a); return false;}

	//	start position
	if (v.HasMember("start position") && v["start position"].IsArray())
	{
		const rapidjson::Value &array = v["start position"];
		if (array.Size() != 2)
			ERROR_EXIT("start position invalid");
		for (rapidjson::SizeType e=0; e < 2; e++)
		{
			if (!array[e].IsFloat())
				ERROR_EXIT("start position invalid");
			data->start_position.v[e] = array[e].GetFloat();
			if ((data->start_position.v[e] < 0.0f) || (data->start_position.v[e] > 1.0f))
				ERROR_EXIT("start position invalid");
		}
	}
	else
		ERROR_EXIT("missing start position");

	//	end position
	if (v.HasMember("end position") && v["end position"].IsArray())
	{
		const rapidjson::Value &array = v["end position"];
		if (array.Size() != 2)
			ERROR_EXIT("end position invalid");
		for (rapidjson::SizeType e=0; e < 2; e++)
		{
			if (!array[e].IsFloat())
				ERROR_EXIT("end position invalid");
			data->end_position.v[e] = array[e].GetFloat();
			if ((data->end_position.v[e] < 0.0f) || (data->end_position.v[e] > 1.0f))
				ERROR_EXIT("end position invalid");
		}
	}
	else
		ERROR_EXIT("missing start position");

	//	colour
	if (v.HasMember("colour") && v["colour"].IsArray())
	{
		const rapidjson::Value &array = v["colour"];
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
		data->colour.red = (uint8)colour[0];
		data->colour.green = (uint8)colour[1];
		data->colour.blue = (uint8)colour[2];
		data->colour.alpha = (uint8)colour[3];
	}
	else
		ERROR_EXIT("missing colour");

	//	width
	if (v.HasMember("width") && v["width"].IsFloat())
	{
		data->width = v["width"].GetFloat();
		if ((data->width < 0.0f) || (data->width > 0.25f))
			ERROR_EXIT("width invalid");
	}
	else
		ERROR_EXIT("missing float");

	//	interpolate
	if (v.HasMember("interpolate") && v["interpolate"].IsBool())
		data->interpolate = v["interpolate"].GetBool();
	else
		ERROR_EXIT("missing interpolate");

	//	start delay
	if (v.HasMember("start delay") && v["start delay"].IsFloat())
	{
		data->start_delay = v["start delay"].GetFloat();
		if ((data->start_delay < 0.0f) || (data->start_delay > 1.0f))
			ERROR_EXIT("start delay invalid");
	}
	else
		ERROR_EXIT("missing start delay");

	//	end delay
	if (v.HasMember("end delay") && v["end delay"].IsFloat())
	{
		data->end_delay = v["end delay"].GetFloat();
		if ((data->end_delay < 0.0f) || (data->end_delay > 1.0f))
			ERROR_EXIT("end delay invalid");
	}
	else
		ERROR_EXIT("missing end delay");

	//	background
	if (v.HasMember("background") && v["background"].IsBool())
		data->background = v["background"].GetBool();
	else
		ERROR_EXIT("missing background");

	//	mask colour
	if (v.HasMember("mask colour") && v["mask colour"].IsArray())
	{
		const rapidjson::Value &array = v["mask colour"];
		if (array.Size() != 4)
			ERROR_EXIT("mask colour invalid");
		uint32_t colour[4];
		for (rapidjson::SizeType e=0; e < 4; e++)
		{
			if (!array[e].IsInt())
				ERROR_EXIT("mask colour invalid");
			colour[e] = array[e].GetInt();
			if ((colour[e] < 0) || (colour[e] > 255))
				ERROR_EXIT("mask colour invalid");
		}
		data->mask_colour.red = (uint8)colour[0];
		data->mask_colour.green = (uint8)colour[1];
		data->mask_colour.blue = (uint8)colour[2];
		data->mask_colour.alpha = (uint8)colour[3];
	}
	else
		ERROR_EXIT("missing background colour");

	//	mask type
	if (v.HasMember("mask type") && v["mask type"].IsInt())
	{
		data->mask_type = v["mask type"].GetInt();
		if ((data->mask_type < 0) || (data->mask_type > 1))
			ERROR_EXIT("mask type invalid");
	}
	else
		ERROR_EXIT("missing mask type");

	//	filter
	if (v.HasMember("mask filter") && v["mask filter"].IsFloat())
	{
		data->mask_filter = v["mask filter"].GetFloat();
		if ((data->mask_filter < 0.0f) || (data->mask_filter > 2.0f))
			ERROR_EXIT("mask filter invalid");
	}
	else
		ERROR_EXIT("missing mask filter");

	
	return true;
}

/*	FUNCTION:		Effect_Marker :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Marker :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectMarkerData *data = (EffectMarkerData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes

	sprintf(buffer, "\t\t\t\t\"start position\": [%f, %f],\n", data->start_position.x, data->start_position.y);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"end position\": [%f, %f],\n", data->end_position.x, data->end_position.y);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"colour\": [%d, %d, %d, %d],\n", data->colour.red, data->colour.green, data->colour.blue, data->colour.alpha);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"width\": %f,\n", data->width);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"interpolate\": %s,\n", data->interpolate ? "true" : "false");
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"start delay\": %f,\n", data->start_delay);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"end delay\": %f\n,", data->end_delay);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"background\": %s,\n", data->background ? "true" : "false");
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"background colour\": [%d, %d, %d, %d],\n", data->mask_colour.red, data->mask_colour.green, data->mask_colour.blue, data->mask_colour.alpha);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"background mask\": %d\n,", data->mask_type);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"background filter\": %f\n", data->mask_filter);
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
