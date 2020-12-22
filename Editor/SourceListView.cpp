/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Source item
 */

#include <cassert>

#include <InterfaceKit.h>
#include <support/String.h>
#include <ControlLook.h>

#ifndef _GRAPHICS_DEFS_H
#include <interface/GraphicsDefs.h>
#endif

#include "ImageUtility.h"
#include "Language.h"
#include "MediaSource.h"
#include "MedoWindow.h"
#include "SourceListView.h"
#include "Theme.h"
#include "TimelineEdit.h"
#include "VideoManager.h"

/*********************************
	SourceListItem
**********************************/

static const float kThumbnailHeight = 84.0f;
static const float kThumbnailWidth = kThumbnailHeight * (16.0f/9.0f);

/*	FUNCTION:		SourceListItem :: SourceListItem
	ARGS:			filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
SourceListItem :: SourceListItem(MediaSource *media_source)
	: fMediaSource(media_source), fBitmap(nullptr)
{
	assert(media_source != nullptr);
	CreateBitmap(media_source);
}

/*	FUNCTION:		SourceListItem :: ~SourceListItem
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
SourceListItem :: ~SourceListItem()
{
	delete fBitmap;
}

/*	FUNCTION:		SourceListItem :: CreateBitmap
	ARGS:			media_source
	RETURN:			n/a
	DESCRIPTION:	Create thumbnail
					Video files may have initial frame, so use subsequent frames as thumbnail
*/
void SourceListItem :: CreateBitmap(MediaSource *media_source)
{
	//	Has video?
	if ((media_source->GetMediaType() == MediaSource::MEDIA_VIDEO) || (media_source->GetMediaType() == MediaSource::MEDIA_VIDEO_AND_AUDIO))
	{
		int64 frame_idx = 0;
		while (!fBitmap)
		{
			BBitmap *frame = frame_idx == 0 ? media_source->GetBitmap() : gVideoManager->GetFrameBitmap(media_source, frame_idx, true);
			float w = frame->Bounds().Width();
			float h = frame->Bounds().Height();
			uint8 *s = (uint8 *)frame->Bits();
			int32 bytes_per_row = frame->BytesPerRow();
			int32 sw = bytes_per_row / 4;
			int32 sh = frame->BitsLength() / bytes_per_row;
			float dx = sw/w;
			float dy = sh/h;

			float intensity = 0.0f;
			//	scan frame diagonally, calculate intesity
			for (int row = 0; row < int(h); row++)
			{
				int col = w * row/h;	//	scaled column
				uint32 x= dx*col;
				uint32 y = dy*row;
				uint32 colour = *((uint32 *)(s + y*bytes_per_row + x*4));
				intensity += 0.3f*((colour>>16)&0xff) + 0.59f*((colour>>8)&0xff) + 0.11f*(colour&0xff);
			}
			if (intensity / h > 256.0f*0.2f)
				fBitmap = CreateThumbnail(frame, ceilf(kThumbnailWidth), kThumbnailHeight);
			else
			{
				frame_idx += kFramesSecond;
				if (frame_idx >= media_source->GetVideoDuration())
					break;
			}
		}
	}

	if (!fBitmap)
		fBitmap = CreateThumbnail(media_source->GetBitmap(), ceilf(kThumbnailWidth), kThumbnailHeight);
}

/*	FUNCTION:		SourceListItem :: Update
	ARGS:			parent
					font
	RETURN:			n/a
	DESCRIPTION:	Hook called when item added to view
*/
void SourceListItem :: Update(BView *parent, const BFont *font)
{
	SetWidth(kThumbnailWidth + 3*be_control_look->DefaultLabelSpacing() + font->StringWidth(fMediaSource->GetFilename()));
	SetHeight(kThumbnailHeight + 2*be_control_look->DefaultLabelSpacing());
	
	font_height h;
	font->GetHeight(&h);
	fBaselineOffset = 0.5f*kThumbnailHeight;
}

/*	FUNCTION:		SourceListItem :: DrawItem
	ARGS:			parent
					frame
					erase_bg
	RETURN:			n/a
	DESCRIPTION:	Draw item
*/
void SourceListItem :: DrawItem(BView *parent, BRect frame, bool erase_bg)
{
	rgb_color lowColor = parent->LowColor();
	erase_bg = true;
	
	if (IsSelected() || erase_bg)
	{
		rgb_color color;
		if (IsSelected())
			color = Theme::GetUiColour(UiColour::eListSelection);
		else
			color = parent->ViewColor();
				
		parent->SetLowColor(color);
		parent->FillRect(frame, B_SOLID_LOW);
	}
	else
		parent->SetLowColor(parent->ViewColor());
	
	const float offset = be_control_look->DefaultLabelSpacing();
	parent->MovePenTo(offset, offset);
	BRect thumb_rect(frame.left + offset, frame.top + offset, frame.left + kThumbnailWidth + offset, frame.top + kThumbnailHeight + offset);

	if (fMediaSource->GetMediaType() != MediaSource::MEDIA_AUDIO)
	{
		//	Adjust aspect (fBitmap is kHumbnailWidth x kThumbnailHeight, so we need to inverse scale in call to DrawBitmapAsync)
		float aspect = (float)fMediaSource->GetVideoWidth() / (float)fMediaSource->GetVideoHeight();
		if (aspect > 1.0f)
		{
			float h = thumb_rect.Width() / aspect;
			thumb_rect.top += 0.5f*(thumb_rect.Height() - h);
			thumb_rect.bottom = thumb_rect.top + h;
		}
		else if (aspect < 1.0f)
		{
			float w = thumb_rect.Height() * aspect;
			thumb_rect.left += 0.5f*(thumb_rect.Width() - w);
			thumb_rect.right = thumb_rect.left + w;
		}
	}
	parent->DrawBitmapAsync(fBitmap, thumb_rect);

	parent->SetHighColor(Theme::GetUiColour(UiColour::eListText));

	font_height fh;
	be_plain_font->GetHeight(&fh);
	BString aString(fMediaSource->GetFilename());
	int trunc = aString.FindLast('/') + 1;
	BString line1(aString.RemoveChars(0, trunc));
	aString.SetTo(fMediaSource->GetFilename());
	BString line2(aString.Truncate(trunc));
	parent->MovePenTo(kThumbnailWidth + frame.left + 3*offset, frame.top + fBaselineOffset);
	be_plain_font->TruncateString(&line1, B_TRUNCATE_MIDDLE, frame.Width() - (kThumbnailWidth + 3*be_control_look->DefaultLabelSpacing()));
	parent->DrawString(line1);
	parent->MovePenTo(kThumbnailWidth + frame.left + 3*offset, frame.top + fBaselineOffset + fh.ascent + fh.descent);
	be_plain_font->TruncateString(&line2, B_TRUNCATE_BEGINNING, frame.Width() - (kThumbnailWidth + 3*be_control_look->DefaultLabelSpacing()));
	parent->DrawString(line2);

	parent->SetLowColor(lowColor);
}

/*********************************/
class InstructionListItem : public BListItem
{
public:
	void DrawItem(BView *parent, BRect frame, bool erase_bg)
	{
		parent->MovePenTo(10, frame.bottom);
		parent->SetHighColor(Theme::GetUiColour(UiColour::eListText));
		parent->DrawString(GetText(TXT_MENU_PROJECT_ADD_SOURCE));
	}
};


/*********************************
	SourceListView
**********************************/

enum kSourceListMessages
{
	kMsgGetInfo			= 'slvm',
	kMsgRemoveSource,
};

/*	FUNCTION:		SourceListView :: SourceListView
	ARGS:			frame
					name
					func
	RETURN:			n/a
	DESCRIPTION:	constructor
*/
SourceListView :: SourceListView(BRect frame, const char *name, std::function<const char *(BListItem *)> func)
#if SOURCE_LIST_VIEW_TOOLTIP
	: ListViewToolTip(frame, name, func)
#else
	: BListView(frame, name)
#endif
{
	AddItem(new InstructionListItem);
	fInstructionItemVisible = true;

	SetViewColor(Theme::GetUiColour(UiColour::eListBackground));

	//	Drag drop message
	fMsgDragDrop = new BMessage(TimelineEdit::eMsgDragDropClip);

	//	Source selected message
	fMsgNotifySourceSelected = new BMessage(MedoWindow::eMsgActionTabSourceSelected);
	fMsgNotifySourceSelected->AddPointer("MediaSource", nullptr);
}

/*	FUNCTION:		SourceListView :: ~SourceListView
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
SourceListView :: ~SourceListView()
{
	delete fMsgDragDrop;
	delete fMsgNotifySourceSelected;
}

/*	FUNCTION:		SourceListView :: SelectionChanged
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when selection changed
*/
void SourceListView :: SelectionChanged()
{
	BListView::SelectionChanged();

	int32 index = CurrentSelection();
	if (index >= 0)
	{
		fMsgNotifySourceSelected->ReplacePointer("MediaSource", ((SourceListItem *)ItemAt(index))->GetMediaSource());
		Window()->PostMessage(fMsgNotifySourceSelected);
	}
}

/*	FUNCTION:		SourceListView :: MouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Process mouse down
					Initiate drag/drop message
*/
void SourceListView :: MouseDown(BPoint point)
{
	if (!Window()->IsActive())
		Window()->Activate();

	if (fInstructionItemVisible)
	{
		Window()->PostMessage(MedoWindow::eMsgMenuProjectAddSource);
		return;
	}

#if SOURCE_LIST_VIEW_TOOLTIP
	ListViewToolTip::MouseDown(point);
#else
	BListView::MouseDown(point);
#endif

	int32 index = CurrentSelection();
	if (index < 0)
		return;

	uint32 buttons;
	BMessage* msg = Window()->CurrentMessage();
	msg->FindInt32("buttons", (int32*)&buttons);
	bool ctrl_modifier = ((MedoWindow *)Window())->GetKeyModifiers() & B_CONTROL_KEY;
	if ((buttons & B_SECONDARY_MOUSE_BUTTON) || ctrl_modifier)
	{
		ContextMenu(point);
		return;
	}
}

/*	FUNCTION:		SourceListView :: InitiateDrag
	ARGS:			point
					index
					wasSelected
	RETURN:			true if processed
	DESCRIPTION:	Initiate drag/drop message
*/
bool SourceListView :: InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	uint32 buttons;
	BMessage* msg = Window()->CurrentMessage();
	msg->FindInt32("buttons", (int32*)&buttons);
	if (buttons & B_SECONDARY_MOUSE_BUTTON)
		return false;

	SourceListItem *item = (SourceListItem *)ItemAt(index);
	if (!item)
		return false;
	MediaSource *media_source = item->GetMediaSource();

	SetMouseEventMask(B_POINTER_EVENTS, 0);
	fMsgDragDrop->MakeEmpty();

	int64 clip_start = 0;
	fMsgDragDrop->AddInt64("start", clip_start);
	int64 clip_end = 0;
	switch (media_source->GetMediaType())
	{
		case MediaSource::MEDIA_VIDEO:
		case MediaSource::MEDIA_VIDEO_AND_AUDIO:
			clip_end = media_source->GetVideoDuration();
			break;
		case MediaSource::MEDIA_AUDIO:
			clip_end = media_source->GetAudioDuration();
			break;
		default:
			clip_end = 2*kFramesSecond;
	}
	fMsgDragDrop->AddInt64("end", clip_end);
	fMsgDragDrop->AddPointer("source", media_source);
	fMsgDragDrop->AddInt64("xoffset", 0);

	DragMessage(fMsgDragDrop, new BBitmap(item->GetBitmap()), BPoint(0.25f*kThumbnailWidth, 0.25f*kThumbnailHeight));
	return true;
}

/*	FUNCTION:		SourceListView :: MouseUp
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Process mouse up
*/
void SourceListView :: MouseUp(BPoint point)
{
	int32 index = CurrentSelection();
	if (index >= 0)
	{
		fMsgNotifySourceSelected->ReplacePointer("MediaSource", ((SourceListItem *)ItemAt(index))->GetMediaSource());
		Window()->PostMessage(fMsgNotifySourceSelected);
	}
}

/*	FUNCTION:		SourceListView :: ContextMenu
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Show context menu for media source
*/
void SourceListView :: ContextMenu(BPoint point)
{
	ConvertToScreen(&point);

	//	Initialise PopUp menu
	BPopUpMenu *aPopUpMenu = new BPopUpMenu("ContextMenuSourceList", false, false);
	aPopUpMenu->SetAsyncAutoDestruct(true);

	BMenuItem *aMenuItem_1 = new BMenuItem("File Info", new BMessage(kMsgGetInfo));
	aPopUpMenu->AddItem(aMenuItem_1);

	int32 index = CurrentSelection();
	assert(index >= 0);
	SourceListItem *item = (SourceListItem *)ItemAt(index);
	MediaSource *media_source = item->GetMediaSource();
	BMenuItem *aMenuItem_2 = new BMenuItem(gProject->IsMediaSourceUsed(media_source) ? "Remove Media (including all References)" : "Remove Media",
										   new BMessage(kMsgRemoveSource));
	aPopUpMenu->AddItem(aMenuItem_2);

	aPopUpMenu->SetTargetForItems(this);
	aPopUpMenu->Go(point, true /*notify*/, false /*stay open when mouse away*/, true /*async*/);
}

/*	FUNCTION:		SourceListView :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void SourceListView :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgGetInfo:
		{
			int32 index = CurrentSelection();
			if (index < 0)
				return;
			SourceListItem *item = (SourceListItem *)ItemAt(index);
			MediaSource *media_source = item->GetMediaSource();

			BString aString;
			media_source->CreateFileInfoString(&aString);
			BAlert *alert = new BAlert("FileInfo", aString.String(), "OK");
			alert->Go();
			break;
		}
		case kMsgRemoveSource:
		{
			int32 index = CurrentSelection();
			if (index < 0)
				return;
			SourceListItem *item = (SourceListItem *)ItemAt(index);
			MediaSource *media_source = item->GetMediaSource();
			gProject->RemoveMediaSource(media_source);
			RemoveItem(index);
			((MedoWindow *)Window())->InvalidatePreview();

			if (CountItems() == 0)
			{
				AddItem(new InstructionListItem);
				fInstructionItemVisible = true;
			}
			break;
		}
		default:
#if SOURCE_LIST_VIEW_TOOLTIP
			ListViewToolTip::MessageReceived(msg);
#else
			BListView::MessageReceived(msg);
#endif
	}
}

/*	FUNCTION:		SourceListView :: AddItem
	ARGS:			item
	RETURN:			true if success
	DESCRIPTION:	Hook function
*/
bool SourceListView :: AddItem(BListItem* item)
{
	if (fInstructionItemVisible)
	{
		InstructionListItem *item = (InstructionListItem *)RemoveItem(0);
		delete item;
		fInstructionItemVisible = false;
	}
	return BListView::AddItem(item);
}

/*	FUNCTION:		SourceListView :: RemoveAllMediaSources
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when new project loaded
*/
void SourceListView :: RemoveAllMediaSources()
{
	while (CountItems() > 0)
	{
		SourceListItem *item = (SourceListItem *)ItemAt(0);
		gProject->RemoveMediaSource(item->GetMediaSource());
		RemoveItem(0);
	}
}
