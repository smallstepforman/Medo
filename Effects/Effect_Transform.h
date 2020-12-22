/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Transform
 */

#ifndef EFFECT_TRANSFORM_H
#define EFFECT_TRANSFORM_H

#ifndef _YARRA_VECTOR_H_
#include "Yarra/Math/Vector.h"
#endif

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class BBitmap;
class BCheckBox;

class Effect_Transform : public EffectNode
{
public:
					Effect_Transform(BRect frame, const char *filename);
					~Effect_Transform()								override;
	void			AttachedToWindow()								override;

	EFFECT_GROUP	GetEffectGroup() const override	{return EffectNode::EFFECT_SPATIAL;}
	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	BBitmap			*GetIcon()										override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	const char		*GetTextA(const uint32 language_idx)			override;
	const char		*GetTextB(const uint32 language_idx)			override;
	const int		GetEffectListPriority()							override	{return 99;}
	
	MediaEffect		*CreateMediaEffect()							override;
	void			MediaEffectSelected(MediaEffect *effect)		override;
	void			OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)		override;
	void			OutputViewMouseMoved(MediaEffect *media_effect, const BPoint &point)	override;
	
	bool			IsSpatialTransform() const						override		{return true;}
	void			ChainedSpatialTransform(MediaEffect *data, int64 frame_idx)	override;
	void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)	override;

	void			MessageReceived(BMessage *msg)					override;

	void			AutoScale(MediaEffect *effect, MediaSource *source);
	
private:
	BPoint				fOutputViewMouseDown;
	ymath::YVector3		fOutputViewTrackPosition;
	BCheckBox			*fCheckboxInterpolate;
};

#endif	//#ifndef EFFECT_TRANSFORM_H
