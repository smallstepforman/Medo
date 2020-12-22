/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect / Background Colour
 */

#include <cstdio>
#include <cassert>
#include <random>
#include <algorithm>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Render/MatrixStack.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/Texture.h"
#include "Yarra/Math/Interpolation.h"
#include "Yarra/Render/Picture.h"

#include "Gui/ValueSlider.h"
#include "Gui/Spinner.h"
#include "Editor/EffectNode.h"
#include "Editor/EffectsWindow.h"
#include "Editor/Language.h"
#include "Editor/Project.h"
#include "Editor/MedoWindow.h"
#include "Editor/OutputView.h"
#include "Editor/RenderActor.h"

#include "Effect_ParticleTrail.h"

const char * Effect_ParticleTrail :: GetVendorName() const	{return "ZenYes";}
const char * Effect_ParticleTrail :: GetEffectName() const	{return "Particle Trail";}

enum
{
	kMsgSliderVelocity			= 'eptv',
	kMsgSliderSpread,
	kMsgSliderPointSize,
	kMsgSliderNumberParticles,
	kMsgSliderSpawnDuration,
	kMsgColourSpawn,
	kMsgColourDelta,
	kMsgButtonAddPath,
	kMsgSpinnerPath,
	kMsgPathSelected,
};

static const int kParticleVelocityRange[2] = {1, 2000};
static const int kParticleSpreadRange[2] = {1, 100};	//	spread1/50, spread2/100
static const int kParticleSizeRange[2] = {10, 200};
static const int kDefaultParticlePointSize = 40;
static const int kNumberParticlesRange[2] = {200, 4000};

//	Note: spawn + delta will be capped to 255
static rgb_color kParticleSpawnColour = {255, 128, 192, 255};
static rgb_color kParticleDeltaColour = {0, 128, 64, 255};


class EffectParticleData
{
public:
	int				velocity;
	float			spread[2];
	int				point_size;
	int				number_particles;
	float			spawn_duration;
	rgb_color		colour_spawn;
	rgb_color		colour_delta;
	std::vector<ymath::YVector3>	path;
	ParticleScene	*particle_scene;
};

using namespace yrender;

/************************
	Particle Shader
*************************/
static const char *kParticleVertexShader = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec4			aColour;\
	out vec4		vColour;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);\
		vColour = aColour;\
	}";

static const char *kParticleFragmentShader = "\
	uniform sampler2D	uTextureUnit0;\
	in vec4				vColour;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour = texture(uTextureUnit0, gl_PointCoord);\
		fFragColour *= vec4(vColour);\
	}";


class ParticleShader : public yrender::YShaderNode
{
private:
	yrender::YShader	*fShader;
	GLint				fLocation_uTransform;
	GLint				fLocation_uTextureUnit0;

public:
	ParticleShader()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aColour");
		fShader = new YShader(&attributes, kParticleVertexShader, kParticleFragmentShader);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uTextureUnit0 = fShader->GetUniformLocation("uTextureUnit0");
	}
	~ParticleShader()
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

/*******************************
	Particle Trail
********************************/

class ParticleScene : public yrender::YSceneNode
{
	int					fFingerTrailIndex;
	YRenderNode			*fRenderNode;
	YGeometry_P3C4U		*fFingerTrailVertices;
	YVector3			*fParticleVelocity;

	float				fElapsedTime;
	float				fVelocity;
	float				fSpread[2];
	float				fPointSize;
	int					fNumberParticles;
	float				fSpawnDuration;
	std::vector<ymath::YVector3>	fPath;

	rgb_color			fSpawnColour;
	rgb_color			fDeltaColouur;

	std::uniform_real_distribution<double>	fUniformDistribution;
	std::mt19937							fRandomEngine;

	void				UpdateVertices(float timestamp);

public:
						ParticleScene();
						~ParticleScene();

	void				SetElapsedTime(float elapsed_time);
	void				SetVelocity(float velocity)			{fVelocity  = velocity;}
	void				SetSpread(float spread1, float spread2)		{fSpread[0] = spread1; fSpread[1] = spread2;}
	void				SetSpawnColour(rgb_color colour)	{fSpawnColour = colour;}
	void				SetDeltaColour(rgb_color colour)	{fDeltaColouur = colour;}
	void				SetPointSize(float size)			{fPointSize = size;}
	void				SetNumberParticles(int count);
	void				SetSpawnDuration(float duration)	{fSpawnDuration = duration;}
	void				SetPath(const std::vector<ymath::YVector3> &path) {fPath = path;}

	void				Render(float delta_time);
	void				Reset();
};

/*	FUNCTION:		ParticleScene :: ParticleScene
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ParticleScene :: ParticleScene()
{
	fVelocity = 0.25f*kParticleVelocityRange[1];
	fSpread[0] = 0.25f*kParticleSpreadRange[1];
	fSpread[1] = 0.05f;
	fPointSize = kDefaultParticlePointSize;
	fNumberParticles = fNumberParticles/2;
	fSpawnDuration = 1.0f;
	fSpawnColour = kParticleSpawnColour;
	fDeltaColouur = kParticleDeltaColour;

	fFingerTrailIndex = 0;
	fFingerTrailVertices = new YGeometry_P3C4U [kNumberParticlesRange[1]];
	memset(fFingerTrailVertices, 0, kNumberParticlesRange[1] * sizeof(YGeometry_P3C4U));
	fParticleVelocity = new YVector3 [kNumberParticlesRange[1]];
	memset(fParticleVelocity, 0, kNumberParticlesRange[1] * sizeof(YVector3));

	fRenderNode = new YRenderNode;
	fRenderNode->mTexture = new YTexture("Resources/smoke.png");
	fRenderNode->mGeometryNode = new YGeometryNode(GL_POINTS, Y_GEOMETRY_P3C4U, (float *)fFingerTrailVertices, kNumberParticlesRange[1], 0, GL_DYNAMIC_DRAW);
	fRenderNode->mShaderNode = new ParticleShader;

	fUniformDistribution = std::uniform_real_distribution<double> (0.0, 1.0);
	std::random_device rand_dev;
	fRandomEngine = std::mt19937(rand_dev());

	Reset();
}

/*	FUNCTION:		ParticleScene :: ~ParticleScene
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ParticleScene :: ~ParticleScene()
{
	delete fRenderNode;
	delete [] fFingerTrailVertices;
	delete [] fParticleVelocity;
}

void ParticleScene :: SetNumberParticles(int count)
{
	assert(count <= kNumberParticlesRange[1]);
	bool reset = (count != fNumberParticles);
	fNumberParticles = count;
	if (reset)
		Reset();
}

void ParticleScene :: Reset()
{
	fFingerTrailIndex = kNumberParticlesRange[1];
	for (int i=0; i < kNumberParticlesRange[1]; i++)
	{
		fFingerTrailVertices[i].mColour[0] = 255;
		fFingerTrailVertices[i].mColour[1] = 255;
		fFingerTrailVertices[i].mColour[2] = 255;
		fFingerTrailVertices[i].mColour[3] = 0;
	}
}

/*	FUNCTION:		ParticleScene :: UpdateVertices
	ARGUMENTS:		mouse_down
	RETURN:			n/a
	DESCRIPTION:	Update particle vertices
*/
void ParticleScene :: UpdateVertices(float timestamp)
{
	if (++fFingerTrailIndex >= fNumberParticles)
		fFingerTrailIndex = 0;

	float t = timestamp;
	YVector3 pos;
	if (!fPath.empty())
		pos = ymath::YInterpolationBezier(fPath, t);
	else
		pos.Set(-1000, -1000, 0);
	fFingerTrailVertices[fFingerTrailIndex].mPosition[0] = pos.x * gProject->mResolution.width;
	fFingerTrailVertices[fFingerTrailIndex].mPosition[1] = pos.y * gProject->mResolution.height;
	fFingerTrailVertices[fFingerTrailIndex].mPosition[2] = 0.0f;

	if (t < fSpawnDuration)
	{
		//	BGRA
		fFingerTrailVertices[fFingerTrailIndex].mColour[0] = std::clamp<unsigned char>(fSpawnColour.blue + (fDeltaColouur.blue*(float)rand()/(float)RAND_MAX), 0, 255);
		fFingerTrailVertices[fFingerTrailIndex].mColour[1] = std::clamp<unsigned char>(fSpawnColour.green + (fDeltaColouur.green*(float)rand()/(float)RAND_MAX), 0, 255);
		fFingerTrailVertices[fFingerTrailIndex].mColour[2] = std::clamp<unsigned char>(fSpawnColour.red	+ (fDeltaColouur.red*(float)rand()/(float)RAND_MAX), 0, 255);
		fFingerTrailVertices[fFingerTrailIndex].mColour[3] = fSpawnColour.alpha;

		fParticleVelocity[fFingerTrailIndex].x = fVelocity * (fSpread[0]*fUniformDistribution(fRandomEngine) - 0.5);
		fParticleVelocity[fFingerTrailIndex].y = fVelocity * (fSpread[0]*fUniformDistribution(fRandomEngine) - 0.5);
		fParticleVelocity[fFingerTrailIndex].z = 0.0f;
	}

	fRenderNode->mGeometryNode->UpdateVertices((float *)fFingerTrailVertices);
}

/*	FUNCTION:		ParticleScene :: SetElapsedTime
	ARGUMENTS:		elapsed_time
	RETURN:			n/a
	DESCRIPTION:	Also check if reset required
*/
void ParticleScene :: SetElapsedTime(float elapsed_time)
{
	if ((fElapsedTime > elapsed_time) && (elapsed_time < 0.1))
		Reset();
	fElapsedTime = elapsed_time;
}

/*	FUNCTION:		ParticleScene :: Render
	ARGUMENTS:		delta_time
	RETURN:			n/a
	DESCRIPTION:	Draw trail
*/
void ParticleScene :: Render(float delta_time)
{
	for (int i=0; i < 20 + fSpread[1]*50; i++)
		UpdateVertices(fElapsedTime + i*0.001f*fSpread[1]);

	glPointSize(fPointSize);

	for (int i=0; i < fNumberParticles; i++)
	{
		int idx;
		if (i < fFingerTrailIndex)
			idx = fNumberParticles - (fFingerTrailIndex - i);
		else
			idx = i - fFingerTrailIndex;

		if (fFingerTrailVertices[idx].mColour[3] > 0)
		{
			fFingerTrailVertices[idx].mColour[3] = (unsigned char)(0.90f*(float)fFingerTrailVertices[idx].mColour[3]);

			fFingerTrailVertices[idx].mPosition[0] += fParticleVelocity[idx].x*delta_time;
			fFingerTrailVertices[idx].mPosition[1] += fParticleVelocity[idx].y*delta_time;
			fFingerTrailVertices[idx].mPosition[2] += fParticleVelocity[idx].z*delta_time;
		}
	}

	glEnable(GL_BLEND);

	glEnable(GL_POINT_SPRITE);
	glEnable(GL_PROGRAM_POINT_SIZE);
	fRenderNode->mGeometryNode->SetVertexCount(fNumberParticles);
	fRenderNode->Render(delta_time);
	glDisable(GL_PROGRAM_POINT_SIZE);
	glDisable(GL_POINT_SPRITE);
}

/************************************
	PathListView
*************************************/
enum
{
	kMsgPathListMoveUp = 'ptv0',
	kMsgPathListMoveDown,
	kMsgPathListRemoveItem,
};

class PathListView : public BListView
{
	BHandler	*fParent;

	void		ContextMenu(BPoint point)
	{
		ConvertToScreen(&point);

		int32 index = CurrentSelection();
		assert(index >= 0);

		//	Initialise PopUp menu
		BPopUpMenu *aPopUpMenu = new BPopUpMenu("ContextMenuSPathList", false, false);
		aPopUpMenu->SetAsyncAutoDestruct(true);

		BMenuItem *menu_time = new BMenuItem(GetText(TXT_EFFECTS_SPECIAL_PARTICLE_MOVE_UP), new BMessage(kMsgPathListMoveUp));
		if (index == 0)
			menu_time->SetEnabled(false);
		aPopUpMenu->AddItem(menu_time);

		menu_time = new BMenuItem(GetText(TXT_EFFECTS_SPECIAL_PARTICLE_MOVE_DOWN), new BMessage(kMsgPathListMoveDown));
		if (index == CountItems() - 1)
			menu_time->SetEnabled(false);
		aPopUpMenu->AddItem(menu_time);

		menu_time = new BMenuItem(GetText(TXT_EFFECTS_SPECIAL_PARTICLE_REMOVE_ITEM), new BMessage(kMsgPathListRemoveItem));
		aPopUpMenu->AddItem(menu_time);

		aPopUpMenu->SetTargetForItems(this);
		aPopUpMenu->Go(point, true /*notify*/, false /*stay open when mouse away*/, true /*async*/);
	}
public:
	PathListView(BRect frame, const char *name, BHandler *parent)
	: BListView(frame, name), fParent(parent)
	{ }

	~PathListView()					override
	{ }

	void	MouseDown(BPoint point)			override
	{
		BListView::MouseDown(point);

		int32 index = CurrentSelection();
		if (index < 0)
			return;

		uint32 buttons;
		BMessage* msg = Window()->CurrentMessage();
		msg->FindInt32("buttons", (int32*)&buttons);
		bool ctrl_modifier = ((EffectsWindow *)Window())->GetKeyModifiers() & B_CONTROL_KEY;
		if ((buttons & B_SECONDARY_MOUSE_BUTTON) || ctrl_modifier)
		{
			ContextMenu(point);
			return;
		}
	}
};

/************************************
	Effect_ParticleTrail
*************************************/
static Effect_ParticleTrail *sEffectParticleTrailInstance = nullptr;
class ParticleMediaEffect : public ImageMediaEffect
{
public:
	~ParticleMediaEffect() override
	{
		EffectParticleData *effect_data = (EffectParticleData *)mEffectData;
		sEffectParticleTrailInstance->fRetiredParticleScenes.push_back(effect_data->particle_scene);
	}
};

/*	FUNCTION:		Effect_ParticleTrail :: Effect_ParticleTrail
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_ParticleTrail :: Effect_ParticleTrail(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
	assert(sEffectParticleTrailInstance == nullptr);
	sEffectParticleTrailInstance = this;

	const float kFontFactor = be_plain_font->Size()/20.0f;
	const float width = frame.Width();

	//	Velocity
	char min_label[32], max_label[32];
	fSliderVelocity = new ValueSlider(BRect(20, 20, width - 20, 110), "Velocity", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_VELOCITY), nullptr, kParticleVelocityRange[0], kParticleVelocityRange[1]);
	fSliderVelocity->SetModificationMessage(new BMessage(kMsgSliderVelocity));
	fSliderVelocity->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderVelocity->SetHashMarkCount(11);
	sprintf(min_label, "%d", kParticleVelocityRange[0]);
	sprintf(max_label, "%d", kParticleVelocityRange[1]);
	fSliderVelocity->SetLimitLabels(min_label, max_label);
	fSliderVelocity->SetStyle(B_BLOCK_THUMB);
	fSliderVelocity->SetFloatingPointPrecision(0);
	fSliderVelocity->SetValue(0.25f*kParticleVelocityRange[1]);
	fSliderVelocity->UpdateTextValue(0.25f*kParticleVelocityRange[1]);
	fSliderVelocity->SetBarColor({255, 0, 0, 255});
	fSliderVelocity->UseFillColor(true);
	mEffectView->AddChild(fSliderVelocity);

	//	Spread #1
	fSliderSpread[0] = new ValueSlider(BRect(20, 110, 0.5f*width - 20, 200), "Spread#1", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_DIRECTION), nullptr, kParticleSpreadRange[0], kParticleSpreadRange[1]);
	fSliderSpread[0]->SetModificationMessage(new BMessage(kMsgSliderSpread));
	fSliderSpread[0]->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderSpread[0]->SetHashMarkCount(11);
	fSliderSpread[0]->SetLimitLabels("-1.0", "1.0");
	fSliderSpread[0]->SetStyle(B_BLOCK_THUMB);
	fSliderSpread[0]->SetFloatingPointPrecision(2);
	fSliderSpread[0]->SetValue(0.25f*kParticleSpreadRange[1]);
	fSliderSpread[0]->UpdateTextValue(0.25f*kParticleSpreadRange[1] / 50.0f);
	fSliderSpread[0]->SetBarColor({255, 255, 0, 255});
	fSliderSpread[0]->UseFillColor(true);
	mEffectView->AddChild(fSliderSpread[0]);
	//	Spread #2
	fSliderSpread[1] = new ValueSlider(BRect(0.5f*width + 20, 110, width - 20, 200), "Spread#2", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_SPREAD), nullptr, kParticleSpreadRange[0], kParticleSpreadRange[1]);
	fSliderSpread[1]->SetModificationMessage(new BMessage(kMsgSliderSpread));
	fSliderSpread[1]->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderSpread[1]->SetHashMarkCount(11);
	sprintf(min_label, "%d", kParticleSpreadRange[0]/100);
	sprintf(max_label, "%d", kParticleSpreadRange[1]/100);
	fSliderSpread[1]->SetLimitLabels(min_label, max_label);
	fSliderSpread[1]->SetStyle(B_BLOCK_THUMB);
	fSliderSpread[1]->SetFloatingPointPrecision(2);
	fSliderSpread[1]->SetValue(0.25f*kParticleSpreadRange[1]);
	fSliderSpread[1]->UpdateTextValue(0.25f*kParticleSpreadRange[1] / 100.0f);
	fSliderSpread[1]->SetBarColor({255, 255, 0, 255});
	fSliderSpread[1]->UseFillColor(true);
	mEffectView->AddChild(fSliderSpread[1]);

	//	Point size
	fSliderPointSize = new ValueSlider(BRect(20, 200, width - 20, 290), "point_size", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_POINT_SIZE), nullptr, kParticleSizeRange[0], kParticleSizeRange[1]);
	fSliderPointSize->SetModificationMessage(new BMessage(kMsgSliderPointSize));
	fSliderPointSize->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderPointSize->SetHashMarkCount(11);
	sprintf(min_label, "%d", kParticleSizeRange[0]);
	sprintf(max_label, "%d", kParticleSizeRange[1]);
	fSliderPointSize->SetLimitLabels(min_label, max_label);
	fSliderPointSize->SetStyle(B_BLOCK_THUMB);
	fSliderPointSize->SetFloatingPointPrecision(0);
	fSliderPointSize->SetValue(kDefaultParticlePointSize);
	fSliderPointSize->UpdateTextValue(kDefaultParticlePointSize);
	fSliderPointSize->SetBarColor({0, 255, 0, 255});
	fSliderPointSize->UseFillColor(true);
	mEffectView->AddChild(fSliderPointSize);

	//	Number particles
	fSliderNumberParticles = new ValueSlider(BRect(20, 290, 0.5f*width - 20, 380), "number_particles", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_NUMBER), nullptr, kNumberParticlesRange[0], kNumberParticlesRange[1]);
	fSliderNumberParticles->SetModificationMessage(new BMessage(kMsgSliderNumberParticles));
	fSliderNumberParticles->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderNumberParticles->SetHashMarkCount(11);
	sprintf(min_label, "%d", kNumberParticlesRange[0]);
	sprintf(max_label, "%d", kNumberParticlesRange[1]);
	fSliderNumberParticles->SetLimitLabels(min_label, max_label);
	fSliderNumberParticles->SetStyle(B_BLOCK_THUMB);
	fSliderNumberParticles->SetFloatingPointPrecision(0);
	fSliderNumberParticles->SetValue(kNumberParticlesRange[1]/2);
	fSliderNumberParticles->UpdateTextValue(kNumberParticlesRange[1]/2);
	fSliderNumberParticles->SetBarColor({0, 255, 255, 255});
	fSliderNumberParticles->UseFillColor(true);
	mEffectView->AddChild(fSliderNumberParticles);

	//	Spawn duration
	fSliderSpawnDuration = new ValueSlider(BRect(0.5f*width + 20, 290, width - 20, 380), "spawn_duration", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_SPAWN), nullptr, 0, 100);
	fSliderSpawnDuration->SetModificationMessage(new BMessage(kMsgSliderSpawnDuration));
	fSliderSpawnDuration->SetHashMarks(B_HASH_MARKS_BOTH);
	fSliderSpawnDuration->SetHashMarkCount(11);
	fSliderSpawnDuration->SetLimitLabels("0.0", "1.0");
	fSliderSpawnDuration->SetStyle(B_BLOCK_THUMB);
	fSliderSpawnDuration->SetFloatingPointPrecision(2);
	fSliderSpawnDuration->SetValue(100);
	fSliderSpawnDuration->UpdateTextValue(1.0f);
	fSliderSpawnDuration->SetBarColor({0, 128, 255, 255});
	fSliderSpawnDuration->UseFillColor(true);
	mEffectView->AddChild(fSliderSpawnDuration);

	//	Colour spawn
	BStringView *title_spawn = new BStringView(BRect(20, 380, 420, 420), "label_spawn", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_BASE_COLOUR));
	title_spawn->SetFont(be_bold_font);
	mEffectView->AddChild(title_spawn);
	fColorControlSpawn = new BColorControl(BPoint(20, 420), B_CELLS_32x8, 6.0f, "colour_spawn", new BMessage(kMsgColourSpawn), true);
	fColorControlSpawn->SetValue(kParticleSpawnColour);
	mEffectView->AddChild(fColorControlSpawn);
	//	Colour delta
	BStringView *title_delta = new BStringView(BRect(20, 520, 420, 560), "label_delta", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_DELTA_COLOUR));
	title_delta->SetFont(be_bold_font);
	mEffectView->AddChild(title_delta);
	fColorControlDelta = new BColorControl(BPoint(20, 560), B_CELLS_32x8, 6.0f, "colour_delta", new BMessage(kMsgColourDelta), true);
	fColorControlDelta->SetValue(kParticleDeltaColour);
	mEffectView->AddChild(fColorControlDelta);

	float kColourRight = fColorControlDelta->Bounds().right + 20;

	//	Particle position
	BStringView *title_motion_path = new BStringView(BRect(kColourRight + 20, 380, kColourRight+20 + 200*kFontFactor, 410), "label_motion", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_MOTION_PATH));
	title_motion_path->SetFont(be_bold_font);
	mEffectView->AddChild(title_motion_path);
	fPathListView = new PathListView(BRect(kColourRight + 20, 410, kColourRight+20 + 200*kFontFactor, 540), "list_position", this);
	fPathListView->SetSelectionMessage(new BMessage(kMsgPathSelected));
	mEffectView->AddChild(new BScrollView("list_scroll", fPathListView, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));
	//	Spinners
	fSpinnerPath[0] = new Spinner(BRect(kColourRight + 20, 550, kColourRight+20 + 200*kFontFactor, 580), "spinner_x", "X", new BMessage(kMsgSpinnerPath));
	fSpinnerPath[0]->SetRange(-1, 2);
	fSpinnerPath[0]->SetValue(0.5f);
	fSpinnerPath[0]->SetSteps(0.01f);
	mEffectView->AddChild(fSpinnerPath[0]);
	fSpinnerPath[1] = new Spinner(BRect(kColourRight + 20, 590, kColourRight+20 + 200*kFontFactor, 620), "spinner_y", "Y", new BMessage(kMsgSpinnerPath));
	fSpinnerPath[1]->SetRange(-1, 2);
	fSpinnerPath[1]->SetValue(0.5f);
	fSpinnerPath[1]->SetSteps(0.01f);
	mEffectView->AddChild(fSpinnerPath[1]);
	//	Button
	fButtonAddPath = new BButton(BRect(kColourRight + 20, 630, kColourRight+20 + 200*kFontFactor, 670), "button_pos", GetText(TXT_EFFECTS_SPECIAL_PARTICLE_ADD_POSITION), new BMessage(kMsgButtonAddPath));
	mEffectView->AddChild(fButtonAddPath);

	//	Populate PathListView
	fPathVector.push_back(ymath::YVector3(0.0f, 0.5f, 0.0f));
	fPathVector.push_back(ymath::YVector3(1.0f, 0.5f, 0.0f));
	char buffer[0x20];
	int idx=1;
	for (auto &i : fPathVector)
	{
		sprintf(buffer, "[%d] %0.2f , %0.2f", idx, i.x, i.y);
		fPathListView->AddItem(new BStringItem(buffer));
		idx++;
	}

	SetViewIdealSize(kColourRight+20 + 200*kFontFactor + 40, 740);
}

/*	FUNCTION:		Effect_ParticleTrail :: ~Effect_ParticleTrail
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_ParticleTrail :: ~Effect_ParticleTrail()
{
}

/*	FUNCTION:		Effect_ParticleTrail :: AttachedToWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when attached to window
*/
void Effect_ParticleTrail :: AttachedToWindow()
{
	fSliderVelocity->SetTarget(this, Window());
	fSliderSpread[0]->SetTarget(this, Window());
	fSliderSpread[1]->SetTarget(this, Window());
	fSliderPointSize->SetTarget(this, Window());
	fSliderNumberParticles->SetTarget(this, Window());
	fSliderSpawnDuration->SetTarget(this, Window());
	fColorControlSpawn->SetTarget(this, Window());
	fColorControlDelta->SetTarget(this, Window());
	fButtonAddPath->SetTarget(this, Window());
	fPathListView->SetTarget(this, Window());
	fSpinnerPath[0]->SetTarget(this, Window());
	fSpinnerPath[1]->SetTarget(this, Window());

	fColorControlSpawn->FrameResized(400, 60);
}

/*	FUNCTION:		Effect_ParticleTrail :: InitRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ParticleTrail :: InitRenderObjects()
{

}

/*	FUNCTION:		Effect_ParticleTrail :: DestroyRenderObjects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called from RenderActor thread
*/
void Effect_ParticleTrail :: DestroyRenderObjects()
{
	for (auto &i : fRetiredParticleScenes)
		delete i;
}

/*	FUNCTION:		Effect_ParticleTrail :: GetIcon
	ARGS:			none
	RETURN:			thumbnail
	DESCRIPTION:	Get thumbnail (for EffectsItem)
					Called acquires ownership
*/
BBitmap * Effect_ParticleTrail :: GetIcon()
{
	BBitmap *icon = BTranslationUtils::GetBitmap("Resources/Effect_ParticleTrail.png");
	return icon;	
}

/*	FUNCTION:		Effect_ParticleTrail :: GetTextEffectName
	ARGS:			language_idx
	RETURN:			effect name
	DESCRIPTION:	Get translated effect name
*/
const char * Effect_ParticleTrail :: GetTextEffectName(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_SPECIAL_PARTICLE);
}

/*	FUNCTION:		Effect_ParticleTrail :: GetTextA
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 1
*/	
const char * Effect_ParticleTrail :: GetTextA(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_SPECIAL_PARTICLE_TEXT_A);
}

/*	FUNCTION:		Effect_ParticleTrail :: GetTextB
	ARGS:			language_idx
	RETURN:			text
	DESCRIPTION:	Get text line 2
*/
const char * Effect_ParticleTrail :: GetTextB(const uint32 language_idx)
{
	return GetText(TXT_EFFECTS_SPECIAL_PARTICLE_TEXT_B);
}

/*	FUNCTION:		Effect_ParticleTrail :: CreateMediaEffect
	ARGS:			none
	RETURN:			MediaEffect
	DESCRIPTION:	Create media effect
*/
MediaEffect * Effect_ParticleTrail :: CreateMediaEffect()
{
	ParticleMediaEffect *media_effect = new ParticleMediaEffect;
	media_effect->mEffectNode = this;
	EffectParticleData *data = new EffectParticleData;
	data->velocity = fSliderVelocity->Value();
	data->spread[0] = fSliderSpread[0]->Value()/50.0f;
	data->spread[1] = fSliderSpread[1]->Value()/100.0f;
	data->point_size = fSliderPointSize->Value();
	data->number_particles = fSliderNumberParticles->Value();
	data->spawn_duration = fSliderSpawnDuration->Value()/100.0f;
	data->colour_spawn = fColorControlSpawn->ValueAsColor();
	data->colour_delta = fColorControlDelta->ValueAsColor();
	data->path = fPathVector;
	data->particle_scene = nullptr;
	media_effect->mEffectData = data;

	//	Populate PathListView
	data->path = fPathVector;
	return media_effect;
}

/*	FUNCTION:		Effect_ParticleTrail :: MediaEffectSelected
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Called from EffectsWindow
*/
void Effect_ParticleTrail :: MediaEffectSelected(MediaEffect *effect)
{
	//printf("Effect_BackgroundColour::MediaEffectSelected(%p)\n", effect);
	if (effect->mEffectData == nullptr)
		return;

	EffectParticleData *effect_data = (EffectParticleData *)effect->mEffectData;

	//printf("MediaEffectSelected() %p %d\n", effect_data, effect_data->path.size());

	//	Update GUI
	fSliderVelocity->SetValue(effect_data->velocity);						fSliderVelocity->UpdateTextValue(effect_data->velocity);
	fSliderSpread[0]->SetValue(effect_data->spread[0] * 50.0f);				fSliderSpread[0]->UpdateTextValue(effect_data->spread[0] - 1.0f);
	fSliderSpread[1]->SetValue(effect_data->spread[1] * 100.0f);			fSliderSpread[1]->UpdateTextValue(effect_data->spread[1]);
	fSliderPointSize->SetValue(effect_data->point_size);					fSliderPointSize->UpdateTextValue(effect_data->point_size);
	fSliderNumberParticles->SetValue(effect_data->number_particles);		fSliderNumberParticles->UpdateTextValue(effect_data->number_particles);
	fSliderSpawnDuration->SetValue(effect_data->spawn_duration * 100.0f);	fSliderSpawnDuration->UpdateTextValue(effect_data->spawn_duration);

	fColorControlSpawn->SetValue(effect_data->colour_spawn);
	fColorControlDelta->SetValue(effect_data->colour_delta);

	fPathVector = effect_data->path;
	fPathListView->RemoveItems(0, fPathListView->CountItems());
	char buffer[0x20];
	int idx=1;
	for (auto &i : fPathVector)
	{
		sprintf(buffer, "[%d] %0.2f , %0.2f", idx, i.x, i.y);
		fPathListView->AddItem(new BStringItem(buffer));
		idx++;
	}
}

/*	FUNCTION:		Effect_ParticleTrail :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_ParticleTrail :: RenderEffect(BBitmap *source, MediaEffect *media_effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	EffectParticleData *effect_data = (EffectParticleData *) media_effect->mEffectData;

	if (effect_data->particle_scene == nullptr)
		effect_data->particle_scene = new ParticleScene;

	float t = float(frame_idx - media_effect->mTimelineFrameStart)/float(media_effect->Duration());
	effect_data->particle_scene->SetVelocity(effect_data->velocity);
	effect_data->particle_scene->SetSpread(effect_data->spread[0], effect_data->spread[1]);
	effect_data->particle_scene->SetPointSize(effect_data->point_size);
	effect_data->particle_scene->SetSpawnColour(effect_data->colour_spawn);
	effect_data->particle_scene->SetDeltaColour(effect_data->colour_delta);
	effect_data->particle_scene->SetNumberParticles(effect_data->number_particles);
	effect_data->particle_scene->SetSpawnDuration(effect_data->spawn_duration);
	effect_data->particle_scene->SetElapsedTime(t);
	effect_data->particle_scene->SetPath(effect_data->path);

	if (source)
	{
		unsigned int w = source->Bounds().IntegerWidth() + 1;
		unsigned int h = source->Bounds().IntegerHeight() + 1;
		yrender::YPicture *picture = gRenderActor->GetPicture(w, h, source);
		picture->mSpatial.SetPosition(ymath::YVector3(0.5f*gProject->mResolution.width, 0.5f*gProject->mResolution.height, 0));
		picture->mSpatial.SetRotation(ymath::YVector3(0, 0, 0));
		picture->mSpatial.SetScale(ymath::YVector3(0.5f*w, 0.5f*h, 1));
		picture->Render(0.0f);
	}

	effect_data->particle_scene->Render(1.0f/60.0f);
}

/*	FUNCTION:		Effect_ParticleTrail :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process view messages
*/
void Effect_ParticleTrail :: MessageReceived(BMessage *msg)
{
	bool invalidate_preview = false;
	EffectParticleData *effect_data = nullptr;
	if (GetCurrentMediaEffect())
		effect_data = (EffectParticleData *) (GetCurrentMediaEffect()->mEffectData);
	int recreate_path_list_idx = -1;

	switch (msg->what)
	{
		case kMsgSliderVelocity:
			fSliderVelocity->UpdateTextValue(fSliderVelocity->Value());
			if (effect_data)
			{
				effect_data->velocity = fSliderVelocity->Value();
				invalidate_preview = true;
			}
			break;

		case kMsgSliderSpread:
			fSliderSpread[0]->UpdateTextValue(fSliderSpread[0]->Value()/50.0f - 1.0f);
			fSliderSpread[1]->UpdateTextValue(fSliderSpread[1]->Value() / 100.0f);
			if (effect_data)
			{
				effect_data->spread[0] = fSliderSpread[0]->Value() / 50.0f;
				effect_data->spread[1] = fSliderSpread[1]->Value() / 100.0f;
				invalidate_preview = true;
			}
			break;

		case kMsgSliderPointSize:
			fSliderPointSize->UpdateTextValue(fSliderPointSize->Value());
			if (GetCurrentMediaEffect())
			{
				effect_data->point_size = fSliderPointSize->Value();
				invalidate_preview = true;
			}
			break;

		case kMsgSliderNumberParticles:
			fSliderNumberParticles->UpdateTextValue(fSliderNumberParticles->Value());
			if (effect_data)
			{
				effect_data->number_particles = fSliderNumberParticles->Value();
				invalidate_preview = true;
			}
			break;

		case kMsgSliderSpawnDuration:
			fSliderSpawnDuration->UpdateTextValue(fSliderSpawnDuration->Value() / 100.0f);
			if (effect_data)
			{
				effect_data->spawn_duration = fSliderSpawnDuration->Value() / 100.0f;
				invalidate_preview = true;
			}
			break;

		case kMsgColourSpawn:
		case kMsgColourDelta:
		{
			const rgb_color spawn_colour = fColorControlSpawn->ValueAsColor();
			rgb_color delta_colour = fColorControlDelta->ValueAsColor();
			//	Cap delta
			if (spawn_colour.red + delta_colour.red > 255)	delta_colour.red = 255 - spawn_colour.red;
			if (spawn_colour.green + delta_colour.green > 255)	delta_colour.green = 255 - spawn_colour.green;
			if (spawn_colour.blue + delta_colour.blue > 255)	delta_colour.blue = 255 - spawn_colour.blue;
			fColorControlDelta->SetValue(delta_colour);
			if (effect_data)
			{
				effect_data->colour_spawn = spawn_colour;
				effect_data->colour_delta = delta_colour;
				invalidate_preview = true;
			}
			break;
		}

		case kMsgButtonAddPath:
		{
			ymath::YVector3 item(fSpinnerPath[0]->Value(), fSpinnerPath[1]->Value(), 0.0f);
			fPathVector.push_back(item);
			char buffer[0x20];
			sprintf(buffer, "[%d]  %0.2f , %0.2f", fPathListView->CountItems() + 1, item.x, item.y);
			fPathListView->AddItem(new BStringItem(buffer));
			invalidate_preview = true;
			break;
		}
		case kMsgPathListMoveDown:
		{
			printf("kMsgPathListMoveDown(%d)\n", fPathListView->CurrentSelection());
			int idx = fPathListView->CurrentSelection();
			ymath::YVector3 item = fPathVector[idx];
			fPathVector.erase(fPathVector.begin() + idx);
			if (idx + 1 < fPathVector.size())
				fPathVector.insert(fPathVector.begin() + idx + 1, item);
			else
				fPathVector.push_back(item);
			recreate_path_list_idx = idx + 1;
			break;
		}
		case kMsgPathListMoveUp:
		{
			printf("kMsgPathListMoveUp(%d)\n", fPathListView->CurrentSelection());
			int idx = fPathListView->CurrentSelection();
			ymath::YVector3 item = fPathVector[idx];
			fPathVector.erase(fPathVector.begin() + idx);
			fPathVector.insert(fPathVector.begin() + idx - 1, item);
			recreate_path_list_idx = idx - 1;
			break;
		}
		case kMsgPathListRemoveItem:
		{
			int idx = fPathListView->CurrentSelection();
			fPathVector.erase(fPathVector.begin() + idx);
			recreate_path_list_idx = idx > 0 ? idx - 1 : 0;
			break;
		}
		case kMsgPathSelected:
		{
			int idx = fPathListView->CurrentSelection();
			if ((idx >= 0) && (idx < fPathVector.size()))
			{
				fSpinnerPath[0]->SetValue(fPathVector[idx].x);
				fSpinnerPath[1]->SetValue(fPathVector[idx].y);
			}
			break;
		}
		case kMsgSpinnerPath:
		{
			int idx = fPathListView->CurrentSelection();
			if ((idx >= 0) && (idx < fPathVector.size()))
			{
				fPathVector[idx].x = fSpinnerPath[0]->Value();
				fPathVector[idx].y = fSpinnerPath[1]->Value();

				char buffer[0x20];
				sprintf(buffer, "[%d] %0.2f , %0.2f", idx, fPathVector[idx].x, fPathVector[idx].y);
				BStringItem *item = (BStringItem *)fPathListView->ItemAt(idx);
				item->SetText(buffer);
				if (effect_data)
					effect_data->path = fPathVector;
				recreate_path_list_idx = idx;
			}
			break;
		}

		default:
			//printf("Effect_ParticleTrail :: MessageReceived()\n");
			//msg->PrintToStream();
			EffectNode::MessageReceived(msg);
			break;
	}

	if (recreate_path_list_idx >= 0)
	{
		fPathListView->RemoveItems(0, fPathListView->CountItems());
		char buffer[0x20];
		int idx=1;
		for (auto &i : fPathVector)
		{
			sprintf(buffer, "[%d] %0.2f , %0.2f", idx, i.x, i.y);
			fPathListView->AddItem(new BStringItem(buffer));
			idx++;
		}
		fPathListView->Select(recreate_path_list_idx);

		if (effect_data)
			effect_data->path = fPathVector;

		invalidate_preview = true;
	}

	if (invalidate_preview)
		InvalidatePreview();
}

/*	FUNCTION:		Effect_ParticleTrail :: OutputViewMouseDown
	ARGS:			media_effect
					point
	RETURN:			none
	DESCRIPTION:	Hook function when mouse down
*/
void Effect_ParticleTrail :: OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)
{
	MedoWindow::GetInstance()->LockLooper();
	BRect frame = MedoWindow::GetInstance()->GetOutputView()->Bounds();
	MedoWindow::GetInstance()->UnlockLooper();

	fSpinnerPath[0]->SetValue(point.x/frame.Width());
	fSpinnerPath[1]->SetValue(point.y/frame.Height());

	int idx = fPathListView->CurrentSelection();
	if ((idx >= 0) && (idx < fPathVector.size()))
	{
		fPathVector[idx].x = fSpinnerPath[0]->Value();
		fPathVector[idx].y = fSpinnerPath[1]->Value();

		char buffer[0x20];
		sprintf(buffer, "[%d] %0.2f , %0.2f", idx, fPathVector[idx].x, fPathVector[idx].y);
		BStringItem *item = (BStringItem *)fPathListView->ItemAt(idx);
		item->SetText(buffer);

		if (GetCurrentMediaEffect())
		{
			EffectParticleData *effect_data = (EffectParticleData *) GetCurrentMediaEffect()->mEffectData;
			effect_data->path = fPathVector;
			InvalidatePreview();
		}
	}
}

/*	FUNCTION:		Effect_ParticleTrail :: LoadParameters
	ARGS:			v
					media_effect
	RETURN:			true if processed
	DESCRIPTION:	Extract project data
*/
bool Effect_ParticleTrail :: LoadParameters(const rapidjson::Value &v, MediaEffect *media_effect)
{
	EffectParticleData *effect_data = (EffectParticleData *) media_effect->mEffectData;

#define ERROR_EXIT(a) {printf("ERROR: Effect_ParticleTrail::LoadParameters(%s)\n", a);	return false;}

	//	velocity
	if (v.HasMember("velocity") && v["velocity"].IsUint())
	{
		effect_data->velocity = v["velocity"].GetUint();
		if (effect_data->velocity < kParticleVelocityRange[0])
			effect_data->velocity = kParticleVelocityRange[0];
		if (effect_data->velocity > kParticleVelocityRange[1])
			effect_data->velocity = kParticleVelocityRange[1];
	}
	else
		ERROR_EXIT("Missing element \"velocity\"");

	//	spread1
	if (v.HasMember("spread1") && v["spread1"].IsFloat())
	{
		effect_data->spread[0] = v["spread1"].GetFloat();
		if (effect_data->spread[0] < -1)
			effect_data->spread[0] = -1;
		if (effect_data->spread[0] > 1.0f)
			effect_data->spread[0] = 1.0f;
	}
	else
		ERROR_EXIT("Missing element \"spread1\"");

	//	spread2
	if (v.HasMember("spread2") && v["spread2"].IsFloat())
	{
		effect_data->spread[1] = v["spread2"].GetFloat();
		if (effect_data->spread[1] < 0)
			effect_data->spread[1] = 0;
		if (effect_data->spread[1] > 1.0f)
			effect_data->spread[1] = 1.0f;
	}
	else
		ERROR_EXIT("Missing element \"spread2\"");

	//	point_size
	if (v.HasMember("point_size") && v["point_size"].IsUint())
	{
		effect_data->point_size = v["point_size"].GetUint();
		if (effect_data->point_size < kParticleSizeRange[0])
			effect_data->point_size = kParticleSizeRange[0];
		if (effect_data->point_size > kParticleSizeRange[1])
			effect_data->point_size = kParticleSizeRange[1];
	}
	else
		ERROR_EXIT("Missing element \"point_size\"");

	//	number_particles
	if (v.HasMember("number_particles") && v["number_particles"].IsUint())
	{
		effect_data->number_particles = v["number_particles"].GetUint();
		if (effect_data->number_particles < kNumberParticlesRange[0])
			effect_data->number_particles = kNumberParticlesRange[0];
		if (effect_data->number_particles > kNumberParticlesRange[1])
			effect_data->number_particles = kNumberParticlesRange[1];
	}
	else
		ERROR_EXIT("Missing element \"number_particles\"");

	//	spawn_duration
	if (v.HasMember("spawn_duration") && v["spawn_duration"].IsFloat())
	{
		effect_data->spawn_duration = v["spawn_duration"].GetFloat();
		if (effect_data->spawn_duration < 0.0f)
			effect_data->spawn_duration = 0.0f;
		if (effect_data->spawn_duration > 1.0f)
			effect_data->spawn_duration = 1.0f;
	}
	else
		ERROR_EXIT("Missing element \"spawn_duration\"");

	//	path
	effect_data->path.clear();
	if (!v.HasMember("path") && !v["path"].IsArray())
		ERROR_EXIT("Missing element \"path\"");
	const rapidjson::Value &path = v["path"];
	for (auto &p : path.GetArray())
	{
		ymath::YVector3 aPath(0, 0, 0);

		if (p.HasMember("x") && p["x"].IsFloat())
		{
			aPath.x = p["x"].GetFloat();
			if (aPath.x < -1.0f)
				aPath.x = -1.0f;
			if (aPath.x > 2.0f)
				aPath.x = 2.0f;
		}
		else
			ERROR_EXIT("Missing element \"path.x\"");

		if (p.HasMember("y") && p["y"].IsFloat())
		{
			aPath.y = p["y"].GetFloat();
			if (aPath.y < -1.0f)
				aPath.y = -1.0f;
			if (aPath.y > 2.0f)
				aPath.y = 2.0f;
		}
		else
			ERROR_EXIT("Missing element \"path.y\"");

		effect_data->path.push_back(aPath);
	}

	//	colour_spawn
	if (v.HasMember("colour_spawn") && v["colour_spawn"].IsUint())
	{
		uint32 colour = v["colour_spawn"].GetUint();
		effect_data->colour_spawn.red = (colour >> 24) & 0xff;
		effect_data->colour_spawn.green = (colour >> 16) & 0xff;
		effect_data->colour_spawn.blue = (colour >> 8) & 0xff;
		effect_data->colour_spawn.alpha = colour & 0xff;
	}
	else
		ERROR_EXIT("Missing element \"colour_spawn\"");

	//	colour_delta
	if (v.HasMember("colour_delta") && v["colour_delta"].IsUint())
	{
		uint32 colour = v["colour_delta"].GetUint();
		effect_data->colour_delta.red = (colour >> 24) & 0xff;
		effect_data->colour_delta.green = (colour >> 16) & 0xff;
		effect_data->colour_delta.blue = (colour >> 8) & 0xff;
		effect_data->colour_delta.alpha = colour & 0xff;
	}
	else
		ERROR_EXIT("Missing element \"colour_delta\"");
	
	return true;
}

/*	FUNCTION:		Effect_ParticleTrail :: PrepareSaveParameters
	ARGS:			file
					media_effect
	RETURN:			true if success
	DESCRIPTION:	Save project data
*/
bool Effect_ParticleTrail :: SaveParameters(FILE *file, MediaEffect *media_effect)
{
	int				point_size;
	int				number_particles;
	float			spawn_duration;
	rgb_color		colour_spawn;
	rgb_color		colour_delta;
	std::vector<ymath::YVector3>	path;

	EffectParticleData *data = (EffectParticleData *) media_effect->mEffectData;
	char buffer[0x80];	//	128 bytes
	sprintf(buffer, "\t\t\t\t\"velocity\": %u,\n", data->velocity);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"spread1\": %f,\n", data->spread[0]);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"spread2\": %f\n,", data->spread[1]);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"point_size\": %u,\n", data->point_size);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"number_particles\": %u,\n", data->number_particles);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"spawn_duration\": %f,\n", data->spawn_duration);
	fwrite(buffer, strlen(buffer), 1, file);

	sprintf(buffer, "\t\t\t\t\"path\": [\n");
	fwrite(buffer, strlen(buffer), 1, file);
	size_t path_count = 0;
	for (auto &i : data->path)
	{
		sprintf(buffer, "\t\t\t\t\t{\n");
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\t\t\"x\": %f,\n", i.x);
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\t\t\t\"y\": %f\n", i.y);
		fwrite(buffer, strlen(buffer), 1, file);

		if (++path_count < data->path.size())
			sprintf(buffer, "\t\t\t\t\t},\n");
		else
			sprintf(buffer, "\t\t\t\t\t}\n");
		fwrite(buffer, strlen(buffer), 1, file);
	}
	sprintf(buffer, "\t\t\t\t],\n");
	fwrite(buffer, strlen(buffer), 1, file);

	sprintf(buffer, "\t\t\t\t\"colour_spawn\": %u,\n", (data->colour_spawn.red << 24) | (data->colour_spawn.green << 16) | (data->colour_spawn.blue << 8) | data->colour_spawn.alpha);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\t\t\"colour_delta\": %u\n", (data->colour_delta.red << 24) | (data->colour_delta.green << 16) | (data->colour_delta.blue << 8) | data->colour_delta.alpha);
	fwrite(buffer, strlen(buffer), 1, file);

	return true;
}
