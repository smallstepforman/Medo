/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	About Window
 */

#ifndef ABOUT_WINDOW_H
#define ABOUT_WINDOW_H

#ifndef PERSISTANT_WINDOW_H
#include "PersistantWindow.h"
#endif

class AboutView;

class AboutWindow : public PersistantWindow
{
public:
						AboutWindow(BRect frame, const char *title);
						~AboutWindow()	override;

private:
	AboutView			*fAboutView;
};


#endif	//#ifndef ABOUT_WINDOW_H
