/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Timeline edit (child of TimelineView)
 */

#ifndef _TIMELINE_EDIT_H_
#define _TIMELINE_EDIT_H_

#ifndef _VIEW_H
#include <interface/View.h>
#endif

#ifndef _PROJECT_H_
#include "Project.h"
#endif

class TimelineTrack;
class ClipTagWindow;
class TimelineView;

class BCursor;
class BMessage;

//=======================
class TimelineEdit : public BView
{
public:
	enum kTimelineEditMessages
	{
		eMsgDragDropEffect = 'ted0',
		eMsgDragDropClip,
		eMsgClipEditTagComplete,
		eMsgClipEditNoteComplete,
		eMsgClipEditTagCancelled,
	};
	static constexpr int64	kDefaultNewEffectDuration = 2*kFramesSecond;

					TimelineEdit(BRect frame, TimelineView *parent);
					~TimelineEdit()								override;
	void			MessageReceived(BMessage *msg)				override;
	void			Draw(BRect frame)							override;
	void			FrameResized(float width, float height)		override;
	
	bool			KeyDownMessage(BMessage *msg);
	void			MouseDown(BPoint point)						override;
	void			MouseMoved(BPoint point, uint32 transit, const BMessage *message) override;
	void			MouseUp(BPoint point)						override;

	bool			OutputViewMouseDown(const BPoint &point);
	bool			OutputViewMouseMoved(const BPoint &point);
	bool			OutputViewZoomed(const float zoom_factor);

	void			SetZoomFactor(const int64 visible_frames);
	float			GetFramesPixel() const {return fFramesPixel;}
	void			SetScrollViewOrigin(const bigtime_t frame_idx, const float top_offset)  {fLeftFrameIndex = frame_idx; fScrollViewOffsetY = top_offset;}
	void			ProjectInvalidated();

	void			SetTrackShowClipTags(const bool visible);
	void			SetTrackShowNotes(const bool visible);
	void			GetTrackOffsets(std::vector<float> &offsets);
	void			SetShowAllVideoThumbnails(const bool show);

	void			DragDropClip(BMessage *msg);
	void			DragDropEffect(BMessage *msg);
	void			SetTimelineScrub(const bool enable);
	
private:
	int64			fFramesPixel;	//	number of frames per pixel
	float			fViewWidth;
	float			fScreenWidth;
	int64			fLeftFrameIndex;
	float			fScrollViewOffsetY;
	bool			fDrawAllVideoThumbnails;
	
	enum class State {eIdle, eMoveClip, eResizeClip, eMoveEffect, eResizeEffect, eTimelineScrub};
	State			fState;
	
	BPoint			fMouseDownPoint;
	BMessage		*fMsgDragDropClip;
	BMessage		*fMsgDragDropEffect;
	BMessage		*fMsgOuptutViewMouseDown;
	BMessage		*fMsgOuptutViewMouseMoved;

	ClipTagWindow	*fClipTagWindow;
	TimelineView	*fTimelineView;
	
	void			DrawTrack(TimelineTrack *track, BRect frame);
	void			DrawTrackEffects(TimelineTrack *track, BRect frame);
	void			DrawTrackNotes(TimelineTrack *track, BRect frame);
	void			DrawTrackThumbnails(const BRect &rect, MediaSource *source, const int64 left_frame, const int64 right_frame);

	void			ContextMenuTrack(BPoint point);
	void			ContextMenuClip(BPoint point);
	void			ContextMenuEffect(BPoint point);
	void			ContextMenuNote(BPoint point);

	//	Selected Clip
	struct ACTIVE_CLIP
	{
		TimelineTrack	*track;
		int32			clip_idx;
		bigtime_t		frame_idx;		//	mouse down (relative to MediaClip::mSourceFrameStart)
		BRect			highlight_rect;
		bool			highlight_rect_visible;
		ACTIVE_CLIP() : track(nullptr), clip_idx(-1) {}
		void Reset()	{track = nullptr; clip_idx = -1;}
	};
	ACTIVE_CLIP				fActiveClip;
	bool					fClipTagsVisible;
	bool					FindClip(const BPoint &point, ACTIVE_CLIP &aClip, const float grace_x = 0.0f);
	void					MoveClipUpdate(BPoint point);
	void					FindClipLinkedEffects();
	void					MoveClipLinkedEffects();
	void					MoveClipLinkedEffects(TimelineTrack *track, int clip_index, int64 delta);
	void					ResizeClipUpdate(BPoint point);
	void					ResizeEffectUpdate(BPoint point);
	int64					CalculateStickyFrameIndex(int64 frame_idx, bool left_reference);
	void					CreateClipTransformEffect(ACTIVE_CLIP &clip);
	void					EffectMatchClipDuration(TimelineTrack *track, MediaEffect *effect);
	BBitmap					*CreateDragDropClipBitmap(BRect frame);

	struct LINKED_EFFECT
	{
		MediaEffect		*effect;
		bigtime_t		frame_offset;	//	relative to clip.mTimelineFrameStart
		LINKED_EFFECT(MediaEffect *_effect, bigtime_t _frame) : effect(_effect), frame_offset(_frame) {}
	};
	std::vector<LINKED_EFFECT>	fClipLinkedEffects;

	//	Selected Effect
	struct ACTIVE_EFFECT
	{
		TimelineTrack	*track;
		int32			effect_idx;
		bigtime_t		frame_idx;		//	mouse down (relative to MediaEffect::mTimelineFrameStart)
		int32			clip_idx;
		BRect			highlight_rect;
		bool			highlight_rect_visible;
		MediaEffect		*media_effect;
		ACTIVE_EFFECT() : track(nullptr), effect_idx(-1), clip_idx(-1), media_effect(nullptr) {}
		void Reset()	{track = nullptr; effect_idx = -1; clip_idx = -1; media_effect = nullptr;}
	};
	ACTIVE_EFFECT			fActiveEffect;
	bool					FindEffect(const BPoint &point, ACTIVE_EFFECT &anEffect, const float grace_x = 0.0f);
	void					MoveEffectUpdate(BPoint point);

	//	Selected Note
	bool					FindNote(const BPoint &point);
	void					CalculateMediaNoteFrame(MediaNote &note);
	void					RemoveActiveNote();
	struct ACTIVE_NOTE
	{
		TimelineTrack	*track;
		int32			note_idx;
		bigtime_t		frame_idx;
		ACTIVE_NOTE() : track(nullptr), note_idx(-1) {}
		void Reset()	{track=nullptr; note_idx = -1;}
	};
	ACTIVE_NOTE				fActiveNote;

	TimelineTrack			*FindTimelineTrack(BPoint point);
	bool					fTrackNotesVisible;

	enum class SelectedItem {eSelectedNone, eSelectedClip, eSelectedEffect, eSelectedNote};
	SelectedItem			fSelectedItem;
	TimelineTrack			*fContextTimelineTrack;

	//	Resize cursors
	enum ACTIVE_CURSOR {CURSOR_DEFAULT = -1, CURSOR_MOVE, CURSOR_RESIZE, NUMBER_CURSORS};
	BCursor					*fCursors[NUMBER_CURSORS];
	ACTIVE_CURSOR			fActiveCursor;
	enum class ResizeDirection {eInactive, eLeft, eRight};
	ResizeDirection			fActiveResizeDirection;
	ResizeDirection			fPendingResizeDirection;
	int64_t					fResizeClipOriginalSourceFrame;
	int64_t					fResizeClipOriginalTimelineFrame;
	enum class ResizeObject {eResizeClip, eResizeEffect};
	ResizeObject			fResizeObject;
	
	//	Animated drag / drop
	TimelineTrack			*fAnimateDragDropTrack;
	bigtime_t				fAnimateDragDropTimestamp;
	std::vector<MediaClip>	fAnimateDragDropClips;
	void					AnimateDragDropDrawTrack(TimelineTrack *track, BRect frame);

	static constexpr float			kTimelineTrackInitialY = 20.0f;
	static constexpr float			kTimelineTrackHeight = 9*6;
	static constexpr float			kTimelineEffectHeight = 32.0f;
	static constexpr float			kTimelineTrackDeltaY = 64.0f;
	static constexpr float			kTimelineTrackSoundY = 24.0f;
	static constexpr float			kTimelineClipResizeGraceX = 4.0f;
	static constexpr int64			kTimelineElasticGraceX = 12;
	static constexpr float			kRoundRectRadius = 8.0f;
	static constexpr std::size_t	kMaxNumberTimelineTracks = 16;
};

#endif	//#ifndef _TIMELINE_EDIT_H_
