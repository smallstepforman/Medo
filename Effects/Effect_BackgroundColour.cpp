/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Background Colour
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/Picture.h"
#include "Yarra/Render/Texture.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"

#include "Editor/EffectNode.h"
#include "Editor/Project.h"

#include "Effect_BackgroundColour.h"

const char * Effect_BackgroundColour :: GetVendorName() const	{return "ZenYes";}
const char * Effect_BackgroundColour :: GetEffectName() const	{return "Background Colour";}

static const uint32 kMsgColorControl		= 'efbc';

struct EffectBackgroundColourData
{
	uint colour;
};

using namespace yrender;

static const YGeometry_P3 kImageGeometry[] = 
{
	{-1, -1, 0},
	{1, -1, 0},
	{-1, 1, 0},
	{1, 1, 0},
};

/************************
	Background Colour Shader
*************************/
static const char *kVertexShader = "\
	uniform mat4	uTransform; \
	in vec3			aPosition; \
	void main(void) {gl_Position = uTransform * vec4(aPosition, 1.0);}";
	
static const char *kFragmentShader = "\
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
		fShader = new YShader(&attributes, kVertexShader, kFragmentShader);
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

/*	FUNCTION:		Effect_BackgroundColour :: Effect_BackgroundColour
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_BackgroundColour :: Effect_BackgroundColour(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	SetViewColor(216, 216, 216, 255);
	
	fRenderNode = nullptr;
	fSourcePicture = nullptr;

	fColorControl = new BColorControl(BPoint(10, 40), B_CELLS_32x8, 6.0f, "BackgroundColourControl", new BMessage(kMsgColorControl), true);
	AddChild(fColorControl);

	fSampleColour = new BView(BRect(10, 10, 100, 30), "BackgroundColourSample", B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);
	fSampleColour->SetViewColor(0, 0, 0);
	AddChild(fSampleColour);
}

/*	FUNCTION:		Effect_BackgroundColour :: ~Effect_BackgroundColour
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_BackgroundColour :: ~Effect_BackgroundColour()
{ }

/*	FUNCTION:		Effect_BackgroundColour :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_BackgroundColour :: AttachedToWindow()
{
	fColorControl->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_BackgroundColour :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_BackgroundColour :: InitRenderObjects()
{
	assert(fSourcePicture == nullptr);
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fSourcePicture = new YPicture(width, height, true, true);
	fSourcePicture->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0));

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new BackgroundColourShader;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3, (float *)kImageGeometry, 4);
}

/*	FUNCTION:		Effect_BackgroundColour :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_BackgroundColour :: DestroyRenderObjects()
{
	delete fSourcePicture;
	delete fRenderNode;
}

/*	FUNCTION:		Effect_BackgroundColour :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_BackgroundColour :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Effects/icon_fade.png");
	return icon;	
}

/*	FUNCTION:		Effect_BackgroundColour :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_BackgroundColour :: GetTextA(const uint32 language_idx)
{
	return "Set Background Colour";	
}

/*	FUNCTION:		Effect_BackgroundColour :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_BackgroundColour :: GetTextB(const uint32 language_idx)
{
	return "";	
}

/*	FUNCTION:		Effect_BackgroundColour :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_BackgroundColour :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectBackgroundColourData *data = new EffectBackgroundColourData;
	rgb_color aColour = fColorControl->ValueAsColor();
	uint8 alpha = 0xff;
	data->colour = (aColour.blue << 24) | (aColour.green << 16) | (aColour.red << 8) | alpha;
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_BackgroundColour :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_BackgroundColour :: MediaEffectSelected(MediaEffect *effect)
{
	printf("Effect_BackgroundColour::MediaEffectSelected(%p)\n", effect);
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
}

/*	FUNCTION:		Effect_BackgroundColour :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_BackgroundColour :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	if (source)
		fSourcePicture->mTexture->Upload(source);
	
	const ymath::YVector4 background_colour(((EffectBackgroundColourData *)data->mEffectData)->colour);
	((BackgroundColourShader *)fRenderNode->mShaderNode)->SetColour(background_colour);
	
	if (source)
		fSourcePicture->Render(0.0f);
	//	TODO
	glEnable(GL_BLEND);
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_BackgroundColour :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_BackgroundColour :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgColorControl:
		{
			const rgb_color colour = fColorControl->ValueAsColor();
			fSampleColour->SetViewColor(colour);
			fSampleColour->Invalidate();

			if (GetCurrentMediaEffect())
			{
				EffectBackgroundColourData *data = (EffectBackgroundColourData *) (GetCurrentMediaEffect()->mEffectData);
				data->colour = (colour.blue << 24) | (colour.green << 16) | (colour.red << 8) | colour.alpha;
				InvalidatePreview();
			}
			break;
		}

		default:
			printf("Effect_BackgroundColour :: MessageReceived()\n");
			msg->PrintToStream();
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_BackgroundColour :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_BackgroundColour :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	EffectBackgroundColourData *data = (EffectBackgroundColourData *) media_effect->mEffectData;

	//	colour
	if (v.HasMember("colour") && v["colour"].IsUint())
	{
		data->colour = v["colour"].GetUint();
		printf("Colour = %08x\n", data->colour);
	}
	else
		printf("\"colour\" not uint\n");
	
	return true;
}

/*	FUNCTION:		Effect_Fade :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_BackgroundColour :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectBackgroundColourData *data = (EffectBackgroundColourData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"colour\": %u\n", data->colour);
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
