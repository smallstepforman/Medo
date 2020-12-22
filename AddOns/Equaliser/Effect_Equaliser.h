/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Node Equaliser
 */

#ifndef EFFECT_EQUALISER_H
#define EFFECT_EQUALISER_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

#include "orfanidis_eq.h"

class BSlider;
class BButton;
class BOptionPopUp;
class BFont;

class EffectNode_Equaliser : public EffectNode
{
public:
					EffectNode_Equaliser(BRect frame, const char *filename);
					~EffectNode_Equaliser()							override;
	void			AttachedToWindow()								override;
	void			Draw(BRect frame)								override;
	
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
	std::vector<BSlider *>			fSliders;
	std::vector<BString>			fLabels;
	BFont							*fRotatedFont;
	BButton							*fButtonReset;
	BOptionPopUp					*fOptionFilter;

	OrfanidisEq::FrequencyGrid		fFrequencyGrid;
	OrfanidisEq::Eq					*fEqualiser;
};

extern "C" __declspec(dllexport) EffectNode_Equaliser *instantiate_effect(BRect frame);

#endif	//#ifndef EFFECT_EQUALISER_H
