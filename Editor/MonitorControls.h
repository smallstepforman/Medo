/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2020
 *	DESCRIPTION:	Monitor Controls
 */

#ifndef MONITOR_CONTROLS_H
#define MONITOR_CONTROLS_H

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class BitmapCheckbox;
class BitmapButton;
class TimelinePlayer;

class MonitorControls : public BView
{
public:
				MonitorControls(BRect frame, TimelinePlayer *player);
				~MonitorControls()						override;

	void		AttachedToWindow()						override;
	void		FrameResized(float width, float height)	override;
	void		MouseDown(BPoint point)					override;
	void		MouseMoved(BPoint point, uint32 transit, const BMessage *message) override;
	void		MouseUp(BPoint point)		override;
	void		MessageReceived(BMessage *msg)			override;
	void		Pulse()									override;

	void		DrawAfterChildren(BRect frame)			override;

	void		SetCurrentFrame(int64 frame_idx);
	void		WindowActivated(bool enable)			override;

	static constexpr float	kControlHeight = 64.0f;

private:
	BitmapButton		*fButtonRewind;
	BitmapCheckbox		*fButtonPlay;
	int64				fCurrentFrame;
	TimelinePlayer		*fTimelinePlayer;

	uint32				fPulseCount;
};

#endif	//#ifndef MONITOR_CONTROLS_H
