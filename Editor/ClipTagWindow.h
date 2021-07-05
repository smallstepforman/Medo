/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	TimlineEdit clip tag window
 */

#ifndef _CLIP_TAG_WINDOW_H_
#define _CLIP_TAG_WINDOW_H_

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

class BTextControl;
class BTextView;
class BMessage;

class ClipTagWindow : public BWindow
{
public:
	enum Type {eClipTag, eNote, eSourceLabel};

						ClipTagWindow(BPoint pos, Type type, BView *parent, const char *text);
						~ClipTagWindow()				override;
	void				MessageReceived(BMessage *msg)	override;
	bool				QuitRequested()					override;		//	Close button
	void				Terminate();									//	called from parent window

private:
	BTextControl		*fTextControl;
	BTextView			*fTextView;
	Type				fClipTagType;
	BView				*fParent;
	BMessage			*fParentMessage;
	bool				fReallyQuit;
};

#endif	//#ifndef _CLIP_TAG_WINDOW_H_
