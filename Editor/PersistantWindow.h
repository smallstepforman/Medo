/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	Persistant window will hide when closed (not quit)
 *					Use Terminate() to actually close the window
 */

#ifndef PERSISTANT_WINDOW_H
#define PERSISTANT_WINDOW_H

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

class PersistantWindow : public BWindow
{
	bool	fReallyQuit;
public:
					PersistantWindow(BRect frame, const char* title, window_type type = B_DOCUMENT_WINDOW, uint32 flags = B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS);
	virtual			~PersistantWindow()		override { }
	virtual bool	QuitRequested()			override;
	virtual void	Terminate();
};

#endif //#ifndef PERSISTANT_WINDOW_H
