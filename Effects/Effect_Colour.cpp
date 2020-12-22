/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Colour + Alpha
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
#include "Gui/AlphaColourControl.h"

#include "Effect_Colour.h"

const char * Effect_Colour :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Colour :: GetEffectName() const	{return "Alpha/Colour";}

enum kColourMessages
{
	kMsgColorControl0		= 'efc0',
	kMsgColorControl1,
	kMsgInterpolate,
};

struct EffectColourData
{
	rgb_color	start_colour;
	rgb_color	end_colour;
	bool		interpolate;
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
	Texture Colour Shader
*************************/
static const char *kTextureVertexShader = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture0;\
	out vec2		vTexCoord0;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);vTexCoord0 = aTexture0;\
	}";

static const char *kTextureFragmentShader = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec4		uColour;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour = texture(uTextureUnit0, vTexCoord0) * uColour;\
	}";


class TextureColourShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;
	GLint				fLocation_uColour;
	ymath::YVector4		fColour;

public:
	TextureColourShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kTextureVertexShader, kTextureFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
		fLocation_uColour = fShader->GetUniformLocation("uColour");
	}
	~TextureColourShader()
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

/************************
	Background Colour Shader
*************************/
static const char *kBackgroundVertexShader = "\
	uniform mat4	uTransform; \
	in vec3			aPosition; \
	void main(void) {gl_Position = uTransform * vec4(aPosition, 1.0);}";

static const char *kBackgroundFragmentShader = "\
	uniform vec4	uColour;\
	out vec4		fFragColour; \
	void main(void) {fFragColour = uColour;}";

class BackgroundColourShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uColour;
	ymath::YVector4		fColour;

public:
	BackgroundColourShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexCoord0");
		fShader = new YShader(&attributes, kBackgroundVertexShader, kBackgroundFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uColour = fShader->GetUniformLocation("uColour");
	}
	~BackgroundColourShader()
	{
		delete fShader;
	}
	void SetColour(const ymath::YVector4 &colour) {fColour = colour;}
	void Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform4fv(fLocation_uColour, 1, fColour.v);
	}
};

/*	FUNCTION:		Effect_Colour :: Effect_Colour
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Colour :: Effect_Colour(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fRenderNodeTexture = nullptr;
	fRenderNodeBackground = nullptr;

	fGuiInterpolate = new BCheckBox(BRect(10, 10, 200, 40), "interpolate", GetText(TXT_EFFECTS_COMMON_INTERPOLATE), new BMessage(kMsgInterpolate));
	fGuiInterpolate->SetValue(0);
	mEffectView->AddChild(fGuiInterpolate);

	BStringView *title = new BStringView(BRect(110, 50, 300, 80), nullptr, GetText(TXT_EFFECTS_COMMON_START));
	title->SetFont(be_bold_font);
	mEffectView->AddChild(title);
	fGuiSampleColours[0] = new BView(BRect(10, 60, 100, 80), nullptr, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);
	fGuiSampleColours[0]->SetViewColor(255, 255, 255);
	mEffectView->AddChild(fGuiSampleColours[0]);
	fGuiColourControls[0] = new AlphaColourControl(BPoint(10, 100), "BackgroundColourControl0", new BMessage(kMsgColorControl0));
	fGuiColourControls[0]->SetValue({255, 255, 255, 255});
	mEffectView->AddChild(fGuiColourControls[0]);

	title = new BStringView(BRect(110, 250, 300, 280), nullptr, GetText(TXT_EFFECTS_COMMON_END));
	title->SetFont(be_bold_font);
	mEffectView->AddChild(title);
	fGuiSampleColours[1] = new BView(BRect(10, 260, 100, 280), nullptr, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);
	fGuiSampleColours[1]->SetViewColor(216, 216, 216);
	mEffectView->AddChild(fGuiSampleColours[1]);
	fGuiColourControls[1] = new AlphaColourControl(BPoint(10, 300), "BackgroundColourControl0", new BMessage(kMsgColorControl1));
	fGuiColourControls[1]->SetValue({255, 255, 255, 255});
	fGuiColourControls[1]->SetEnabled(false);
	mEffectView->AddChild(fGuiColourControls[1]);
}

/*	FUNCTION:		Effect_Colour :: ~Effect_Colour
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Colour :: ~Effect_Colour()
{ }

/*	FUNCTION:		Effect_Colour :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Colour :: AttachedToWindow()
{
	fGuiInterpolate->SetTarget(this, Window());
	fGuiColourControls[0]->SetTarget(this, Window());
	fGuiColourControls[1]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Colour :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Colour :: InitRenderObjects()
{
	assert(fRenderNodeTexture == nullptr);
	assert(fRenderNodeBackground == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNodeTexture = new yrender::YRenderNode;
	fRenderNodeTexture->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNodeTexture->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNodeTexture->mShaderNode = new TextureColourShader;
	fRenderNodeTexture->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kFadeGeometry, 4);

	fRenderNodeBackground = new yrender::YRenderNode;
	fRenderNodeBackground->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNodeBackground->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNodeBackground->mShaderNode = new BackgroundColourShader;
	fRenderNodeBackground->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kFadeGeometry, 4);
}

/*	FUNCTION:		Effect_Colour :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Colour :: DestroyRenderObjects()
{
	delete fRenderNodeTexture;		fRenderNodeTexture = nullptr;
	delete fRenderNodeBackground;	fRenderNodeBackground = nullptr;
}

/*	FUNCTION:		Effect_Colour :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Colour :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Colour.png");
	return icon;	
}

/*	FUNCTION:		Effect_Colour :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Colour :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR);
}

/*	FUNCTION:		Effect_Colour :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Colour :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_TEXT_A);
}

/*	FUNCTION:		Effect_Colour :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Colour :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_TEXT_B);
}

/*	FUNCTION:		Effect_Colour :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Colour :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectColourData *data = new EffectColourData;
	data->start_colour = fGuiColourControls[0]->ValueAsColor();
	data->start_colour.alpha = 255;
	data->end_colour = fGuiColourControls[1]->ValueAsColor();
	data->end_colour.alpha = 0;
	data->interpolate = fGuiInterpolate->Value();
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_Colour :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Colour :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	EffectColourData *data = (EffectColourData *)effect->mEffectData;

	//	Update GUI
	fGuiInterpolate->SetValue(data->interpolate);
	fGuiColourControls[0]->SetValue(data->start_colour);
	fGuiColourControls[1]->SetValue(data->end_colour);
	fGuiSampleColours[0]->SetViewColor(data->start_colour);
	fGuiSampleColours[1]->SetViewColor(data->end_colour);
}

/*	FUNCTION:		Effect_Colour :: ChainedColourEffect
	ARGS:			data
					frame_idx
	RETURN:			colour factor
	DESCRIPTION:	Get colour factor
*/
rgb_color Effect_Colour :: ChainedColourEffect(MediaEffect *effect, int64 frame_idx)
{
	EffectColourData *data = (EffectColourData *)effect->mEffectData;
	if (!data->interpolate)
		return data->start_colour;

	float t = float(frame_idx - effect->mTimelineFrameStart)/float(effect->Duration());
	if (t > 1.0f)
		t = 1.0f;
	rgb_color colour;
	colour.red = data->start_colour.red + (data->end_colour.red - data->start_colour.red)*t;
	colour.green = data->start_colour.green + (data->end_colour.green - data->start_colour.green)*t;
	colour.blue = data->start_colour.blue + (data->end_colour.blue - data->start_colour.blue)*t;
	colour.alpha = data->start_colour.alpha + (data->end_colour.alpha - data->start_colour.alpha)*t;
	return colour;
}

/*	FUNCTION:		Effect_Colour :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Colour :: RenderEffect(BBitmap *source, MediaEffect *effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	EffectColourData *data = (EffectColourData *)effect->mEffectData;
	rgb_color colour;
	if (!data->interpolate)
		colour = data->start_colour;
	else
	{
		float t = float(frame_idx - effect->mTimelineFrameStart)/float(effect->Duration());
		if (t > 1.0f)
			t = 1.0f;
		colour.red = data->start_colour.red + (data->end_colour.red - data->start_colour.red)*t;
		colour.green = data->start_colour.green + (data->end_colour.green - data->start_colour.green)*t;
		colour.blue = data->start_colour.blue + (data->end_colour.blue - data->start_colour.blue)*t;
		colour.alpha = data->start_colour.alpha + (data->end_colour.alpha - data->start_colour.alpha)*t;
	}

	const ymath::YVector4 shader_colour(colour.blue/255.0f, colour.green/255.0f, colour.red/255.0f, colour.alpha/255.0f);	//	BGRA

	if (source != gRenderActor->GetBackgroundBitmap())
	{
		((TextureColourShader *)fRenderNodeTexture->mShaderNode)->SetColour(shader_colour);
		fRenderNodeTexture->mTexture = gRenderActor->GetPicture((unsigned int)source->Bounds().Width() + 1, (unsigned int)source->Bounds().Height() + 1, source)->mTexture;
		fRenderNodeTexture->Render(0.0f);
	}
	else
	{
		((BackgroundColourShader *)fRenderNodeBackground->mShaderNode)->SetColour(shader_colour);
		fRenderNodeBackground->Render(0.0f);
	}
}

/*	FUNCTION:		Effect_Colour :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Colour :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgInterpolate:
			fGuiColourControls[1]->SetEnabled(fGuiInterpolate->Value());
			if (fGuiInterpolate->Value() > 0)
				fGuiSampleColours[1]->SetViewColor(fGuiColourControls[1]->ValueAsColor());
			else
				fGuiSampleColours[1]->SetViewColor(ViewColor());
			fGuiSampleColours[1]->Invalidate();

			if (GetCurrentMediaEffect())
			{
				((EffectColourData *)GetCurrentMediaEffect()->mEffectData)->interpolate = fGuiInterpolate->Value();
				InvalidatePreview();
			}
			break;
				
		case kMsgColorControl0:
			fGuiSampleColours[0]->SetViewColor(fGuiColourControls[0]->ValueAsColor());
			fGuiSampleColours[0]->Invalidate();
			if (GetCurrentMediaEffect())
			{
				((EffectColourData *)GetCurrentMediaEffect()->mEffectData)->start_colour = fGuiColourControls[0]->ValueAsColor();
				InvalidatePreview();
			}
			break;

		case kMsgColorControl1:
			fGuiSampleColours[1]->SetViewColor(fGuiColourControls[1]->ValueAsColor());
			fGuiSampleColours[1]->Invalidate();
			if (GetCurrentMediaEffect())
			{
				((EffectColourData *)GetCurrentMediaEffect()->mEffectData)->end_colour = fGuiColourControls[1]->ValueAsColor();
				InvalidatePreview();
			}
			break;

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_Colour :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Colour :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	EffectColourData *data = (EffectColourData *) media_effect->mEffectData;

#define ERROR_EXIT(a) {printf("[Effect_Colour] Error - %s\n", a); return false;}

	//	start
	if (v.HasMember("start") && v["start"].IsArray())
	{
		const rapidjson::Value &array = v["start"];
		if (array.Size() != 4)
			ERROR_EXIT("start invalid");
		uint32_t colour[4];
		for (rapidjson::SizeType e=0; e < 4; e++)
		{
			if (!array[e].IsInt())
				ERROR_EXIT("start invalid");
			colour[e] = array[e].GetInt();
			if ((colour[e] < 0) || (colour[e] > 255))
				ERROR_EXIT("start invalid");
		}
		data->start_colour.red = (uint8)colour[0];
		data->start_colour.green = (uint8)colour[1];
		data->start_colour.blue = (uint8)colour[2];
		data->start_colour.alpha = (uint8)colour[3];
	}
	else
		ERROR_EXIT("missing start");

	//	end
	if (v.HasMember("end") && v["end"].IsArray())
	{
		const rapidjson::Value &array = v["end"];
		if (array.Size() != 4)
			ERROR_EXIT("end invalid");
		uint32_t colour[4];
		for (rapidjson::SizeType e=0; e < 4; e++)
		{
			if (!array[e].IsInt())
				ERROR_EXIT("end invalid");
			colour[e] = array[e].GetInt();
			if ((colour[e] < 0) || (colour[e] > 255))
				ERROR_EXIT("end invalid");
		}
		data->end_colour.red = (uint8)colour[0];
		data->end_colour.green = (uint8)colour[1];
		data->end_colour.blue = (uint8)colour[2];
		data->end_colour.alpha = (uint8)colour[3];
	}
	else
		ERROR_EXIT("missing end");

	//	interpolate
	if (v.HasMember("interpolate") && v["interpolate"].IsBool())
		data->interpolate = v["interpolate"].GetBool();
	else
		ERROR_EXIT("missing interpolate");
	
	return true;
}

/*	FUNCTION:		Effect_Colour :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Colour :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectColourData *data = (EffectColourData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes

	sprintf(buffer, "\t\t\t\t\"start\": [%d, %d, %d, %d],\n", data->start_colour.red, data->start_colour.green, data->start_colour.blue, data->start_colour.alpha);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"end\": [%d, %d, %d, %d],\n", data->end_colour.red, data->end_colour.green, data->end_colour.blue, data->end_colour.alpha);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"interpolate\": %s\n", data->interpolate ? "true" : "false");
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
