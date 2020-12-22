/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect text counter
 */

#ifndef EFFECT_TEXT_COUNTER_H
#define EFFECT_TEXT_COUNTER_H

#ifndef EFFECT_TEXT_H
#include "Effect_Text.h"
#endif

class BTextControl;
class BRadioButton;
class BChannelSlider;

class Effect_TextCounter : public Effect_Text
{
public:
					Effect_TextCounter(BRect frame, const char *filename);
					~Effect_TextCounter()							override;
	void			AttachedToWindow()								override;

	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	const int		GetEffectListPriority()							override	{return 97;}
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
	friend class	TextCounterMediaEffect;

	enum kCounterType
	{
		kCounterCurrency,
		kCounterNumber,
		kCounterTimeMinSec,
		kCounterTimeHourMinSec,
		kCounterDate,
		kNumberCounters,
	};

	BTextControl	*fTextRange[2];
	BChannelSlider	*fSliderThreshold[2];
	BRadioButton	*fRadioControl[kNumberCounters];
	BTextControl	*fTextFormat;
	kCounterType	fCounterType;

	void			GenerateText_Currency(const float t, Effect_Text::EffectTextData *data);
	void			GenerateText_Number(const float t, Effect_Text::EffectTextData *data);
	void			GenerateText_TimeMinSec(const float t, Effect_Text::EffectTextData *data);
	void			GenerateText_TimeHourMinSec(const float t, Effect_Text::EffectTextData *data);
	void			GenerateText_Date(const float t, Effect_Text::EffectTextData *data);
};

#endif	//#ifndef EFFECT_TEXT_COUNTER_H
