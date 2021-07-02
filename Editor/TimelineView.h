/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	Timeline view
 */

#ifndef TIMELINE_VIEW_H
#define TIMELINE_VIEW_H

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class BScrollView;
class BSlider;
class BMessage;

class MedoWindow;
class TimelineEdit;
class TimelinePlayer;
class TimelinePosition;
class TimelineControlView;
class BitmapCheckbox;
class BitmapButton;

//=======================
class TimelineView : public BView
{
public:
	enum INVALIDATE_MASK
	{
		INVALIDATE_VIEW					= 1 << 0,
		INVALIDATE_VERTICAL_SLIDER		= 1 << 1,
		INVALIDATE_HORIZONTAL_SLIDER	= 1 << 2,
		INVALIDATE_EDIT_TRACKS			= 1 << 3,
		INVALIDATE_POSITION_SLIDER		= 1 << 4,
		INVALIDATE_CONTROL_VIEW			= 1 << 5,
	};

					TimelineView(BRect frame, MedoWindow *parent);
					~TimelineView()								override;

	void			Draw(BRect frame)							override;
	void			FrameResized(float width, float height)		override;
	void			MouseDown(BPoint point)						override;
	void			MessageZoomSlider(BMessage *msg);
	void			MessageReceived(BMessage *msg)				override;
	void			AttachedToWindow()							override;
	
	void			ScrollToHorizontal(const float x);
	void			ScrollToVertical(const float y);
	
	void			InvalidateItems(uint32 mask);
	int64			GetCurrrentFrame() const	{return fCurrentFrame;}
	int64			GetLeftFrameIndex() const	{return fLeftFrameIndex;}
	void			PositionUpdate(const int64 position, const bool generate_output_preview = true);
	void			PositionKeyframeUpdate();
	void			PlayComplete();

	void			OutputViewMouseDown(const BPoint &point);
	void			OutputViewMouseMoved(const BPoint &point);
	void			OutputViewZoomed(const float zoom_factor);
	bool			KeyDownMessage(BMessage *msg);
	bool			KeyUpMessage(BMessage *msg);

	TimelinePlayer	*GetTimelinePlayer() {return fTimelinePlayer;}
	TimelineEdit	*GetTimelineEdit() {return fTimelineEdit;}
	
private:
	BScrollView			*fVerticalScrollView;
	BScrollView			*fHorizontalScrollView;
	BSlider				*fZoomSlider;
	BBitmap				*fZoomIcon;
	BMessage			*fMsgZoomSlider;
	
	TimelineEdit		*fTimelineEdit;
	TimelinePosition	*fTimelinePosition;
	MedoWindow			*fParent;

	float				fViewWidth;
	int64				fLeftFrameIndex;
	float				fEditViewScrollOffsetY;
	int64				fCurrentFrame;
	int32				fZoomSliderValue;
	int32				fZoomKeyRestoreValue;
	void				UpdateZoom();
	void				SetTimeSliderLabels();
	void				UpdateHorizontalScrollBar();
	void				UpdateVerticalScrollBar();
	float				GetTimelineEditWidth() const;

	TimelinePlayer		*fTimelinePlayer;
	BitmapCheckbox		*fButtonPlay;
	BitmapCheckbox		*fButtonPlayAb;
	BitmapButton		*fButtonFrameNext;
	BitmapButton		*fButtonFramePrev;
	enum PLAY_MODE {PLAY_OFF, PLAY_ALL, PLAY_AB};
	PLAY_MODE			fPlayMode;
	TimelineControlView	*fControlView;

	struct TRACK_SETTINGS
	{
		BitmapCheckbox	*visual;
		BitmapCheckbox	*audio;
	};
	std::vector<TRACK_SETTINGS>	fTrackSettings;
	void				UpdateControlView();

public:
	struct SESSION
	{
		float		horizontal_scroll;
		float		vertical_scroll;
		int32		zoom_index;
		int64		current_frame;
		int64		marker_a;
		int64		marker_b;
	};
	void			SetSession(const SESSION &session);
	const SESSION	GetSession();
	void			ProjectLoaded();
};

#endif	//#ifndef TIMELINE_VIEW_H
