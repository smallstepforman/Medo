/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Colour scope
 */

#ifndef COLOUR_SCOPE_H
#define COLOUR_SCOPE_H

#ifndef PERSISTANT_WINDOW_H
#include "PersistantWindow.h"
#endif

class BBitmap;
class ScopeView;

class ColourScope : public PersistantWindow
{
public:
						ColourScope(BRect frame, const char *title);
						~ColourScope()					override;
	void				MessageReceived(BMessage *msg)	override;

private:
	ScopeView			*fScopeView;
};


#endif	//#ifndef COLOUR_SCOPE_H
