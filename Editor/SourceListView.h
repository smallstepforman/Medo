/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Source view + item
 */
 
#ifndef _SOURCE_LIST_VIEW_H_
#define _SOURCE_LIST_VIEW_H_

#ifndef _LIST_ITEM_H
#include <interface/ListItem.h>
#endif

#ifndef LIST_VIEW_TOOLTIP_H
#include "Gui/ListViewToolTip.h"
#endif

class BBitmap;
class MediaSource;

class ClipTagWindow;

#define SOURCE_LIST_VIEW_TOOLTIP		0

//=======================
class SourceListItem : public BListItem
{
public:
	
					SourceListItem(MediaSource *media_source);
					~SourceListItem()											override;
			
	void			DrawItem(BView *parent, BRect frame, bool erase_bg=false)	override;
	void			Update(BView *parent, const BFont *font)					override;
	MediaSource		*GetMediaSource() {return fMediaSource;}
	BBitmap			*GetBitmap() const {return fBitmap;}
	
private:
    MediaSource		*fMediaSource;
    BBitmap			*fBitmap;
    float			fBaselineOffset;
	void			CreateBitmap(MediaSource *media_source);
};

//=======================
#if SOURCE_LIST_VIEW_TOOLTIP
class SourceListView : public ListViewToolTip
#else
class SourceListView : public BListView
#endif
{
	BMessage		*fMsgDragDrop;				//	TODO share with ControlSource
	BMessage		*fMsgNotifySourceSelected;
	bool			fInstructionItemVisible;
	ClipTagWindow	*fClipTagWindow;
	BPoint			fMouseDownPoint;

	void			ContextMenu(BPoint point);

public:
				SourceListView(BRect frame, const char *name, std::function<const char *(BListItem *)> func);
				~SourceListView()				override;
	void		MouseDown(BPoint point)			override;
	void		MouseUp(BPoint point)			override;
	void		MessageReceived(BMessage *msg)	override;
	void		SelectionChanged()				override;
	bool		InitiateDrag(BPoint point, int32 index, bool wasSelected) override;
	bool		AddItem(BListItem* item)		override;
	void		RemoveAllMediaSources();

	enum SourceListMessages
	{
		eMsgGetInfo			= 'slvm',
		eMsgEditLabel,
		eMsgEditLabelComplete,
		eMsgEditLabelCancel,
		eMsgRemoveSource,
	};
};

#endif	//#ifndef _SOURCE_LIST_VIEW_H_
