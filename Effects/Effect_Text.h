/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect text
 */

#ifndef EFFECT_TEXT_H
#define EFFECT_TEXT_H

#ifndef _B_STRING_H
#include <support/String.h>
#endif

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

#ifndef _YARRA_VECTOR_H_
#include "Yarra/Math/Vector.h"
#endif

namespace yrender
{
	class YRenderNode;
	class YTextScene;

};

class BBitmap;
class BButton;
class BCheckBox;
class BPoint;
class BMessenger;
class BStringView;
class BTextView;

class AlphaColourControl;
class FontPanel;
class Spinner;

class Effect_Text : public EffectNode
{
public:
						Effect_Text(BRect frame, const char *filename);
						~Effect_Text()								override;
	virtual void		AttachedToWindow()							override;

	void				InitRenderObjects();
	void				DestroyRenderObjects();
	
	EFFECT_GROUP		GetEffectGroup() const	override {return EffectNode::EFFECT_TEXT;}
	virtual const int	GetEffectListPriority()						override	{return 99;}
	virtual const char	*GetVendorName() const						override;
	virtual const char	*GetEffectName() const						override;
	virtual bool		LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	virtual bool		SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	virtual BBitmap		*GetIcon()									override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	virtual const char	*GetTextA(const uint32 language_idx)		override;
	virtual const char	*GetTextB(const uint32 language_idx)		override;
	
	virtual MediaEffect	*CreateMediaEffect()						override;
	virtual void		MediaEffectSelected(MediaEffect *effect)	override;
	void				OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)		override;
	void				OutputViewMouseMoved(MediaEffect *media_effect, const BPoint &point)	override;
	virtual void		RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)	override;

	virtual void		MessageReceived(BMessage *msg)				override;

	virtual void		TextUpdated();

	struct EffectTextData
	{
		int					direction;
		ymath::YVector3		position;
		rgb_color			font_colour;
		BString				font_path;
		int					font_size;
		bool				background;
		rgb_color			background_colour;
		int					background_offset;
		BString				text;
		bool				shadow;
		ymath::YVector2		shadow_offset;
		void				*derived_data;
	};

protected:
	BTextView				*fTextView;
	yrender::YRenderNode 	*fRenderNode;
	yrender::YTextScene		*fTextScene;
	bool					fOpenGLPendingUpdate;
	bool					fIs3dFont;

	void					InitMediaEffect(MediaEffect *effect);
	virtual void			CreateOpenGLObjects(EffectTextData *data);	//	Called from OpenGL thread
	bool					SaveParametersBase(FILE *file, MediaEffect *media_effect, const bool append_comma);

private:
	FontPanel				*fFontPanel;
	AlphaColourControl		*fFontColourControl;
	BButton					*fFontButton;
	BMessenger				*fFontMessenger;

protected:
	BStringView				*fBackgroundTitle;
	BCheckBox				*fBackgroundCheckBox;
	AlphaColourControl		*fBackgroundColourControl;
	Spinner					*fBackgroundOffset;

	BCheckBox				*fShadowCheckBox;
	Spinner					*fShadowSpinners[2];
};

#endif	//#ifndef EFFECT_TEXT_H
