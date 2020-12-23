/*	PROJECT:			Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	GUI Keyframe slider
 */
 
 #ifndef _KEYFRAME_SLIDER_H_
 #define _KEYFRAME_SLIDER_H_

#ifndef _VIEW_H
#include <interface/View.h>
#endif
 
class KeyframeSlider : public BView
{
public:
			KeyframeSlider(BRect frame);
			~KeyframeSlider() override;

	void	MouseDown(BPoint where)	override;
	void	MouseMoved(BPoint where, uint32 code, const BMessage *drag_message) override;
	void	MouseUp(BPoint where) override;

	void	Draw(BRect) override;

	void	SetPoints(std::vector<float> &points);
	void	GetPoints(std::vector<float> &points);
	void	Select(int index);
	void	SetObserver(BLooper *looper, BHandler *handler, BMessage *message);

private:
	std::vector<float>		fPoints;
	int						fMouseTrackingIndex;
	int						fSelectIndex;

	BLooper					*fTargetLooper;
	BHandler				*fTargetHandler;
	BMessage				*fTargetMessage;
};

#endif
