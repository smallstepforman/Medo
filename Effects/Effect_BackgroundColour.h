/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Background Colour
 */

#ifndef EFFECT_BACKGROUND_COLOUR_H
#define EFFECT_BACKGROUND_COLOUR_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

namespace yrender
{
	class YPicture;
	class YRenderNode;
};
class BBitmap;
class BView;
class BColorControl;

class Effect_BackgroundColour : public EffectNode
{
public:
					Effect_BackgroundColour(BRect frame, const char *filename);
					~Effect_BackgroundColour()						override;
	void			AttachedToWindow()								override;

	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const	override {return EffectNode::EFFECT_BACKGROUND;}
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
	yrender::YPicture		*fSourcePicture;
	BColorControl			*fColorControl;
	BView					*fSampleColour;
};

#endif	//#ifndef EFFECT_BACKGROUND_COLOUR_H
