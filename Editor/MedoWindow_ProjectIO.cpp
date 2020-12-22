/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Medo Window, project IO handling
 */
 
#include <cstdio>
#include <cassert>

#include <AppKit.h>
#include <StorageKit.h>
#include <InterfaceKit.h>

#include "AudioMixer.h"
#include "FileUtility.h"
#include "EffectsManager.h"
#include "EffectsWindow.h"
#include "EffectNode.h"
#include "ExportMediaWindow.h"
#include "Language.h"
#include "MediaSource.h"
#include "MedoWindow.h"
#include "OutputView.h"
#include "Project.h"
#include "ProjectSettings.h"
#include "RenderActor.h"
#include "SourceListView.h"
#include "StatusView.h"
#include "TabMainView.h"
#include "TimelineView.h"
#include "TimelineEdit.h"

#include "3rdParty/stb_image_write.h"

/*	FUNCTION:		MedoWindow :: CreateFilePanel
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Destroy file panel resources
*/
void MedoWindow :: CreateFilePanel()
{
	fFilePanel = nullptr;
	fFilePanelSaveProjectMessage = new BMessage(eMsgActionProjectSaveFilename);
	fFilePanelMode = FILE_PANEL_ADD_SOURCE;
	fPreviousFilePanelMode = FILE_PANEL_ADD_SOURCE;
}

/*	FUNCTION:		MedoWindow :: DestroyFilePanel
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Destroy file panel resource
*/
void MedoWindow :: DestroyFilePanel()
{
	delete fFilePanel;
	delete fFilePanelSaveProjectMessage;
}

/*	FUNCTION:		MedoWindow :: ProjectIOMessage
	ARGS:			msg
	RETURN:			none
	DESCRIPTION:	Process ProjectIO messages
*/
void MedoWindow :: ProjectIOMessage(BMessage *msg)
{
	switch (msg->what)
	{
		case eMsgMenuProjectNew:
		{
			BAlert *alert = new BAlert("MessageReceived", "File/New Project", "OK");
			alert->Go();
			break;	
		}
		case eMsgMenuProjectOpen:
		{
			if (!fFilePanel || (fFilePanel && (fPreviousFilePanelMode != FILE_PANEL_LOAD_PROJECT)))
			{
				delete fFilePanel;
				fFilePanel = new BFilePanel(B_OPEN_PANEL,	//	file_panel_mode
								nullptr,		//	target	
								nullptr,		//	directory
								0,				//	nodeFlavors 	
								false,			//	allowMultipleSelection 
								nullptr,		//	message 
								this,			//	refFilter 
								true,			//	modal 
								true);			//	hideWhenDone
				fFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, GetText(TXT_PROJECT_LOAD_OPEN_BUTTON));
				fFilePanel->Window()->SetTitle(GetText(TXT_PROJECT_LOAD_PROJECT_TITLE));
				fFilePanel->SetRefFilter(this);
				ReplaceFilePanelCancelMessage();
			}
			fFilePanelMode = FILE_PANEL_LOAD_PROJECT;
			fPreviousFilePanelMode = FILE_PANEL_LOAD_PROJECT;
			fFilePanel->Window()->ResizeBy(0, 0);
			fFilePanel->Show();
			break;	
		}
		case eMsgMenuProjectSave:
		{
			if (!fFilePanel || (fFilePanel && (fPreviousFilePanelMode != FILE_PANEL_SAVE_PROJECT)))
			{
				delete fFilePanel;
				fFilePanel = new BFilePanel(B_SAVE_PANEL,	//	file_panel_mode
									nullptr,		//	target
									nullptr,		//	directory
									0,				//	nodeFlavors
									false,			//	allowMultipleSelection
									fFilePanelSaveProjectMessage,		//	message
									this,			//	refFilter
									true,			//	modal
									true);			//	hideWhenDone
				fFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, GetText(TXT_PROJECT_SAVE_PROJECT_TITLE));
				fFilePanel->Window()->SetTitle(GetText(TXT_PROJECT_SAVE_PROJECT_TITLE));
				fFilePanel->SetRefFilter(this);
				fFilePanel->SetTarget(this);
				ReplaceFilePanelCancelMessage();
			}
			fFilePanelMode = FILE_PANEL_SAVE_PROJECT;
			fPreviousFilePanelMode = FILE_PANEL_SAVE_PROJECT;
			fFilePanel->Window()->ResizeBy(0, 0);
			fFilePanel->Show();	
			break;	
		}
		case eMsgActionProjectSaveFilename:
		{
			entry_ref ref;
			BString name;
			if ((msg->FindRef("directory", &ref) != B_OK) || (msg->FindString("name", &name) != B_OK))
			{
				BAlert *alert = new BAlert("Save Project Error", "BMessage::missing entry_ref(\"directory\")/string(\"name\")", "OK");
				alert->Go();
				break;
			}
			BEntry entry(&ref);
			if (entry.InitCheck() != B_NO_ERROR)
			{
				BAlert *alert = new BAlert("Save Project Error", "BMessage::invalid BEntry(directory)", "OK");
				alert->Go();
				break;
			}	
			BPath path;
			entry.GetPath(&path);
			//printf("Save Project: directory=%s, name=%s\n", path.Path(), name.String());

			if (fFilePanelMode == FILE_PANEL_EXPORT_FRAME)
			{
				if (name.FindLast(".bmp") == B_ERROR)
					name.Append(".bmp");
				path.Append(name);
				if (ExportFrame(&path))
					fStatusView->SetText(GetText(TXT_PROJECT_SAVE_PROJECT_SUCCESS));
				else
					fStatusView->SetText(GetText(TXT_PROJECT_SAVE_PROJECT_ERROR));
				return;
			}

			if (name.FindLast(".medo") == B_ERROR)
				name.Append(".medo");
			path.Append(name);

			FILE *file = fopen(path.Path(), "wb");
			if (file)
			{
				if (gProject->SaveProject(file))
				{
					fStatusView->SetText(GetText(TXT_PROJECT_SAVE_PROJECT_SUCCESS));
					SetMimeType(&path);
				}
				else
					fStatusView->SetText(GetText(TXT_PROJECT_SAVE_PROJECT_ERROR));
				fclose(file);
			}
			else
				fStatusView->SetText(GetText(TXT_PROJECT_SAVE_PROJECT_ERROR));
			break;
		}
		case eMsgMenuProjectAddSource:
		{
			if (!fFilePanel || (fFilePanel && (fPreviousFilePanelMode != FILE_PANEL_ADD_SOURCE)))
			{
				delete fFilePanel;
				fFilePanel = new BFilePanel(B_OPEN_PANEL,	//	file_panel_mode
								nullptr,		//	target	
								nullptr,		//	directory
								0,				//	nodeFlavors 	
								true,			//	allowMultipleSelection 
								nullptr,		//	message 
								this,			//	refFilter 
								true,			//	modal 
								true);			//	hideWhenDone
				fFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, GetText(TXT_PROJECT_ADD_MEDIA_BUTTON));
				fFilePanel->Window()->SetTitle(GetText(TXT_PROJECT_ADD_MEDIA_TITLE));
				fFilePanel->SetRefFilter(this);
				ReplaceFilePanelCancelMessage();
			}
			fFilePanelMode = FILE_PANEL_ADD_SOURCE;
			fPreviousFilePanelMode = FILE_PANEL_ADD_SOURCE;
			fFilePanel->Window()->ResizeBy(0, 0);
			fFilePanel->Show();
			break;
		}
		case eMsgMenuProjectExportFfmpeg:
		case eMsgMenuProjectExportMediaKit:
		{
			if (!fExportMediaWindow)
			{
				fExportMediaWindow = new ExportMediaWindow(this,
							msg->what == eMsgMenuProjectExportFfmpeg ? ExportMediaWindow::EXPORT_USING_FFMPEG : ExportMediaWindow::EXPORT_USING_MEDIA_KIT);
				fExportMediaWindow->Show();
			}
			break;
		}
		case eMsgMenuProjectExportFrame:
		{
			if (!fFilePanel || (fFilePanel && (fPreviousFilePanelMode != FILE_PANEL_EXPORT_FRAME)))
			{
				delete fFilePanel;
				fFilePanel = new BFilePanel(B_SAVE_PANEL,	//	file_panel_mode
									nullptr,		//	target
									nullptr,		//	directory
									0,				//	nodeFlavors
									false,			//	allowMultipleSelection
									fFilePanelSaveProjectMessage,		//	message
									this,			//	refFilter
									true,			//	modal
									true);			//	hideWhenDone
				fFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, GetText(TXT_PROJECT_SAVE_BUTTON));
				fFilePanel->Window()->SetTitle(GetText(TXT_MENU_PROJECT_EXPORT_FRAME));
				fFilePanel->SetRefFilter(this);
				fFilePanel->SetTarget(this);
				ReplaceFilePanelCancelMessage();
			}
			fFilePanelMode = FILE_PANEL_EXPORT_FRAME;
			fPreviousFilePanelMode = FILE_PANEL_EXPORT_FRAME;
			fFilePanel->Window()->ResizeBy(0, 0);
			fFilePanel->Show();
			break;
		}
		case eMsgActionEffectsFilePanelOpen:
		{
			EffectNode *effect_node = EffectsWindow::GetInstance()->GetCurrentEffectNode();
			if (!effect_node)
				break;

			if (!fFilePanel || (fFilePanel && (fPreviousFilePanelMode != FILE_PANEL_EFFECT)))
			{
				delete fFilePanel;
				fFilePanel = effect_node->CreateFilePanel(0);
				assert(fFilePanel != nullptr);
			}
			fFilePanelMode = FILE_PANEL_EFFECT;
			fPreviousFilePanelMode = FILE_PANEL_EFFECT;
			fFilePanel->Window()->ResizeBy(0, 0);
			fFilePanel->Show();
			break;
		}
		case eMsgActionFilePanelCancel:
			assert(fFilePanel);
			fFilePanelMode = FILE_PANEL_ADD_SOURCE;
			fFilePanel->Window()->PostMessage('Tcnl');	//	From kits/Tracker/Commands.h
			break;

		default:
			assert(0);
	}
}

/*	FUNCTION:		MedoWindow :: ReplaceFilePanelCancelMessage
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	FilePanel will not notify MedoWindow when cancelled,
					causing RefsReceived to fail on subsequent drag/drop
*/
void MedoWindow :: ReplaceFilePanelCancelMessage()
{
	assert(fFilePanel);
	BView *cancel_view = fFilePanel->Window()->FindView("cancel button");
	if (cancel_view)
	{
		BButton *cancel_button = (BButton *)cancel_view;
		cancel_button->SetMessage(new BMessage(eMsgActionFilePanelCancel));
		cancel_button->SetTarget(this, this);
	}
}

/*	FUNCTION:		MedoWindow :: RefsReceived
	ARGS:			message
	RETURN:			n/a
	DESCRIPTION:	Invoked when files dragged to window
*/
void MedoWindow :: RefsReceived(BMessage* message)
{
	entry_ref ref;
	for (int i = 0; message->FindRef("refs", i, &ref) == B_OK; i++)
	{
		BEntry entry(&ref, true);
		if (entry.InitCheck() == B_NO_ERROR)
		{
			BPath path;
			entry.GetPath(&path);

			//	TODO - work around Haiku issue #16441, Tracker fails to prepend Volume name in some scenarios causing resulting path to not resolve

			MediaSource *media_source = AddMediaSource(path.Path());
			UpdateIfNeeded();

			//	Add to timeline
			if ((i == 0) && media_source)
			{
				BPoint drop_point;
				if (message->FindPoint("_drop_point_", &drop_point) == B_OK)
				{
					BPoint converted_point = ConvertFromScreen(drop_point);
					if (converted_point.y > fDividerPositionY)
					{
						BMessage *drop_clip = new BMessage(TimelineEdit::eMsgDragDropClip);
						int64 clip_start = 0;
						drop_clip->AddInt64("start", clip_start);
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
						drop_clip->AddInt64("end", clip_end);
						drop_clip->AddPointer("source", media_source);
						drop_clip->AddInt64("xoffset", 0);
						drop_clip->AddPoint("_drop_point_", drop_point);
						fTimelineView->GetTimelineEdit()->DragDropClip(drop_clip);
						//	TODO delete drop_clip message
					}
				}
			}
		}
	}
}

/*	FUNCTION:		MedoWindow :: Filter
	ARGS:			ref
					node
					stat
					mimeType
	RETURN:			true if allowed
	DESCRIPTION:	Called by BFilePanel to filter items
*/
bool MedoWindow :: Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat, const char* mimeType)
{
	bool filter = true;
	switch (fFilePanelMode)
	{
		case FILE_PANEL_LOAD_PROJECT:
		case FILE_PANEL_SAVE_PROJECT:
		{
			BEntry entry(ref);
			if (!entry.IsDirectory())
			{
				if (strstr(ref->name, ".medo") == 0)
					filter = false;
			}
			break;
		}
		case FILE_PANEL_EXPORT_FRAME:
		{
			BEntry entry(ref);
			if (!entry.IsDirectory())
			{
				if (strstr(ref->name, ".bmp") == 0)
					filter = false;
			}
			break;
		}
		case FILE_PANEL_EFFECT:
			//	EffectNode requires custom BRefFilter
			assert(0);
			break;

		default:
			break;	
	}
	return filter;	
}

/*	FUNCTION:		MedoWindow :: ExportFrame
	ARGS:			path
	RETURN:			true if success
	DESCRIPTION:	Save screenshot
*/
bool MedoWindow :: ExportFrame(BPath *path)
{
	bool success = false;
	BBitmap *src = GetOutputView()->GetBitmap();
	if (src)
	{
		int32 bytes_per_row = src->BytesPerRow();
		int32 bits_length = src->BitsLength();
		int32 width = bytes_per_row/4;
		int32 height = bits_length/bytes_per_row;

		//	Need to convert BGR to RGB for STBI
		src->Lock();
		BBitmap dest(src->Bounds(), B_RGB32);
		uint32 *s = (uint32 *)src->Bits();
		uint32 *d = (uint32 *)dest.Bits();
		for (int32 row=0; row < height; row++)
		{
			for (int32 col=0; col < width; col++)
			{
				uint32 v = *s++;
				uint8_t b = v & 0xff;
				uint8_t g = (v & 0xff00) >> 8;
				uint8_t r = (v & 0xff0000) >> 16;
				uint8_t a = (v & 0xff000000) >> 24;
				*d++ = (a << 24) | (b << 16) | (g << 8) | r;
			}
		}
		src->Unlock();
		success = stbi_write_bmp(path->Path(), width, height, 4, dest.Bits());
	}
	return success;
}
	
/*	FUNCTION:		MedoWindow :: AddMediaSource
	ARGS:			path
	RETURN:			none
	DESCRIPTION:	Add source media to fMedoTabView->SourceListView
*/
MediaSource * MedoWindow :: AddMediaSource(const char *path)
{
	MediaSource *source = nullptr;
	if (path)
	{
		//printf("MedoWindow::AddMediaSource() fFilePanelMode = %d\n", fFilePanelMode);
		switch (fFilePanelMode)
		{
			case FILE_PANEL_LOAD_PROJECT:
				LoadProject(path);
				break;
		
			case FILE_PANEL_SAVE_PROJECT:
				printf("MedoWindow :: AddMediaSource(FILE_PANEL_SAVE_PROJECT) - Shouldn't see this\n");
				break;
			case FILE_PANEL_EXPORT_FRAME:
				printf("MedoWindow :: AddMediaSource(FILE_PANEL_EXPORT_FRAME) - Shouldn't see this\n");
				break;

			case FILE_PANEL_EFFECT:
			{
				EffectNode *effect_node = EffectsWindow::GetInstance()->GetCurrentEffectNode();
				effect_node->FilePanelOpen(path);
				break;
			}

			case FILE_PANEL_ADD_SOURCE:
			{
				bool is_new;
				source = fProject->AddMediaSource(path, is_new);
				if (source && is_new)
				{
					SourceListItem *item = new SourceListItem(source);
					fTabMainView->GetSourceListView()->AddItem(item);
				}
				break;
			}
			default:
				assert(0);
		}
	}
	fFilePanelMode = FILE_PANEL_ADD_SOURCE;
	fTimelineView->InvalidateItems(-1);
	return source;
}

/*	FUNCTION:		MedoWindow :: RemoveAllMediaSources
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when new project loaded
*/
void MedoWindow :: RemoveAllMediaSources()
{
	fTabMainView->GetSourceListView()->RemoveAllMediaSources();
}

/*	FUNCTION:		MedoWindow :: LoadProject
	ARGS:			filename
	RETURN:			n/a
	DESCRIPTION:	Load project
*/
void MedoWindow :: LoadProject(const char *filename)
{
	fFilePanelMode = FILE_PANEL_ADD_SOURCE;		//	reset state for LoadProject() since it will add media source
	char *data = ReadFileToBuffer(filename);
	if (data)
	{
		gRenderActor->WaitIdle();
		gProject->ResetSnapshots(false);
		bool success = gProject->LoadProject(data, true);
		delete [] data;		//	TODO Add snapshot
		if (success)
		{
			LoadProjectSuccess(GetText(TXT_PROJECT_LOAD_PROJECT_SUCCESS));
		}
		gProject->ResetSnapshots(true);
	}
	fFilePanelMode = FILE_PANEL_ADD_SOURCE;
}

/*	FUNCTION:		MedoWindow :: LoadProjectSuccess
	ARGS:			status_msg
	RETURN:			n/a
	DESCRIPTION:	Load project success
*/
void MedoWindow :: LoadProjectSuccess(const char *status_msg)
{
	//	Init project
	SetActiveControl(CONTROL_OUTPUT);
	fStatusView->SetText(status_msg);
	fTimelineView->ProjectLoaded();
	if (fAudioMixer)
		fAudioMixer->PostMessage(AudioMixer::kMsgProjectInvalidated);
}

/*****************************************
	Project MIME type
******************************************/
static const uint8 kIconName[] = {
	0x6e, 0x63, 0x69, 0x66, 0x07, 0x05, 0x00, 0x04, 0x00, 0x5c, 0x02, 0x01,
	0x06, 0x02, 0x3d, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d,
	0xc0, 0x00, 0x4a, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0xff, 0x80, 0x8f,
	0xff, 0xff, 0x17, 0x3f, 0x02, 0x01, 0x06, 0x02, 0x3d, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x80, 0x00, 0x4a, 0x00, 0x00, 0x49,
	0x00, 0x00, 0xc7, 0xff, 0xff, 0xff, 0xff, 0xe1, 0xf5, 0xf3, 0x02, 0x01,
	0x06, 0x02, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a,
	0x00, 0x00, 0x4a, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x93, 0xd7, 0xb4,
	0xff, 0x03, 0xa1, 0x51, 0x02, 0x01, 0x06, 0x02, 0x39, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x4a, 0x90, 0x00, 0x47,
	0xc0, 0x00, 0x00, 0x83, 0xca, 0xe8, 0xff, 0x05, 0x94, 0xd0, 0x02, 0x01,
	0x06, 0x02, 0x3b, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b,
	0x40, 0x00, 0x49, 0x00, 0x00, 0x47, 0xc0, 0x00, 0x00, 0xff, 0x8c, 0x9f,
	0xff, 0xff, 0x1b, 0x41, 0x11, 0x0a, 0x04, 0x27, 0x27, 0x59, 0x27, 0x59,
	0x59, 0x27, 0x59, 0x02, 0x0c, 0x28, 0x3c, 0x28, 0x46, 0x28, 0x39, 0x2a,
	0x35, 0x29, 0x37, 0x26, 0x34, 0x24, 0x2d, 0x24, 0x31, 0x24, 0x28, 0x2d,
	0x24, 0x29, 0x24, 0x31, 0x24, 0x36, 0x2c, 0x36, 0x27, 0x39, 0x2b, 0x40,
	0x2a, 0x3d, 0x2a, 0x43, 0x2a, 0x4a, 0x2c, 0x47, 0x2b, 0x4a, 0x27, 0x52,
	0x24, 0x4e, 0x24, 0x57, 0x24, 0x5b, 0x2d, 0x5b, 0x28, 0x5b, 0x31, 0x55,
	0x35, 0x59, 0x34, 0x56, 0x37, 0x57, 0x3c, 0x57, 0x39, 0x57, 0x46, 0x40,
	0x4f, 0x4d, 0x4f, 0x33, 0x4f, 0x02, 0x0e, 0x52, 0x3c, 0x52, 0x3a, 0x52,
	0x44, 0x40, 0x4a, 0x4a, 0x4a, 0x36, 0x4a, 0x2d, 0x3c, 0x2d, 0x44, 0x2d,
	0x3a, 0x2f, 0x35, 0x2f, 0x35, 0x2c, 0x34, 0x2a, 0x2f, 0x2a, 0x32, 0x2a,
	0x2c, 0x30, 0x29, 0x2d, 0x29, 0x32, 0x29, 0x35, 0x2d, 0x35, 0x2a, 0x35,
	0x2d, 0x37, 0x30, 0x37, 0x30, 0x39, 0x2f, 0x40, 0x2e, 0x3d, 0x2e, 0x43,
	0x2e, 0x49, 0x30, 0x47, 0x2f, 0x49, 0x30, 0x4b, 0x2d, 0x4b, 0x2d, 0x4b,
	0x2a, 0x50, 0x29, 0x4e, 0x29, 0xc7, 0x77, 0x29, 0x55, 0x2f, 0x55, 0x2c,
	0x55, 0x32, 0x51, 0x35, 0x54, 0x34, 0x51, 0x35, 0x08, 0x02, 0x35, 0x2d,
	0x37, 0x2f, 0x08, 0x02, 0x2b, 0x34, 0x2f, 0x35, 0x08, 0x02, 0x35, 0x4c,
	0x37, 0x49, 0x08, 0x02, 0x3d, 0x4a, 0x3c, 0x4e, 0x08, 0x02, 0x25, 0x2f,
	0x2a, 0x30, 0x08, 0x02, 0x27, 0x28, 0x2c, 0x2b, 0x08, 0x02, 0x2e, 0x25,
	0x31, 0x29, 0x08, 0x02, 0x29, 0x3b, 0x2e, 0x3b, 0x0a, 0x02, 0x2b, 0x43,
	0x2f, 0x41, 0x08, 0x02, 0x30, 0x48, 0x33, 0x46, 0x08, 0x02, 0x3c, 0x2e,
	0x3b, 0x2b, 0x02, 0x04, 0xbc, 0x5d, 0x2f, 0xbd, 0xcb, 0x2f, 0xba, 0xee,
	0x2f, 0x31, 0xbb, 0x91, 0x31, 0xba, 0x22, 0x31, 0xbc, 0xff, 0xbc, 0x5d,
	0x3c, 0xba, 0xee, 0x3c, 0xbd, 0xcb, 0x3c, 0x3e, 0xbb, 0x91, 0x3e, 0xbc,
	0xff, 0x3e, 0xba, 0x22, 0x0a, 0x04, 0x43, 0x30, 0x4e, 0x30, 0x4e, 0x3b,
	0x43, 0x3b, 0x06, 0x05, 0xca, 0x01, 0x40, 0x4b, 0x39, 0x41, 0x47, 0x40,
	0x4b, 0xc0, 0xad, 0xc2, 0xce, 0x40, 0x4b, 0x4b, 0x0e, 0x0a, 0x00, 0x01,
	0x00, 0x10, 0x01, 0x17, 0x8e, 0x02, 0x04, 0x0a, 0x02, 0x01, 0x00, 0x10,
	0x01, 0x15, 0x8a, 0x02, 0x04, 0x0a, 0x01, 0x01, 0x01, 0x20, 0x20, 0x24,
	0x0a, 0x00, 0x01, 0x01, 0x00, 0x0a, 0x03, 0x01, 0x01, 0x10, 0x01, 0x15,
	0x7c, 0x00, 0x04, 0x0a, 0x00, 0x01, 0x02, 0x10, 0x01, 0x17, 0x81, 0x00,
	0x04, 0x0a, 0x00, 0x0b, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	0x0b, 0x0c, 0x0d, 0x10, 0x01, 0x17, 0x81, 0x00, 0x04, 0x0a, 0x00, 0x0b,
	0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x12,
	0xc0, 0x10, 0xd6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
	0x4b, 0xfe, 0xaf, 0x00, 0x00, 0x00, 0x01, 0x17, 0x81, 0x00, 0x04, 0x0a,
	0x00, 0x01, 0x0e, 0x10, 0x01, 0x17, 0x82, 0x00, 0x04, 0x0a, 0x06, 0x01,
	0x0e, 0x00, 0x0a, 0x00, 0x01, 0x0f, 0x10, 0x01, 0x17, 0x84, 0x02, 0x04,
	0x0a, 0x05, 0x01, 0x0f, 0x10, 0x01, 0x15, 0x82, 0x02, 0x04, 0x0a, 0x00,
	0x01, 0x10, 0x10, 0x01, 0x17, 0x84, 0x22, 0x04, 0x0a, 0x04, 0x01, 0x10,
	0x10, 0x01, 0x15, 0x82, 0x02, 0x04
};

/*	FUNCTION:		MedoWindow :: SetMimeType
	ARGS:			entry
	RETURN:			n/a
	DESCRIPTION:	Set project file mime type
*/
void MedoWindow :: SetMimeType(BPath *path)
{
	BNode node(path->Path());
	BNodeInfo node_info(&node);
	node_info.SetType("text/Medo");
	node_info.SetPreferredApp("application/x-vnd.ZenYes.Medo");
	node_info.SetIcon(kIconName, sizeof(kIconName)/sizeof(unsigned char));
}

