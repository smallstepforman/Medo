/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect text 3D
 */

#ifndef EFFECT_TEXT_3D_H
#define EFFECT_TEXT_3D_H

#ifndef EFFECT_TEXT_H
#include "Effect_Text.h"
#endif

class ValueSlider;

class Effect_Text3D : public Effect_Text
{
public:
					Effect_Text3D(BRect frame, const char *filename);
					~Effect_Text3D()								override;
	void			AttachedToWindow()								override;

	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	const int		GetEffectListPriority()							override	{return 98;}
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
	ValueSlider		*fSliderDepth;
	void			CreateOpenGLObjects(EffectTextData *data);	//	Called from OpenGL thread
};

#endif	//#ifndef EFFECT_TEXT_3D_H
