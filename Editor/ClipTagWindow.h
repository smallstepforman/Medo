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
class TimelineEdit;

class ClipTagWindow : public BWindow
{
public:
	enum CLIP_TAG_TYPE {CLIP_TAG, CLIP_NOTE};

						ClipTagWindow(BPoint pos, CLIP_TAG_TYPE type, TimelineEdit *parent, const char *text);
						~ClipTagWindow()				override;
	void				MessageReceived(BMessage *msg)	override;
	bool				QuitRequested()					override;		//	Close button
	void				Terminate();									//	called from parent window

private:
	BTextControl		*fTextControl;
	BTextView			*fTextView;
	CLIP_TAG_TYPE		fClipTagType;
	TimelineEdit		*fParent;
	BMessage			*fParentMessage;
	bool				fReallyQuit;
};

#endif	//#ifndef _CLIP_TAG_WINDOW_H_
