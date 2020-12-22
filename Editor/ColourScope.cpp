/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Colour scope
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include <app/MessageQueue.h>

#include "ColourScope.h"
#include "Language.h"
#include "MedoWindow.h"
#include "PersistantWindow.h"
#include "Project.h"

/*************************************
	ScopeView
**************************************/
class ScopeView : public BView
{
public:
	enum class ScopeType {eScopeHistogramSeparate, eScopeHistogramUnified};

private:
	BBitmap		*fBitmap;
	struct COLOUR_VALUES
	{
		uint16		lum;
		uint16		red;
		uint16		green;
		uint16		blue;
		void SetOne() {lum = 1, red = 1, green = 1, blue = 1;}
	};
	enum ScopeMessage {eMsgHistogramSeparate = 'esp0', eMsgHistogramUnified};
	ScopeType fScopeType;

	/*	FUNCTION:		ScopeView :: PrepareHistogram
		ARGS:			source
		RETURN:			n/a
		DESCRIPTION:	Prepare Histogram (luminance, red/green/blue)
	*/
	void	PrepareHistogramSeparate(BBitmap *source)
	{
		assert(source);
		int32 source_width = source->Bounds().IntegerWidth() + 1;
		int32 source_height = source->Bounds().IntegerHeight() + 1;

		int32 bytes_per_row = source->BytesPerRow();
		assert(bytes_per_row == source_width*4);	//	B_RGBA32

		std::vector<COLOUR_VALUES> histogram;
		histogram.resize(source_width*256);
		memset(histogram.data(), 0, source_width*256*sizeof(COLOUR_VALUES));

		std::vector<COLOUR_VALUES> max_colour;
		max_colour.resize(source_width);
		memset(max_colour.data(), 0, source_width*sizeof(COLOUR_VALUES));

		//	Colour Histogram
		uint8 *s = (uint8 *)source->Bits();
		for (int32 row=0; row < source_height; ++row)
		{
			for (int32 col=0; col < source_width; col++)
			{
				//	BGRA
				uint8 b = *s++;
				uint8 g = *s++;
				uint8 r = *s++;
				s++;

				//const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
				uint8 lum = 0.2125f*r + 0.7154f*g + 0.0721f*b;
				histogram[col*256 + lum].lum++;

				histogram[col*256 + r].red++;
				histogram[col*256 + g].green++;
				histogram[col*256 + b].blue++;
			}
		}

		//	Determine max occurance
		for (int32 col=0; col < source_width; col++)
		{
			COLOUR_VALUES &max = max_colour[col];
			max.SetOne();

			for (int32 row=1; row < 256; ++row)
			{
				const COLOUR_VALUES &h = histogram[col*256 + row];
				if (h.lum > max.lum)
					max.lum = h.lum;
				if (h.red > max.red)
					max.red = h.red;
				if (h.green > max.green)
					max.green = h.green;
				if (h.blue > max.blue)
					max.blue = h.blue;
			}

		}

		//	Create fBitmap
		uint8 *d = (uint8 *)fBitmap->Bits();

		//	Luminance
		for (int32 row=255; row >= 0; --row)
		{
			for (int32 col=0; col < source_width; col++)
			{
				uint8 v = 256*histogram[col*256 + row].lum / max_colour[col].lum;
				*d++ = v;
				*d++ = v;
				*d++ = v;
				*d++ = 255;
			}
		}

		//	Red
		for (int32 row=255; row >= 0; --row)
		{
			for (int32 col=0; col < source_width; col++)
			{
				uint8 v = 256*histogram[col*256 + row].red / max_colour[col].red;
				*d++ = 0;
				*d++ = 0;
				*d++ = v;
				*d++ = 255;
			}
		}
		//	Green
		for (int32 row=255; row >= 0; --row)
		{
			for (int32 col=0; col < source_width; col++)
			{
				uint8 v = 256*histogram[col*256 + row].green / max_colour[col].green;
				*d++ = 0;
				*d++ = v;
				*d++ = 0;
				*d++ = 255;
			}
		}
		//	Blue
		for (int32 row=255; row >= 0; --row)
		{
			for (int32 col=0; col < source_width; col++)
			{
				uint8 v = 256*histogram[col*256 + row].blue / max_colour[col].blue;
				*d++ = v;
				*d++ = 0;
				*d++ = 0;
				*d++ = 255;
			}
		}
	}

	/*	FUNCTION:		ScopeView :: PrepareHistogramUnified
		ARGS:			source
		RETURN:			n/a
		DESCRIPTION:	Prepare Histogram (luminance, red/green/blue) unified
	*/
	void	PrepareHistogramUnified(BBitmap *source)
	{
		assert(source);
		int32 source_width = source->Bounds().IntegerWidth() + 1;
		int32 source_height = source->Bounds().IntegerHeight() + 1;

		int32 bytes_per_row = source->BytesPerRow();
		assert(bytes_per_row == source_width*4);	//	B_RGBA32

		std::vector<COLOUR_VALUES> histogram;
		histogram.resize(source_width*256);
		memset(histogram.data(), 0, source_width*256*sizeof(COLOUR_VALUES));

		std::vector<COLOUR_VALUES> max_colour;
		max_colour.resize(source_width);
		memset(max_colour.data(), 0, source_width*sizeof(COLOUR_VALUES));

		//	Colour Histogram
		uint8 *s = (uint8 *)source->Bits();
		for (int32 row=0; row < source_height; ++row)
		{
			for (int32 col=0; col < source_width; col++)
			{
				//	BGRA
				uint8 b = *s++;
				uint8 g = *s++;
				uint8 r = *s++;
				s++;

				//const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
				uint8 lum = 0.2125f*r + 0.7154f*g + 0.0721f*b;
				histogram[col*256 + lum].lum++;

				histogram[col*256 + r].red++;
				histogram[col*256 + g].green++;
				histogram[col*256 + b].blue++;
			}
		}

		//	Determine max occurance
		for (int32 col=0; col < source_width; col++)
		{
			COLOUR_VALUES &max = max_colour[col];
			max.SetOne();

			for (int32 row=1; row < 256; ++row)
			{
				const COLOUR_VALUES &h = histogram[col*256 + row];
				if (h.lum > max.lum)
					max.lum = h.lum;
				if (h.red > max.red)
					max.red = h.red;
				if (h.green > max.green)
					max.green = h.green;
				if (h.blue > max.blue)
					max.blue = h.blue;
			}

		}

		//	Create fBitmap
		uint8 *d = (uint8 *)fBitmap->Bits();

		//	Luminance
		for (int32 row=256; row >= 0; --row)
		{
			for (int32 col=0; col < source_width; col++)
			{
				uint8 v = 256*histogram[col*256 + row].lum / max_colour[col].lum;
				*d++ = v;
				*d++ = v;
				*d++ = v;
				*d++ = 255;
			}
		}

		//	Unified
		for (int32 row=256; row >= 0; --row)
		{
			for (int32 col=0; col < source_width; col++)
			{
				*d++ = 256*histogram[col*256 + row].blue / max_colour[col].blue;
				*d++ = 256*histogram[col*256 + row].green / max_colour[col].green;
				*d++ = 256*histogram[col*256 + row].red / max_colour[col].red;
				*d++ = 255;
			}
		}
	}

public:
	ScopeView(BRect bounds)
	: BView(bounds, nullptr, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
	{
		fScopeType = ScopeType::eScopeHistogramSeparate;
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fBitmap = new BBitmap(BRect(0, 0, gProject->mResolution.width - 1, 4*256 - 1), B_RGBA32);
	}
	~ScopeView()	override
	{
		delete fBitmap;
	}
	void Draw(BRect frame)
	{
		if (fBitmap)
			DrawBitmapAsync(fBitmap, Bounds());

		int num_segments = 4;
		if (fScopeType == ScopeType::eScopeHistogramUnified)
			num_segments = 2;

		BRect bounds = Bounds();
		const float kSegmentSize = bounds.Height()/num_segments;

		for (int i=0; i < num_segments; i++)
		{
			SetHighColor(255, 255, 255);
			StrokeLine(BPoint(bounds.left, i*kSegmentSize), BPoint(bounds.right, i*kSegmentSize));

			SetHighColor(128, 128, 128);

			for (int j=1; j < 4; j++)
			{
				StrokeLine(BPoint(bounds.left, i*kSegmentSize + j*kSegmentSize/4.0f), BPoint(bounds.right, i*kSegmentSize + j*kSegmentSize/4.0f));
			}
		}
	}
	void FrameResized(float width, float height) override
	{
		Invalidate();
	}
	void SetBitmap(BBitmap *bitmap)
	{
		switch (fScopeType)
		{
			case ScopeType::eScopeHistogramSeparate:		PrepareHistogramSeparate(bitmap);			break;
			case ScopeType::eScopeHistogramUnified:			PrepareHistogramUnified(bitmap);			break;
			default:	assert(0);
		}
		Invalidate();
	}
	void MouseDown(BPoint point)
	{
		uint32 buttons;
		BMessage* msg = Window()->CurrentMessage();
		msg->FindInt32("buttons", (int32*)&buttons);
		bool ctrl_modifier = ((MedoWindow *)Window())->GetKeyModifiers() & B_CONTROL_KEY;
		if (ctrl_modifier || (buttons & B_SECONDARY_MOUSE_BUTTON))
		{
			ConvertToScreen(&point);
			//	Initialise PopUp menu
			BPopUpMenu *aPopUpMenu = new BPopUpMenu("ContextMenuColourScope", false, false);
			aPopUpMenu->SetAsyncAutoDestruct(true);

			BMenuItem *aMenuItem = new BMenuItem(GetText(TXT_COLOUR_SCOPE_SEPARATE_COLOURS), new BMessage(ScopeMessage::eMsgHistogramSeparate));
			if (fScopeType == ScopeType::eScopeHistogramSeparate)
				aMenuItem->SetMarked(true);
			aPopUpMenu->AddItem(aMenuItem);
			aMenuItem = new BMenuItem(GetText(TXT_COLOUR_SCOPE_UNIFIED_COLOURS), new BMessage(ScopeMessage::eMsgHistogramUnified));
			if (fScopeType == ScopeType::eScopeHistogramUnified)
				aMenuItem->SetMarked(true);
			aPopUpMenu->AddItem(aMenuItem);

			aPopUpMenu->SetTargetForItems(this);
			aPopUpMenu->Go(point, true /*notify*/, false /*stay open when mouse away*/, true /*async*/);

			//All BPopUpMenu items are freed when the popup is closed
		}
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what)
		{
			case ScopeMessage::eMsgHistogramSeparate:
				fScopeType = ScopeType::eScopeHistogramSeparate;
				delete fBitmap;
				fBitmap = new BBitmap(BRect(0, 0, gProject->mResolution.width - 1, 4*256 - 1), B_RGBA32);
				gProject->InvalidatePreview();
				break;
			case ScopeMessage::eMsgHistogramUnified:
				delete fBitmap;
				fBitmap = new BBitmap(BRect(0, 0, gProject->mResolution.width - 1, 2*256 - 1), B_RGBA32);
				fScopeType = ScopeType::eScopeHistogramUnified;
				gProject->InvalidatePreview();
				break;
			default:
				BView::MessageReceived(msg);
				break;
		}
	}
};

/*************************************
	Colour scope
**************************************/

/*	FUNCTION:		ColourScope :: ColourScope
	ARGS:			frame
					title
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ColourScope :: ColourScope(BRect frame, const char *title)
	: PersistantWindow(frame, title, B_DOCUMENT_WINDOW, B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS)
{
	fScopeView = new ScopeView(Bounds());
	AddChild(fScopeView);
}

/*	FUNCTION:		ColourScope :: ~ColourScope
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ColourScope :: ~ColourScope()
{
}

/*	FUNCTION:		ColourScope :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process window messages
*/
void ColourScope :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case MedoWindow::eMsgActionAsyncPreviewReady:	//	caution - must be unique msg->what for this window
		{
			BBitmap *bitmap;
			if (msg->FindPointer("BBitmap", (void **)&bitmap) == B_OK)
			{
				BMessage *msg;
				while ((msg = MessageQueue()->FindMessage(MedoWindow::eMsgActionAsyncPreviewReady, 0)) != nullptr)
				{
					if (msg->FindPointer("BBitmap", (void **)&bitmap) == B_OK)
					{
						MessageQueue()->RemoveMessage(msg);
						delete msg;
					}
				}
				fScopeView->SetBitmap(bitmap);
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

