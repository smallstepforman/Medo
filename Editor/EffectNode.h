/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Node
 */

#ifndef EFFECT_NODE_H
#define EFFECT_NODE_H

#ifndef _GLIBCXX_DEQUE
#include <deque>
#endif

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

#ifndef RAPIDJSON_DOCUMENT_H_
#include "rapidjson/document.h"
#endif

class BBitmap;
class BFilePanel;
class BCheckBox;

class MediaEffect;
class TimelineTrack;
class MediaClip;
class EffectDragDropButton;

struct FRAME_ITEM
{
	TimelineTrack	*track;
	MediaClip		*clip;
	MediaEffect		*effect;
	bool			secondary_framebuffer;
	FRAME_ITEM(TimelineTrack *t, MediaClip *c, MediaEffect *e, bool sec_buf) : track(t), clip(c), effect(e), secondary_framebuffer(sec_buf) {}
};

class EffectNode : public BView
{
public:
	enum EFFECT_GROUP {EFFECT_SPATIAL, EFFECT_COLOUR, EFFECT_IMAGE, EFFECT_TRANSITION, EFFECT_SPECIAL, EFFECT_AUDIO, EFFECT_TEXT, NUMBER_EFFECT_GROUPS};
	static constexpr float	kThumbnailHeight = 44.0f;
	static constexpr float	kThumbnailWidth = 80.0f;

							EffectNode(BRect frame, const char *view_name, const bool scroll_view = true);
	virtual					~EffectNode()								override { }
	virtual void			FrameResized(float width, float height)		override;
	virtual void			MouseDown(BPoint where)						override;

	//	Render objects (called from RenderActor thread)
	virtual void			InitRenderObjects() { }
	virtual void			DestroyRenderObjects() { }
	bool					mRenderObjectsInitialised;
		
	//	Project load/save
	virtual EFFECT_GROUP	GetEffectGroup() const = 0;
	virtual const int		GetEffectListPriority() {return 0;}		//	higher appears first in EffectsTab, otherwise sorted by name
	virtual const char		*GetVendorName() const = 0;
	virtual const char		*GetEffectName() const = 0;				//	Language independant name (used in .medo project files)
	virtual bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect) = 0;
	virtual bool			SaveParameters(FILE *file, MediaEffect *media_effect) = 0;
	//	Icon to display in Effects tab
	virtual BBitmap			*GetIcon() = 0;	//	client acquires ownership
	//	Text description in Effects tab
	virtual const char		*GetTextEffectName(const uint32 language_idx) = 0;	//	Translated effect name
	virtual const char		*GetTextA(const uint32 language_idx) = 0;
	virtual const char		*GetTextB(const uint32 language_idx) = 0;
	//	Create MediaEffect
	virtual MediaEffect		*CreateMediaEffect() = 0;
	virtual void			MediaEffectSelected(MediaEffect *effect) { }		//	Called from EffectsWindow
	virtual void			OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)		{ }
	virtual void			OutputViewMouseMoved(MediaEffect *media_effect, const BPoint &point)	{ }
	virtual void			OutputViewZoomed(MediaEffect *media_effect)								{ }
	//	Render effect
	virtual void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx,
										std::deque<FRAME_ITEM> & chained_effects) { }
	virtual int				AudioEffect(MediaEffect *effect, uint8 *destination, uint8 *source,
										const int64 start_frame, const int64 end_frame,
										const int64 audio_start, const int64 audio_end,
										const int count_channels, const size_t sample_size, const size_t count_samples)
										{return 0;}

	virtual bool			IsSpatialTransform() const {return false;}
	virtual void			ChainedSpatialTransform(MediaEffect *data, int64 frame_idx) { }
	virtual void			ChainedMaterialEffect(MediaEffect *data, int64 frame_idx) { }
	virtual bool			IsSpeedEffect() const {return false;}
	virtual bool			UseSecondaryFrameBuffer() const {return false;}

	virtual bool			IsColourEffect() const {return false;}
	virtual rgb_color		ChainedColourEffect(MediaEffect *data, int64 frame_idx) {return {0, 0, 0, 0}; }

//	Inherited drag/drop button
private:
	EffectDragDropButton	*fEffectDragDropButton;
	static MediaEffect		*sCurrentMediaEffect;
	static BMessage			*sMsgEffectSelected;
protected:
	BScrollView				*mScrollView;
	BView					*mEffectView;		//	child of fScrollView
	void					SetViewIdealSize(const float width, const float height);
	float					mEffectViewIdealWidth;
	float					mEffectViewIdealHeight;
	
	void					SetDragDropButtonPosition(BPoint position);
	void					InvalidatePreview();

	void					InitSwapTexturesCheckbox();
	bool					AreTexturesSwapped();
	BCheckBox				*mSwapTexturesCheckbox;
	enum					{kMsgSwapTextureUnits = 'estu'};

public:
	MediaEffect				*GetCurrentMediaEffect() {return sCurrentMediaEffect;}
	void					MediaEffectSelectedBase(MediaEffect *effect);
	void					MediaEffectDeselectedBase(MediaEffect *effect);

//	File panel open
public:
	virtual BFilePanel		*CreateFilePanel(int language_index)				{return nullptr;}
	virtual void			FilePanelOpen(const char *path)						{ }
};

#endif	//#ifndef EFFECT_NODE_H
