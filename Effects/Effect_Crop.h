/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Crop
 */

#ifndef EFFECT_CROP_H
#define EFFECT_CROP_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

namespace yrender
{
	class YRenderNode;
};
class BBitmap;
class BScrollView;
class Spinner;

class Effect_Crop : public EffectNode
{
public:
					Effect_Crop(BRect frame, const char *filename);
					~Effect_Crop()									override;
	void			AttachedToWindow()								override;
	
	EFFECT_GROUP	GetEffectGroup() const							override		{return EffectNode::EFFECT_SPATIAL;}
	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	BBitmap			*GetIcon()										override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	const char		*GetTextA(const uint32 language_idx)			override;
	const char		*GetTextB(const uint32 language_idx)			override;
	
	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	MediaEffect		*CreateMediaEffect()							override;
	void			MediaEffectSelected(MediaEffect *effect)		override;
	void			RenderEffect(BBitmap *source, MediaEffect *effect, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)	override;
	
	void			MessageReceived(BMessage *msg)					override;
	void			OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)		override;
	void			OutputViewMouseMoved(MediaEffect *media_effect, const BPoint &point)	override;
	
private:
	yrender::YRenderNode	*fRenderNode;
	void					UpdateGeometry(MediaEffect *effect);
	float					fOldGeometry[4];

	enum CropSpinner {eCenterX, eCenterY, eSizeX, eSizeY};
	Spinner					*fSpinners[4];
	enum CropString {ePixelCenter, ePixelSize};
	BStringView				*fStringView[2];
};

#endif	//#ifndef EFFECT_CROP_H
