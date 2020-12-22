/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Move
 */

#ifndef EFFECT_MOVE_H
#define EFFECT_MOVE_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

namespace yrender
{
	class YPicture;
};
class BBitmap;

class Effect_Move : public EffectNode
{
public:
					Effect_Move(BRect frame, const char *filename);
					~Effect_Move()									override;
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
	
	MediaEffect		*CreateMediaEffect()							override;
	void			MediaEffectSelected(MediaEffect *effect)		override;
	
	bool			IsSpatialTransform() const						override		{return true;}
	void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects);
	void			ChainedSpatialTransform(MediaEffect *data, int64 frame_idx) override;
	
	void			MessageReceived(BMessage *msg)					override;
	
private:
	yrender::YPicture 		*fSourcePicture;
};

#endif	//#ifndef EFFECT_MOVE_H
