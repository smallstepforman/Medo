/*
	BitmapButton.h: A button which uses a BBitmap for its looks
	Written by DarkWyrm <darkwyrm@earthlink.net>, Copyright 2007
	Released under the MIT license.
*/
#ifndef BITMAP_BUTTON_H
#define BITMAP_BUTTON_H

#include <Looper.h>
#include <Application.h>
#include <Window.h>
#include <Button.h>
#include <Bitmap.h>
#include <Rect.h>

class BitmapButton : public BButton
{
public:
						BitmapButton(const BRect &frame, const char *name, BBitmap *up, 
									BBitmap *down, BMessage *msg,
									const int32 &resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									const int32 &flags = B_WILL_DRAW | B_NAVIGABLE);
	virtual				~BitmapButton(void);

			void 		SetBitmaps(BBitmap *up, BBitmap *down);
			BBitmap *	UpBitmap(void);
			BBitmap *	DownBitmap(void);
			
			void 		SetDisabledBitmap(BBitmap *disabled);
			BBitmap *	DisabledBitmap(void) const;
			
			void 		SetFocusBitmaps(BBitmap *up, BBitmap *down);
			BBitmap *	UpFocusBitmap(void);
			BBitmap *	DownFocusBitmap(void);
			
	virtual	void		Draw(BRect update);
			
			void 		ResizeToPreferred(void);
private:
	
	BBitmap	*fUp,
			*fDown,
			*fDisabled,
			*fFocusUp,
			*fFocusDown;
};

#endif
