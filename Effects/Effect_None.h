/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect None (private use from Project::CreateOutputBitmap)
 */

#ifndef EFFECT_NONE_H
#define EFFECT_NONE_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class BBitmap;

class Effect_None : public EffectNode
{
public:
					Effect_None(BRect frame, const char *filename);
					~Effect_None()									override;
	
	EFFECT_GROUP	GetEffectGroup() const							override	{return EffectNode::EFFECT_IMAGE;}
	const char		*GetVendorName() const							override	{return nullptr;}
	const char		*GetEffectName() const							override	{return nullptr;}
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override	{return false;}
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override	{return false;}
	
	BBitmap			*GetIcon()										override	{return nullptr;}
	const char		*GetTextEffectName(const uint32 language_idx)	override	{return nullptr;}
	const char		*GetTextA(const uint32 language_idx)			override	{return nullptr;}
	const char		*GetTextB(const uint32 language_idx)			override	{return nullptr;}
	MediaEffect		*CreateMediaEffect()							override	{return nullptr;}
	
	void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects) override;
};

#endif	//#ifndef EFFECT_NONE_H
