/*
	FontPanel.cpp: A general-purpose font selection class
	Written by DarkWyrm <bpmagic@columbus.rr.com>, Copyright 2004
	Released under the MIT license.

	ZenYes 2019 - since the OpenGL font engine use Freetype library, we need to manually
	parse B_BEOS_FONTS_DIRECTORY for list of supported font families and styles.
	Add custom font file panel.
*/
#ifndef FONTPANEL_H_
#define FONTPANEL_H_

#include <View.h>
#include <Window.h>
#include <Button.h>
#include <ListView.h>
#include <ListItem.h>
#include <Font.h>
#include <Message.h>
#include <Messenger.h>

class FontWindow;

enum
{
	M_FONT_SELECTED='mfsl'
};

class FontPanel
{
public:
						FontPanel(BMessenger *target = NULL,
									BMessage *message = NULL,
									float size = 64.0f,
									bool modal = false,
									bool hide_when_done = true);
	virtual				~FontPanel(void);
			void		SelectFont(const BFont &font);
			void		SelectFont(font_family family, font_style style,
									float size = 128);
			void		Show();
			void		Hide();
			bool		IsShowing(void) const;
			void		SetTarget(BMessenger msgr);
			void		SetMessage(BMessage *msg);
			BWindow *	Window(void) const;
			void		SetHideWhenDone(bool value);
			bool		HideWhenDone(void) const;
			void		SetFontSize(uint16 size);
private:
	FontWindow *fWindow;
	
};

#endif
