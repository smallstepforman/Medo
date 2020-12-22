/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _YARRA_ACTOR_H_
#include "Actor/Actor.h"
#endif

#include <Application.h>
#include <Box.h>
#include <FilePanel.h>
#include <MenuBar.h>
#include <View.h>
#include <Window.h>

//******************************************************************************
namespace Magnify
{

class TMagnify;
class TOSMagnify : public BView {
public:
						TOSMagnify(BRect, TMagnify* parent, color_space space);
						~TOSMagnify();
						
		void			InitObject();
		void			FrameResized(float width, float height);
		void			SetSpace(color_space space);
		
		void			Resize(int32 width, int32 height);
						
		bool			CreateImage(BPoint, bool force=false);
		bool			CopyScreenRect(BRect);

		void			DrawGrid(int32 width, int32 height,
							BRect dest, int32 pixelSize);
		void			DrawSelection();
		
		rgb_color		ColorAtSelection();
		
		BBitmap*		Bitmap() { return fBitmap; }
		
private:
		color_map*		fColorMap;
		color_space		fColorSpace;
		char*			fOldBits;
		long			fBytesPerPixel;
		
		TMagnify*		fParent;
		BBitmap*		fBitmap;
		BBitmap*		fPixel;
		BView*			fPixelView;
};

//******************************************************************************

class TWindow;
class TMagnify : public BView, public yarra::Actor {
public:
						TMagnify(BRect, TWindow*);
						~TMagnify();
		void			AttachedToWindow();
		void			InitBuffers(int32 hPixelCount, int32 vPixelCount,
							int32 pixelSize, bool showGrid);
		
		void			Draw(BRect);
		
		void			KeyDown(const char *bytes, int32 numBytes);
		void			FrameResized(float, float);
		void			MouseDown(BPoint where);
		void			ScreenChanged(BRect bounds, color_space cs);

		void			SetSelection(bool state);
		void			MoveSelection(int32 x, int32 y);
		void			ShowSelection();
		
		short 			Selection();
		bool			SelectionIsShowing();
		void			SelectionLoc(float* x, float* y);
		void			SetSelectionLoc(float, float);
		rgb_color		SelectionColor();
		
		void			CrossHair1Loc(float* x, float* y);
		void			CrossHair2Loc(float* x, float* y);
		BPoint			CrossHair1Loc();		
		BPoint			CrossHair2Loc();
		
		void			NudgeMouse(float x, float y);
		
		void			WindowActivated(bool);

static	long			MagnifyTask(void *);

		void			Update(bool force);
		bool			NeedToUpdate();
		void			SetUpdate(bool);
				
		long			ThreadID() { return fThread; }

		void			MakeActive(bool);
		bool			Active()	{ return fActive; }
		
		void			SetCrossHairsShowing(bool ch1=false, bool ch2=false);
		void			CrossHairsShowing(bool*, bool*);
				
		void			PixelCount(int32* width, int32* height);
		int32 			PixelSize();
		bool			ShowGrid();
						
private:
		bool			fNeedToUpdate;
		long			fThread;				//	magnify thread id
		bool			fActive;				//	magnifying toggle
		

		BBitmap*		fImageBuf;				// os buffer
		TOSMagnify*		fImageView;				// os view
		BPoint			fLastLoc;

		short			fSelection;

		bool			fShowSelection;
		BPoint			fSelectionLoc;

		bool			fShowCrossHair1;
		BPoint			fCrossHair1;
		bool			fShowCrossHair2;
		BPoint			fCrossHair2;
		
		TWindow*		fParent;
		
		void			AsyncColourSelected(rgb_color colour);
};

//******************************************************************************

class TInfoView : public BBox {
public:
					TInfoView(BRect frame);
					~TInfoView();
				
		void		AttachedToWindow();
		void		Draw(BRect updateRect);
		void		FrameResized(float width, float height);
		
		void		SetMagView(TMagnify* magView);

private:
		float 		fFontHeight;		
		TMagnify*	fMagView;
		
		int32 		fHPixelCount;
		int32 		fVPixelCount;
		int32		fPixelSize;
		
		rgb_color	fSelectionColor;
		
		BPoint		fCH1Loc;
		BPoint		fCH2Loc;

		char		fInfoStr[64];
		char		fRGBStr[64];
		char		fCH1Str[64];
		char		fCH2Str[64];
};

//******************************************************************************

class TWindow : public BWindow {
public:
					TWindow(BHandler *handler, BMessage *msg);
					~TWindow();
		
		bool		QuitRequested();
		void		Terminate();
		
		void		FrameResized(float w, float h);
		void		ScreenChanged(BRect screen_size, color_space depth);
		
		void		Minimize(bool);
		void		Zoom(BPoint rec_position, float rec_width, float rec_height);
		
		void		CalcViewablePixels();
		void		GetPreferredSize(float* width, float* height);
		
		void		ResizeWindow(int32 rowCount, int32 columnCount);
		void		ResizeWindow(bool direction);

		bool		ShowGrid();
		
		void		ShowInfo(bool);
		bool		InfoIsShowing();
		void		UpdateInfo();
		
		void		CrossHairsShowing(bool* ch1, bool* ch2);
		
		void		PixelCount(int32* h, int32 *v);
		int32		PixelSize();
		
		bool		IsActive();

		void		WindowActivated(bool activated) override;
		void		ColourSelected(rgb_color colour);
		
private:
		float		fInfoHeight;
		bool		fShowInfo;
		float 		fFontHeight;

		bool		fShowGrid;
		
		int32		fHPixelCount;
		int32		fVPixelCount;
		int32 		fPixelSize;
		
		TMagnify*	fFatBits;
		TInfoView*	fInfo;

		BHandler	*fParentHandler;
		BMessage	*fParentMessage;
		bool		fReallyQuit;
		bool		fProcessColourSelected;
};

};	//	namespace Magnify
