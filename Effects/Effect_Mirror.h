/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Mirror
 */

#ifndef EFFECT_MIRROR_H
#define EFFECT_MIRROR_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class BRadioButton;

namespace yrender
{
	class YRenderNode;
	class YGeometryNode;
};
class BBitmap;

class Effect_Mirror : public EffectNode
{
public:
					Effect_Mirror(BRect frame, const char *filename);
					~Effect_Mirror()								override;

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
	
	MediaEffect		*CreateMediaEffect()							override;
	void			MediaEffectSelected(MediaEffect *effect)		override;
	void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)	override;
	
	void			MessageReceived(BMessage *msg)					override;
	
private:
	enum {eNumberRadioButtons = 4};
	yrender::YRenderNode 	*fRenderNode;
	yrender::YGeometryNode 	*fGeometryNodes[eNumberRadioButtons];
	BRadioButton			*fGuiButtons[eNumberRadioButtons];
};

#endif	//#ifndef EFFECT_MIRROR_H
