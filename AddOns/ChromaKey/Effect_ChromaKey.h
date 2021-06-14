/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Chroma Key (Multiple0
 */

#ifndef EFFECT_CHROMA_KEY_H
#define EFFECT_CHROMA_KEY_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class BBitmap;
class BCheckBox;
class BColorControl;

namespace yrender
{
	class YRenderNode;
};

class ValueSlider;
class LanguageJson;

namespace Magnify {class TWindow;};
class BitmapCheckbox;

class Effect_ChromaKey : public EffectNode
{
public:
					Effect_ChromaKey(BRect frame, const char *filename);
					~Effect_ChromaKey()								override;

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
	bool			IsColourEffect() const override {return true;}
	
	void			MessageReceived(BMessage *msg)					override;
	
	static const int kNumberChromaColours = 4;

private:
	yrender::YRenderNode 	*fRenderNodeTexture;
	LanguageJson			*fLanguage;
	ValueSlider				*fGuiSliderThresholds[kNumberChromaColours];
	ValueSlider				*fGuiSliderSmoothings[kNumberChromaColours];
	BColorControl			*fGuiColourControls[kNumberChromaColours];
	BView					*fGuiSampleColours[kNumberChromaColours];
	BCheckBox				*fGuiCheckboxEnabled[kNumberChromaColours];

	BitmapCheckbox			*fColourPickerButtons[kNumberChromaColours];
	Magnify::TWindow		*fColourPickerWindow;
	BMessage				*fColourPickerMessage;
	void					MessageColourPickerSelected(int32 index);
};

extern "C" __declspec(dllexport) Effect_ChromaKey *instantiate_effect(BRect frame);

#endif	//#ifndef EFFECT_CHROMA_KEY_H
