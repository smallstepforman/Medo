/*
	FontPanel.cpp: A general-purpose font selection class
	Written by DarkWyrm <darkwyrm@earthlink.net>, Copyright 2007
	Released under the MIT license.

	ZenYes 2019 - since the OpenGL font engine uses Freetype library, we need to manually
	parse B_BEOS_FONTS_DIRECTORY for list of supported font families and styles.
	Add custom font file panel.
*/

#include <cassert>
#include <vector>
#include <algorithm>

#include <Application.h>
#include "FontPanel.h"
#include "Spinner.h"
#include <Invoker.h>
#include <String.h>
#include <stdio.h>
#include <ScrollView.h>
#include <ScrollBar.h>

#include "Editor/Language.h"

#include <storage/Path.h>
#include <storage/Directory.h>
#include <storage/Entry.h>
#include <storage/FindDirectory.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// TODO: Add Escape key as a shortcut for cancelling

enum
{
	M_OK = 'm_ok',
	M_CANCEL,
	M_SIZE_CHANGED,
	M_FAMILY_SELECTED,
	M_STYLE_SELECTED,
	M_HIDE_WINDOW
};

/***************************************************
	FontPreview
****************************************************/
class FontPreview : public BView
{
public:
					FontPreview(const BRect &frame);
			
	void			SetPreviewText(const char *text);
	const char *	PreviewText(void) const;
			
	void			Draw(BRect r);
	
private:
	BString	fPreviewText;
};


FontPreview::FontPreview(const BRect &frame)
 :	BView(frame,"fontpreview",B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW),
 	fPreviewText("AaBbCcDdEeFfGg")
{
}


void
FontPreview::SetPreviewText(const char *text)
{
	fPreviewText = text ? text : "Medo";
}


const char *
FontPreview::PreviewText(void) const
{
	return fPreviewText.String();
} 


void
FontPreview::Draw(BRect r)
{
	BFont font;
	GetFont(&font);
	
	int32 width = (int32)font.StringWidth(fPreviewText.String());
	
	BPoint drawpt;
	if (width < Bounds().IntegerWidth())
		drawpt.x = (Bounds().IntegerWidth() - width) / 2;
	else
		drawpt.x = 10;
	
	font_height fheight;
	font.GetHeight(&fheight);
	int32 size = (int32)(fheight.ascent + fheight.descent + fheight.leading);
	
	if (size < Bounds().IntegerHeight() - 10)
		drawpt.y = (Bounds().IntegerHeight() + fheight.ascent) / 2;
	else
		drawpt.y = Bounds().bottom - 10;
	DrawString(fPreviewText.String(), drawpt);
}

/***************************************************
	FontView
****************************************************/
class FontView : public BView
{
public:
			FontView(const BRect &frame, float size);
			~FontView(void);
	
	void	AttachedToWindow(void);
	void	MessageReceived(BMessage *msg);
	void	SetHideWhenDone(bool value);
	bool	HideWhenDone(void) const;
	void	SetTarget(BMessenger msgr);
	void	SetMessage(BMessage *msg);
	void	SetFontSize(uint16 size);
	void	SelectFont(const BFont &font);
	void	SelectFont(font_family family, font_style style, float size);
	
private:
	BMessenger	*fMessenger;
	BMessage	fMessage;
	bool		fHideWhenDone;
	FontPreview	*fPreview;
	BListView	*fFamilyList,
				*fStyleList;
	BButton		*fOK,
				*fCancel;
	Spinner		*fSpinner;
	BScrollView	*fFamilyScroller,
				*fStyleScroller;

	struct FONT_DATA
	{
		BString			*path;
		BString			*family;
		BString			*style;
	};
	std::vector<FONT_DATA>	fFontData;
	void	ParseFonts();
	void	ParseFontDirectory(BDirectory &dir);
	void	CreateStyleList(int32 family_idx);
};


FontView :: FontView(const BRect &frame, float size)
 :	BView(frame, "fontview", B_FOLLOW_ALL, B_WILL_DRAW),
 	fMessenger(new BMessenger(be_app_messenger)),
 	fMessage(M_FONT_SELECTED),
 	fHideWhenDone(true)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect r(Bounds().InsetByCopy(10,10));
	r.bottom = 200;
	
	fPreview = new FontPreview(r);
	
	r.top = r.bottom + 10;
	r.bottom = Bounds().bottom - 10;
	r.left = 10;
	r.right = r.left + 100;
	BStringView *flabel = new BStringView(r,"familylabel", GetText(TXT_FONT_FAMILY));
	flabel->ResizeToPreferred();
	
	r.Set(0,0,100,50);
	fOK = new BButton(r,"OK",GetText(TXT_CANCEL),new BMessage(M_OK),B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);		//	Original code assumes GetWidth("Cancel") longer than "OK"
	fOK->ResizeToPreferred();
	r = fOK->Frame();
	fOK->MoveTo(Bounds().right - r.Width() - 10,
				Bounds().bottom - r.Height() - 10);
	r = fOK->Frame();
	fOK->SetLabel(GetText(TXT_OK));
	
	r.OffsetBy(-r.Width() - 10,0);
	fCancel = new BButton(r,"Cancel",GetText(TXT_CANCEL),new BMessage(M_CANCEL),B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	
	r = Bounds();
	r.left = 10;
	r.top = flabel->Frame().bottom + 1;
	r.right = (r.right / 2.0) - 5.0 - B_V_SCROLL_BAR_WIDTH;
	r.bottom = fOK->Frame().top - 10;
	fFamilyList = new BListView(r,"familylist", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	fFamilyList->SetSelectionMessage(new BMessage(M_FAMILY_SELECTED));
	
	fFamilyScroller = new BScrollView("familyscroller",fFamilyList,
									B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0,false,true);
	fFamilyScroller->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	r.left = r.right + 10 + B_V_SCROLL_BAR_WIDTH;
	r.right = Bounds().right - 10 - B_V_SCROLL_BAR_WIDTH;
	fStyleList = new BListView(r,"stylelist", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	fStyleList->SetSelectionMessage(new BMessage(M_STYLE_SELECTED));
	
	fStyleScroller = new BScrollView("stylescroller",fStyleList,
									B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0,false,true);
	fStyleScroller->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BStringView *slabel = new BStringView(BRect(0,0,10,10),"stylelabel", GetText(TXT_FONT_STYLE));
	slabel->ResizeToPreferred();
	slabel->MoveTo(fStyleScroller->Frame().left,flabel->Frame().top);
	
	r = fOK->Frame();
	r.OffsetTo(10, r.top);
	r.right += 100;
	fSpinner = new Spinner(r,"fontsize", GetText(TXT_FONT_SIZE), new BMessage(M_SIZE_CHANGED),
							B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fSpinner->SetValue(size);
	fSpinner->SetRange(6, 1000);
	
	// We save all the child adding until the end because we constructed the controls
	// in a different order than keyboard navigation for layout (and laziness) reasons
	AddChild(fPreview);
	AddChild(flabel);
	AddChild(slabel);
	AddChild(fFamilyScroller);
	AddChild(fStyleScroller);
	AddChild(fSpinner);
	AddChild(fCancel);
	AddChild(fOK);
	
	ParseFonts();
}


FontView :: ~FontView(void)
{
	delete fMessenger;

	for (auto i : fFontData)
	{
		delete i.path;
		delete i.family;
		delete i.style;
	}
}


void FontView :: AttachedToWindow(void)
{
	Window()->SetDefaultButton(fOK);
	fOK->SetTarget(this);
	fCancel->SetTarget(this);
	fFamilyList->SetTarget(this);
	fStyleList->SetTarget(this);
	fSpinner->SetTarget(this);
	
	fFamilyList->Select(0);
	fFamilyList->MakeFocus(true);
}


void FontView :: SetHideWhenDone(bool value)
{
	fHideWhenDone = value;
}


bool FontView :: HideWhenDone(void) const
{
	return fHideWhenDone;
}


void FontView :: SetTarget(BMessenger msgr)
{
	delete fMessenger;
	fMessenger = new BMessenger(msgr);
}


void FontView :: SetMessage(BMessage *msg)
{
	fMessage = (msg) ? *msg : BMessage(M_FONT_SELECTED);
}


void FontView :: SetFontSize(uint16 size)
{
	fSpinner->SetValue(size);
	fPreview->SetFontSize(size);
	fPreview->Invalidate();
	if (Window())
		Window()->UpdateIfNeeded();
}


void FontView :: SelectFont(font_family family, font_style style, float size)
{
	int32 familyIndex = -1;
	
	for (int32 i = 0; i < fFamilyList->CountItems(); i++) {
		BStringItem *item = (BStringItem*) fFamilyList->ItemAt(i);
		if (item && strcmp(item->Text(), (const char *)family) == 0) {
			familyIndex = i;
			break;
		}
	}
	
	if (familyIndex < 0)
		return;
	
	// We temporarily set the family list's target to the window because calling
	// Select invokes a message and we want to skip it this time.
	fFamilyList->SetTarget(Window());
	fFamilyList->Select(familyIndex);
	fFamilyList->SetTarget(this);
	fFamilyList->ScrollToSelection();

	CreateStyleList(familyIndex);
	SetFontSize(size);

	for (int32 i = 0; i < fStyleList->CountItems(); i++)
	{
		BStringItem *item = (BStringItem*) fStyleList->ItemAt(i);
		if (item && strcmp(item->Text(), (const char *)style) == 0)
		{
			printf("FontView::SelectFont(%d) %s, %s\n", i, family, item->Text());
			fStyleList->Select(i);
			break;
		}
	}
}


void FontView :: SelectFont(const BFont &font)
{
	font_family fam;
	font_style sty;
	font.GetFamilyAndStyle(&fam,&sty);
	
	SelectFont(fam,sty,font.Size());
}


void
FontView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case M_SIZE_CHANGED:
		{
			SetFontSize(fSpinner->Value());
			break;
		}
		case M_CANCEL:
		{
			BMessage *cancel = new BMessage(B_CANCEL);
			cancel->AddPointer("source",this);
			fMessenger->SendMessage(cancel);
			
			if (fHideWhenDone)
				Window()->Hide();
			
			break;
		}
		case M_FAMILY_SELECTED:
		{
			int32 familyIndex = fFamilyList->CurrentSelection();
			if (familyIndex < 0)
				break;
			
			BStringItem *item = (BStringItem*)fFamilyList->ItemAt(familyIndex);
			if (!item)
				break;
			
			CreateStyleList(familyIndex);
			break;
		}
		case M_STYLE_SELECTED:
		{
			if (fFamilyList->CurrentSelection() < 0 ||
				fStyleList->CurrentSelection() < 0)
				break;
			
			BStringItem *fitem = (BStringItem *)fFamilyList->ItemAt(fFamilyList->CurrentSelection());
			BStringItem *sitem = (BStringItem *)fStyleList->ItemAt(fStyleList->CurrentSelection());
			if (!fitem || !sitem)
				break;
			BFont font;
			fPreview->GetFont(&font);
			font.SetFamilyAndStyle(fitem->Text(), sitem->Text());
			fPreview->SetFont(&font);
			fPreview->Invalidate();
			break;
		}
		case M_OK:
		{
			BMessage *ok = new BMessage;
			*ok = fMessage;
			
			if (fFamilyList->CurrentSelection() < 0 ||
				fStyleList->CurrentSelection() < 0)
				break;
			
			BStringItem *fitem = (BStringItem *)fFamilyList->ItemAt(fFamilyList->CurrentSelection());
			BStringItem *sitem = (BStringItem *)fStyleList->ItemAt(fStyleList->CurrentSelection());
			
			ok->AddString("family",fitem->Text());
			ok->AddString("style",sitem->Text());
			ok->AddFloat("size",fSpinner->Value());
			for (auto i : fFontData)
			{
				if ((i.family->Compare(fitem->Text()) == 0) && (i.style->Compare(sitem->Text()) == 0))
					ok->AddString("path", i.path->String());
			}
			
			fMessenger->SendMessage(ok);
			
			if (fHideWhenDone)
				Window()->Hide();
			break;
		}
		default: {
			BView::MessageReceived(msg);
			break;
		}
	}
}

void FontView :: CreateStyleList(int32 family_idx)
{
	if ((family_idx < 0) || (family_idx >= fFamilyList->CountItems()))
		assert(0);

	BStringItem *item = (BStringItem*)fFamilyList->ItemAt(family_idx);
	assert(item);

	if (Window())
		Window()->Lock();

	while (fStyleList->CountItems() > 0)
	{
		BStringItem *item = (BStringItem*)fStyleList->RemoveItem(int32(0));
		delete item;
	}

	BString family(item->Text());
	int32 styleCount = 0;
	for (std::vector<FONT_DATA>::iterator i = fFontData.begin(); i != fFontData.end(); i++)
	{
		if (family.Compare(*(*i).family) == 0)
		{
			fStyleList->AddItem(new BStringItem((*i).style->String()));
			styleCount++;
		}
	}

	if (styleCount && fStyleList->CurrentSelection() < 0)
		fStyleList->Select(0);

	if (Window())
		Window()->Unlock();
}

/***************************************************
	Parse Fonts
****************************************************/
static FT_Library sFtLibrary;

/*	FUNCTION:		FontView :: ParseFontDirectory
	ARGS:			dir
	RETURN:			n/a
	DESCRIPTION:	Recursively parse font directory
*/
void FontView :: ParseFontDirectory(BDirectory &dir)
{
	BEntry entry;
	while (dir.GetNextEntry(&entry, false) == B_OK)
	{
		if (entry.IsDirectory())
		{
			BDirectory sub(&entry);
			ParseFontDirectory(sub);
		}
		else
		{
			BPath a;
			if (entry.GetPath(&a) == B_OK)
			{
				FT_Face face;
				int face_idx = 0;
				while (FT_New_Face(sFtLibrary, a.Path(), face_idx, &face) == 0)
				{
					FONT_DATA data;
					data.path = new BString(a.Path());
					data.family = new BString(face->family_name);
					data.style = new BString(face->style_name);
					fFontData.push_back(data);
					//printf("Family: %s, Style=%s\n", face->family_name, face->style_name);
					FT_Done_Face(face);
					face_idx++;
				}
			}
		}
	}
}


/*	FUNCTION:		FontView :: ParseFonts
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Get installed fonts
*/
void FontView :: ParseFonts()
{
	bigtime_t ts = system_time();
	if (FT_Init_FreeType(&sFtLibrary))
	{
		printf("FontView::ParseFonts() Error:  FT_Init_FreeType()\n");
		return;
	}

	BPath fonts_path;
	if (find_directory(B_BEOS_FONTS_DIRECTORY, &fonts_path) == B_OK)
	{
		BDirectory fonts_dir(fonts_path.Path());
		ParseFontDirectory(fonts_dir);
	}
	FT_Done_FreeType(sFtLibrary);
	std::sort(fFontData.begin(), fFontData.end(), [](const FONT_DATA &a, const FONT_DATA &b) {return (a.family->Compare(*b.family) < 0);});
	printf("FontView::ParseFonts() %g seconds\n", double(system_time() - ts)/1000000.0);

	BString *previous_family = nullptr;
	size_t previous_len = 0;
	for (std::vector<FONT_DATA>::iterator i = fFontData.begin(); i != fFontData.end(); i++)
	{
		bool match = false;
		if (previous_family && previous_family->Compare(*(*i).family) == 0)
				match = true;

		if (!match)
		{
			previous_family = (*i).family;
			previous_len = (*i).family->Length();
			fFamilyList->AddItem(new BStringItem(previous_family->String()));
		}
	}
}

/***************************************************
	FontWindow
****************************************************/
class FontWindow : public BWindow
{
public:
			FontWindow(const BRect &frame, float fontsize);
			~FontWindow(void);
	bool	QuitRequested(void);
	void	MessageReceived(BMessage *msg);
	
	void	ReallyQuit(void) { fReallyQuit = true; }
	
			FontView *fView;

private:
	bool	fReallyQuit;
};


FontWindow::FontWindow(const BRect &frame, float fontsize)
 : BWindow(frame, GetText(TXT_FONT_TITLE), B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,0)
{
	RemoveShortcut('w',B_COMMAND_KEY);
	AddShortcut('w',B_COMMAND_KEY,new BMessage(M_HIDE_WINDOW));
	SetSizeLimits(400,2400,300,2400);
	fReallyQuit = false;
	
	fView = new FontView(Bounds(), fontsize);
	AddChild(fView);
}


FontWindow::~FontWindow(void)
{
}


bool
FontWindow::QuitRequested(void)
{
	if (!fReallyQuit)
		PostMessage(M_HIDE_WINDOW);
	
	return fReallyQuit;
}

 
void
FontWindow::MessageReceived(BMessage *msg)
{
	if (msg->what == M_HIDE_WINDOW)
		Hide();
	else
		BWindow::MessageReceived(msg);
}

/***************************************************
	FontPanel
****************************************************/
FontPanel::FontPanel(BMessenger *target,BMessage *msg, float size, bool modal,
					bool hide_when_done)
{
	fWindow = new FontWindow(BRect(200,200,200+640,200+640),size);
	
	if (target)
		fWindow->fView->SetTarget(*target);
	
	if (msg)
		fWindow->fView->SetMessage(msg);
	
	if (modal)
		fWindow->SetFeel(B_MODAL_APP_WINDOW_FEEL);
	
	
	//fWindow->fView->SetFontSize(size);
	fWindow->fView->SetHideWhenDone(hide_when_done);

	font_family plain_family;
	font_style plain_style;
	be_plain_font->GetFamilyAndStyle(&plain_family, &plain_style);
	SelectFont(plain_family, plain_style);
}


FontPanel::~FontPanel(void)
{
	fWindow->ReallyQuit();
	fWindow->PostMessage(B_QUIT_REQUESTED);
}


void
FontPanel::SelectFont(const BFont &font)
{
	fWindow->fView->SelectFont(font);
}


void
FontPanel::SelectFont(font_family family, font_style style, float size)
{
	fWindow->fView->SelectFont(family,style,size);
}


void
FontPanel::Show()
{
	fWindow->Show();
}


void
FontPanel::Hide()
{
	fWindow->Hide();
}


bool
FontPanel::IsShowing(void) const
{
	return !fWindow->IsHidden();
}


BWindow *
FontPanel::Window(void) const
{
	return fWindow;
}


void
FontPanel::SetTarget(BMessenger msgr)
{
	fWindow->fView->SetTarget(msgr);
}


void
FontPanel::SetMessage(BMessage *msg)
{
	fWindow->fView->SetMessage(msg);
}


void
FontPanel::SetHideWhenDone(bool value)
{
	fWindow->fView->SetHideWhenDone(value);
}


bool
FontPanel::HideWhenDone(void) const
{
	return fWindow->fView->HideWhenDone();
}


void
FontPanel::SetFontSize(uint16 size)
{
	fWindow->fView->SetFontSize(size);
}

