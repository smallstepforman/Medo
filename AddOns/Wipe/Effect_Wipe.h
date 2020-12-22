/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Wipe
 */

#ifndef EFFECT_WIPE_H
#define EFFECT_WIPE_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class BRadioButton;

namespace yrender
{
	class YRenderNode;
};
class BBitmap;

class Effect_Wipe : public EffectNode
{
public:
					Effect_Wipe(BRect frame, const char *filename);
					~Effect_Wipe()									override;

	void			AttachedToWindow()								override;
	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const							override	{return EffectNode::EFFECT_TRANSITION;}
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
	yrender::YRenderNode 	*fRenderNode;
	enum {kNumberWipes = 4};
	BRadioButton			*fGuiButtons[kNumberWipes];
};

extern "C" __declspec(dllexport) Effect_Wipe *instantiate_effect(BRect frame);

#endif	//#ifndef EFFECT_WIPE_H
