/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Marker
 */

#ifndef EFFECT_MARKER_H
#define EFFECT_MARKER_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

#ifndef _YARRA_VECTOR_H_
#include "Yarra/Math/Vector.h"
#endif

class BColorControl;
class BCheckBox;
class BChannelSlider;
class BColorControl;
class BOptionPopUp;
class Spinner;
class ValueSlider;

namespace yrender
{
	class YRenderNode;
	class YShaderNode;
};

class BBitmap;
class AlphaColourControl;

class Effect_Marker : public EffectNode
{
public:
					Effect_Marker(BRect frame, const char *filename);
					~Effect_Marker()								override;

	void			AttachedToWindow()								override;
	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const							override	{return EffectNode::EFFECT_TEXT;}
	const int		GetEffectListPriority()							override	{return 0;}
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

	void			OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)		override;
	void			OutputViewMouseMoved(MediaEffect *media_effect, const BPoint &point)	override;
	
private:
	yrender::YRenderNode 	*fRenderNode;
	yrender::YShaderNode 	*fShaderColour;
	yrender::YShaderNode 	*fShaderBackground;

	AlphaColourControl		*fGuiColourControl;
	BView					*fGuiSampleColour;
	Spinner					*fGuiPositionSpinners[4];
	ValueSlider				*fGuiSliderWidth;

	BCheckBox				*fGuiCheckboxInterpolate;
	BChannelSlider			*fGuiSlidersDelay[2];

	BCheckBox				*fGuiCheckboxBackground;
	BColorControl			*fGuiColourMaskColour;
	BOptionPopUp			*fGuiOptionMaskType;
	ValueSlider				*fGuiSliderMaskFilter;

	ymath::YVector2			fPreviousStartPosition;
	ymath::YVector2			fPreviousEndPosition;
	float					fPreviousWidth;

	BPoint					fPreviousConvertedMouseDownPosition;
	int						fMouseTrackingIndex;
	ymath::YVector2			fMouseMovedStartPosition;
	ymath::YVector2			fMouseMovedEndPosition;

	void					CreateRenderGeometry(const ymath::YVector2 &start, const ymath::YVector2 &end, const float width);
};

#endif	//#ifndef EFFECT_MARKER_H
