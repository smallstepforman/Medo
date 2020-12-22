/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Mask
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/Texture.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/Picture.h"
#include "Yarra/Render/MatrixStack.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/MedoWindow.h"
#include "Editor/OutputView.h"
#include "Editor/Project.h"
#include "Editor/RenderActor.h"

#include "Gui/PathView.h"
#include "Gui/BitmapCheckbox.h"
#include "Gui/KeyframeSlider.h"

#include "Effect_Mask.h"

const char * Effect_Mask :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Mask :: GetEffectName() const	{return "Mask";}

enum MASK_MESSAGES
{
	kMsgPathCheckbox		= 'emm0',
	kMsgPathViewUpdate,
	kMsgShowFillCheckbox,
	kMsgInverseCheckbox,
	kMsgKeyframeSelect,
	kMsgKeyframeAdd,
	kMsgKeyframeRemove,
	kMsgKeyframeSlider,
};

struct KeyframeData
{
	std::vector<BPoint>		path;
	float					timeline;
};
struct EffectMaskData
{
	int							inverse;
	std::vector<KeyframeData>	keyframes;
};

static BBitmap *sBitmap = nullptr;
static yrender::YTexture *sTexture1 = nullptr;

using namespace yrender;

static const YGeometry_P3T2 kFadeGeometry[] =
{
	{-1, -1, 0,		0, 0},
	{1, -1, 0,		1, 0},
	{-1, 1, 0,		0, 1},
	{1, 1, 0,		1, 1},
};

/************************
	Mask Shader
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
	uniform sampler2D	uTextureUnit1;\
	uniform int			uInverse;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour = texture(uTextureUnit0, vTexCoord0);\
		if (uInverse > 0)\
			fFragColour *= vec4(1,1,1,1)-texture(uTextureUnit1, vTexCoord0);\
		else\
			fFragColour *= texture(uTextureUnit1, vTexCoord0);\
	}";

class MaskShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;
	GLint				fLocation_uTextureUnit1;
	GLint				fLocation_uInverse;
	int					fInverse;
	
public:
	MaskShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kVertexShader, kFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
		fLocation_uTextureUnit1 = fShader->GetUniformLocation("uTextureUnit1");
		fLocation_uInverse = fShader->GetUniformLocation("uInverse");
	}
	~MaskShader()
	{
		delete fShader;
	}
	void	SetInverse(int value) {fInverse = value;}
	void	Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0, 0);
		glUniform1i(fLocation_uTextureUnit1, 1);
		glUniform1i(fLocation_uInverse, fInverse);
	}	
};

/*************************************
	Effect_Mask
**************************************/

/*	FUNCTION:		Effect_Mask :: Effect_Mask
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Mask :: Effect_Mask(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	const float kFontFactor = be_plain_font->Size()/20.0f;

	fRenderNode = nullptr;
	fPathView = nullptr;
	fPathViewAttachedToWindow = false;

	if (!sBitmap)
		sBitmap = new BBitmap(BRect(0, 0, 1919, 1079), B_RGBA32, true);

	fPathView = new PathView(frame);

	//	Path
	fPathCheckbox = new BitmapCheckbox(BRect(20, 20, 60, 60), "path",
											 BTranslationUtils::GetBitmap("Resources/icon_path_off.png"),
											 BTranslationUtils::GetBitmap("Resources/icon_path_on.png"),
											 new BMessage(kMsgPathCheckbox));
	fPathCheckbox->SetValue(1);
	mEffectView->AddChild(fPathCheckbox);

	//	Inverse
	fInverseCheckbox = new BCheckBox(BRect(100, 20, 300, 50), "inverse", GetText(TXT_EFFECTS_IMAGE_MASK_INVERSE), new BMessage(kMsgInverseCheckbox));
	mEffectView->AddChild(fInverseCheckbox);

	//	fill polygon
	fShowFillCheckbox = new BCheckBox(BRect(100, 50, 300, 80), "fill", GetText(TXT_EFFECTS_IMAGE_MASK_SHOW), new BMessage(kMsgShowFillCheckbox));
	mEffectView->AddChild(fShowFillCheckbox);

	//	Keyframe GUI
	BStringView *title = new BStringView(BRect(20, 100, 200, 130), nullptr, GetText(TXT_EFFECTS_COMMON_KEYFRAMES));
	title->SetFont(be_bold_font);
	mEffectView->AddChild(title);

	fKeyframeList = new BListView(BRect(20, 150, 200, 250), "keyframes");
	fKeyframeList->Select(0);
	fKeyframeList->SetSelectionMessage(new BMessage(kMsgKeyframeSelect));
	mEffectView->AddChild(new BScrollView("list_scroll", fKeyframeList, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));

	char buffer[32];
	sprintf(buffer, "%s #1", GetText(TXT_EFFECTS_COMMON_KEYFRAME));
	fKeyframeList->AddItem(new BStringItem(buffer));
	fKeyframeList->Select(0);

	fKeyframeAddButton = new BButton(BRect(240, 150, 340, 180), "add_keyframe", GetText(TXT_EFFECTS_COMMON_ADD), new BMessage(kMsgKeyframeAdd));
	mEffectView->AddChild(fKeyframeAddButton);
	fKeyframeRemoveButton = new BButton(BRect(240, 190, 340, 220), "remove_keyframe", GetText(TXT_EFFECTS_COMMON_REMOVE), new BMessage(kMsgKeyframeRemove));
	fKeyframeRemoveButton->SetEnabled(false);
	mEffectView->AddChild(fKeyframeRemoveButton);

	fKeyframeSlider = new KeyframeSlider(BRect(20, 280, 600*kFontFactor, 320));
	mEffectView->AddChild(fKeyframeSlider);
	fCurrentKeyframe = 0;
}

/*	FUNCTION:		Effect_Mask :: ~Effect_Mask
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Mask :: ~Effect_Mask()
{
	if (!fPathViewAttachedToWindow)
		delete fPathView;
}

/*	FUNCTION:		Effect_Mask :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Mask :: AttachedToWindow()
{
	fPathView->SetObserver((BLooper *)Window(), this, new BMessage(kMsgPathViewUpdate));

	MedoWindow::GetInstance()->LockLooper();
	MedoWindow::GetInstance()->GetOutputView()->AddChild(fPathView);
	fPathViewAttachedToWindow = true;
	MedoWindow::GetInstance()->UnlockLooper();

	fPathCheckbox->SetTarget(this, Window());
	fInverseCheckbox->SetTarget(this, Window());
	fShowFillCheckbox->SetTarget(this, Window());
	fKeyframeList->SetTarget(this, Window());
	fKeyframeAddButton->SetTarget(this, Window());
	fKeyframeRemoveButton->SetTarget(this, Window());

	fKeyframeSlider->SetObserver((BLooper *)Window(), this, new BMessage(kMsgKeyframeSlider));
}

/*	FUNCTION:		Effect_Mask :: DetachedFromWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_Mask :: DetachedFromWindow()
{
	if (MedoWindow::GetInstance() && MedoWindow::GetInstance()->LockLooper())
	{
		MedoWindow::GetInstance()->GetOutputView()->RemoveChild(fPathView);
		fPathViewAttachedToWindow = false;
		MedoWindow::GetInstance()->UnlockLooper();
	}
}

/*	FUNCTION:		Effect_Mask :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Mask :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new MaskShader;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kFadeGeometry, 4);

	sTexture1 = new YTexture(width, height);
	sTexture1->SetTextureUnitIndex(1);
}

/*	FUNCTION:		Effect_Mask :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Mask :: DestroyRenderObjects()
{
	delete fRenderNode;
}

/*	FUNCTION:		Effect_Mask :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Mask :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Mask.png");
	return icon;	
}

/*	FUNCTION:		Effect_Mask :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Mask :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_MASK);
}

/*	FUNCTION:		Effect_Mask :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Mask :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_MASK_TEXT_A);
}

/*	FUNCTION:		Effect_Mask :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Mask :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_IMAGE_MASK_TEXT_B);
}

/*	FUNCTION:		Effect_Mask :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Mask :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectMaskData *data = new EffectMaskData;
	data->inverse = 0;
	media_effect->mEffectData = data;

	//	Initial keyframe with a path
	KeyframeData keyframe;
	keyframe.timeline = 0.0f;
	keyframe.path.push_back(BPoint(0.25, 0.25));
	keyframe.path.push_back(BPoint(0.75, 0.25));
	keyframe.path.push_back(BPoint(0.75, 0.75));
	keyframe.path.push_back(BPoint(0.25, 0.75));
	data->keyframes.push_back(keyframe);

	return media_effect;
}

/*	FUNCTION:		Effect_Mask :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Mask :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	if (!fPathViewAttachedToWindow)
	{
		printf("Adding fPathView\n");
		MedoWindow *medo_window = MedoWindow::GetInstance();
		medo_window->LockLooper();
		if (medo_window->GetOutputView() != fPathView->Parent())
			medo_window->GetOutputView()->AddChild(fPathView);
		fPathViewAttachedToWindow = true;
		medo_window->UnlockLooper();
	}

	EffectMaskData *data = (EffectMaskData *)effect->mEffectData;

#if 0
	printf("Effect_Mask::MediaEffectSelected()\n");
	for (auto &i : data->keyframes)
	{
		printf("Keyframe: (timeline=%f)\n", i.timeline);
		for (auto &j : i.path)
			printf("BPoint(%f, %f) ", j.x, j.y);
		printf("\n");
	}
#endif

	OutputView *output_view = MedoWindow::GetInstance()->GetOutputView();
	output_view->LockLooper();
	fPathView->ResizeTo(output_view->Bounds().Width(), output_view->Bounds().Height());
	fPathView->SetPath(data->keyframes[0].path);
	output_view->UnlockLooper();

	std::vector<float> slider_points;
	fKeyframeList->RemoveItems(0, fKeyframeList->CountItems());
	char buffer[20];
	for (int i=0; i < data->keyframes.size(); i++)
	{
		sprintf(buffer, "%s #%d", GetText(TXT_EFFECTS_COMMON_KEYFRAME), i+1);
		fKeyframeList->AddItem(new BStringItem(buffer));
		slider_points.push_back(data->keyframes[i].timeline);
	}
	fKeyframeList->Select(0);
	fKeyframeRemoveButton->SetEnabled(data->keyframes.size() > 1);
	fKeyframeSlider->SetPoints(slider_points);
	fKeyframeSlider->Select(fKeyframeList->CurrentSelection());

	fPathView->AllowSizeChange(fKeyframeList->CountItems() == 1);
}

/*	FUNCTION:		Effect_Mask :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Mask :: RenderEffect(BBitmap *source, MediaEffect *media_effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	assert(media_effect);
	EffectMaskData *data = (EffectMaskData *)media_effect->mEffectData;
	if (!data)
		return;

	KeyframeData keyframe;
	if (data->keyframes.size() == 1)
		keyframe = data->keyframes[0];
	else
	{
		float t = float(frame_idx - media_effect->mTimelineFrameStart)/float(media_effect->Duration());
		if (t > 1.0f)
			t = 1.0f;
		int idx = 0;
		while (idx < data->keyframes.size() - 1)
		{
			if (data->keyframes[idx + 1].timeline >= t)
				break;
			idx++;
		}
		for (int p=0; p < data->keyframes[idx].path.size(); p++)
		{
			BPoint &p0 = data->keyframes[idx].path[p];
			BPoint &p1 = data->keyframes[idx+1].path[p];
			float frame_duration = data->keyframes[idx+1].timeline - data->keyframes[idx].timeline;
			float frame_time = t - data->keyframes[idx].timeline;
			float s = frame_time / frame_duration;
			BPoint ip(p0.x + s*(p1.x-p0.x), p0.y + s*(p1.y-p0.y));
			keyframe.path.push_back(ip);
		}
	}

#if 1	//	needed to display current path
	OutputView *output_view = MedoWindow::GetInstance()->GetOutputView();
	output_view->LockLooper();
	fPathView->ResizeTo(output_view->Bounds().Width(), output_view->Bounds().Height());
	if (fCurrentKeyframe < data->keyframes.size())
		fPathView->SetPath(data->keyframes[fCurrentKeyframe].path);
	else
		fPathView->SetPath(data->keyframes[0].path);
	output_view->UnlockLooper();
#endif

	fPathView->FillBitmap(sBitmap, keyframe.path);
	sTexture1->Upload(sBitmap);
	if (source)
		fRenderNode->mTexture = gRenderActor->GetPicture((unsigned int)source->Bounds().Width() + 1, (unsigned int)source->Bounds().Height() + 1, source)->mTexture;
	((MaskShader *)fRenderNode->mShaderNode)->SetInverse(data->inverse);
	fRenderNode->Render(0.0f);
}

/*	FUNCTION:		Effect_Mask :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_Mask :: MessageReceived(BMessage *msg)
{
	EffectMaskData *effect_data = GetCurrentMediaEffect() ? (EffectMaskData *)GetCurrentMediaEffect()->mEffectData : nullptr;

	switch (msg->what)
	{
		case kMsgPathCheckbox:
			fPathView->ShowPath(fPathCheckbox->Value() > 0);
			break;

		case kMsgPathViewUpdate:
		{
			if (effect_data)
			{
				fPathView->GetPath(effect_data->keyframes[fKeyframeList->CurrentSelection()].path);
				InvalidatePreview();
			}
			break;
		}

		case kMsgInverseCheckbox:
			if (effect_data)
			{
				effect_data->inverse = fInverseCheckbox->Value();
				InvalidatePreview();
			}
			break;

		case kMsgShowFillCheckbox:
			fPathView->ShowFill(fShowFillCheckbox->Value());
			break;

		case kMsgKeyframeSelect:
			if (fKeyframeList->CurrentSelection() < 0)
				fKeyframeList->Select(0);
			fCurrentKeyframe = fKeyframeList->CurrentSelection();
			fKeyframeSlider->Select(fKeyframeList->CurrentSelection());

			if (effect_data)
			{
				fPathView->SetPath(effect_data->keyframes[fKeyframeList->CurrentSelection()].path);
				InvalidatePreview();
			}
			break;

		case kMsgKeyframeAdd:
		{
			if (effect_data)
			{
				char buffer[32];
				sprintf(buffer, "%s #%d", GetText(TXT_EFFECTS_COMMON_KEYFRAME), fKeyframeList->CountItems() + 1);
				fKeyframeList->AddItem(new BStringItem(buffer));

				int selection = fKeyframeList->CurrentSelection();
				KeyframeData data = effect_data->keyframes[selection];
				data.timeline = 1.0f;
				effect_data->keyframes.push_back(data);

				std::vector<float> slider_points;
				for (auto &i : effect_data->keyframes)
					slider_points.push_back(i.timeline);
				fKeyframeSlider->SetPoints(slider_points);

				fKeyframeRemoveButton->SetEnabled(true);
				fKeyframeList->Select(fKeyframeList->CountItems() - 1);
				fPathView->AllowSizeChange(false);
				fCurrentKeyframe = fKeyframeList->CurrentSelection();
			}
			break;
		}

		case kMsgKeyframeRemove:
		{
			if (effect_data)
			{
				int selection = fKeyframeList->CurrentSelection();
				fKeyframeList->RemoveItem(fKeyframeList->ItemAt(selection));

				effect_data->keyframes.erase(effect_data->keyframes.begin() + selection);
				if (effect_data->keyframes.size() == 1)
					effect_data->keyframes[0].timeline = 0.0f;
				else if (effect_data->keyframes.size() == 2)
				{
					effect_data->keyframes[0].timeline = 0.0f;
					effect_data->keyframes[1].timeline = 1.0f;
				}
				std::vector<float> slider_points;
				for (auto &i : effect_data->keyframes)
					slider_points.push_back(i.timeline);
				fKeyframeSlider->SetPoints(slider_points);

				if (fKeyframeList->CountItems() == 1)
					fPathView->AllowSizeChange(true);

				if (--selection < 0)
					selection = 0;
				fKeyframeList->Select(selection);
				fCurrentKeyframe = fKeyframeList->CurrentSelection();
				fKeyframeSlider->Select(selection);
				if (fKeyframeList->CountItems() == 1)
					fKeyframeRemoveButton->SetEnabled(false);

				char buffer[20];
				for (int i=0; i < fKeyframeList->CountItems(); i++)
				{
					BStringItem *item = (BStringItem *)fKeyframeList->ItemAt(i);
					sprintf(buffer, "%s #%d", GetText(TXT_EFFECTS_COMMON_KEYFRAME), i+1);
					item->SetText(buffer);
				}
			}
			break;
		}

		case kMsgKeyframeSlider:
		{
			int selection;
			if (msg->FindInt32("selection", &selection) == B_OK)
			{
				assert((selection >= 0) && (selection < fKeyframeList->CountItems()));
				fKeyframeList->Select(selection);
			}
			if (effect_data)
			{
				std::vector<float> slider_points;
				fKeyframeSlider->GetPoints(slider_points);
				assert(slider_points.size() == effect_data->keyframes.size());
				for (int i=0; i < slider_points.size(); i++)
					effect_data->keyframes[i].timeline = slider_points[i];
			}

			break;
		}

		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		Effect_Mask :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Mask :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	EffectMaskData *effect_data = (EffectMaskData *)media_effect->mEffectData;
	effect_data->keyframes.clear();

#define ERROR_EXIT(a) {printf("ERROR: Effect_Mask::LoadParameters(%s)\n", a);	return false;}

	//	keyframes
	if (!v.HasMember("keyframes") && !v["keyframes"].IsArray())
		ERROR_EXIT("Missing element \"keyframes\"");
	const rapidjson::Value &keyframes = v["keyframes"];
	for (auto &k : keyframes.GetArray())
	{
		KeyframeData aKeyframe;

		//	time
		if (!k.HasMember("time") && !k["time"].IsFloat())
			ERROR_EXIT("Missing keyframe element \"time\"");
		aKeyframe.timeline = k["time"].GetFloat();
		if (aKeyframe.timeline < 0.0f)	aKeyframe.timeline = 0.0f;
		if (aKeyframe.timeline > 1.0f)	aKeyframe.timeline = 1.0f;

		//	path
		if (!k.HasMember("path") && !k["path"].IsArray())
			ERROR_EXIT("Missing keyframe element \"path\"");
		const rapidjson::Value &path = k["path"];
		for (auto &p : path.GetArray())
		{
			BPoint aPath(0, 0);

			if (p.HasMember("x") && p["x"].IsFloat())
			{
				aPath.x = p["x"].GetFloat();
			}
			else
				ERROR_EXIT("Missing element \"path.x\"");

			if (p.HasMember("y") && p["y"].IsFloat())
			{
				aPath.y = p["y"].GetFloat();
			}
			else
				ERROR_EXIT("Missing element \"path.y\"");

			aKeyframe.path.push_back(aPath);
		}
		effect_data->keyframes.push_back(aKeyframe);
	}

	return true;
}

/*	FUNCTION:		Effect_Mask :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Mask :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectMaskData *data = (EffectMaskData *) media_effect->mEffectData;

	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"keyframes\": [\n");
	fwrite(buffer, strlen(buffer), 1, file);
	int keyframe_idx = 0;
	for (auto &k : data->keyframes)
	{
		sprintf(buffer, "\t\t\t\t\t{\n");
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\t\t\"time\": %f,\n", k.timeline);
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\t\t\"path\": [\n");
		fwrite(buffer, strlen(buffer), 1, file);
		size_t path_count = 0;

		for (auto &i : data->keyframes[keyframe_idx].path)
		{
			sprintf(buffer, "\t\t\t\t\t\t\t{\n");
			fwrite(buffer, strlen(buffer), 1, file);

			sprintf(buffer, "\t\t\t\t\t\t\t\t\"x\": %f,\n", i.x);
			fwrite(buffer, strlen(buffer), 1, file);

			sprintf(buffer, "\t\t\t\t\t\t\t\t\"y\": %f\n", i.y);
			fwrite(buffer, strlen(buffer), 1, file);

			if (++path_count < data->keyframes[keyframe_idx].path.size())
				sprintf(buffer, "\t\t\t\t\t\t\t},\n");
			else
				sprintf(buffer, "\t\t\t\t\t\t\t}\n");
			fwrite(buffer, strlen(buffer), 1, file);
		}
		sprintf(buffer, "\t\t\t\t\t\t]\n");
		fwrite(buffer, strlen(buffer), 1, file);

		if (++keyframe_idx < data->keyframes.size())
			sprintf(buffer, "\t\t\t\t\t},\n");
		else
			sprintf(buffer, "\t\t\t\t\t}\n");
		fwrite(buffer, strlen(buffer), 1, file);
	}
	sprintf(buffer, "\t\t\t\t]\n");
	fwrite(buffer, strlen(buffer), 1, file);

	return true;
}
