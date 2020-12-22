/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Node Audio Gain
 */

#ifndef EFFECT_AUDIO_GAIN_H
#define EFFECT_AUDIO_GAIN_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class BCheckBox;
class DualSlider;

class Effect_AudioGain : public EffectNode
{
public:
					Effect_AudioGain(BRect frame, const char *filename);
					~Effect_AudioGain()								override;
	void			AttachedToWindow()								override;

	EFFECT_GROUP	GetEffectGroup() const							override {return EffectNode::EFFECT_AUDIO;}
	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	BBitmap			*GetIcon()										override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	const char		*GetTextA(const uint32 language_idx)			override;
	const char		*GetTextB(const uint32 language_idx)			override;
	
	MediaEffect		*CreateMediaEffect() override;
	void			MediaEffectSelected(MediaEffect *effect)		override;
	int				AudioEffect(MediaEffect *effect, uint8 *destination, uint8 *source,
								const int64 start_frame, const int64 end_frame,
								const int64 audio_start, const int64 audio_end,
								const int count_channels, const size_t sample_size, const size_t count_samples) override;
	
	void			MessageReceived(BMessage *msg)					override;

private:
	DualSlider		*fSliderStart;
	DualSlider		*fSliderEnd;
	BCheckBox		*fCheckboxInterpolate;
};

#endif	//#ifndef EFFECT_AUDIO_GAIN_H
