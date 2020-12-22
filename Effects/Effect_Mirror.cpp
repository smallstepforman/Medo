/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Mirror
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

#include "Effect_Mirror.h"

const char * Effect_Mirror :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Mirror :: GetEffectName() const	{return "Mirror";}

enum kMirrorMessages
{
	kMsgLeftRight		= 'efm0',
	kMsgRightLeft,
	kMsgUpDown,
	kMsgDownUp,
};

struct EffectMirrorData
{
	uint32		direction;
};

using namespace yrender;

static const YGeometry_P3T2 kMirrorGeometryLeftRight[] =
{
	{-1, 1, 0,		0, 1},
	{-1, -1, 0,		0, 0},
	{0, 1, 0,		0.5, 1},
	{0, -1, 0,		0.5, 0},
	{1, 1, 0,		0, 1},
	{1, -1, 0,		0, 0},
};
static const YGeometry_P3T2 kMirrorGeometryRightLeft[] =
{
	{-1, 1, 0,		1, 1},
	{-1, -1, 0,		1, 0},
	{0, 1, 0,		0.5, 1},
	{0, -1, 0,		0.5, 0},
	{1, 1, 0,		1, 1},
	{1, -1, 0,		1, 0},
};
static const YGeometry_P3T2 kMirrorGeometryUpDown[] =
{
	{1, 1, 0,		1, 0},
	{-1, 1, 0,		0, 0},
	{1, 0, 0,		1, 0.5},
	{-1, 0, 0,		0, 0.5},
	{1, -1, 0,		1, 0},
	{-1, -1, 0,		0, 0},
};
static const YGeometry_P3T2 kMirrorGeometryDownUp[] =
{
	{1, 1, 0,		1, 1},
	{-1, 1, 0,		0, 1},
	{1, 0, 0,		1, 0.5},
	{-1, 0, 0,		0, 0.5},
	{1, -1, 0,		1, 1},
	{-1, -1, 0,		0, 1},
};

/************************
	Mirror Shader
*************************/
static const char *kVertexShader = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture0;\
	out vec2		vTexCoord0;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);vTexCoord0 = aTexture0;\
	}";
	
static const char *kFragmentShader = "\
	uniform sampler2D	uTextureUnit0;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour = texture(uTextureUnit0, vTexCoord0);\
	}";


class MirrorShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;
	ymath::YVector4		fColour;
	
public:
	MirrorShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kVertexShader, kFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
	}
	~MirrorShader()
	{
		delete fShader;
	}
	void Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0, 0);
	}	
};

/*	FUNCTION:		Effect_Mirror :: Effect_Mirror
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Mirror :: Effect_Mirror(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fRenderNode = nullptr;
	for (int i=0; i < eNumberRadioButtons; i++)
		fGeometryNodes[i] = nullptr;

	fGuiButtons[0]= new BRadioButton(BRect(40, 40, 300, 70), "mirror_0", GetText(TXT_EFFECTS_IMAGE_MIRROR_LEFT_RIGHT), new BMessage(kMsgLeftRight));
	fGuiButtons[1]= new BRadioButton(BRect(40, 80, 300, 110), "mirror_1", GetText(TXT_EFFECTS_IMAGE_MIRROR_RIGHT_LEFT), new BMessage(kMsgRightLeft));
	fGuiButtons[2] = new BRadioButton(BRect(40, 120, 300, 150), "mirror_2", GetText(TXT_EFFECTS_IMAGE_MIRROR_UP_DOWN), new BMessage(kMsgUpDown));
	fGuiButtons[3] = new BRadioButton(BRect(40, 160, 300, 190), "mirror_3", GetText(TXT_EFFECTS_IMAGE_MIRROR_DOWN_UP), new BMessage(kMsgDownUp));
	fGuiButtons[0]->SetValue(1);
	for (int i=0; i < eNumberRadioButtons; i++)
		mEffectView->AddChild(fGuiButtons[i]);
}

/*	FUNCTION:		Effect_Mirror :: ~Effect_Mirror
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Mirror :: ~Effect_Mirror()
{ }

/*	FUNCTION:		Effect_Mirror :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Mirror :: AttachedToWindow()
{
	for (int i=0; i < eNumberRadioButtons; i++)
		fGuiButtons[i]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Mirror :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Mirror :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new MirrorShader;

	fGeometryNodes[0] = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kMirrorGeometryLeftRight, sizeof(kMirrorGeometryLeftRight)/sizeof(YGeometry_P3T2));
	fGeometryNodes[1] = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kMirrorGeometryRightLeft, sizeof(kMirrorGeometryRightLeft)/sizeof(YGeometry_P3T2));
	fGeometryNodes[2] = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kMirrorGeometryUpDown, sizeof(kMirrorGeometryUpDown)/sizeof(YGeometry_P3T2));
	fGeometryNodes[3] = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kMirrorGeometryDownUp, sizeof(kMirrorGeometryDownUp)/sizeof(YGeometry_P3T2));

	fRenderNode->mGeometryNode = fGeometryNodes[0];
	//fRenderNode->mTexture = new YTexture(width, height);
}

/*	FUNCTION:		Effect_Mirror :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Mirror :: DestroyRenderObjects()
{
	for (int i=0; i < eNumberRadioButtons; i++)
		delete fGeometryNodes[i];
	fRenderNode->mGeometryNode = nullptr;
	delete fRenderNode;
	fRenderNode = nullptr;
}

/*	FUNCTION:		Effect_Mirror :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Mirror :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Mirror.png");
	return icon;	
}

/*	FUNCTION:		Effect_Mirror :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Mirror :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_MIRROR);
}

/*	FUNCTION:		Effect_Mirror :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Mirror :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_MIRROR);
}

/*	FUNCTION:		Effect_Mirror :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Mirror :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_MIRROR_TEXT_B);
}

/*	FUNCTION:		Effect_Mirror :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Mirror :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectMirrorData *data = new EffectMirrorData;
	data->direction = 0;
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_Mirror :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Mirror :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
	for (int i=0; i < eNumberRadioButtons; i++)
		fGuiButtons[i]->SetValue(((EffectMirrorData *)effect->mEffectData)->direction == i ? 1 : 0);
}

/*	FUNCTION:		Effect_Mirror :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Mirror :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	int direction = ((EffectMirrorData *)data->mEffectData)->direction;
	fRenderNode->mGeometryNode = fGeometryNodes[direction];
	if (source)
		fRenderNode->mTexture = gRenderActor->GetPicture((unsigned int)source->Bounds().Width() + 1, (unsigned int)source->Bounds().Height() + 1, source)->mTexture;
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_Mirror :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Mirror :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgLeftRight:
			if (GetCurrentMediaEffect())
				((EffectMirrorData *)GetCurrentMediaEffect()->mEffectData)->direction = 0;
			InvalidatePreview();
			break;
				
		case kMsgRightLeft:
			if (GetCurrentMediaEffect())
				((EffectMirrorData *)GetCurrentMediaEffect()->mEffectData)->direction = 1;
			InvalidatePreview();
			break;

		case kMsgUpDown:
			if (GetCurrentMediaEffect())
				((EffectMirrorData *)GetCurrentMediaEffect()->mEffectData)->direction = 2;
			InvalidatePreview();
			break;

		case kMsgDownUp:
			if (GetCurrentMediaEffect())
				((EffectMirrorData *)GetCurrentMediaEffect()->mEffectData)->direction = 3;
			InvalidatePreview();
			break;

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_Mirror :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Mirror :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	//	direction
	if (v.HasMember("direction") && v["direction"].IsUint())
	{
		EffectMirrorData *data = (EffectMirrorData *) media_effect->mEffectData;
		data->direction = v["direction"].GetUint();
		if (data->direction >= eNumberRadioButtons)
			data->direction = 0;
	}
	
	return true;
}

/*	FUNCTION:		Effect_Mirror :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Mirror :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectMirrorData *data = (EffectMirrorData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"direction\": \"%u\"\n", data->direction);
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}
