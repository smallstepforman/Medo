/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Mask
 */

#ifndef EFFECT_MASK_H
#define EFFECT_MASK_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

namespace yrender
{
	class YRenderNode;
};

class BBitmap;
class BCheckBox;
class BListView;
class BButton;

class PathView;
class BitmapCheckbox;
class KeyframeSlider;

class Effect_Mask : public EffectNode
{
public:
					Effect_Mask(BRect frame, const char *filename);
					~Effect_Mask()									override;

	void			AttachedToWindow()								override;
	void			DetachedFromWindow()							override;
	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const							override	{return EffectNode::EFFECT_IMAGE;}
	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	bool			UseSecondaryFrameBuffer() const					override	{return true;}

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
	yrender::YRenderNode 	*fRenderNode;
	PathView				*fPathView;
	bool					fPathViewAttachedToWindow;
	BitmapCheckbox			*fPathCheckbox;
	BCheckBox				*fShowFillCheckbox;
	BCheckBox				*fInverseCheckbox;

	BListView				*fKeyframeList;
	BButton					*fKeyframeAddButton;
	BButton					*fKeyframeRemoveButton;
	KeyframeSlider			*fKeyframeSlider;
	int						fCurrentKeyframe;
};

#endif	//#ifndef Effect_Mask
