/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Speed
 */

#ifndef EFFECT_SPEED_H
#define EFFECT_SPEED_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class ValueSlider;

class Effect_Speed : public EffectNode
{
public:
					Effect_Speed(BRect frame, const char *filename);
					~Effect_Speed();
	void			AttachedToWindow()								override;

	EFFECT_GROUP	GetEffectGroup() const							override	{return EffectNode::EFFECT_SPATIAL;}
	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	BBitmap			*GetIcon()										override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	const char		*GetTextA(const uint32 language_idx)			override;
	const char		*GetTextB(const uint32 language_idx)			override;

	bool			IsSpeedEffect()	const							override {return true;}
	int64			GetSpeedTime(const int64 frame_idx, MediaEffect *effect);
	
	MediaEffect		*CreateMediaEffect();
	void			MediaEffectSelected(MediaEffect *effect)		override;
	
	void			MessageReceived(BMessage *msg)					override;
	
private:
	ValueSlider		*fSpeedSlider;
};

#endif	//#ifndef EFFECT_SPEED_H
