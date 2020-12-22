/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Colour Correction
 */

#ifndef EFFECT_COLOUR_CORRECTION_H
#define EFFECT_COLOUR_CORRECTION_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

namespace yrender
{
	class YRenderNode;
};
namespace Magnify {class TWindow;};

class BBitmap;
class BOptionPopUp;
class BButton;
class BRadioButton;
class BCheckBox;
class CurvesView;
class BitmapCheckbox;
class ValueSlider;

class Effect_ColourCorrection : public EffectNode
{
public:
					Effect_ColourCorrection(BRect frame, const char *filename);
					~Effect_ColourCorrection()						override;
	void			AttachedToWindow()								override;

	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const							override	{return EffectNode::EFFECT_COLOUR;}
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
	void			MessageReceived(BMessage *msg)					override;
	
private:
	yrender::YRenderNode 	*fRenderNode;
	CurvesView				*fCurvesView;
	BOptionPopUp			*fOptionInterpolation;
	enum {NUMBER_COLOUR_BUTTONS = 3};
	BRadioButton			*fButtonColours[NUMBER_COLOUR_BUTTONS];
	BButton					*fButtonReset;
	ValueSlider				*fWhiteBalanceSliders[3];

	Magnify::TWindow		*fColourPickerWindow;
	BitmapCheckbox			*fColourPickerButton;
	BMessage				*fColourPickerMessage;
};

#endif	//#ifndef EFFECT_COLOUR_CORRECTION_H
