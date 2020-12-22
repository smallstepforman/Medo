/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Portrait Blur
 */

#ifndef EFFECT_PORTRAIT_BLUR_H
#define EFFECT_PORTRAIT_BLUR_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

namespace yrender
{
	class YPicture;
	class YRenderNode;
};

class BBitmap;
class BScrollView;
class ValueSlider;

class Effect_PortraitBlur : public EffectNode
{
public:
					Effect_PortraitBlur(BRect frame, const char *filename);
					~Effect_PortraitBlur()							override;
	void			AttachedToWindow()								override;
	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;

	EFFECT_GROUP	GetEffectGroup() const override	{return EffectNode::EFFECT_IMAGE;}
	const int		GetEffectListPriority()							override	{return 98;}
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
	
private:
	yrender::YRenderNode 	*fBlurRenderNode;
	ValueSlider				*fBlurSlider;
};

#endif	//#ifndef EFFECT_PORTRAIT_BLUR_H
