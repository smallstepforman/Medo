/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Audio Effect / IIR Filter
 *					Designing Audio Effects Plugins in C++, Will C. Pirkle, 2nd Edition
 */

#ifndef EFFECT_IIR_FILTER_H
#define EFFECT_IIR_FILTER_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class BSlider;
class BButton;
class BOptionPopUp;
class LanguageJson;

class AudioFilter;

class EffectNode_IIRFilter : public EffectNode
{
public:
					EffectNode_IIRFilter(BRect frame, const char *filename);
					~EffectNode_IIRFilter()							override;
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
	LanguageJson					*fLanguage;
	std::vector<BSlider *>			fSliders;
	BOptionPopUp					*fOptionAlgorithm;
	BButton							*fButtonReset;
	void							SetSliderValue(int index, float value);

	struct FilterCache
	{
		AudioFilter		*audio_filter;
		int64			audio_end_frame;
		float			sample_rate;
	};
	std::vector<FilterCache>		fFilterCache;
};

extern "C" __declspec(dllexport) EffectNode_IIRFilter *instantiate_effect(BRect frame);

#endif	//#ifndef EFFECT_IIR_FILTER_H
