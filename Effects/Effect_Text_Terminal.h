/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect text terminal (teletype)
 */

#ifndef EFFECT_TEXT_TERMINAL_H
#define EFFECT_TEXT_TERMINAL_H

#ifndef EFFECT_TEXT_H
#include "Effect_Text.h"
#endif

class BTextControl;
class BRadioButton;
class BChannelSlider;

class Effect_TextTerminal : public Effect_Text
{
public:
					Effect_TextTerminal(BRect frame, const char *filename);
					~Effect_TextTerminal()							override;
	void			AttachedToWindow()								override;

	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	const int		GetEffectListPriority()							override	{return 90;}
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

	void			TextUpdated()									override;

private:
	enum kAlignment
	{
		kAlignmentLeft,
		kAlignmentCenter,
		kAlignmentRight,
		kNumberAlignmentButtons
	};
	kAlignment		fAlignment;

	BRadioButton	*fAlignmentRadioButtons[kNumberAlignmentButtons];
	BChannelSlider	*fSliderThreshold[2];
};

#endif	//#ifndef EFFECT_TEXT_TERMINAL_H
