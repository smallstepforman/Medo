/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2020
 *	DESCRIPTION:	Monitor Window
 */

#ifndef MONITOR_WINDOW_H
#define MONITOR_WINDOW_H

#ifndef PERSISTANT_WINDOW_H
#include "PersistantWindow.h"
#endif

class BBitmap;
class MonitorView;
class TimelinePlayer;

class MonitorWindow : public PersistantWindow
{
public:
						MonitorWindow(BRect frame, const char *title, TimelinePlayer *player);
						~MonitorWindow()					override;

	void				MessageReceived(BMessage *msg)		override;
	void				Zoom(BPoint p, float w, float h)	override;
	void				RestoreZoom();
	const bool			IsFullscreen() const {return fFullscreen;}
private:
	MonitorView			*fMonitorView;
	BRect				fPreZoomFrame;
	bool				fFullscreen;
};


#endif	//#ifndef MONITOR_WINDOW_H
