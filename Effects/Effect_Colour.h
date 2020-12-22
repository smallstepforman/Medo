/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Alpha / Colour
 */

#ifndef EFFECT_COLOUR_H
#define EFFECT_COLOUR_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class BColorControl;
class BCheckBox;

namespace yrender
{
	class YRenderNode;
};

class BBitmap;
class AlphaColourControl;

class Effect_Colour : public EffectNode
{
public:
					Effect_Colour(BRect frame, const char *filename);
					~Effect_Colour()								override;

	void			AttachedToWindow()								override;
	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const							override	{return EffectNode::EFFECT_COLOUR;}
	const int		GetEffectListPriority()							override	{return 99;}
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
	bool			IsColourEffect() const {return true;}
	rgb_color		ChainedColourEffect(MediaEffect *data, int64 frame_idx);
	
	void			MessageReceived(BMessage *msg)					override;
	
private:
	yrender::YRenderNode 	*fRenderNodeTexture;
	yrender::YRenderNode 	*fRenderNodeBackground;
	AlphaColourControl		*fGuiColourControls[2];
	BView					*fGuiSampleColours[2];
	BCheckBox				*fGuiInterpolate;
};

#endif	//#ifndef EFFECT_COLOUR_H
