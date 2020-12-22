/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	About window
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <translation/TranslationUtils.h>

#include "PersistantWindow.h"
#include "AboutWindow.h"

/*************************************
	AboutView
**************************************/
class AboutView : public BView
{
private:
	BBitmap		*fBitmap;
	BFont		fFont;

public:
	AboutView(BRect bounds)
	: BView(bounds, nullptr, B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS)
	{
		SetViewColor(216, 216, 216, 255);
		fBitmap = BTranslationUtils::GetBitmap("Resources/Icon/Medo_Logo.png");

		fFont.SetFace(B_ITALIC_FACE);
	}
	~AboutView()	override
	{
		delete fBitmap;
	}
	void Draw(BRect frame) override
	{
		//	Left column
		BRect bitmap_bounds = fBitmap->Bounds();
		BRect bitmap_frame(20, 20, bitmap_bounds.Width(), bitmap_bounds.Height());
		DrawBitmapAsync(fBitmap, bitmap_frame);

		float y_offset = Bounds().Height() - 20;
		SetFont(&fFont);
		SetHighColor({128, 128, 128, 255});
		DrawString("Medo in Slavic languages is a friendly Teddy Bear",		BPoint(20, y_offset));

		//	Right column
		float x_offset = 20 + bitmap_bounds.Width() + 40;
		y_offset = 30;

		SetFont(be_bold_font);
		SetHighColor({0, 0, 0, 255});
		DrawString("Haiku Media Editor", BPoint(x_offset, y_offset));							y_offset += 30;

		SetFont(be_plain_font);
		SetHighColor({128, 128, 128, 255});
		DrawString("Copyright " B_UTF8_COPYRIGHT " Zen Yes Pty Ltd, 2019-2021", BPoint(x_offset, y_offset));	y_offset += 25;
		DrawString("Melbourne, Australia", BPoint(x_offset, y_offset));							y_offset += 25;
		DrawString("Released under Open Source MIT license", BPoint(x_offset, y_offset));		y_offset += 25;

		y_offset += 25;
		SetFont(be_bold_font);
		SetHighColor({0, 0, 128, 255});
		DrawString("Written by Zenja Solaja", BPoint(x_offset, y_offset));					y_offset += 30;

		y_offset += 25;
		SetHighColor({128, 0, 0, 255});
		DrawString("Contributors:", BPoint(x_offset, y_offset));								y_offset += 30;

		SetFont(be_plain_font);
		SetHighColor({64, 64, 64, 255});
		DrawString("Logo by Dave Lewis", BPoint(x_offset, y_offset));							y_offset += 25;
	}
};

/*************************************
	About Window
**************************************/

/*	FUNCTION:		AboutWindow :: AboutWindow
	ARGS:			frame
					title
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
AboutWindow :: AboutWindow(BRect frame, const char *title)
	: PersistantWindow(frame, title, B_TITLED_WINDOW, B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fAboutView = new AboutView(Bounds());
	AddChild(fAboutView);
}

/*	FUNCTION:		AboutWindow :: ~AboutWindow
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
AboutWindow :: ~AboutWindow()
{
}

