/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Timeline Position
 */
 
#ifndef TIMELINE_POSITION_H_
#define TIMELINE_POSITION_H_

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class TimelineView;
class BCursor;

class TimelinePosition : public BView
{
public:
					TimelinePosition(BRect frame, TimelineView *parent);
					~TimelinePosition() override;

	void			SetZoomFactor(const int64 visible_frames);
	void			SetPosition(const int64 position);
	const int64		GetCurrrentPosition() const {return fCurrentPosition;}
	void			SetKeyframeMarkerPosition(const uint32 index, const int64 position);
	const int64		GetKeyframeMarkerPosition(const uint32 index);
	void			InitTimelineLabels();

	void			FrameResized(float width, float height) override;
	void			MouseDown(BPoint point) override;
	void			MouseMoved(BPoint point, uint32 transit, const BMessage *message) override;
	void			MouseUp(BPoint point) override;
	void			MessageReceived(BMessage *msg) override;
	void			Draw(BRect frame) override;

private:
	TimelineView	*fTimelineView;
	int64			fFramesPixel;	//	number of frames per pixel
	int64			fCurrentPosition;
	int64			fZoomTimingIndex;
	float			fScreenWidth;

	enum {NUMBER_KEYFRAME_MARKERS = 2};
	int64			fKeyframeMarkers[NUMBER_KEYFRAME_MARKERS];
	int64			fKeyframeMarkerEditPosition;


	enum class DragState {eIdle, eShowPosition, eShowMarkerA, eShowMarkerB, eMovePosition, eMoveMarkerA, eMoveMarkerB};
	DragState		fDragState;
	BCursor			*fDragCursor;
	void			UpdateDragPosition(int64 timeline);
	void			ContextMenu(BPoint point);

	struct LABEL
	{
		char			text[16];
		float			width;
		float			position;
	};
	std::vector<LABEL>	fLabels;

	struct MARK
	{
		float		x;
		float		y0;
		float		y1;
	};
	std::vector<MARK>	fMarks;
};

#endif	//#ifndef TIMELINE_POSITION_H_


