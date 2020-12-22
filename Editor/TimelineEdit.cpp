/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Timeline edit
 */
 
#include <cstdio>
#include <cassert>

#include <app/Application.h>
#include <app/Cursor.h>
#include <InterfaceKit.h>
#include <support/StringList.h>

#include "ClipTagWindow.h"
#include "CursorDefinitions.inc"
#include "EffectNode.h"
#include "EffectsWindow.h"
#include "Language.h"
#include "MediaUtility.h"
#include "MedoWindow.h"
#include "Project.h"
#include "RenderActor.h"
#include "TimelineEdit.h"
#include "TimelineView.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

enum kMsgTimelineEditPrivate
{
	kMessageContextClipDeleteLeaveEffects	= 'TLED',
	kMessageContextClipDeleteRemoveEffects,
	kMessageContextClipEditTag,
	kMessageContextClipAddNote,
	kMessageContextClipSplit,
	kMessageContextEffectDeleteEffect,
	kMessageContextEffectPriorityDown,
	kMessageContextEffectPriorityUp,
	kMessageContextEffectEnable,
	kMessageContextEffectStretchClipLength,
	kMessageContextNoteEdit,
	kMessageContextNoteDelete,
	kMessageContextClipEnableVideo,
	kMessageContextClipEnableAudio,
	kMessageContextTrackInsertAbove,
	kMessageContextTrackInsertBelow,
	kMessageContextTrackMoveUp,
	kMessageContextTrackMoveDown,
	kMessageContextTrackDelete,
	kMessageContextFileInfo,
};

/*	FUNCTION:		TimelineEdit :: TimelineEdit
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
TimelineEdit :: TimelineEdit(BRect frame, TimelineView *parent)
	: BView(frame, "TimelineEdit", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_TRANSPARENT_BACKGROUND),
	fTimelineView(parent), fClipTagsVisible(true), fTrackNotesVisible(true), fAnimateDragDropTrack(nullptr)
{
	frame = Bounds();
	fViewWidth = frame.Width();

	BScreen screen(B_MAIN_SCREEN_ID);
	fScreenWidth = screen.Frame().Width();
	
	fDrawAllVideoThumbnails = true;
	fState = State::eIdle;
	fLeftFrameIndex = 0;
	fScrollViewOffsetY = 0;
	fActiveCursor = CURSOR_DEFAULT;
	fCursors[CURSOR_MOVE] = new BCursor(B_CURSOR_ID_GRAB);
	fCursors[CURSOR_RESIZE] = new BCursor(kCursorResizeHorizontal);

	fActiveResizeDirection = ResizeDirection::eInactive;
	fPendingResizeDirection = ResizeDirection::eInactive;
	
	fFramesPixel = 60*kFramesSecond / fScreenWidth;

	fMsgDragDropClip = new BMessage(eMsgDragDropClip);
	fMsgDragDropEffect = new BMessage(eMsgDragDropEffect);
	fMsgOuptutViewMouseDown = new BMessage(EffectsWindow::eMsgOutputViewMouseDown);
	fMsgOuptutViewMouseMoved = new BMessage(EffectsWindow::eMsgOutputViewMouseMoved);

	fMsgOuptutViewMouseDown->AddPoint("point", BPoint(0.0f, 0.0f));
	fMsgOuptutViewMouseDown->AddPointer("effect", nullptr);
	fMsgOuptutViewMouseMoved->AddPoint("point", BPoint(0.0f, 0.0f));
	fMsgOuptutViewMouseMoved->AddPointer("effect", nullptr);

	fSelectedItem = SelectedItem::eSelectedNone;
	fClipTagWindow = nullptr;
}

/*	FUNCTION:		TimelineEdit :: ~TimelineEdit
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
TimelineEdit :: ~TimelineEdit()
{
	if (fActiveCursor != CURSOR_DEFAULT)
		be_app->SetCursor(B_HAND_CURSOR);
	for (int i=0; i < NUMBER_CURSORS; i++)
		delete fCursors[i];
	delete fMsgDragDropClip;
	delete fMsgDragDropEffect;
	delete fMsgOuptutViewMouseDown;
	delete fMsgOuptutViewMouseMoved;

	if (fClipTagWindow)
		fClipTagWindow->Terminate();
}

/*	FUNCTION:		TimelineEdit :: SetZoomFactor
	ARGS:			visible_frames
	RETURN:			n/a
	DESCRIPTION:	Set zoom factor
*/
void TimelineEdit :: SetZoomFactor(const int64 visible_frames)
{
	fFramesPixel = visible_frames / fScreenWidth;
}

/*	FUNCTION:		TimelineEdit :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void TimelineEdit :: FrameResized(float width, float height)
{
   fViewWidth = width;
}

/*	FUNCTION:		TimelineEdit :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Hook function
*/
void TimelineEdit :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case eMsgDragDropClip:
			DragDropClip(msg);
			break;

		case eMsgDragDropEffect:
			DragDropEffect(msg);
			break;
			
		case kMessageContextClipDeleteLeaveEffects:
			gProject->Snapshot();
			fActiveClip.track->RemoveClip(fActiveClip.track->mClips[fActiveClip.clip_idx], false);
			fActiveClip.Reset();
			gProject->InvalidatePreview();
			fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;

		case kMessageContextClipSplit:
			gProject->Snapshot();
			fActiveClip.track->SplitClip(fActiveClip.track->mClips[fActiveClip.clip_idx], fActiveClip.track->mClips[fActiveClip.clip_idx].mTimelineFrameStart + fActiveClip.frame_idx);
			fActiveClip.Reset();
			gProject->InvalidatePreview();
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;
			
		case kMessageContextClipDeleteRemoveEffects:
			gProject->Snapshot();
			fActiveClip.track->RemoveClip(fActiveClip.track->mClips[fActiveClip.clip_idx], true);
			fActiveClip.Reset();
			fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
			gProject->InvalidatePreview();
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;

		case kMessageContextEffectDeleteEffect:
			gProject->Snapshot();
			fActiveEffect.track->RemoveEffect(fActiveEffect.media_effect);
			fActiveEffect.Reset();
			fTimelineView->InvalidateItems(TimelineView::INVALIDATE_POSITION_SLIDER | TimelineView::INVALIDATE_HORIZONTAL_SLIDER);
			gProject->InvalidatePreview();
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;

		case kMessageContextEffectPriorityDown:
		{
			MediaEffect *effect = fActiveEffect.media_effect;
			if (effect)
			{
				gProject->Snapshot();
				fActiveEffect.track->SetEffectPriority(effect, effect->mPriority - 1);
				gProject->InvalidatePreview();
				gRenderActor->AsyncInvalidateTimelineEdit();
			}
			break;
		}

		case kMessageContextEffectPriorityUp:
		{
			MediaEffect *effect = fActiveEffect.media_effect;
			if (effect)
			{
				gProject->Snapshot();
				fActiveEffect.track->SetEffectPriority(effect, effect->mPriority + 1);
				gProject->InvalidatePreview();
				gRenderActor->AsyncInvalidateTimelineEdit();
			}
			break;
		}

		case kMessageContextEffectEnable:
		{
			MediaEffect *effect = fActiveEffect.media_effect;
			if (effect)
			{
				gProject->Snapshot();
				effect->mEnabled = !effect->mEnabled;
				gProject->InvalidatePreview();
				gRenderActor->AsyncInvalidateTimelineEdit();
			}
			break;
		}

		case kMessageContextEffectStretchClipLength:
		{
			MediaEffect *effect = fActiveEffect.media_effect;
			if (effect)
			{
				gProject->Snapshot();
				EffectMatchClipDuration(fActiveEffect.track, effect);
			}
			break;
		}

		case kMessageContextClipEnableVideo:
		{
			gProject->Snapshot();
			MediaClip &clip = fActiveClip.track->mClips[fActiveClip.clip_idx];
			clip.mVideoEnabled = !clip.mVideoEnabled;
			gProject->InvalidatePreview();
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;
		}
		case kMessageContextClipEnableAudio:
		{
			gProject->Snapshot();
			MediaClip &clip = fActiveClip.track->mClips[fActiveClip.clip_idx];
			clip.mAudioEnabled = !clip.mAudioEnabled;
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;
		}

		case kMessageContextFileInfo:
		{
			const MediaClip &clip = fActiveClip.track->mClips[fActiveClip.clip_idx];
			MediaSource *media_source = clip.mMediaSource;
			BString aString;
			media_source->CreateFileInfoString(&aString);
			aString.Append("\nClip Duration: ");
			aString.Append(MediaDuration(clip.Duration(), (media_source->GetMediaType() != MediaSource::MEDIA_AUDIO) ? gProject->mResolution.frame_rate : 0).Print());
			BAlert *alert = new BAlert("Clip Info", aString.String(), "OK");
			alert->Go();
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;
		}

		case kMessageContextTrackInsertAbove:
		{
			gProject->Snapshot();
			gProject->AddTimelineTrack(new TimelineTrack, gProject->GetTimelineTrackIndex(fContextTimelineTrack));
			MedoWindow::GetInstance()->InvalidatePreview();
			break;
		}
		case kMessageContextTrackInsertBelow:
		{
			gProject->Snapshot();
			gProject->AddTimelineTrack(new TimelineTrack, gProject->GetTimelineTrackIndex(fContextTimelineTrack) + 1);
			MedoWindow::GetInstance()->InvalidatePreview();
			break;
		}
		case kMessageContextTrackMoveUp:
		{
			int index = gProject->GetTimelineTrackIndex(fContextTimelineTrack);
			if (index >= 1)
			{
				gProject->Snapshot();
				gProject->mTimelineTracks[index] = gProject->mTimelineTracks[index-1];
				gProject->mTimelineTracks[index-1] = fContextTimelineTrack;
			}
			MedoWindow::GetInstance()->InvalidatePreview();
			break;
		}
		case kMessageContextTrackMoveDown:
		{
			int index = gProject->GetTimelineTrackIndex(fContextTimelineTrack);
			if ((index >= 0) && (index < (int)gProject->mTimelineTracks.size()))
			{
				gProject->Snapshot();
				gProject->mTimelineTracks[index] = gProject->mTimelineTracks[index+1];
				gProject->mTimelineTracks[index+1] = fContextTimelineTrack;
			}
			MedoWindow::GetInstance()->InvalidatePreview();
			break;
		}
		case kMessageContextTrackDelete:
		{
			gProject->Snapshot();
			gProject->RemoveTimelineTrack(fContextTimelineTrack);
			MedoWindow::GetInstance()->InvalidatePreview();
			break;
		}

		case kMessageContextClipEditTag:
		{
			BPoint mouse_pos = fMouseDownPoint;
			ConvertToScreen(&mouse_pos);
			fClipTagWindow = new ClipTagWindow(mouse_pos, ClipTagWindow::CLIP_TAG, this, fActiveClip.track->mClips[fActiveClip.clip_idx].mTag.String());	//	TODO design mechanism to close window
			fClipTagWindow->Show();
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;
		}
		case kMessageContextClipAddNote:
		{
			MediaNote aNote;
			aNote.mTimelineFrame = fLeftFrameIndex + fMouseDownPoint.x*fFramesPixel;
			aNote.mText.SetTo("Note");
			CalculateMediaNoteFrame(aNote);
			fActiveClip.track->mNotes.push_back(aNote);

			fActiveNote.track = fActiveClip.track;
			fActiveNote.frame_idx = aNote.mTimelineFrame;
			fActiveNote.note_idx = fActiveNote.track->mNotes.size() - 1;
			// fall through
		}
		case kMessageContextNoteEdit:
		{
			BPoint mouse_pos = fMouseDownPoint;
			ConvertToScreen(&mouse_pos);
			fClipTagWindow = new ClipTagWindow(mouse_pos, ClipTagWindow::CLIP_NOTE, this, fActiveNote.track->mNotes[fActiveNote.note_idx].mText.String());	//	TODO design mechanism to close window
			fClipTagWindow->Show();
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;
		}
		case kMessageContextNoteDelete:
			RemoveActiveNote();
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;

		case eMsgClipEditTagComplete:
		{
			const char *aString = nullptr;
			if (msg->FindString("tag", &aString) == B_OK)
			{
				assert(fActiveClip.track);
				fActiveClip.track->mClips[fActiveClip.clip_idx].mTag.SetTo(aString);
			}
			fClipTagWindow->Terminate();
			fClipTagWindow = nullptr;
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;
		}
		case eMsgClipEditNoteComplete:
		{
			const char *aString = nullptr;
			if (msg->FindString("tag", &aString) == B_OK)
			{
				assert(fActiveNote.track);
				fActiveNote.track->mNotes[fActiveNote.note_idx].mText.SetTo(aString);
				CalculateMediaNoteFrame(fActiveNote.track->mNotes[fActiveNote.note_idx]);
			}
			fClipTagWindow->Terminate();
			fClipTagWindow = nullptr;
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;
		}
		case eMsgClipEditTagCancelled:
		{
			fClipTagWindow->Terminate();
			fClipTagWindow = nullptr;
			gRenderActor->AsyncInvalidateTimelineEdit();
			break;
		}

		default:
#if 0
			printf("TimelineEdit::MessageReceived()\n");
			msg->PrintToStream();
#endif
			BView::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		TimelineEdit :: KeyDownMessage
	ARGS:			msg
	RETURN:			true if processed
	DESCRIPTION:	Called by MedoWindow, intercept keydown messages
*/
bool TimelineEdit :: KeyDownMessage(BMessage *msg)
{
	const char *bytes;
	msg->FindString("bytes",&bytes);
	switch (bytes[0])
	{
		case 0x7f:		//	delete
			switch (fSelectedItem)
			{
				case SelectedItem::eSelectedClip:
					gProject->Snapshot();
					assert(fActiveClip.track && (fActiveClip.clip_idx  >= 0));
					fActiveClip.track->RemoveClip(fActiveClip.track->mClips[fActiveClip.clip_idx], false);
					fActiveClip.Reset();
					gProject->InvalidatePreview();
					gRenderActor->AsyncInvalidateTimelineEdit();
					break;
				case SelectedItem::eSelectedEffect:
					gProject->Snapshot();
					assert(fActiveEffect.track && (fActiveEffect.effect_idx >= 0));
					fActiveEffect.track->RemoveEffect(fActiveEffect.media_effect);
					fActiveEffect.Reset();
					gProject->InvalidatePreview();
					gRenderActor->AsyncInvalidateTimelineEdit();
					break;
				case SelectedItem::eSelectedNote:
					gProject->Snapshot();
					assert(fActiveNote.track && (fActiveNote.note_idx >= 0));
					RemoveActiveNote();
					fActiveNote.Reset();
					gProject->InvalidatePreview();
					gRenderActor->AsyncInvalidateTimelineEdit();
					break;
				case SelectedItem::eSelectedNone:
					break;
			}
			fSelectedItem = SelectedItem::eSelectedNone;
			return true;

		case 'e':
		case 'E':
		{
			if (fActiveEffect.track && (fActiveEffect.effect_idx >= 0))
			{
				fActiveEffect.media_effect->mEnabled = !fActiveEffect.media_effect->mEnabled;
				gProject->InvalidatePreview();
				gRenderActor->AsyncInvalidateTimelineEdit();
				return true;
			}
			else
				return false;
		}

		default:
			return false;
	}
}

/*	FUNCTION:		TimelineEdit :: MouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse down
*/
void TimelineEdit :: MouseDown(BPoint point)
{
	if (!Window()->IsActive())
		Window()->Activate();

	MediaEffect *current_effect = fActiveEffect.media_effect;

	fActiveClip.Reset();
	fActiveEffect.Reset();
	fActiveNote.Reset();
	fSelectedItem = SelectedItem::eSelectedNone;

	uint32 buttons;
	BMessage* msg = Window()->CurrentMessage();
	msg->FindInt32("buttons", (int32*)&buttons);
	fMouseDownPoint = point;
	point.y += fScrollViewOffsetY;

	bool ctrl_modifier = ((MedoWindow *)Window())->GetKeyModifiers() & B_CONTROL_KEY;

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) && (fActiveCursor != CURSOR_DEFAULT))
	{
		be_app->SetCursor(B_HAND_CURSOR);
		fActiveCursor = CURSOR_DEFAULT;
		fState = State::eIdle;
	}

	//	Note selected?
	if (fTrackNotesVisible && FindNote(point))
	{
		fSelectedItem = SelectedItem::eSelectedNote;
		if (ctrl_modifier || (buttons & B_SECONDARY_MOUSE_BUTTON))
		{
			ContextMenuNote(fMouseDownPoint);
		}
		return;
	}

	//	Clip selected?
	ACTIVE_CLIP aClip;
	if (FindClip(point, aClip, kTimelineClipResizeGraceX))
	{
		fActiveClip = aClip;
		fContextTimelineTrack = aClip.track;
		fSelectedItem = SelectedItem::eSelectedClip;
		if (ctrl_modifier || (buttons & B_SECONDARY_MOUSE_BUTTON))
		{
			ContextMenuClip(fMouseDownPoint);
			return;
		}	
				
		if (fActiveCursor == CURSOR_RESIZE)
		{
			fState = State::eResizeClip;
			fActiveResizeDirection = fPendingResizeDirection;
		}
		else
		{
			fState = State::eMoveClip;
			be_app->SetCursor(fCursors[CURSOR_MOVE]);
			fActiveCursor = CURSOR_MOVE;
			FindClipLinkedEffects();
		}
		//fTimelineView->PositionUpdate(fLeftFrameIndex + point.x*fFramesPixel, true);
		return;
	}
	
	//	Effect selected?
	ACTIVE_EFFECT anEffect;
	if (FindEffect(point, anEffect, kTimelineClipResizeGraceX))
	{
		fActiveEffect = anEffect;
		fSelectedItem = SelectedItem::eSelectedEffect;
		if (ctrl_modifier || (buttons & B_SECONDARY_MOUSE_BUTTON))
		{
			ContextMenuEffect(fMouseDownPoint);
			return;
		}	
		
		if (fActiveCursor == CURSOR_RESIZE)
		{
			fState = State::eResizeEffect;
			fActiveResizeDirection = fPendingResizeDirection;
		}
		else
		{
			fState = State::eMoveEffect;
			be_app->SetCursor(fCursors[CURSOR_MOVE]);
			fActiveCursor = CURSOR_MOVE;

			MediaEffect *media_effect = anEffect.track->mEffects[anEffect.effect_idx];
			media_effect->mEffectNode->MediaEffectSelectedBase(media_effect);
		}
		//fTimelineView->PositionUpdate(fLeftFrameIndex + point.x*fFramesPixel, true);
		return;	
	}

	//	Generic context menu (eg. Add Note)
	if (ctrl_modifier || (buttons & B_SECONDARY_MOUSE_BUTTON))
	{
		fContextTimelineTrack = FindTimelineTrack(point);
		if (fContextTimelineTrack)
		{
			ContextMenuTrack(fMouseDownPoint);
			return;
		}
	}

	if (current_effect)
		current_effect->mEffectNode->MediaEffectDeselectedBase(current_effect);

}

/*	FUNCTION:		TimelineEdit :: MouseMoved
	ARGS:			point
					transit
					message
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse moved
*/
void TimelineEdit :: MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	switch (fState)
	{
		case State::eMoveClip:
			MoveClipUpdate(point);
			return;
		
		case State::eMoveEffect:
			MoveEffectUpdate(point);
			return;

		case State::eTimelineScrub:
			return;
		
		default:
			break;
	}

	point.y += fScrollViewOffsetY;

	int64 frame_idx = fLeftFrameIndex + point.x*fFramesPixel;

	switch (transit)
	{
		case B_ENTERED_VIEW:
			break;
			
		case B_EXITED_VIEW:
			if ((fActiveCursor == CURSOR_RESIZE) && (fActiveResizeDirection == ResizeDirection::eInactive))
			{
				be_app->SetCursor(B_HAND_CURSOR);
				fActiveCursor = CURSOR_DEFAULT;
				fState = State::eIdle;
			}
			break;

		case B_INSIDE_VIEW:
			if (fTrackNotesVisible && (fActiveCursor == CURSOR_DEFAULT) && FindNote(point))
				break;

			if ((fActiveCursor == CURSOR_DEFAULT) || (fActiveCursor == CURSOR_RESIZE))
			{
				ACTIVE_CLIP active_clip;
				ACTIVE_EFFECT anEffect;
				if (FindClip(point, active_clip, kTimelineClipResizeGraceX))
				{
					//	check if within kTimelineClipResizeGraceX
					MediaClip &media_clip = active_clip.track->mClips[active_clip.clip_idx];
					float left = (media_clip.mTimelineFrameStart - fLeftFrameIndex)/fFramesPixel;
					float right = (media_clip.GetTimelineEndFrame() - fLeftFrameIndex)/fFramesPixel;
					
					bool resize_left = false, resize_right = false;
					if ((point.x >= left - kTimelineClipResizeGraceX) && (point.x <= left + kTimelineClipResizeGraceX))
						resize_left = true;
					else if ((point.x >= right - kTimelineClipResizeGraceX) && (point.x <= right + kTimelineClipResizeGraceX))
						resize_right = true;
					if (resize_left || resize_right)
					{
						if (fActiveCursor == CURSOR_DEFAULT)
						{
							fActiveClip = active_clip;
							be_app->SetCursor(fCursors[CURSOR_RESIZE]);
							fActiveCursor = CURSOR_RESIZE;
							if (resize_left)
							{
								fPendingResizeDirection = ResizeDirection::eLeft;
								fResizeClipOriginalSourceFrame = media_clip.mSourceFrameStart;
							}
							else
							{
								fPendingResizeDirection = ResizeDirection::eRight;
								fResizeClipOriginalSourceFrame = media_clip.mSourceFrameEnd;
							}
							if (media_clip.mMediaSource->GetMediaType() != MediaSource::MEDIA_AUDIO)
								fResizeClipOriginalTimelineFrame = CalculateStickyFrameIndex(frame_idx, fPendingResizeDirection == ResizeDirection::eLeft);
							else
								fResizeClipOriginalTimelineFrame = frame_idx;
							fResizeObject = ResizeObject::eResizeClip;
						}
						break;
					}
					else
					{
						//	Setting tooltip dirties BView, and would require a redraw.  Try updating status window instead.
						//	SetToolTip("Hello");
					}
				}
				else if (FindEffect(point, anEffect, kTimelineClipResizeGraceX))
				{
					//	check if within kTimelineClipResizeGraceX
					MediaEffect *effect = anEffect.track->mEffects[anEffect.effect_idx];
					float left = (effect->mTimelineFrameStart - fLeftFrameIndex)/fFramesPixel;
					float right = (effect->mTimelineFrameStart - fLeftFrameIndex + effect->Duration())/fFramesPixel;
					
					bool resize_left = false, resize_right = false;
					if ((point.x >= left - kTimelineClipResizeGraceX) && (point.x <= left + kTimelineClipResizeGraceX))
						resize_left = true;
					else if ((point.x >= right - kTimelineClipResizeGraceX) && (point.x <= right + kTimelineClipResizeGraceX))
						resize_right = true;
					if (resize_left || resize_right)
					{
						if (fActiveCursor == CURSOR_DEFAULT)
						{
							fActiveEffect = anEffect;
							be_app->SetCursor(fCursors[CURSOR_RESIZE]);
							fActiveCursor = CURSOR_RESIZE;
							if (resize_left)
							{
								fPendingResizeDirection = ResizeDirection::eLeft;
								fResizeClipOriginalSourceFrame = effect->mTimelineFrameStart;
							}
							else
							{
								fPendingResizeDirection = ResizeDirection::eRight;
								fResizeClipOriginalSourceFrame = effect->mTimelineFrameEnd;
							}
							if (effect->Type() != MediaEffect::MEDIA_EFFECT_AUDIO)
								fResizeClipOriginalTimelineFrame = CalculateStickyFrameIndex(frame_idx, fPendingResizeDirection == ResizeDirection::eLeft);
							else
								fResizeClipOriginalTimelineFrame = frame_idx;
							fResizeObject = ResizeObject::eResizeEffect;
						}
						break;
					}
				}
				
				if ((fActiveCursor == CURSOR_RESIZE) && (fState == State::eIdle))
				{
					be_app->SetCursor(B_HAND_CURSOR);
					fActiveCursor = CURSOR_DEFAULT;
				}
				if (fState == State::eIdle)
					break;
			}
			//	else fall through
			
		case B_OUTSIDE_VIEW:
			if (fActiveCursor == CURSOR_RESIZE)
			{
				switch (fResizeObject)
				{
					case ResizeObject::eResizeClip:
						ResizeClipUpdate(point);
						break;
					case ResizeObject::eResizeEffect:
						ResizeEffectUpdate(point);
						break;
					default:
						assert(0);
				}
				Invalidate();			
			}
			break;
		default:
			assert(0); 	
	}
}

/*	FUNCTION:		TimelineEdit :: MouseUp
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse up
*/
void TimelineEdit :: MouseUp(BPoint point)
{
	switch (fState)
	{
		case State::eResizeClip:
		case State::eResizeEffect:
			fActiveResizeDirection = ResizeDirection::eInactive;
			break;

		default:
			break;
	}

	if (fActiveCursor != CURSOR_DEFAULT)
	{
		be_app->SetCursor(B_HAND_CURSOR);
		fActiveCursor = CURSOR_DEFAULT;
	}
	fState = State::eIdle;

	Invalidate();
}

/*	FUNCTION:		TimelineEdit :: ContextMenuTrack
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Show context menu for track
*/
void TimelineEdit :: ContextMenuTrack(BPoint point)
{
	ConvertToScreen(&point);

	//	Initialise PopUp menu
	BPopUpMenu *aPopUpMenu = new BPopUpMenu("ContextMenuClip", false, false);
	aPopUpMenu->SetAsyncAutoDestruct(true);

	BMenuItem *aMenuItem;

	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_INSERT_TRACK_ABOVE), new BMessage(kMessageContextTrackInsertAbove));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_INSERT_TRACK_BELOW), new BMessage(kMessageContextTrackInsertBelow));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_MOVE_TRACK_UP), new BMessage(kMessageContextTrackMoveUp));
	aPopUpMenu->AddItem(aMenuItem);
	if ((gProject->mTimelineTracks.size() == 0) || (gProject->mTimelineTracks[0] == fContextTimelineTrack))
		aMenuItem->SetEnabled(false);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_MOVE_TRACK_DOWN), new BMessage(kMessageContextTrackMoveDown));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_DELETE_TRACK), new BMessage(kMessageContextTrackDelete));
	aPopUpMenu->AddItem(aMenuItem);
	if (gProject->mTimelineTracks.size() <= 1)
		aMenuItem->SetEnabled(false);


	aPopUpMenu->SetTargetForItems(this);
	aPopUpMenu->Go(point, true /*notify*/, false /*stay open when mouse away*/, true /*async*/);

	//All BPopUpMenu items are freed when the popup is closed
}

/*	FUNCTION:		TimelineEdit :: ContextMenuClip
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Show context menu for clip
*/
void TimelineEdit :: ContextMenuClip(BPoint point)
{
	ConvertToScreen(&point);
	
	//	Initialise PopUp menu
	BPopUpMenu *aPopUpMenu = new BPopUpMenu("ContextMenuClip", false, false);
	aPopUpMenu->SetAsyncAutoDestruct(true);

	BMenuItem *aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_SPLIT_CLIP), new BMessage(kMessageContextClipSplit));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_DELETE_CLIP), new BMessage(kMessageContextClipDeleteLeaveEffects));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_DELETE_CLIP_AND_EFFECTS), new BMessage(kMessageContextClipDeleteRemoveEffects));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_EDIT_CLIP_TAG), new BMessage(kMessageContextClipEditTag));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_ADD_NOTE), new BMessage(kMessageContextClipAddNote));
	aPopUpMenu->AddItem(aMenuItem);

	const MediaClip &clip = fActiveClip.track->mClips[fActiveClip.clip_idx];
	if (clip.mMediaSource->GetVideoTrack())
	{
		aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_ENABLE_VIDEO), new BMessage(kMessageContextClipEnableVideo));
		if (clip.mVideoEnabled)
			aMenuItem->SetMarked(true);
		aPopUpMenu->AddItem(aMenuItem);
	}
	if (clip.mMediaSource->GetAudioTrack())
	{
		aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_ENABLE_AUDIO), new BMessage(kMessageContextClipEnableAudio));
		if (clip.mAudioEnabled)
			aMenuItem->SetMarked(true);
		aPopUpMenu->AddItem(aMenuItem);
	}

	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_CLIP_INFO), new BMessage(kMessageContextFileInfo));
	aPopUpMenu->AddItem(aMenuItem);

	//	Track items
	aPopUpMenu->AddSeparatorItem();
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_INSERT_TRACK_ABOVE), new BMessage(kMessageContextTrackInsertAbove));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_INSERT_TRACK_BELOW), new BMessage(kMessageContextTrackInsertBelow));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_MOVE_TRACK_UP), new BMessage(kMessageContextTrackMoveUp));
	aPopUpMenu->AddItem(aMenuItem);
	if ((gProject->mTimelineTracks.size() == 0) || (gProject->mTimelineTracks[0] == fContextTimelineTrack))
		aMenuItem->SetEnabled(false);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_MOVE_TRACK_DOWN), new BMessage(kMessageContextTrackMoveDown));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_DELETE_TRACK), new BMessage(kMessageContextTrackDelete));
	aPopUpMenu->AddItem(aMenuItem);
	if (gProject->mTimelineTracks.size() <= 1)
		aMenuItem->SetEnabled(false);

		
	aPopUpMenu->SetTargetForItems(this);
	aPopUpMenu->Go(point, true /*notify*/, false /*stay open when mouse away*/, true /*async*/);
	
	//All BPopUpMenu items are freed when the popup is closed 	
}

/*	FUNCTION:		TimelineEdit :: ContextMenuEffect
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Show context menu for effect
*/
void TimelineEdit :: ContextMenuEffect(BPoint point)
{
	ConvertToScreen(&point);

	//	Initialise PopUp menu
	BPopUpMenu *aPopUpMenu = new BPopUpMenu("ContextMenuEffect", false, false);
	aPopUpMenu->SetAsyncAutoDestruct(true);

	BMenuItem *aMenuItem;

	const MediaEffect *effect = fActiveEffect.media_effect;
	int64 frame_idx = effect->mTimelineFrameStart + fActiveEffect.frame_idx;

	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_MATCH_CLIP_DURATION), new BMessage(kMessageContextEffectStretchClipLength));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem->SetEnabled(fActiveEffect.track->mClips.size() > 0);

	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_MOVE_EFFECT_DOWN), new BMessage(kMessageContextEffectPriorityDown));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem->SetEnabled(effect->mPriority > 0);

	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_MOVE_EFFECT_UP), new BMessage(kMessageContextEffectPriorityUp));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem->SetEnabled(effect->mPriority != fActiveEffect.track->mNumberEffectLayers - 1);

	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_DELETE_EFFECT), new BMessage(kMessageContextEffectDeleteEffect));
	aPopUpMenu->AddItem(aMenuItem);

	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_ENABLE_EFFECT), new BMessage(kMessageContextEffectEnable));
	if (effect->mEnabled)
		aMenuItem->SetMarked(true);
	aPopUpMenu->AddItem(aMenuItem);

	aPopUpMenu->SetTargetForItems(this);
	aPopUpMenu->Go(point, true /*notify*/, false /*stay open when mouse away*/, true /*async*/);
}

/*	FUNCTION:		TimelineEdit :: ContextMenuNote
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Show context menu for effect
*/
void TimelineEdit :: ContextMenuNote(BPoint point)
{
	ConvertToScreen(&point);

	//	Initialise PopUp menu
	BPopUpMenu *aPopUpMenu = new BPopUpMenu("ContextMenuNote", false, false);
	aPopUpMenu->SetAsyncAutoDestruct(true);

	BMenuItem *aMenuItem;

	aMenuItem= new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_EDIT_NOTE), new BMessage(kMessageContextNoteEdit));
	aPopUpMenu->AddItem(aMenuItem);
	aMenuItem = new BMenuItem(GetText(TXT_TIMELINE_CONTEXT_DELETE_NOTE), new BMessage(kMessageContextNoteDelete));
	aPopUpMenu->AddItem(aMenuItem);

	aPopUpMenu->SetTargetForItems(this);
	aPopUpMenu->Go(point, true /*notify*/, false /*stay open when mouse away*/, true /*async*/);
}

/*	FUNCTION:		TimelineEdit :: OutputViewMouseDown
	ARGS:			point
	RETURN:			true if redraw required
	DESCRIPTION:	Mouse down initiated on Output View
*/
bool TimelineEdit::OutputViewMouseDown(const BPoint &point)
{
	if (fActiveEffect.media_effect)
	{
		fMsgOuptutViewMouseDown->ReplacePoint("point", point);
		fMsgOuptutViewMouseDown->ReplacePointer("effect", fActiveEffect.media_effect);
		EffectsWindow::GetInstance()->PostMessage(fMsgOuptutViewMouseDown);
		return true;
	}
	return false;
}

/*	FUNCTION:		TimelineEdit :: OutputViewMouseMoved
	ARGS:			point
	RETURN:			true if redraw required
	DESCRIPTION:	Mouse move initiated on Output View
*/
bool TimelineEdit::OutputViewMouseMoved(const BPoint &point)
{
	if (fActiveEffect.media_effect)
	{
		fMsgOuptutViewMouseMoved->ReplacePoint("point", point);
		fMsgOuptutViewMouseMoved->ReplacePointer("effect", fActiveEffect.media_effect);
		EffectsWindow::GetInstance()->PostMessage(fMsgOuptutViewMouseMoved);
		return true;
	}
	return false;
}

/*	FUNCTION:		TimelineEdit :: OutputViewZoomed
	ARGS:			point
	RETURN:			true if redraw required
	DESCRIPTION:	Zoom initiated on Output View
*/
bool TimelineEdit :: OutputViewZoomed(const float zoom_factor)
{
	if (fActiveEffect.media_effect)
	{
		fActiveEffect.media_effect->mEffectNode->OutputViewZoomed(fActiveEffect.media_effect);
		return true;
	}
	return false;
}

/*	FUNCTION:		TimelineEdit :: SetTrackShowClipTags
	ARGS:			visible
	RETURN:			n/a
	DESCRIPTION:	Show / Hide clip tags
*/
void TimelineEdit :: SetTrackShowClipTags(const bool visible)
{
	fClipTagsVisible = visible;
	Invalidate();
}

/*	FUNCTION:		TimelineEdit :: SetTrackShowNotes
	ARGS:			visible
	RETURN:			n/a
	DESCRIPTION:	Show / Hide clip tags
*/
void TimelineEdit :: SetTrackShowNotes(const bool visible)
{
	fTrackNotesVisible = visible;
	Invalidate();
}

/*	FUNCTION:		TimelineEdit :: SetShowAllVideoThumbnails
	ARGS:			show
	RETURN:			n/a
	DESCRIPTION:	Toggle drawing of video thumbnails
*/
void TimelineEdit :: SetShowAllVideoThumbnails(const bool show)
{
	fDrawAllVideoThumbnails = show;
	Invalidate();
}

/*	FUNCTION:		TimelineEdit :: ProjectInvalidated
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Invalidate cached values
*/
void TimelineEdit :: ProjectInvalidated()
{
	for (auto &track : gProject->mTimelineTracks)
	{
		for (auto &note : track->mNotes)
			CalculateMediaNoteFrame(note);
	}
	fActiveClip.Reset();
	fActiveEffect.Reset();
}

/*	FUNCTION:		TimelineEdit :: SetTimelineScrub
	ARGS:			enable
	RETURN:			n/a
	DESCRIPTION:	Notification that Timeline scrub active
					Used to prevent MouseMoved() from changing cursor/selecting fActiveClip/fActiveEffect
*/
void TimelineEdit :: SetTimelineScrub(const bool enable)
{
	if (enable)
		fState = State::eTimelineScrub;
	else
		fState = State::eIdle;
}

/******************************
	Note support
*******************************/

/*	FUNCTION:		TimelineEdit :: FindNote
	ARGS:			point
	RETURN:			true if note found
	DESCRIPTION:	See if point corresponds to displayed note
*/
bool TimelineEdit :: FindNote(const BPoint &point)
{
	float y_pos = kTimelineTrackInitialY;
	for (auto &track : gProject->mTimelineTracks)
	{
		float track_y = y_pos + track->mNumberEffectLayers*kTimelineEffectHeight + 0.5f*kTimelineTrackHeight;
		for (int32 ni = 0; ni < track->mNotes.size(); ni++)
		{
			const MediaNote &note = track->mNotes[ni];
			BPoint pos((note.mTimelineFrame - fLeftFrameIndex)/fFramesPixel, track_y);
			if ((point.x >= pos.x - note.mWidth) && (point.x <= pos.x + note.mWidth) &&
				(point.y >= pos.y - note.mHeight) && (point.y <= pos.y + note.mHeight))
			{
				fActiveNote.track = track;
				fActiveNote.note_idx = ni;
				fActiveNote.frame_idx = note.mTimelineFrame;
				fMouseDownPoint.y = track_y - fScrollViewOffsetY;
				return true;
			}
		}
		y_pos += kTimelineTrackDeltaY + (kTimelineTrackHeight + track->mNumberEffectLayers*kTimelineEffectHeight);

	}
	return false;
}

/*	FUNCTION:		TimelineEdit :: CalculateMediaNoteFrame
	ARGS:			note
	RETURN:			n/a
	DESCRIPTION:	Show / Hide clip tags
*/
void TimelineEdit :: CalculateMediaNoteFrame(MediaNote &note)
{
	font_height fh;
	be_plain_font->GetHeight(&fh);

	BStringList string_list;
	note.mText.Split("\n", true, string_list);

	float max_width = 0.0f;
	note.mTextWidths.clear();
	for (int si = 0; si < string_list.CountStrings(); si++)
	{
		float w = 1.1f*be_plain_font->StringWidth(string_list.StringAt(si).String());
		note.mTextWidths.push_back(w);
		 if (w > max_width)
			max_width = w;
	}
	assert(note.mTextWidths.size() == string_list.CountStrings());

	note.mWidth = 0.5f*max_width;
	if (note.mWidth < 32.0f)
		note.mWidth = 32.0f;
	float y_height = (fh.ascent + 0.5f*fh.descent)*1.025f * note.mTextWidths.size();
	note.mHeight = 0.5f*y_height;
	if (note.mHeight <= 1.0f*(fh.ascent + fh.descent))
		note.mHeight = 1.0f*(fh.ascent + fh.descent);
}

/*	FUNCTION:		TimelineEdit :: RemoveActiveNote
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Remove active node
*/
void TimelineEdit :: RemoveActiveNote()
{
	assert(fActiveNote.track && (fActiveNote.note_idx >= 0));
	for (std::vector<MediaNote>::iterator i = fActiveNote.track->mNotes.begin(); i != fActiveNote.track->mNotes.end(); i++)
	{
		if ((*i).mTimelineFrame == fActiveNote.frame_idx)
		{
			fActiveNote.track->mNotes.erase(i);
			fActiveNote.Reset();
			return;
		}
	}
	assert(0);
}
