/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Blur
 */

#ifndef EFFECT_BLUR_H
#define EFFECT_BLUR_H

#ifndef _SLIDER_H
#include <interface/Slider.h>
#endif

#ifndef _B_STRING_H
#include <support/String.h>
#endif

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class ValueSlider;
class BOptionPopUp;
class BCheckBox;

namespace yrender
{
	class YRenderNode;
};

//=========================
class Effect_Blur : public EffectNode
{
public:
					Effect_Blur(BRect frame, const char *filename);
					~Effect_Blur()									override;

	void			AttachedToWindow()								override;
	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const							override	{return EffectNode::EFFECT_IMAGE;}
	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	BBitmap			*GetIcon()										override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	const char		*GetTextA(const uint32 language_idx)			override;
	const char		*GetTextB(const uint32 language_idx)			override;
	const int		GetEffectListPriority()							override	{return 99;}
	
	MediaEffect		*CreateMediaEffect()							override;
	void			MediaEffectSelected(MediaEffect *effect)		override;
	void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)	override;
	
	void			MessageReceived(BMessage *msg)					override;
	
private:
	yrender::YRenderNode 	*fRenderNode;
	ValueSlider				*fBlurSliders[2];
	BOptionPopUp			*fMethodPopup;
	BCheckBox				*fCheckboxInterpolate;
};

#endif	//#ifndef EFFECT_BLUR_H
