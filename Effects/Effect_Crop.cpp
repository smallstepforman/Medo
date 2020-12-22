/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Crop
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <interface/OptionPopUp.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/Picture.h"
#include "Yarra/Render/Texture.h"
#include "Yarra/Render/MatrixStack.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/MedoWindow.h"
#include "Editor/OutputView.h"
#include "Editor/Project.h"
#include "Editor/RenderActor.h"

#include "Gui/BitmapButton.h"
#include "Gui/Spinner.h"

#include "Effect_Crop.h"

const char * Effect_Crop :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Crop :: GetEffectName() const	{return "Crop";}

enum kMoveMessages
{
	kMsgCenter		= 'ecr0',
	kMsgSize,
};

struct CROP_SPINNER
{
	BRect			rect;
	const char		*name;
	LANGUAGE_TEXT	text;
	const char		*label;
	kMoveMessages	message;
	float			value;
	float			range_min;
	float			range_max;
};
static const CROP_SPINNER kSpinners[] =
{
	{BRect(20, 20, 200, 50),	"center_x",		TXT_EFFECTS_COMMON_CENTER,		" X",	kMsgCenter,		0.5f, 0.0f, 1.0f},
	{BRect(20, 60, 200, 90),	"center_y",		TXT_EFFECTS_COMMON_CENTER,		" Y",	kMsgCenter,		0.5f, 0.0f, 1.0f},
	{BRect(320, 20, 500, 50),	"size_x",		TXT_EFFECTS_COMMON_SIZE,		" X",	kMsgSize,		0.25f, 0.0f, 0.5f},
	{BRect(320, 60, 500, 90),	"size_y",		TXT_EFFECTS_COMMON_SIZE,		" Y",	kMsgSize,		0.25f, 0.0f, 0.5f},
};
static_assert(sizeof(kSpinners)/sizeof(CROP_SPINNER) == 4);

static const BRect kIdealWindowSize(0, 0, 640 + B_V_SCROLL_BAR_WIDTH, 480 + B_H_SCROLL_BAR_HEIGHT);

//	Move Data
struct EffectCropData
{
	ymath::YVector2		center;
	ymath::YVector2		size;
};

static int					sMouseTrackingMode = -1;
static BPoint				sMouseDownPosition;
static ymath::YVector2		sMouseTrackingOriginalCenter;
static ymath::YVector2		sMouseTrackingOriginalSize;

using namespace yrender;


/************************
	TestCropShader
*************************/
#define TEST_CROP_SHADER		0
#if TEST_CROP_SHADER
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
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour = texture(uTextureUnit0, vTexCoord0) * vec4(1,0.25, 0.25, 1.0);\
	}";


class TestCropShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;

public:
	TestCropShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kTextureVertexShader, kTextureFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
	}
	~TestCropShader()
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
#endif

//	End test shader

/*	FUNCTION:		Effect_Crop :: Effect_Move
	ARGS:			frame
					filename
					is_window
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Crop :: Effect_Crop(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	const float kFontFactor = be_plain_font->Size()/20.0f;

	for (int i=0; i < 4; i++)
	{
		char buffer[32];
		sprintf(buffer, "%s%s", GetText(kSpinners[i].text), kSpinners[i].label);
		BRect spinner_rect(kSpinners[i].rect.left*kFontFactor, kSpinners[i].rect.top,
						kSpinners[i].rect.right*kFontFactor, kSpinners[i].rect.bottom);
		fSpinners[i] = new Spinner(spinner_rect, kSpinners[i].name, buffer, new BMessage(kSpinners[i].message));
		fSpinners[i]->SetValue(kSpinners[i].value);
		fSpinners[i]->SetSteps(0.001f);
		fSpinners[i]->SetRange(kSpinners[i].range_min, kSpinners[i].range_max);
		mEffectView->AddChild(fSpinners[i]);
	}

	fStringView[ePixelCenter] = new BStringView(BRect(20*kFontFactor, 100, 300*kFontFactor, 130), nullptr, "Pixels:");
	mEffectView->AddChild(fStringView[ePixelCenter]);
	fStringView[ePixelSize] = new BStringView(BRect(320*kFontFactor, 100, 600*kFontFactor, 130), nullptr, "Pixels:");
	mEffectView->AddChild(fStringView[ePixelSize]);

	char buffer[80];
	unsigned int width = gProject->mResolution.width;
	unsigned int height = gProject->mResolution.height;
	sprintf(buffer, "%s: %d x %d", GetText(TXT_EFFECTS_COMMON_PIXELS), (unsigned int)(kSpinners[0].value*width), (unsigned int)(kSpinners[1].value*height));
	fStringView[ePixelCenter]->SetText(buffer);
	sprintf(buffer, "%s: %d x %d", GetText(TXT_EFFECTS_COMMON_PIXELS), (unsigned int)(kSpinners[2].value*width), (unsigned int)(kSpinners[3].value*height));
	fStringView[ePixelSize]->SetText(buffer);
	
	fRenderNode = nullptr;
}

/*	FUNCTION:		Effect_Crop :: ~Effect_Crop
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Crop :: ~Effect_Crop()
{ }

/*	FUNCTION:		Effect_Crop :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_Crop :: AttachedToWindow()
{
	for (int i=0; i < 4; i++)
		fSpinners[i]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Crop :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Crop :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Crop.png");
	return icon;	
}

/*	FUNCTION:		Effect_Crop :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Crop :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_CROP);
}

/*	FUNCTION:		Effect_Crop :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Crop :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_CROP_TEXT_A);
}

/*	FUNCTION:		Effect_Crop :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Crop :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_CROP_TEXT_B);
}

/*	FUNCTION:		Effect_Colour :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Crop :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
#if TEST_CROP_SHADER
	fRenderNode->mShaderNode = new TestCropShader;
#else
	fRenderNode->mShaderNode = new YMinimalShader;
#endif

	fRenderNode->mGeometryNode =  nullptr;

	for (int i=0; i < 4; i++)
		fOldGeometry[i] = 0.0f;
}

/*	FUNCTION:		Effect_Crop :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Crop :: DestroyRenderObjects()
{
	delete fRenderNode;
	fRenderNode = nullptr;
}

/*	FUNCTION:		Effect_Crop :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Crop :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;

	EffectCropData *data = new EffectCropData;
	media_effect->mEffectData = data;
	memset(data, 0, sizeof(EffectCropData));
	data->center.x = fSpinners[eCenterX]->Value();
	data->center.y = fSpinners[eCenterY]->Value();
	data->size.x = fSpinners[eSizeX]->Value();
	data->size.y = fSpinners[eSizeY]->Value();

	return media_effect;	
}

/*	FUNCTION:		Effect_Crop :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Crop :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;
	
	//	Update GUI
	EffectCropData *data = (EffectCropData *) effect->mEffectData;
	fSpinners[eCenterX]->SetValue(data->center.x);
	fSpinners[eCenterY]->SetValue(data->center.y);
	fSpinners[eSizeX]->SetValue(data->size.x);
	fSpinners[eSizeY]->SetValue(data->size.y);

	char buffer[80];
	unsigned int width = gProject->mResolution.width;
	unsigned int height = gProject->mResolution.height;
	sprintf(buffer, "%s: %d x %d", GetText(TXT_EFFECTS_COMMON_PIXELS), (unsigned int)(data->center.x * width), (unsigned int)(data->center.y*height));
	fStringView[ePixelCenter]->SetText(buffer);
	sprintf(buffer, "%s: %d x %d", GetText(TXT_EFFECTS_COMMON_PIXELS), (unsigned int)(data->size.x*width), (unsigned int)(data->size.y*height));
	fStringView[ePixelSize]->SetText(buffer);
}

/*	FUNCTION:		Effect_Crop :: RenderEffect
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Update geometry
*/
void Effect_Crop :: UpdateGeometry(MediaEffect *effect)
{
	assert(effect != nullptr);
	EffectCropData *data = (EffectCropData *) effect->mEffectData;
	if ((data->center.x == fOldGeometry[0]) &&
		(data->center.y == fOldGeometry[1]) &&
		(data->size.x == fOldGeometry[2]) &&
		(data->size.y == fOldGeometry[3]))
		return;

	delete fRenderNode->mGeometryNode;

	YGeometry_P3T2	geometry[4];
	float *p = (float *)geometry;
	*p++ = data->center.x - data->size.x;		*p++ = data->center.y - data->size.y;		*p++ = 0.0f;		*p++ = data->center.x - data->size.x;	*p++ = data->center.y - data->size.y;
	*p++ = data->center.x + data->size.x;		*p++ = data->center.y - data->size.y;		*p++ = 0.0f;		*p++ = data->center.x + data->size.x;	*p++ = data->center.y - data->size.y;
	*p++ = data->center.x - data->size.x;		*p++ = data->center.y + data->size.y;		*p++ = 0.0f;		*p++ = data->center.x - data->size.x;	*p++ = data->center.y + data->size.y;
	*p++ = data->center.x + data->size.x;		*p++ = data->center.y + data->size.y;		*p++ = 0.0f;		*p++ = data->center.x + data->size.x;	*p++ = data->center.y + data->size.y;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)geometry, 4);

	fOldGeometry[0] = data->center.x;
	fOldGeometry[1] = data->center.y;
	fOldGeometry[2] = data->size.x;
	fOldGeometry[3] = data->size.y;
}

/*	FUNCTION:		Effect_Crop :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Crop :: RenderEffect(BBitmap *source, MediaEffect *effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	if (effect->mEffectData == nullptr)
		return;

	UpdateGeometry(effect);
	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0, 0, 0));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(width, height, 0));

	if (source != gRenderActor->GetBackgroundBitmap())
	{
		fRenderNode->mTexture = gRenderActor->GetPicture((unsigned int)source->Bounds().Width() + 1, (unsigned int)source->Bounds().Height() + 1, source)->mTexture;
	}
	gRenderActor->EffectResetPrimaryRenderBuffer();
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_Crop :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Crop :: MessageReceived(BMessage *msg)
{
	EffectCropData *data = nullptr;
	if (GetCurrentMediaEffect())
		data = (EffectCropData *) GetCurrentMediaEffect()->mEffectData;
	
	switch (msg->what)
	{
		case kMsgCenter:
			if (data)
			{
				data->center.x = fSpinners[eCenterX]->Value();
				data->center.y = fSpinners[eCenterY]->Value();

				char buffer[80];
				sprintf(buffer, "%s: %d x %d", GetText(TXT_EFFECTS_COMMON_PIXELS), (unsigned int)(data->center.x * gProject->mResolution.width), (unsigned int)(data->center.y * gProject->mResolution.height));
				fStringView[ePixelCenter]->SetText(buffer);

				InvalidatePreview();
			}
			break;
				
		case kMsgSize:
			if (data)
			{
				data->size.x = fSpinners[eSizeX]->Value();
				data->size.y = fSpinners[eSizeY]->Value();

				char buffer[80];
				sprintf(buffer, "%s: %d x %d", GetText(TXT_EFFECTS_COMMON_PIXELS), (unsigned int)(data->size.x*gProject->mResolution.width), (unsigned int)(data->size.y*gProject->mResolution.height));
				fStringView[ePixelSize]->SetText(buffer);

				InvalidatePreview();
			}
			break;
		
		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		Effect_Crop :: OutputViewMouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Output view mouse down
*/
void Effect_Crop :: OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)
{
	if (!media_effect)
	{
		sMouseTrackingMode = -1;
		return;
	}

	EffectCropData *data = (EffectCropData *) media_effect->mEffectData;
	sMouseDownPosition = point;

	//	Default to move
	sMouseTrackingMode = 0;
	sMouseTrackingOriginalCenter = data->center;

	MedoWindow::GetInstance()->LockLooper();
	BRect bounds =  MedoWindow::GetInstance()->GetOutputView()->Bounds();
	MedoWindow::GetInstance()->UnlockLooper();

	ymath::YVector2 down_point(point.x/bounds.Width(), point.y/bounds.Height());
	const float kGrace = 0.05f;
	const ymath::YVector2 kOffset[4] = {{-1, -1}, {1,-1}, {-1, 1}, {1, 1}};
	for (int i=0; i < 4; i++)
	{
		if ((down_point.x > data->center.x + kOffset[i].x*data->size.x - kGrace) &&
			(down_point.x < data->center.x + kOffset[i].x*data->size.x + kGrace) &&
			(down_point.y > data->center.y + kOffset[i].y*data->size.y - kGrace) &&
			(down_point.y < data->center.y + kOffset[i].y*data->size.y + kGrace))
		{
			//	Resize
			sMouseTrackingMode = i+1;
			sMouseTrackingOriginalSize = data->size;
			break;
		}
	}
}

/*	FUNCTION:		Effect_Crop :: OutputViewMouseMoved
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Output view mouse moved
*/
void Effect_Crop :: OutputViewMouseMoved(MediaEffect *media_effect, const BPoint &point)
{
	if (sMouseTrackingMode < 0)
		return;

	MedoWindow::GetInstance()->LockLooper();
	BRect bounds =  MedoWindow::GetInstance()->GetOutputView()->Bounds();
	MedoWindow::GetInstance()->UnlockLooper();

	EffectCropData *data = (EffectCropData *) media_effect->mEffectData;
	ymath::YVector2 offset(point.x - sMouseDownPosition.x, point.y - sMouseDownPosition.y);
	switch (sMouseTrackingMode)
	{
		case 0:	//	Move
			data->center = sMouseTrackingOriginalCenter + ymath::YVector2(offset.x/bounds.Width(), offset.y/bounds.Height());
			break;

		case 1:	//	Left/top
			data->center = sMouseTrackingOriginalCenter + ymath::YVector2(offset.x/bounds.Width(), offset.y/bounds.Height())*0.5f;
			data->size = sMouseTrackingOriginalSize + ymath::YVector2(-offset.x/bounds.Width(), -offset.y/bounds.Height())*0.5f;
			break;
		case 2:	//	Right/top
			data->center = sMouseTrackingOriginalCenter + ymath::YVector2(offset.x/bounds.Width(), offset.y/bounds.Height())*0.5f;
			data->size = sMouseTrackingOriginalSize + ymath::YVector2(offset.x/bounds.Width(), -offset.y/bounds.Height())*0.5f;
			break;
		case 3:	//	Left/bottom
			data->center = sMouseTrackingOriginalCenter + ymath::YVector2(offset.x/bounds.Width(), offset.y/bounds.Height())*0.5f;
			data->size = sMouseTrackingOriginalSize + ymath::YVector2(-offset.x/bounds.Width(), offset.y/bounds.Height())*0.5f;
			break;
		case 4:	//	Right/bottom
			data->center = sMouseTrackingOriginalCenter + ymath::YVector2(offset.x/bounds.Width(), offset.y/bounds.Height())*0.5f;
			data->size = sMouseTrackingOriginalSize + ymath::YVector2(offset.x/bounds.Width(), offset.y/bounds.Height())*0.5f;
			break;

			break;
		default:
			assert(0);
	}

	if (!Window()->IsHidden())
	{
		fSpinners[eCenterX]->SetValue(data->center.x);
		fSpinners[eCenterY]->SetValue(data->center.y);
		fSpinners[eSizeX]->SetValue(data->size.x);
		fSpinners[eSizeY]->SetValue(data->size.y);

		char buffer[80];
		sprintf(buffer, "%s: %d x %d", GetText(TXT_EFFECTS_COMMON_PIXELS), (unsigned int)(data->center.x * gProject->mResolution.width), (unsigned int)(data->center.y * gProject->mResolution.height));
		fStringView[ePixelCenter]->SetText(buffer);
		sprintf(buffer, "%s: %d x %d", GetText(TXT_EFFECTS_COMMON_PIXELS), (unsigned int)(data->size.x*gProject->mResolution.width), (unsigned int)(data->size.y*gProject->mResolution.height));
		fStringView[ePixelSize]->SetText(buffer);
	}
	InvalidatePreview();
}

/*	FUNCTION:		Effect_Crop :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Crop :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	bool valid = true;	
	EffectCropData *data = (EffectCropData *) media_effect->mEffectData;
	memset(data, 0, sizeof(EffectCropData));
	
#define ERROR_EXIT(a) {printf("[Effect_Colour] Error - %s\n", a); return false;}

	//	center
	if (v.HasMember("center") && v["center"].IsArray())
	{
		const rapidjson::Value &array = v["center"];
		if (array.Size() != 2)
			ERROR_EXIT("center invalid");
		for (rapidjson::SizeType e=0; e < 2; e++)
		{
			if (!array[e].IsFloat())
				ERROR_EXIT("center invalid");
			data->center.v[e] = array[e].GetFloat();
		}
	}
	else
		ERROR_EXIT("missing center");

	//	size
	if (v.HasMember("size") && v["size"].IsArray())
	{
		const rapidjson::Value &array = v["size"];
		if (array.Size() != 2)
			ERROR_EXIT("size invalid");
		for (rapidjson::SizeType e=0; e < 2; e++)
		{
			if (!array[e].IsFloat())
				ERROR_EXIT("size invalid");
			data->size.v[e] = array[e].GetFloat();
		}
	}
	else
		ERROR_EXIT("missing size");
	
	return valid;
}

/*	FUNCTION:		Effect_Crop :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Save project data
*/
bool Effect_Crop :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectCropData *d = (EffectCropData *) media_effect->mEffectData;
	if (d)
	{
		char buffer[0x80];	//	128 bytes
		sprintf(buffer, "\t\t\t\t\"center\": [%f, %f],\n", d->center.x, d->center.y);
		fwrite(buffer, strlen(buffer), 1, file);	
		sprintf(buffer, "\t\t\t\t\"size\": [%f, %f]\n", d->size.x, d->size.y);
		fwrite(buffer, strlen(buffer), 1, file);	
	}
	return true;	
}
