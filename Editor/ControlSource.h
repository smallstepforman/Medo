/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016
 *	DESCRIPTION:	Control source + clip generator
 */
 
#ifndef _CONTROL_SOURCE_H_
#define _CONTROL_SOURCE_H_

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class BBitmap;

class MediaSource;
class ClipTimeline;

//=======================
class ControlSource : public BView
{
public:
					ControlSource(BRect frame);
					~ControlSource();
	void			FrameResized(float width, float height);
	void			Draw(BRect frame);
				
	void			SetMediaSource(MediaSource *media);
	void			ShowPreview(int64 frame);

private:
	BBitmap			*fBitmap;
	MediaSource		*fMediaSource;
	ClipTimeline	*fClipTimeline;
};

#endif	//#ifndef _CONTROL_SOURCE_H_
