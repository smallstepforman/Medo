/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	Persistant window will hide when closed (not quit)
 *					Use Terminate() to actually close the window
 */

#include "PersistantWindow.h"

/*	FUNCTION:		PersistantWindow :: PersistantWindow
	ARGS:			frame
					title
					window_type
					flags
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
PersistantWindow :: PersistantWindow(BRect frame, const char* title, window_type type, uint32 flags)
	: BWindow(frame, title, type, flags), fReallyQuit(false)
{ }

/*	FUNCTION:		PersistantWindow :: QuitRequested
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function when close button pressed
*/
bool PersistantWindow :: QuitRequested()
{
	if (!fReallyQuit)
		Hide();
	return fReallyQuit;
}

/*	FUNCTION:		PersistantWindow :: Terminate
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Invoked by main application
*/
void PersistantWindow :: Terminate()
{
	fReallyQuit = true;
	PostMessage(B_QUIT_REQUESTED);
}
