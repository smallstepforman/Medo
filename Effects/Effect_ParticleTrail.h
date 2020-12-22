/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Particle Trail
 */

#ifndef EFFECT_PARTICLE_TRAIL_H
#define EFFECT_PARTICLE_TRAIL_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

#ifndef _YARRA_VECTOR_H_
#include "Yarra/Math/Vector.h"
#endif

class BColorControl;
class BButton;
class ValueSlider;
class ParticleScene;
class PathListView;
class Spinner;

class Effect_ParticleTrail : public EffectNode
{
public:
					Effect_ParticleTrail(BRect frame, const char *filename);
					~Effect_ParticleTrail()							override;
	void			AttachedToWindow()								override;

	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const	override {return EffectNode::EFFECT_SPECIAL;}
	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	BBitmap			*GetIcon()										override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	const char		*GetTextA(const uint32 language_idx)			override;
	const char		*GetTextB(const uint32 language_idx)			override;
	
	MediaEffect		*CreateMediaEffect()							override;
	void			MediaEffectSelected(MediaEffect *effect)		override;
	void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)	override;
	void			MessageReceived(BMessage *msg)					override;
	void			OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)		override;
	
private:
	ValueSlider				*fSliderVelocity;
	ValueSlider				*fSliderSpread[2];
	ValueSlider				*fSliderPointSize;
	ValueSlider				*fSliderNumberParticles;
	ValueSlider				*fSliderSpawnDuration;

	BColorControl			*fColorControlSpawn;
	BColorControl			*fColorControlDelta;

	PathListView			*fPathListView;
	BButton					*fButtonAddPath;
	Spinner					*fSpinnerPath[2];
	std::vector<ymath::YVector3>	fPathVector;

	friend class ParticleMediaEffect;
	std::vector<ParticleScene *> fRetiredParticleScenes;
};

#endif	//#ifndef EFFECT_PARTICLE_TRAIL_H
