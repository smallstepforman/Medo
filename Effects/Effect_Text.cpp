/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Text
 */

#include <cstdio>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>
#include <support/StringList.h>

#include "Yarra/Render/Font.h"
#include "Yarra/Render/Picture.h"
#include "Yarra/Render/Texture.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/Project.h"

#include "Gui/AlphaColourControl.h"
#include "Gui/FontPanel.h"
#include "Gui/Spinner.h"

#include "Effect_Text.h"

const char * Effect_Text :: GetVendorName() const	{return "ZenYes";}
const char * Effect_Text :: GetEffectName() const	{return "Text";}

enum kMsgText
{
	kMsgFontButton				= 'etf1',
	kMsgFontSelected,
	kMsgFontColourControl,
	kMsgBackgroundCheckBox,
	kMsgBackgroundColourControl,
	kMsgBackgroundSpinnerOffset,
	kMsgShadowCheckBox,
	kMsgShadowSpinners,
};

static const rgb_color kDefaultFontColour		= {255,255,0,255};	//	RGBA
static const rgb_color kDefaultBackgroundColour	= {0,0,0,255};		//	RGBA

using namespace yrender;

static const YGeometry_P3 kBackgroundImageGeometry[] =
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

static BPoint				sMouseDownPosition;
static ymath::YVector3		sTextPositionMouseDown;

/***************************************
 *	TextViewKeyUp
 ***************************************/
class TextViewKeyUp : public BTextView
{
	Effect_Text		*fParent;
public:
	TextViewKeyUp(Effect_Text *parent, BRect frame, const char* name, BRect textRect, uint32 resizeMask)
		: BTextView(frame, name, textRect, resizeMask)
	{
		fParent = parent;
	}

	void KeyUp(const char *bytes, int32 numBytes)
	{
		BTextView::KeyUp(bytes, numBytes);
		fParent->TextUpdated();
	}
};

/*	FUNCTION:		Effect_Text :: Effect_Text
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_Text :: Effect_Text(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fRenderNode = nullptr;
	fTextScene = nullptr;
	fOpenGLPendingUpdate = false;
	fIs3dFont = false;

	//	Text view
	float scroll_scale = be_plain_font->Size()/12.0f;
	fTextView = new TextViewKeyUp(this, BRect(10, 10, frame.Width() - (10+scroll_scale*B_V_SCROLL_BAR_WIDTH), 10+160), "text_view", BRect(0, 0, frame.Width(), frame.Height()), B_FOLLOW_LEFT_TOP);
	fTextView->SetText("Haiku Media Editor");
	fTextView->SetAlignment(B_ALIGN_CENTER);
	mEffectView->AddChild(fTextView);

	//	Text
	BStringView *title = new BStringView(BRect(20, 200, 100, 240), nullptr, GetText(TXT_EFFECTS_TEXT_SIMPLE));
	title->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	title->SetFont(be_bold_font);
	mEffectView->AddChild(title);

	//	Font panel
	fFontPanel = nullptr;
	fFontMessenger = nullptr;
	fFontButton = new BButton(BRect(20, 240, 100, 270), "font_button", GetText(TXT_EFFECTS_TEXT_SIMPLE_FONT), new BMessage(kMsgFontButton));
	mEffectView->AddChild(fFontButton);

	fFontColourControl = new AlphaColourControl(BPoint(240, 240), "TextColourControl", new BMessage(kMsgFontColourControl));
	mEffectView->AddChild(fFontColourControl);
	fFontColourControl->SetValue(kDefaultFontColour);

	//	Background
	fBackgroundTitle = new BStringView(BRect(20, 360, 240, 400), nullptr, GetText(TXT_EFFECTS_TEXT_SIMPLE_BACKGROUND));
	fBackgroundTitle->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	fBackgroundTitle->SetFont(be_bold_font);
	mEffectView->AddChild(fBackgroundTitle);

	fBackgroundCheckBox = new BCheckBox(BRect(20, 400, 220, 440), "background_check", GetText(TXT_EFFECTS_TEXT_SIMPLE_BACKGROUND_ENABLE), new BMessage(kMsgBackgroundCheckBox));
	mEffectView->AddChild(fBackgroundCheckBox);
	fBackgroundColourControl = new AlphaColourControl(BPoint(240, 400), "TextBackgroundColourControl", new BMessage(kMsgBackgroundColourControl));
	mEffectView->AddChild(fBackgroundColourControl);
	fBackgroundColourControl->SetValue(kDefaultBackgroundColour);
	fBackgroundOffset = new Spinner(BRect(10, 460, 150, 500), "background_offset", GetText(TXT_EFFECTS_TEXT_SIMPLE_Y_OFFSET), new BMessage(kMsgBackgroundSpinnerOffset));
	fBackgroundOffset->SetRange(-200, 200);
	mEffectView->AddChild(fBackgroundOffset);

	//	Shadow
	fShadowCheckBox = new BCheckBox(BRect(20, 580, 160, 620), "shadow_check", GetText(TXT_EFFECTS_TEXT_SIMPLE_SHADOW), new BMessage(kMsgShadowCheckBox));
	mEffectView->AddChild(fShadowCheckBox);

	fShadowSpinners[0] = new Spinner(BRect(170, 580, 320, 620), "shadow_spinner_x", GetText(TXT_EFFECTS_TEXT_SIMPLE_X_OFFSET), new BMessage(kMsgShadowSpinners));
	fShadowSpinners[1] = new Spinner(BRect(350, 580, 500, 620), "shadow_spinner_y", GetText(TXT_EFFECTS_TEXT_SIMPLE_Y_OFFSET), new BMessage(kMsgShadowSpinners));
	for (int i=0; i < 2; i++)
	{
		fShadowSpinners[i]->SetRange(-100, 100);
		mEffectView->AddChild(fShadowSpinners[i]);
	}
}

/*	FUNCTION:		Effect_Text :: ~Effect_Text
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_Text :: ~Effect_Text()
{
	delete fFontPanel;
	delete fFontMessenger;
}

/*	FUNCTION:		Effect_Text :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Text :: InitRenderObjects()
{
	assert(fTextScene == nullptr);
	assert(fRenderNode == nullptr);

	const float width = gProject->mResolution.width;
	const float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new BackgroundColourShader;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3, (float *)kBackgroundImageGeometry, 4);
}

/*	FUNCTION:		Effect_Text :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_Text :: DestroyRenderObjects()
{
	delete fTextScene;		fTextScene = nullptr;
	delete fRenderNode;		fRenderNode = nullptr;
}

/*	FUNCTION:		Effect_Text :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_Text :: AttachedToWindow()
{
	fFontColourControl->SetTarget(this, Window());
	fFontButton->SetTarget(this, Window());

	fBackgroundCheckBox->SetTarget(this, Window());
	fBackgroundColourControl->SetTarget(this, Window());
	fBackgroundOffset->SetTarget(this, Window());

	fShadowCheckBox->SetTarget(this, Window());
	fShadowSpinners[0]->SetTarget(this, Window());
	fShadowSpinners[1]->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_Text :: TextUpdated
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from TextViewKeyUp()
*/
void Effect_Text :: TextUpdated()
{
	if (GetCurrentMediaEffect())
	{
		EffectTextData *data = (EffectTextData *) (GetCurrentMediaEffect()->mEffectData);
		data->text = fTextView->Text();
		gProject->InvalidatePreview();
	}
}

/*	FUNCTION:		Effect_Text :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_Text :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_Text.png");
	return icon;	
}

/*	FUNCTION:		Effect_Text :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_Text :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_SIMPLE);
}

/*	FUNCTION:		Effect_Text :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_Text :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_SIMPLE_TEXT_A);
}

/*	FUNCTION:		Effect_Text :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_Text :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_TEXT_SIMPLE_TEXT_B);
}

/*	FUNCTION:		Effect_Text :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_Text :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	InitMediaEffect(media_effect);
	return media_effect;
}

/*	FUNCTION:		Effect_Text :: InitMediaEffect
	ARGS:			media_effect
	RETURN:			MediaEffect
	DESCRIPTION:	Called by derived classes to allow deriving MediaEffect constructors/destructors
*/
void Effect_Text :: InitMediaEffect(MediaEffect *media_effect)
{
	media_effect->mEffectNode = this;
	EffectTextData *data = new EffectTextData;
	data->direction = 0;
	data->position = ymath::YVector3(0.5f*gProject->mResolution.width, 0.5f*gProject->mResolution.height, 0);
	media_effect->mEffectData = data;
	data->font_colour = fFontColourControl->ValueAsColor();
	data->font_path.SetTo("/system/data/fonts/ttfonts/NotoSansDisplay-Regular.ttf");
	data->font_size = 128;
	data->background = fBackgroundCheckBox->Value() > 0;
	data->background_colour = fBackgroundColourControl->ValueAsColor();
	data->background_offset = 0;
	data->shadow = fShadowCheckBox->Value() > 0;
	data->shadow_offset.Set(4, -6);
	data->text.SetTo(fTextView->Text());
	data->derived_data = nullptr;
}

/*	FUNCTION:		Effect_Text :: CreateOpenGLObjects
	ARGS:			data
	RETURN:			n/a
	DESCRIPTION:	Recreate YTextScene (called when font changed)
*/
void Effect_Text :: CreateOpenGLObjects(EffectTextData *data)
{
	assert(data != nullptr);
	if (fTextScene)
		delete fTextScene;
	fTextScene = new yrender::YTextScene(new yrender::YFontFreetype(data->font_size, data->font_path.String()), true);
	fTextScene->SetText(data->text.String());
	fTextScene->SetColour(YVector4(data->font_colour.red/255.0f, data->font_colour.green/255.0f, data->font_colour.blue/255.0f, data->font_colour.alpha/255.0f));
	//fTextScene->mSpatial.SetPosition(data->position);
	fOpenGLPendingUpdate = false;
}

/*	FUNCTION:		Effect_Text :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_Text :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	EffectTextData *data = (EffectTextData *)effect->mEffectData;

	fFontColourControl->SetValue(data->font_colour);
	fBackgroundCheckBox->SetValue(data->background);
	fBackgroundColourControl->SetValue(data->background_colour);
	fBackgroundOffset->SetValue(data->background_offset);
	fShadowCheckBox->SetValue(data->shadow);
	fShadowSpinners[0]->SetValue(int32(data->shadow_offset.x));
	fShadowSpinners[1]->SetValue(int32(data->shadow_offset.y));
	fTextView->SetText(data->text);
	fOpenGLPendingUpdate = true;
	InvalidatePreview();
}

/*	FUNCTION:		Effect_Text :: OutputViewMouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Output view mouse down
*/
void Effect_Text :: OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)
{
	EffectTextData *data = (EffectTextData *) media_effect->mEffectData;
	sMouseDownPosition = point;
	sTextPositionMouseDown = data->position;
}

/*	FUNCTION:		Effect_Text :: OutputViewMouseMoved
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Output view mouse moved
*/
void Effect_Text :: OutputViewMouseMoved(MediaEffect *media_effect, const BPoint &point)
{
	EffectTextData *data = (EffectTextData *) media_effect->mEffectData;
	ymath::YVector3 offset(point.x - sMouseDownPosition.x, point.y - sMouseDownPosition.y, 0);
	data->position = ymath::YVector3(sTextPositionMouseDown.x, sTextPositionMouseDown.y, 0) + offset;
}

/*	FUNCTION:		Effect_Text :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_Text :: RenderEffect(BBitmap *source, MediaEffect *media_effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	EffectTextData *data = (EffectTextData *) media_effect->mEffectData;
	assert(data);

	if (fTextScene == nullptr)
		CreateOpenGLObjects(data);

	//	Split text into lines (use "\n" as seperator)
	BStringList string_list;
	data->text.Split("\n", true, string_list);
	if (string_list.IsEmpty())
		return;

	glEnable(GL_BLEND);

	// Text
	if (fOpenGLPendingUpdate)
		CreateOpenGLObjects(data);

	rgb_color colour = {255, 255, 255, 255};

	//	Chained spatial transform
	yMatrixStack.Push();
	bool chained_transform = false;

	if (!chained_effects.empty())
	{
		//	Spatial?
		std::deque<FRAME_ITEM>::iterator i = chained_effects.begin();
		if ((*i).effect && (*i).effect->mEffectNode->IsSpatialTransform())
		{
			(*i).effect->mEffectNode->ChainedSpatialTransform((*i).effect, frame_idx);
			chained_effects.erase(i);
			chained_transform = true;
		}
		//	Colour?
		if (!chained_effects.empty())
		{
			std::deque<FRAME_ITEM>::iterator i = chained_effects.begin();
			if ((*i).effect && (*i).effect->mEffectNode->IsColourEffect())
			{
				colour = (*i).effect->mEffectNode->ChainedColourEffect((*i).effect, frame_idx);
				chained_effects.erase(i);
			}
		}
	}
	if (!chained_transform)
		yMatrixStack.Translate(data->position);


	float x_width = 0.0f;
	for (int si = 0; si < string_list.CountStrings(); si++)
	{
		fTextScene->SetText(string_list.StringAt(si).String());
		if (fTextScene->GetWidth() > x_width)
			x_width = fTextScene->GetWidth();
	}
	float background_height = (fTextScene->GetAscent() - 0.5f*fTextScene->GetDescent());


	for (int si = string_list.CountStrings() - 1; si >= 0; si--)	//	backward due to descend characters (yg)
	{
		yMatrixStack.Push();
		float y_offset = data->font_size * (0.5f*(string_list.CountStrings()-1) - si) * 1.025;
		yMatrixStack.Translate(0.0f, -y_offset, 0.0f);
		fTextScene->SetText(string_list.StringAt(si).String());
		//printf("Effect_Text::RenderEffect(%s), y_offset=%f\n", string_list.StringAt(si).String(), y_offset);

		//	Background
		if (data->background)
		{
			((BackgroundColourShader *)fRenderNode->mShaderNode)->SetColour(YVector4((data->background_colour.blue/255.0f) * (colour.blue/255.0f),
																					(data->background_colour.green/255.0f) * (colour.green/255.0f),
																					(data->background_colour.red/255.0f) * (colour.red/255.0f),
																					(data->background_colour.alpha/255.0f) * (colour.alpha/255.0f)));		//	BGRA);
			fRenderNode->mSpatial.SetPosition(YVector3(0.0f,
													   0.0f - 0.5f*background_height + 0.4f*fTextScene->GetDescent() - data->background_offset,
													   0.0f));
			//fRenderNode->mSpatial.SetScale(YVector3(0.52f*x_width, 0.55f*data->font_size, 1));
			fRenderNode->mSpatial.SetScale(YVector3(0.52f*x_width, 0.5f*background_height, 1));
			fRenderNode->Render(0.0f);
			//printf("ascent=%f, descent=%f, height=%f\n", fTextScene->GetAscent(), fTextScene->GetDescent(), fTextScene->GetHeight());
		}

		if (data->shadow)
		{
			yMatrixStack.Push();
			yMatrixStack.Translate(data->shadow_offset.x, -data->shadow_offset.y, 0);
			fTextScene->SetColour(YVector4(0,0,0, (data->font_colour.alpha/255.0f) * (colour.alpha/255.0f)));
			fTextScene->Render(0.0f);
			yMatrixStack.Pop();
		}

		fTextScene->SetColour(YVector4((data->font_colour.blue/255.0f) * (colour.blue/255.0f),
									   (data->font_colour.green/255.0f) * (colour.green/255.0f),
									   (data->font_colour.red/255.0f) * (colour.red/255.0f),
									   (data->font_colour.alpha/255.0f) * (colour.alpha/255.0f)));		//	BGRA

		if (fIs3dFont)
		{
			fTextScene->mSpatial.SetRotation(ymath::YVector3(0, 180, 180));	//	TODO FTGL Extruded font has conflicting coordinate convention
		}

		fTextScene->Render(0.0f);
		yMatrixStack.Pop();
	}

	yMatrixStack.Pop();
}

/*	FUNCTION:		Effect_Text :: MessageReceived
	ARGS:			msg
	RETURN:			true if processed
	DESCRIPTION:	Process view messages
*/
void Effect_Text :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgFontButton:
			if (!fFontPanel)
			{
				float size = 128.0f;
				if (GetCurrentMediaEffect())
				{
					size = (float) ((EffectTextData *)(GetCurrentMediaEffect()->mEffectData))->font_size;
					if (size <= 0.0f)
						size = 128.0f;
				}
				fFontMessenger = new BMessenger(this, nullptr);
				fFontPanel = new FontPanel(fFontMessenger, nullptr, size);
			}
			fFontPanel->Show();
			break;

		case kMsgFontColourControl:
			if (GetCurrentMediaEffect())
			{
				EffectTextData *data = (EffectTextData *) (GetCurrentMediaEffect()->mEffectData);
				data->font_colour = fFontColourControl->ValueAsColor();
				InvalidatePreview();
			}
			break;

		case M_FONT_SELECTED:
		{
			BString path;
			float size;
			if ((msg->FindString("path", &path) == B_OK) && (msg->FindFloat("size", &size) == B_OK))
			{
				printf("Selected Font: %s, size=%f\n", path.String(), size);
				if (GetCurrentMediaEffect())
				{
					EffectTextData *data = ((EffectTextData *)GetCurrentMediaEffect()->mEffectData);
					data->font_size = (int)size;
					data->font_path = path;
					fOpenGLPendingUpdate = true;
					InvalidatePreview();
				}
			}
			break;
		}

		case kMsgBackgroundCheckBox:
			if (GetCurrentMediaEffect())
			{
				EffectTextData *data = (EffectTextData *) (GetCurrentMediaEffect()->mEffectData);
				data->background = fBackgroundCheckBox->Value();
				InvalidatePreview();
			}
			break;

		case kMsgBackgroundColourControl:
			if (GetCurrentMediaEffect())
			{
				EffectTextData *data = (EffectTextData *) (GetCurrentMediaEffect()->mEffectData);
				data->background_colour = fBackgroundColourControl->ValueAsColor();
				InvalidatePreview();
			}
			break;

		case kMsgBackgroundSpinnerOffset:
			if (GetCurrentMediaEffect())
			{
				EffectTextData *data = (EffectTextData *) (GetCurrentMediaEffect()->mEffectData);
				data->background_offset = fBackgroundOffset->Value();
				InvalidatePreview();
			}


		case kMsgShadowCheckBox:
			if (GetCurrentMediaEffect())
			{
				EffectTextData *data = (EffectTextData *) (GetCurrentMediaEffect()->mEffectData);
				data->shadow = fShadowCheckBox->Value();
				InvalidatePreview();
			}
			break;
		case kMsgShadowSpinners:
			if (GetCurrentMediaEffect())
			{
				EffectTextData *data = (EffectTextData *) (GetCurrentMediaEffect()->mEffectData);
				data->shadow_offset.Set(fShadowSpinners[0]->Value(), fShadowSpinners[1]->Value());
				InvalidatePreview();
			}
			break;

		case B_MOUSE_MOVED:
		case B_MOUSE_IDLE:
			break;

		default:
			//printf("Effect_Text::MessageReceived()\n");
			//msg->PrintToStream();
			EffectNode::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		Effect_Text :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_Text :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
#define ERROR_EXIT(a) {printf("[Effect_Text] Error - %s\n", a); return false;}

	bool valid = true;
	EffectTextData *data = (EffectTextData *) media_effect->mEffectData;
	//	Cannot use memset() due to BString

	//	direction
	if (v.HasMember("direction") && v["direction"].IsUint())
	{
		data->direction = v["direction"].GetUint();
		if (data->direction > 1)
			data->direction = 1;
		else if (data->direction < 0)
			data->direction = 0;
	}
	else
		ERROR_EXIT(" missing parameter \"direction\"");

	//	position
	if (v.HasMember("position") && v["position"].IsArray())
	{
		const rapidjson::Value &arr = v["position"];
		rapidjson::SizeType num_elems = arr.GetArray().Size();
		if (num_elems == 3)
		{
			data->position.x = arr.GetArray()[0].GetFloat();
			data->position.y = arr.GetArray()[1].GetFloat();
			data->position.z = arr.GetArray()[2].GetFloat();
		}
		else
			ERROR_EXIT(" corrupt \"position\"");
	}
	else
		ERROR_EXIT("missing parameter \"position\"");

	//	font_colour
	if (v.HasMember("font_colour") && v["font_colour"].IsArray())
	{
		const rapidjson::Value &array = v["font_colour"];
		if (array.Size() != 4)
			ERROR_EXIT("\"font_colour\" invalid");
		uint32_t colour[4];
		for (rapidjson::SizeType e=0; e < 4; e++)
		{
			if (!array[e].IsInt())
				ERROR_EXIT("\"font_colour\" invalid");
			colour[e] = array[e].GetInt();
			if ((colour[e] < 0) || (colour[e] > 255))
				ERROR_EXIT("\"font_colour\" invalid");
		}
		data->font_colour.red = (uint8)colour[0];
		data->font_colour.green = (uint8)colour[1];
		data->font_colour.blue = (uint8)colour[2];
		data->font_colour.alpha = (uint8)colour[3];
	}
	else
		ERROR_EXIT("missing parameter \"font_colour\"");

	//	background_enable
	if (v.HasMember("background_enable") && v["background_enable"].IsBool())
	{
		data->background = v["background_enable"].GetBool();
	}
	else
		ERROR_EXIT("missing parameter \"background_enable\"");

	//	background_colour
	if (v.HasMember("background_colour") && v["background_colour"].IsArray())
	{
		const rapidjson::Value &array = v["background_colour"];
		if (array.Size() != 4)
			ERROR_EXIT("\"background_colour\" invalid");
		uint32_t colour[4];
		for (rapidjson::SizeType e=0; e < 4; e++)
		{
			if (!array[e].IsInt())
				ERROR_EXIT("\"background_colour\" invalid");
			colour[e] = array[e].GetInt();
			if ((colour[e] < 0) || (colour[e] > 255))
				ERROR_EXIT("\"background_colour\" invalid");
		}
		data->background_colour.red = (uint8)colour[0];
		data->background_colour.green = (uint8)colour[1];
		data->background_colour.blue = (uint8)colour[2];
		data->background_colour.alpha = (uint8)colour[3];
	}
	else
		ERROR_EXIT("missing paramater \"background_colour\"");

	if (v.HasMember("background_offset") && v["background_offset"].IsInt())
	{
		data->background_offset = v["background_offset"].GetInt();
	}
	else
		ERROR_EXIT("missing parameter \"background_offset\"");

	//	Shadow
	if (v.HasMember("shadow") && v["shadow"].IsBool())
	{
		data->shadow = (int) v["shadow"].GetBool();
	}
	else
		ERROR_EXIT("missing parameter \"shadow\"");

	//	Shadow offset
	if (v.HasMember("shadow_offset") && v["shadow_offset"].IsArray())
	{
		const rapidjson::Value &arr = v["shadow_offset"];
		rapidjson::SizeType num_elems = arr.GetArray().Size();
		if (num_elems == 2)
		{
			data->shadow_offset.x = arr.GetArray()[0].GetInt();
			data->shadow_offset.y = arr.GetArray()[1].GetInt();
		}
		else
			ERROR_EXIT("error in \"shadow_offset\"\n");
	}
	else
		ERROR_EXIT("missing parameter \"shadow_offset\"");

	//	Text
	if (v.HasMember("text") && v["text"].IsString())
	{
		std::string aString;
		aString.assign(v["text"].GetString());
		//printf("v[text]=%s\n", aString.c_str());
		data->text.SetTo(aString.c_str());
	}
	else
	{
		printf("Effect_Text :: LoadParameters() - missing parameter \"text\"\n");
		valid = false;
	}

	return valid;
}

/*	FUNCTION:		Effect_Text :: SaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Text :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	return SaveParametersBase(file, media_effect, false);
}

/*	FUNCTION:		Effect_Text :: SaveParametersBase
	ARGS:			file
					media_effect
					append_comma
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_Text :: SaveParametersBase(FILE *file, MediaEffect *media_effect, const bool append_comma)
{
	EffectTextData *data = (EffectTextData *) media_effect->mEffectData;
	if (data)
	{
		char buffer[0x200];	//	512 bytes
		sprintf(buffer, "\t\t\t\t\"direction\": %u,\n", data->direction);
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\"position\": [%f, %f, %f],\n", data->position.x, data->position.y, data->position.z);
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\"font_colour\": [%u, %u, %u, %u],\n",
				data->font_colour.red,
				data->font_colour.green,
				data->font_colour.blue,
				data->font_colour.alpha);
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\"background_enable\": %s,\n", data->background ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\"background_colour\": [%u, %u, %u, %u],\n",
				data->background_colour.red,
				data->background_colour.green,
				data->background_colour.blue,
				data->background_colour.alpha);
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\"background_offset\": %d,\n", data->background_offset);
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\"shadow\": %s,\n", data->shadow ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\"shadow_offset\": [%d, %d],\n",
				uint32(data->shadow_offset.x),
				uint32(data->shadow_offset.y));
		fwrite(buffer, strlen(buffer), 1, file);

		if (append_comma)
			sprintf(buffer, "\t\t\t\t\"text\": \"%s\",\n", data->text.ReplaceAll("\n", "\\n").String());
		else
			sprintf(buffer, "\t\t\t\t\"text\": \"%s\"\n", data->text.ReplaceAll("\n", "\\n").String());
		fwrite(buffer, strlen(buffer), 1, file);
	}

	return true;
}

