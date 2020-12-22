/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Colour LUT
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/Texture.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/MatrixStack.h"

#include "Editor/EffectNode.h"
#include "Editor/Language.h"
#include "Editor/MedoWindow.h"
#include "Editor/Project.h"

#include "3rdParty/LutCube.h"
#include "Effect_ColourLut.h"

const char * Effect_ColourLut :: GetVendorName() const	{return "ZenYes";}
const char * Effect_ColourLut :: GetEffectName() const	{return "Colour LUT";}

enum kLutMessages
{
	kMsgLutLoad		= 'ecl0',
};

using namespace yrender;

static const YGeometry_P3T2 kLutGeometry[] =
{
	{-1, -1, 0,		0, 0},
	{1, -1, 0,		1, 0},
	{-1, 1, 0,		0, 1},
	{1, 1, 0,		1, 1},
};

struct LUT_CACHE
{
	bool		texture_load_pending;
	GLuint		texture_id;
	BString		filename;
	int			count;
};
static std::vector<LUT_CACHE> sLutCache;
class EffectLutData
{
public:
	int		mCacheIndex;
	~EffectLutData()
	{
		--sLutCache[mCacheIndex].count;
		//	TODO schedule glDeleteTextures
		printf("~EffectLutData()\n");
		for (auto &i : sLutCache)
		{
			printf("sLutCache: %s, texture_id(%d), count(%d)\n", i.filename.String(), i.texture_id, i.count);
		}
	}
};

/************************
	LUT Shader
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
	
static const char *kFragmentShader = "\
	uniform sampler2D	uTextureUnit0;\
	uniform sampler3D	uTextureUnit1;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		vec4 initial_colour = texture(uTextureUnit0, vTexCoord0);\
		fFragColour = texture(uTextureUnit1, initial_colour.rgb);\
		fFragColour.a = initial_colour.a;\
	}";


class ColourLUTShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint		fLocation_uTransform;
	GLint		fLocation_uTextureUnit0;
	GLint		fLocation_uTextureUnit1;
	bool		fValidationPending;
	
public:
	ColourLUTShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kVertexShader, kFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
		fLocation_uTextureUnit1 = fShader->GetUniformLocation("uTextureUnit1");
		fValidationPending = true;
	}
	~ColourLUTShader()
	{
		delete fShader;
	}
	void Render(float delta_time)
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform1i(fLocation_uTextureUnit0, 0);
		glUniform1i(fLocation_uTextureUnit1, 1);

		if (fValidationPending)
		{
			fShader->ValidateProgram();
			fValidationPending = false;
		}
	}	
};

/*	FUNCTION:		Effect_ColourLut :: Effect_ColourLut
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_ColourLut :: Effect_ColourLut(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	fRenderNode = nullptr;
	fLutCacheIndex = 0;

	//	Load LUT button
	float load_button_width = be_plain_font->StringWidth(GetText(TXT_EFFECTS_COLOUR_LUT_LOAD)) + be_plain_font->Size();
	fLoadLutButton = new BButton(BRect(20, 20, 40+load_button_width, 20 + 1.5f*be_plain_font->Size()), "load_button", GetText(TXT_EFFECTS_COLOUR_LUT_LOAD), new BMessage(kMsgLutLoad));
	mEffectView->AddChild(fLoadLutButton);

	fLoadLutString = new BStringView(BRect(60 + load_button_width, 20, frame.Width()-20, 20+1.5f*be_plain_font->Size()), "load_string", GetText(TXT_EFFECTS_COLOUR_LUT_NO_FILE));
	mEffectView->AddChild(fLoadLutString);

	BStringView *description_text = new BStringView(BRect(20, 100, frame.Width()-20, 240), nullptr,
													GetText(TXT_EFFECTS_COLOUR_LUT_INSTRUCTIONS));

	mEffectView->AddChild(description_text);
}

/*	FUNCTION:		Effect_ColourLut :: ~Effect_ColourLut
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_ColourLut :: ~Effect_ColourLut()
{ }

/*	FUNCTION:		Effect_ColourLut :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void Effect_ColourLut :: AttachedToWindow()
{
	//	Bind GUI
	fLoadLutButton->SetTarget(this, Window());
}

/*	FUNCTION:		Effect_ColourLut :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ColourLut :: InitRenderObjects()
{
	assert(fRenderNode == nullptr);

	float width = gProject->mResolution.width;
	float height = gProject->mResolution.height;

	fRenderNode = new yrender::YRenderNode;
	fRenderNode->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, 0.5f));
	fRenderNode->mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
	fRenderNode->mShaderNode = new ColourLUTShader;
	fRenderNode->mGeometryNode = new yrender::YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, (float *)kLutGeometry, 4);
	fRenderNode->mTexture = new YTexture(width, height);
}

/*	FUNCTION:		Effect_ColourLut :: LoadCubeFile
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ColourLut :: LoadCubeFile(int index)
{
	if (sLutCache[index].texture_id > 0)
		glDeleteTextures(1, &sLutCache[index].texture_id);

	//	Load CUBE file
	timecube::Cube cube;
	try {
		cube = timecube::read_cube_from_file(sLutCache[index].filename.String());
	}
	catch (const std::exception &e)
	{
		printf("Effect_ColourLut::CreateMediaEffect() - failed to parse CUBE file(%s).  Error(%s)\n", sLutCache[index].filename.String(), e.what());
		exit(1);
	}
	assert(cube.is_3d);

	//	Create 3D texture
	glGenTextures(1, &sLutCache[index].texture_id);
	glBindTexture(GL_TEXTURE_3D, sLutCache[index].texture_id);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	//	Uploaad CUBE data to texture
	unsigned char *texture_buffer = new unsigned char [cube.n * cube.n * cube.n * 4];
	unsigned char *p = texture_buffer;
	for (uint32_t i=0; i < cube.n; i++)
	{
		for (uint32_t j=0; j < cube.n; j++)
		{
			for (uint32_t k=0; k < cube.n; k++)
			{
				size_t idx = 3*(i*cube.n*cube.n + j*cube.n + k);
				assert(idx + 2 < cube.lut.size());
				*p++ = (unsigned char) (cube.lut[idx + 0] * 255.0f);
				*p++ = (unsigned char) (cube.lut[idx + 1] * 255.0f);
				*p++ = (unsigned char) (cube.lut[idx + 2] * 255.0f);
				*p++ = 255;
				assert(p <= texture_buffer + cube.n*cube.n*cube.n*4);
			}
		}
	}
	assert(p == texture_buffer + cube.n*cube.n*cube.n*4);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, cube.n, cube.n, cube.n, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_buffer);
	delete [] texture_buffer;
	sLutCache[index].texture_load_pending = false;
}

/*	FUNCTION:		Effect_ColourLut :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ColourLut :: DestroyRenderObjects()
{
	delete fRenderNode;
	for (auto &i : sLutCache)
	{
		glDeleteTextures(1, &i.texture_id);
		i.texture_id = 0;
	}
}

/*	FUNCTION:		Effect_ColourLut :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_ColourLut :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_ColourLut.png");
	return icon;	
}

/*	FUNCTION:		Effect_ColourLut :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_ColourLut :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_LUT);
}

/*	FUNCTION:		Effect_ColourLut :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_ColourLut :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_LUT_TEXT_A);
}

/*	FUNCTION:		Effect_ColourLut :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_ColourLut :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_COLOUR_LUT_TEXT_B);
}

/*	FUNCTION:		Effect_ColourLut :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_ColourLut :: CreateMediaEffect()
{
	ImageMediaEffect *media_effect = new ImageMediaEffect;
	media_effect->mEffectNode = this;
	EffectLutData *data = new EffectLutData;
	data->mCacheIndex = fLutCacheIndex;

	if (sLutCache.empty())
	{
		LUT_CACHE item;
		item.count = 0;
		item.texture_id = 0;
		item.texture_load_pending = false;
		sLutCache.push_back(item);
	}
	sLutCache[fLutCacheIndex].count++;
	media_effect->mEffectData = data;

	return media_effect;
}

/*	FUNCTION:		Effect_ColourLut :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_ColourLut :: MediaEffectSelected(MediaEffect *effect)
{
	if (effect->mEffectData == nullptr)
		return;

	//	Update GUI
	fLutCacheIndex = ((EffectLutData *)effect->mEffectData)->mCacheIndex;
	LockLooper();
	BString aString(sLutCache[fLutCacheIndex].filename);
	if (aString.Length() == 0)
		aString.SetTo(GetText(TXT_EFFECTS_COLOUR_LUT_NO_FILE));
	be_plain_font->TruncateString(&aString, B_TRUNCATE_BEGINNING, fLoadLutString->Frame().Width() - 40);
	fLoadLutString->SetText(aString);
	UnlockLooper();
}

/*	FUNCTION:		Effect_ColourLut :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_ColourLut :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	float t = float(frame_idx - data->mTimelineFrameStart)/float(data->Duration());
	if (t > 1.0f)
		t = 1.0f;

	EffectLutData *effect_data = (EffectLutData *)data->mEffectData;
	if (sLutCache[effect_data->mCacheIndex].texture_load_pending)
		LoadCubeFile(effect_data->mCacheIndex);

	fRenderNode->mTexture->Upload(source);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, sLutCache[effect_data->mCacheIndex].texture_id);
	fRenderNode->Render(0.0f);
	glActiveTexture(GL_TEXTURE0);
}

/*	FUNCTION:		Effect_ColourLut :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_ColourLut :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgLutLoad:
			//printf("kMsgLutLoad\n");
			MedoWindow::GetInstance()->PostMessage(new BMessage(MedoWindow::eMsgActionEffectsFilePanelOpen));
			break;
				
		default:
			EffectNode::MessageReceived(msg);
			break;
	}
}	

/*	FUNCTION:		Effect_ColourLut :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_ColourLut :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	//	direction
	if (v.HasMember("cube") && v["cube"].IsString())
	{
		EffectLutData *data = (EffectLutData *) media_effect->mEffectData;
		BString aString(v["cube"].GetString());
		int ci=0;
		for (auto &i : sLutCache)
		{
			if (i.filename.Compare(aString) == 0)
			{
				data->mCacheIndex = ci;
				i.count++;
				break;
			}
			ci++;
		}
		if (ci == sLutCache.size())
		{
			LUT_CACHE item;
			item.filename = aString;
			item.count = 1;
			item.texture_id = 0;
			item.texture_load_pending = true;
			sLutCache.push_back(item);
			data->mCacheIndex = sLutCache.size() - 1;
		}
	}
	
	return true;
}

/*	FUNCTION:		Effect_ColourLut :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_ColourLut :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	EffectLutData *data = (EffectLutData *) media_effect->mEffectData;
	char buffer[0x100];	//	256 bytes
	sprintf(buffer, "\t\t\t\t\"cube\": \"%s\"\n", sLutCache[data->mCacheIndex].filename.String());
	fwrite(buffer, strlen(buffer), 1, file);
	return true;
}

/*	FUNCTION:		MedoWindow :: CreateFilePanel
	ARGS:			language_index
	RETURN:			BFilePanel *
	DESCRIPTION:	Called from MedoWindow to create file panel
*/
BFilePanel * Effect_ColourLut :: CreateFilePanel(int language_index)
{
	BFilePanel *file_panel = new BFilePanel(B_OPEN_PANEL,	//	file_panel_mode
					nullptr,		//	target		TODO get EffectNode to receive refs ...
					nullptr,		//	directory
					0,				//	nodeFlavors
					false,			//	allowMultipleSelection
					nullptr,		//	message
					this,			//	refFilter
					true,			//	modal
					true);			//	hideWhenDone
	file_panel->SetButtonLabel(B_DEFAULT_BUTTON, "Load LUT");
	file_panel->Window()->SetTitle("Load LUT (.cube) File:");
	return file_panel;
}

/*	FUNCTION:		MedoWindow :: Filter
	ARGS:			ref
					node
					stat
					mimeType
	RETURN:			true if allowed
	DESCRIPTION:	Called by BFilePanel to filter items
*/
bool Effect_ColourLut :: Filter(const entry_ref* ref, BNode* node, struct stat_beos *stat, const char* mimeType)
{
	bool filter = true;
	BEntry entry(ref);
	if (!entry.IsDirectory())
	{
		BString name(ref->name);
		if (strstr(name.ToLower(), ".cube") == 0)
			filter = false;
	}
	return filter;
}

/*	FUNCTION:		Effect_ColourLut :: FilePanelOpen
	ARGS:			path
	RETURN:			n/a
	DESCRIPTION:	Called when file panel open refs received
					Caution - called from MedoWindow looper
*/
void Effect_ColourLut :: FilePanelOpen(const char *path)
{
	if (strstr(BString(path).ToLower(), ".cube") == 0)
		return;

	EffectLutData *effect_data = nullptr;
	if (GetCurrentMediaEffect() && (GetCurrentMediaEffect()->mEffectNode == this))
		effect_data = (EffectLutData *)GetCurrentMediaEffect()->mEffectData;

	bool create_item = sLutCache.empty();

	if (effect_data)
	{
		fLutCacheIndex = effect_data->mCacheIndex;
		if (sLutCache[fLutCacheIndex].count > 1)
		{
			sLutCache[fLutCacheIndex].count--;
			create_item = true;
		}
	}

	if (create_item)
	{
		LUT_CACHE item;
		item.filename = path;
		item.texture_id = 0;
		item.count = (effect_data ? 1 : 0);
		sLutCache.push_back(item);
		fLutCacheIndex = sLutCache.size() - 1;
		if (effect_data)
			effect_data->mCacheIndex = fLutCacheIndex;
	}
	assert(fLutCacheIndex >= 0);
	sLutCache[fLutCacheIndex].texture_load_pending = true;
	sLutCache[fLutCacheIndex].filename.SetTo(path);

	LockLooper();
	BString aString(path);
	be_plain_font->TruncateString(&aString, B_TRUNCATE_BEGINNING, fLoadLutString->Frame().Width() - 40);
	fLoadLutString->SetText(aString);
	UnlockLooper();

	InvalidatePreview();
}
