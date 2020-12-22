/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Colour Grading
 */

#ifndef EFFECT_COLOUR_GRADING_H
#define EFFECT_COLOUR_GRADING_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

namespace yrender
{
	class YRenderNode;
};

class BBitmap;
class ValueSlider;
class BButton;

class Effect_ColourGrading : public EffectNode
{
public:
					Effect_ColourGrading(BRect frame, const char *filename);
					~Effect_ColourGrading()							override;
	void			AttachedToWindow()								override;

	void			InitRenderObjects();
	void			DestroyRenderObjects();
	
	EFFECT_GROUP	GetEffectGroup() const	{return EffectNode::EFFECT_COLOUR;}
	const int		GetEffectListPriority() {return 98;}
	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	BBitmap			*GetIcon()										override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	const char		*GetTextA(const uint32 language_idx)			override;
	const char		*GetTextB(const uint32 language_idx)			override;
	
	MediaEffect		*CreateMediaEffect();
	void			MediaEffectSelected(MediaEffect *effect)		override;
	void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)	override;
	
	void			MessageReceived(BMessage *msg)					override;
	
private:
	yrender::YRenderNode 	*fRenderNode;
	ValueSlider				*fSliderSaturation;
	ValueSlider				*fSliderBrightness;
	ValueSlider				*fSliderContrast;
	ValueSlider				*fSliderGamma;
	ValueSlider				*fSliderExposure;
	ValueSlider				*fSliderTemperature;
	ValueSlider				*fSliderTint;
	BButton					*fButtonReset;
};

#endif	//#ifndef EFFECT_COLOUR_GRADING_H
