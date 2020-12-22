//******************************************************************************

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "Actor/Actor.h"
#include "Actor/ActorManager.h"

#include <Alert.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextView.h>
#include <PopUpMenu.h>
#include <Clipboard.h>

#include "Magnify.h"

namespace Magnify
{

static const rgb_color kGridGray = {130, 130, 130, 255 };
static const rgb_color kBlack = { 0, 0, 0, 255};
static const rgb_color kRedColor = { 255, 10, 50, 255 };
static const rgb_color kBlueColor = { 10, 50, 255, 255 };

//******************************************************************************

static float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = ceil(finfo.ascent) + ceil(finfo.descent);

	if (full)
		h += ceil(finfo.leading);
	
	return h;
}

static color_map*
ColorMap()
{
	color_map* cmap;
	
	BScreen screen(B_MAIN_SCREEN_ID);
	cmap = (color_map*)screen.ColorMap();
	
	return cmap;
}

static void
CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}

//******************************************************************************

//	each info region will be:
//	top-bottom: 5 fontheight 5 fontheight 5
//	left-right: 10 minwindowwidth 10

TWindow::TWindow(BHandler *handler, BMessage *msg)
	: BWindow( BRect(0,0,0,0), "White Balance", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	fParentHandler = handler;
	fParentMessage = msg;
	fReallyQuit = false;

	// 	if prefs dont yet exist or the window is not onscreen, center the window
	CenterWindowOnScreen(this);

	//	set all the settings to defaults if we get here
	fShowGrid = true;
	fShowInfo = true;
	fHPixelCount = 32;
	fVPixelCount = 32;
	fPixelSize = 8;

	//	add info view
	BRect infoRect(Bounds());
	infoRect.InsetBy(-1, -1);
	fInfo = new TInfoView(infoRect);
	AddChild(fInfo);
	
	fFontHeight = FontHeight(fInfo, true);
	fInfoHeight = (fFontHeight * 2) + (3 * 5);

	BRect fbRect(0, 0, (fHPixelCount*fPixelSize), (fHPixelCount*fPixelSize));
	if (InfoIsShowing())
		fbRect.OffsetBy(10, fInfoHeight);
	fFatBits = new TMagnify(fbRect, this);
	fInfo->AddChild(fFatBits);	

	fFatBits->SetSelection(fShowInfo);
	fInfo->SetMagView(fFatBits);

	ResizeWindow(fHPixelCount, fVPixelCount);
}

TWindow::~TWindow()
{
	delete fParentMessage;
}

/*	FUNCTION:		TWindow :: ColourSelected
	ARGUMENTS:		colour
	RETURN:			n/a
	DESCRIPTION:	Invoked by TMagnify when MouseDown() called
*/
void TWindow :: ColourSelected(rgb_color colour)
{
	printf("Magnify::TWindow::ColourSelected() (%d,%d,%d)\n", colour.red, colour.green, colour.blue);
	//if (fProcessColourSelected)
	{
		fParentMessage->ReplaceColor("colour", colour);
		fParentMessage->ReplaceBool("active", true);
		fParentHandler->Looper()->PostMessage(fParentMessage, fParentHandler);
	}
}

/*	FUNCTION:		TWindow :: WindowActivated
	ARGUMENTS:		activated
	RETURN:			n/a
	DESCRIPTION:	Hook function.  Trap all mouse events when activated
*/
void TWindow :: WindowActivated(bool activated)
{
	if (activated)
	{
		fFatBits->SetEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		fProcessColourSelected = true;
	}
	else
	{
		fProcessColourSelected = false;
		fFatBits->SetEventMask(0, 0);
		fParentMessage->ReplaceBool("active", false);
		fParentHandler->Looper()->PostMessage(fParentMessage, fParentHandler);
	}
}

/*	FUNCTION:		TWindow :: QuitRequested
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Hook function.  Hide instead of quit
*/
bool TWindow::QuitRequested()
{
	if (IsActive())
	{
		LockLooper();
		WindowActivated(false);
		UnlockLooper();

	}
	return fReallyQuit;
}

/*	FUNCTION:		TWindow :: Terminate
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Called by Medo, really quit
*/
void TWindow :: Terminate()
{
	fReallyQuit = true;
	PostMessage(B_QUIT_REQUESTED);
}

void
TWindow::FrameResized(float w, float h)
{
	CalcViewablePixels();
	
	float width;
	float height;
	GetPreferredSize(&width, &height);		
	ResizeTo(width, height);
	
	fFatBits->InitBuffers(fHPixelCount, fVPixelCount, fPixelSize, ShowGrid());
}

void
TWindow::ScreenChanged(BRect screen_size, color_space depth)
{
	BWindow::ScreenChanged(screen_size, depth);
	//	reset all bitmaps
	fFatBits->ScreenChanged(screen_size,depth);
}

void
TWindow::Minimize(bool m)
{
	BWindow::Minimize(m);
}

void
TWindow::Zoom(BPoint _UNUSED(rec_position), float _UNUSED(rec_width), float _UNUSED(rec_height))
{
	if (fFatBits->Active())
		ShowInfo(!fShowInfo);
}

void
TWindow::CalcViewablePixels()
{

	float w = Bounds().Width();
	float h = Bounds().Height();
	
	if (InfoIsShowing()) {
		w -= 20;							// remove the gutter
		h = h-fInfoHeight-10;				// remove info and gutter
	}

	bool ch1, ch2;
	fFatBits->CrossHairsShowing(&ch1, &ch2);	
	if (ch1)
		h -= fFontHeight;
	if (ch2)
		h -= fFontHeight + 5;

	fHPixelCount = (int32)w / fPixelSize;			// calc h pixels
	if (fHPixelCount < 16)
		fHPixelCount = 16;

	fVPixelCount = (int32)h / fPixelSize;			// calc v pixels
	if (fVPixelCount < 4)
		fVPixelCount = 4;
}

void
TWindow::GetPreferredSize(float* width, float* height)
{
	*width = fHPixelCount * fPixelSize;			// calc window width
	*height = fVPixelCount * fPixelSize;		// calc window height
	if (InfoIsShowing()) {
		*width += 20;			
		*height += fInfoHeight + 10;
	}
		
	bool ch1, ch2;
	fFatBits->CrossHairsShowing(&ch1, &ch2);	
	if (ch1)
		*height += fFontHeight;
	if (ch2)
		*height += fFontHeight + 5;		
}

void
TWindow::ResizeWindow(int32 hPixelCount, int32 vPixelCount)
{
	fHPixelCount = hPixelCount;
	fVPixelCount = vPixelCount;
			
	float width, height;
	GetPreferredSize(&width, &height);
	
	ResizeTo(width, height);
}

void
TWindow::ResizeWindow(bool direction)
{
	int32 x = fHPixelCount;
	int32 y = fVPixelCount;
	
	if (direction) {
		x += 4;
		y += 4;
	} else {
		x -= 4;
		y -= 4;
	}

	if (x < 4)
		x = 4;

	if (y < 4)
		y = 4;

	ResizeWindow(x, y);	
}

bool
TWindow::ShowGrid()
{
	return fShowGrid;
}

void
TWindow::ShowInfo(bool i)
{
	if (i == fShowInfo)
		return;
		
	fShowInfo = i;

	if (fShowInfo)
		fFatBits->MoveTo(10, fInfoHeight);
	else {
		fFatBits->MoveTo(1,1);
		fFatBits->SetCrossHairsShowing(false, false);
	}

	fFatBits->SetSelection(fShowInfo);
	ResizeWindow(fHPixelCount, fVPixelCount);
}

bool
TWindow::InfoIsShowing()
{
	return fShowInfo;
}

void
TWindow::UpdateInfo()
{
	fInfo->Draw(fInfo->Bounds());
}

void
TWindow::CrossHairsShowing(bool* ch1, bool* ch2)
{
	fFatBits->CrossHairsShowing(ch1, ch2);
}

void
TWindow::PixelCount(int32* h, int32 *v)
{
	*h = fHPixelCount;
	*v = fVPixelCount;
}

int32
TWindow::PixelSize()
{
	return fPixelSize;
}

bool
TWindow::IsActive()
{
	return fFatBits->Active();
}

//******************************************************************************

TInfoView::TInfoView(BRect frame)
	: BBox(frame, "rgb", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS,
		B_NO_BORDER)
{
	SetFont(be_plain_font);
	fFontHeight = FontHeight(this, true);	
	fMagView = NULL;
	
	fSelectionColor = kBlack;
	fCH1Loc.x = fCH1Loc.y = fCH2Loc.x = fCH2Loc.y = 0;
	
	fInfoStr[0] = 0;
	fRGBStr[0] = 0;
	fCH1Str[0] = 0;
	fCH2Str[0] = 0;
}

TInfoView::~TInfoView()
{
}

void
TInfoView::AttachedToWindow()
{
	BBox::AttachedToWindow();
	
	dynamic_cast<TWindow*>(Window())->PixelCount(&fHPixelCount, &fVPixelCount);
	fPixelSize = dynamic_cast<TWindow*>(Window())->PixelSize();	
}

void 
TInfoView::Draw(BRect updateRect)
{
	PushState();
	
	char str[64];

	SetLowColor(ViewColor());
	
	BRect invalRect;
	
	int32 hPixelCount, vPixelCount;
	dynamic_cast<TWindow*>(Window())->PixelCount(&hPixelCount, &vPixelCount);
	int32 pixelSize = dynamic_cast<TWindow*>(Window())->PixelSize();
	
	MovePenTo(10, fFontHeight+5);
	sprintf(str, "%li x %li  @ %li pixels/pixel", hPixelCount, vPixelCount,
		pixelSize);
	invalRect.Set(10, 5, 10 + StringWidth(fInfoStr), fFontHeight+7);
	SetHighColor(ViewColor());
	FillRect(invalRect);
	SetHighColor(0,0,0,255);
	strcpy(fInfoStr,str);
	DrawString(fInfoStr);
	
	rgb_color c = { 0,0,0, 255 };
	if (fMagView) {
		c = fMagView->SelectionColor();
	}
	MovePenTo(10, fFontHeight*2+5);
	sprintf(str, "R: %i G: %i B: %i",
		c.red, c.green, c.blue);
	invalRect.Set(10, fFontHeight+7, 10 + StringWidth(fRGBStr), fFontHeight*2+7);
	SetHighColor(ViewColor());
	FillRect(invalRect);
	SetHighColor(0,0,0,255);
	strcpy(fRGBStr,str);
	DrawString(fRGBStr);
		
	bool ch1Showing, ch2Showing;
	dynamic_cast<TWindow*>(Window())->CrossHairsShowing(&ch1Showing, &ch2Showing);
	
	if (fMagView) {
		BPoint pt1(fMagView->CrossHair1Loc());
		BPoint pt2(fMagView->CrossHair2Loc());
		
		float h = Bounds().Height();
		if (ch2Showing) {
			MovePenTo(10, h-12);
			sprintf(str, "2) x: %li y: %li   y: %i", (int32)pt2.x, (int32)pt2.y,
				abs((int)(pt1.y - pt2.y)));
			invalRect.Set(10, h-12-fFontHeight, 10 + StringWidth(fCH2Str), h-10);
			SetHighColor(ViewColor());
			FillRect(invalRect);
			SetHighColor(0,0,0,255);
			strcpy(fCH2Str,str);
			DrawString(fCH2Str);
		}
	
		if (ch1Showing && ch2Showing) {
			MovePenTo(10, h-10-fFontHeight-2);
			sprintf(str, "1) x: %li  y: %li   x: %i", (int32)pt1.x, (int32)pt1.y,
				abs((int)(pt1.x - pt2.x)));
			invalRect.Set(10, h-10-2*fFontHeight-2, 10 + StringWidth(fCH1Str), h-10-fFontHeight);
			SetHighColor(ViewColor());
			FillRect(invalRect);
			SetHighColor(0,0,0,255);
			strcpy(fCH1Str,str);
			DrawString(fCH1Str);
		} else if (ch1Showing) {
			MovePenTo(10, h-10);
			sprintf(str, "x: %li  y: %li", (int32)pt1.x, (int32)pt1.y);
			invalRect.Set(10, h-10-fFontHeight, 10 + StringWidth(fCH1Str), h-8);
			SetHighColor(ViewColor());
			FillRect(invalRect);
			SetHighColor(0,0,0,255);
			strcpy(fCH1Str,str);
			DrawString(fCH1Str);
		}
	}
	
	PopState();
}

void
TInfoView::FrameResized(float width, float height)
{
	BBox::FrameResized(width, height);
}

void
TInfoView::SetMagView(TMagnify* magView)
{
	fMagView = magView;
}

//******************************************************************************

TMagnify::TMagnify(BRect r, TWindow* parent)
	:BView(r, "MagView", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
		fParent(parent)
{
	fLastLoc.Set(-1, -1);
	fSelectionLoc.x = 15; fSelectionLoc.y = 15;
	fActive = true;
	fShowSelection = false;
	fShowCrossHair1 = false;
	fCrossHair1.x = -1; fCrossHair1.y = -1;
	fShowCrossHair2 = false;
	fCrossHair2.x = -1; fCrossHair2.y = -1;
	fSelection = -1;

	fImageBuf = NULL;
	fImageView = NULL;
}

TMagnify::~TMagnify()
{
	kill_thread(fThread);
	delete(fImageBuf);
}

void
TMagnify::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	int32 width, height;
	fParent->PixelCount(&width, &height);
	InitBuffers(width, height, fParent->PixelSize(), fParent->ShowGrid());
	
	fThread = spawn_thread((status_t (*)(void *))TMagnify::MagnifyTask, "MagnifyTask",
		B_NORMAL_PRIORITY, this);

	resume_thread(fThread);

	MakeFocus();
}

void
TMagnify::InitBuffers(int32 hPixelCount, int32 vPixelCount,
	int32 pixelSize, bool showGrid)
{
	color_space colorSpace =  BScreen(Window()).ColorSpace();

	BRect r(0, 0, (pixelSize * hPixelCount)-1, (pixelSize * vPixelCount)-1);
	if (Bounds().Width() != r.Width() || Bounds().Height() != r.Height())
		ResizeTo(r.Width(), r.Height());
	
	if (fImageView) {
		fImageBuf->Lock();
		fImageView->RemoveSelf();
		fImageBuf->Unlock();
		
		fImageView->Resize((int32)r.Width(), (int32)r.Height());
		fImageView->SetSpace(colorSpace);
	} else
		fImageView = new TOSMagnify(r, this, colorSpace);

	delete(fImageBuf);
	fImageBuf = new BBitmap(r, colorSpace, true);
	fImageBuf->Lock();
	fImageBuf->AddChild(fImageView);
	fImageBuf->Unlock();
}

void
TMagnify::Draw(BRect)
{
	BRect bounds(Bounds());
	DrawBitmap(fImageBuf, bounds, bounds);
	dynamic_cast<TWindow*>(Window())->UpdateInfo();
}

void
TMagnify::KeyDown(const char *key, int32 numBytes)
{
	if (!fShowSelection)
		BView::KeyDown(key,numBytes);
		
	uint32 mods = modifiers();
	
	switch (key[0]) {
		case B_TAB:
			if (fShowCrossHair1) {
				fSelection++;
				
				if (fShowCrossHair2) {
					if (fSelection > 2)
						fSelection = 0;
				} else if (fShowCrossHair1) {
					if (fSelection > 1)
						fSelection = 0;
				}
				fNeedToUpdate = true;
				Draw(Bounds());
			}
			break;
			
		case B_LEFT_ARROW:
			if (mods & B_OPTION_KEY)
				NudgeMouse(-1,0);
			else
				MoveSelection(-1,0);
			break;
		case B_RIGHT_ARROW:
			if (mods & B_OPTION_KEY)
				NudgeMouse(1, 0);
			else
				MoveSelection(1,0);
			break;
		case B_UP_ARROW:
			if (mods & B_OPTION_KEY)
				NudgeMouse(0, -1);
			else
				MoveSelection(0,-1);
			break;
		case B_DOWN_ARROW:
			if (mods & B_OPTION_KEY)
				NudgeMouse(0, 1);
			else
				MoveSelection(0,1);
			break;

		case B_ESCAPE:
			fParent->PostMessage(B_CLOSE_REQUESTED);
			break;

		
		default:
			BView::KeyDown(key,numBytes);
			break;
	}
}

void
TMagnify::FrameResized(float newW, float newH)
{
	int32 w, h;
	PixelCount(&w, &h);
	
	if (fSelectionLoc.x >= w)
		fSelectionLoc.x = 0;
	if (fSelectionLoc.y >= h)
		fSelectionLoc.y = 0;
		
	if (fShowCrossHair1) {
	
		if (fCrossHair1.x >= w) {
			fCrossHair1.x = fSelectionLoc.x + 2;
			if (fCrossHair1.x >= w)
				fCrossHair1.x = 0;
		}
		if (fCrossHair1.y >= h) {
			fCrossHair1.y = fSelectionLoc.y + 2;
			if (fCrossHair1.y >= h)
				fCrossHair1.y = 0;
		}

		if (fShowCrossHair2) {
			if (fCrossHair2.x >= w) {
				fCrossHair2.x = fCrossHair1.x + 2;
				if (fCrossHair2.x >= w)
					fCrossHair2.x = 0;
			}
			if (fCrossHair2.y >= h) {
				fCrossHair2.y = fCrossHair1.y + 2;
				if (fCrossHair2.y >= h)
					fCrossHair2.y = 0;
			}
		}
	}
}

/*	FUNCTION:		TMagnify :: MouseDown
	ARGUMENTS:		where
	RETURN:			n/a
	DESCRIPTION:	Hook function when mouse down.
					Delay processing to allow trapping of Magnify close buttons.
*/
void TMagnify::MouseDown(BPoint where)
{
	yarra::ActorManager::GetInstance()->AddTimer(50, this, std::bind(&TMagnify::AsyncColourSelected, this, SelectionColor()));
}

/*	FUNCTION:		TMagnify :: AsyncColourSelected
	ARGUMENTS:		colour
	RETURN:			n/a
	DESCRIPTION:	Notify parent window of colour selection
*/
void TMagnify :: AsyncColourSelected(rgb_color colour)
{
	fParent->ColourSelected(colour);
}

void
TMagnify::ScreenChanged(BRect, color_space)
{
	int32 width, height;
	fParent->PixelCount(&width, &height);
	InitBuffers(width, height, fParent->PixelSize(), fParent->ShowGrid());
}

void
TMagnify::SetSelection(bool state)
{
	if (fShowSelection == state)
		return;
		
	fShowSelection = state;
	fSelection = 0;
	Draw(Bounds());
}

static void
BoundsSelection(int32 incX, int32 incY, float* x, float* y,
	int32 xCount, int32 yCount)
{
	*x += incX;
	*y += incY;
	
	if (*x < 0)
		*x = xCount-1;
	if (*x >= xCount)
		*x = 0;
		
	if (*y < 0)
		*y = yCount-1;
	if (*y >= yCount)
		*y = 0;
}

void
TMagnify::MoveSelection(int32 x, int32 y)
{
	if (!fShowSelection)
		return;
		
	int32 xCount, yCount;
	PixelCount(&xCount, &yCount);

	float xloc, yloc;
	if (fSelection == 0) {
		xloc = fSelectionLoc.x;
		yloc = fSelectionLoc.y;
		BoundsSelection(x, y, &xloc, &yloc, xCount, yCount);
		fSelectionLoc.x = xloc;
		fSelectionLoc.y = yloc;
	} else if (fSelection == 1) {
		xloc = fCrossHair1.x;
		yloc = fCrossHair1.y;
		BoundsSelection(x, y, &xloc, &yloc, xCount, yCount);
		fCrossHair1.x = xloc;
		fCrossHair1.y = yloc;
	} else if (fSelection == 2) {
		xloc = fCrossHair2.x;
		yloc = fCrossHair2.y;
		BoundsSelection(x, y, &xloc, &yloc, xCount, yCount);
		fCrossHair2.x = xloc;
		fCrossHair2.y = yloc;
	}

	fNeedToUpdate = true;
	Draw(Bounds());
}

void
TMagnify::ShowSelection()
{
}

short
TMagnify::Selection()
{
	return fSelection;
}

bool
TMagnify::SelectionIsShowing()
{
	return fShowSelection;
}

void
TMagnify::SelectionLoc(float* x, float* y)
{
	*x = fSelectionLoc.x;
	*y = fSelectionLoc.y;
}

void
TMagnify::SetSelectionLoc(float x, float y)
{
	fSelectionLoc.x = x;
	fSelectionLoc.y = y;
}

rgb_color
TMagnify::SelectionColor()
{
	return fImageView->ColorAtSelection();
}

void
TMagnify::CrossHair1Loc(float* x, float* y)
{
	*x = fCrossHair1.x;
	*y = fCrossHair1.y;
}

void
TMagnify::CrossHair2Loc(float* x, float* y)
{
	*x = fCrossHair2.x;
	*y = fCrossHair2.y;
}

BPoint
TMagnify::CrossHair1Loc()
{
	return fCrossHair1;
}

BPoint
TMagnify::CrossHair2Loc()
{
	return fCrossHair2;
}

#include <WindowScreen.h>
void
TMagnify::NudgeMouse(float x, float y)
{
	BPoint		loc;
	uint32		button;

	GetMouse(&loc, &button);
	ConvertToScreen(&loc);
	loc.x += x;
	loc.y += y;

//	set_mouse_position((int32)loc.x, (int32)loc.y);
}

void
TMagnify::WindowActivated(bool active)
{
	if (active)
		MakeFocus();
}

long
TMagnify::MagnifyTask(void *arg)
{
	TMagnify*	view = (TMagnify*)arg;

	// static data members can't access members, methods without
	// a pointer to an instance of the class
	TWindow* window = (TWindow*)view->Window();

	while (true) {
		if (window->Lock()) {

//			if (!window->Minimized() && view->Active() || view->NeedToUpdate())
			if (view->NeedToUpdate() || view->Active())
				view->Update(view->NeedToUpdate());

			window->Unlock();
		}
		snooze(35000);
	}

	return B_NO_ERROR;
}

void
TMagnify::Update(bool force)
{
	BPoint		loc;
	uint32		button;
	static long counter = 0;

	GetMouse(&loc, &button);

	ConvertToScreen(&loc);
	if (force || (fLastLoc != loc) || (counter++ % 35 == 0)) {

		if (fImageView->CreateImage(loc, force))
			Draw(Bounds());

		counter = 0;
		if (force)
			SetUpdate(false);

	}
	fLastLoc = loc;
}

bool
TMagnify::NeedToUpdate()
{
	return fNeedToUpdate;
}

void
TMagnify::SetUpdate(bool s)
{
	fNeedToUpdate = s;
}

void
TMagnify::SetCrossHairsShowing(bool ch1, bool ch2)
{
	fShowCrossHair1 = ch1;
	fShowCrossHair2 = ch2;
}

void
TMagnify::CrossHairsShowing(bool* ch1, bool* ch2)
{
	*ch1 = fShowCrossHair1;
	*ch2 = fShowCrossHair2;
}

void
TMagnify::MakeActive(bool s)
{
	fActive = s;
}

void
TMagnify::PixelCount(int32* width, int32* height)
{
	fParent->PixelCount(width, height);
}

int32
TMagnify::PixelSize()
{
	return fParent->PixelSize();
}

bool
TMagnify::ShowGrid()
{
	return fParent->ShowGrid();
}

//******************************************************************************

TOSMagnify::TOSMagnify(BRect r, TMagnify* parent, color_space space)
	:BView(r, "ImageView", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
		fColorSpace(space), fParent(parent)
{
	fColorMap = ColorMap();

	switch (space) {
		case B_COLOR_8_BIT:
			fBytesPerPixel = 1;
			break;
		case B_RGB15:
 		case B_RGBA15:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB16:
        case B_RGB16_BIG:
			fBytesPerPixel = 2;
			break;
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			fBytesPerPixel = 4;
			break;
		default:
			// uh, oh -- a color space we don't support
			fprintf(stderr, "Tried to run in an unsupported color space; exiting\n");
			exit(1);
			break;
	}

	fPixel = NULL;
	fBitmap = NULL;
	fOldBits = NULL;
	InitObject();
}

TOSMagnify::~TOSMagnify()
{
	delete fPixel;
	delete(fBitmap);
	free(fOldBits);
}

void TOSMagnify::SetSpace(color_space space)
{
	fColorSpace = space;
	InitObject();
};

void TOSMagnify::InitObject()
{
	int32 w, h;
	fParent->PixelCount(&w, &h);

	if (fBitmap) delete fBitmap;
	BRect bitsRect(0, 0, w-1, h-1);
	fBitmap = new BBitmap(bitsRect, fColorSpace);
	
	if (fOldBits) free(fOldBits);
	fOldBits = (char*)malloc(fBitmap->BitsLength());

	if (!fPixel) {
		#if B_HOST_IS_BENDIAN
		#define native B_RGBA32_BIG
		#else
		#define native B_RGBA32_LITTLE
		#endif
		fPixel = new BBitmap(BRect(0,0,0,0), native, true);
		#undef native
		fPixelView = new BView(BRect(0,0,0,0),NULL,0,0);
		fPixel->Lock();
		fPixel->AddChild(fPixelView);
		fPixel->Unlock();
	};
}

void
TOSMagnify::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	InitObject();
}

void
TOSMagnify::Resize(int32 width, int32 height)
{
	ResizeTo(width, height);
	InitObject();
}

bool
TOSMagnify::CreateImage(BPoint mouseLoc, bool force)
{
	bool created = false;
	if (Window() && Window()->Lock()) {
		int32 width, height;
		fParent->PixelCount(&width, &height);
		int32 pixelSize = fParent->PixelSize();
		
		BRect srcRect(0, 0, width - 1, height - 1);
		srcRect.OffsetBy(	mouseLoc.x - (width / 2),
							mouseLoc.y - (height / 2));
		if (force || CopyScreenRect(srcRect)) {

			srcRect.OffsetTo(BPoint(0, 0));
			BRect destRect(Bounds());

			DrawBitmap(fBitmap, srcRect, destRect);

			DrawGrid(width, height, destRect, pixelSize);		
			DrawSelection();

			Sync();
			created = true;
		}
		Window()->Unlock();
	} else
		printf("window problem\n");

	return(created);
}

bool
TOSMagnify::CopyScreenRect(BRect srcRect)
{
	// constrain src rect to legal screen rect
	BScreen screen( Window() );
	BRect scrnframe = screen.Frame();

	if (srcRect.right > scrnframe.right)
		srcRect.OffsetTo(scrnframe.right - srcRect.Width(),
				 		 srcRect.top);
	if (srcRect.top < 0)
		srcRect.OffsetTo(srcRect.left, 0);

	if (srcRect.bottom > scrnframe.bottom)
		srcRect.OffsetTo(srcRect.left,
				 		 scrnframe.bottom - srcRect.Height());
	if (srcRect.left < 0)
		srcRect.OffsetTo(0, srcRect.top);

	// save a copy of the bits for comparison later
	memcpy(fOldBits, fBitmap->Bits(), fBitmap->BitsLength());

#ifdef PR31
	_get_screen_bitmap_(fBitmap, srcRect);
#else
	screen.ReadBitmap(fBitmap, false, &srcRect);
#endif

	// let caller know whether bits have actually changed
	return(memcmp(fBitmap->Bits(), fOldBits, fBitmap->BitsLength()) != 0);
}

void
TOSMagnify::DrawGrid(int32 width, int32 height, BRect destRect, int32 pixelSize)
{
	// draw grid
	if (fParent->ShowGrid() && fParent->PixelSize() > 2) {
		BeginLineArray(width * height);
	
		// horizontal lines
		for (int32 i = pixelSize; i < (height * pixelSize); i += pixelSize)
			AddLine(BPoint(0, i), BPoint(destRect.right, i), kGridGray);
			
		// vertical lines
		for (int32 i = pixelSize; i < (width * pixelSize); i += pixelSize)
			AddLine(BPoint(i, 0), BPoint(i, destRect.bottom), kGridGray);
	
		EndLineArray();
	}
	
	SetHighColor(kGridGray);
	StrokeRect(destRect);
}

void
TOSMagnify::DrawSelection()
{
	if (!fParent->SelectionIsShowing())
		return;

	float x, y;
	int32 pixelSize = fParent->PixelSize();
	int32 squareSize = pixelSize - 2;
	
	fParent->SelectionLoc(&x, &y);
	x *= pixelSize; x++;
	y *= pixelSize; y++;
	BRect selRect(x, y, x+squareSize, y+squareSize);
	
	short selection = fParent->Selection();
	
	PushState();
	SetLowColor(ViewColor());
	SetHighColor(kRedColor);
	StrokeRect(selRect);
	if (selection == 0) {
		StrokeLine(BPoint(x,y), BPoint(x+squareSize,y+squareSize));
		StrokeLine(BPoint(x,y+squareSize), BPoint(x+squareSize,y));
	}	
	
	bool ch1Showing, ch2Showing;
	fParent->CrossHairsShowing(&ch1Showing, &ch2Showing);
	if (ch1Showing) {
		SetHighColor(kBlueColor);
		fParent->CrossHair1Loc(&x, &y);
		x *= pixelSize; x++;
		y *= pixelSize; y++;
		selRect.Set(x, y,x+squareSize, y+squareSize);
		StrokeRect(selRect);
		BeginLineArray(4);
		AddLine(BPoint(0, y+(squareSize/2)),
			BPoint(x, y+(squareSize/2)), kBlueColor);					//	left
		AddLine(BPoint(x+squareSize,y+(squareSize/2)),
			BPoint(Bounds().Width(), y+(squareSize/2)), kBlueColor);	// right
		AddLine(BPoint(x+(squareSize/2), 0),
			BPoint(x+(squareSize/2), y), kBlueColor);					// top
		AddLine(BPoint(x+(squareSize/2), y+squareSize),
			BPoint(x+(squareSize/2), Bounds().Height()), kBlueColor);	// bottom
		EndLineArray();
		if (selection == 1) {
			StrokeLine(BPoint(x,y), BPoint(x+squareSize,y+squareSize));
			StrokeLine(BPoint(x,y+squareSize), BPoint(x+squareSize,y));
		}	
	}
	if (ch2Showing) {
		SetHighColor(kBlueColor);
		fParent->CrossHair2Loc(&x, &y);
		x *= pixelSize; x++;
		y *= pixelSize; y++;
		selRect.Set(x, y,x+squareSize, y+squareSize);
		StrokeRect(selRect);
		BeginLineArray(4);
		AddLine(BPoint(0, y+(squareSize/2)),
			BPoint(x, y+(squareSize/2)), kBlueColor);					//	left
		AddLine(BPoint(x+squareSize,y+(squareSize/2)),
			BPoint(Bounds().Width(), y+(squareSize/2)), kBlueColor);	// right
		AddLine(BPoint(x+(squareSize/2), 0),
			BPoint(x+(squareSize/2), y), kBlueColor);					// top
		AddLine(BPoint(x+(squareSize/2), y+squareSize),
			BPoint(x+(squareSize/2), Bounds().Height()), kBlueColor);	// bottom
		EndLineArray();
		if (selection == 2) {
			StrokeLine(BPoint(x,y), BPoint(x+squareSize,y+squareSize));
			StrokeLine(BPoint(x,y+squareSize), BPoint(x+squareSize,y));
		}	
	}
	
	PopState();
}

rgb_color
TOSMagnify::ColorAtSelection()
{			
	float x, y;
	fParent->SelectionLoc(&x, &y);
	BRect srcRect(x,y,x,y);
	BRect dstRect(0,0,0,0);
	fPixel->Lock();
	fPixelView->DrawBitmap(fBitmap,srcRect,dstRect);
	fPixelView->Sync();
	fPixel->Unlock();

	uint32 pixel = *((uint32*)fPixel->Bits());	
	rgb_color c;
	c.alpha = pixel >> 24;
	c.red = (pixel >> 16) & 0xFF;
	c.green = (pixel >> 8) & 0xFF;
	c.blue = pixel & 0xFF;

	return c;	
}

};	//	namespace Magnify
