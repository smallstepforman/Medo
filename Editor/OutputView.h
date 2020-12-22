/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Control output
 */
 
#ifndef _OUTPUT_VIEW_H_
#define _OUTPUT_VIEW_H_

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class BBitmap;
class TimelineView;

class OutputView : public BView
{
private:
	BBitmap	*fBitmap;
public:
				OutputView(BRect frame);
				~OutputView()						override;
		
	void		Draw(BRect frame)					override;
	void		MouseDown(BPoint point)				override;
	void		MouseMoved(BPoint point, uint32 transit, const BMessage *message)	override;
	void		MouseUp(BPoint point)				override;
	void		MessageReceived(BMessage *msg)		override;

	void		SetBitmap(BBitmap *bitmap);
	void		SetTimelineView(TimelineView *view);
	void		Zoom(bool in);
	BBitmap		*GetBitmap()	{return fBitmap;}

	float			GetZoomFactor() const {return fZoomFactor;}
	const BPoint	&GetZoomOffset() const	{return fZoomOffset;}

	BPoint			GetProjectConvertedMouseDown(const BPoint &point);

private:
	bool			fMouseTracking;
	TimelineView	*fTimelineView;

	float			fZoomFactor;
	BPoint			fZoomOffset;
	BPoint			fMouseDownPoint;
};

#endif	//#ifndef _OUTPUT_VIEW_H_
