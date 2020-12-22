/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Medo Window
 */
 
#include <cstdio>
#include <cassert>

#include <AppKit.h>
#include <StorageKit.h>
#include <InterfaceKit.h>

#include "Gui/DividerView.h"

#include "AboutWindow.h"
#include "AudioManager.h"
#include "AudioMixer.h"
#include "ControlSource.h"
#include "ColourScope.h"
#include "EffectNode.h"
#include "EffectsTab.h"
#include "EffectsWindow.h"
#include "ExportMediaWindow.h"
#include "Language.h"
#include "MediaSource.h"
#include "MonitorWindow.h"
#include "OutputView.h"
#include "Project.h"
#include "ProjectSettings.h"
#include "RenderActor.h"
#include "SettingsWindow.h"
#include "SourceListView.h"
#include "StatusView.h"
#include "TabMainView.h"
#include "TextTab.h"
#include "TimelineEdit.h"
#include "TimelineView.h"


#include "MedoWindow.h"

MedoWindow * MedoWindow::sMedoWindow = nullptr;

static const float	kWindowOffsetX				= 40.0f;
static const float	kWindowOffsetY				= 40.0f;

static const float	kMenuBarHeight				= 18.0f;						//	actually varies based on user preferences
static const float	kControlExtraHeight			= 60.0f;						//	clip preview timeline height (9*4+2+2)
static const float	kControlViewWidth			= 960.0f;						//	0.5*1920 HD width 16:9
static const float	kControlViewHeight			= 540.0f+kControlExtraHeight;	//	0.5*1080 HD height 16:9
static const float	kTabViewWidth				= 480.0f;						//	arbitrary
static const float	kTabViewHeight				= kControlViewHeight;	
static const float	kTimeViewWidth				= kTabViewWidth + kControlViewWidth;
static const float	kTimeViewHeight				= 0.4f*kControlViewWidth;		//	also includes B_H_SCROLL_BAR_HEIGHT
static const float	kDividerViewHeight			= 6.0f;
static const float	kStatusViewWidth			= 200;

/*	FUNCTION:		MedoWindow :: MedoWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
MedoWindow :: MedoWindow()
	: BWindow(BRect(kWindowOffsetX, kWindowOffsetY, kWindowOffsetX+kTimeViewWidth, kWindowOffsetY+kMenuBarHeight+kControlViewHeight+kDividerViewHeight+kTimeViewHeight),
			"Medo Window", B_DOCUMENT_WINDOW, B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS)
{
	assert(sMedoWindow == nullptr);
	sMedoWindow = this;

	LoadSettings();
	SetTitle(GetText(TXT_MENU_MEDO));

	fProject = new Project;
	CreateFilePanel();

	fAboutWindow = nullptr;
	fAudioMixer = nullptr;
	fColourScope = nullptr;
	fExportMediaWindow = nullptr;
	fMonitorWindow = nullptr;
	fProjectSettings = nullptr;
	fSettingsWindow = nullptr;

	new RenderActor(BRect(0, 0, gProject->mResolution.width, gProject->mResolution.height));

    BScreen screen;
    SetSizeLimits(0.5f*(kTimeViewWidth), screen.Frame().Width()*1.25f, 0.5f*(kMenuBarHeight + kControlViewHeight + kTimeViewHeight), screen.Frame().Height()*1.25f);
	
	//	Menu Bar
	fMenuBar = new BMenuBar(BRect(0, 0, 0, 0), "MenuBar");
	AddChild(fMenuBar);
	const float menu_height = fMenuBar->Frame().Height();
	
	BMenu *medo_menu = new BMenu(GetText(TXT_MENU_MEDO));
	fMenuBar->AddItem(medo_menu);
	medo_menu->AddItem(new BMenuItem(GetText(TXT_MENU_MEDO_ABOUT), new BMessage(eMsgMenuMedoAbout)));
	medo_menu->AddItem(new BMenuItem(GetText(TXT_MENU_MEDO_SETTINGS), new BMessage(eMsgMenuMedoSettings), '.'));
	medo_menu->AddItem(new BMenuItem(GetText(TXT_MENU_MEDO_QUIT), new BMessage(B_QUIT_REQUESTED), 'Q'));
	
	BMenu *menu_project = new BMenu(GetText(TXT_MENU_PROJECT));
	fMenuBar->AddItem(menu_project);
	menu_project->AddItem(new BMenuItem(GetText(TXT_MENU_PROJECT_NEW), new BMessage(eMsgMenuProjectNew), 'N'));
	menu_project->AddItem(new BMenuItem(GetText(TXT_MENU_PROJECT_OPEN), new BMessage(eMsgMenuProjectOpen), 'O'));
	menu_project->AddItem(new BMenuItem(GetText(TXT_MENU_PROJECT_SAVE), new BMessage(eMsgMenuProjectSave), 'S'));
	menu_project->AddItem(new BMenuItem(GetText(TXT_MENU_PROJECT_SETTINGS), new BMessage(eMsgMenuProjectSettings)));
	menu_project->AddSeparatorItem();
	menu_project->AddItem(new BMenuItem(GetText(TXT_MENU_PROJECT_ADD_SOURCE), new BMessage(eMsgMenuProjectAddSource)));
	menu_project->AddSeparatorItem();
	menu_project->AddItem(new BMenuItem(GetText(TXT_MENU_PROJECT_EXPORT_FFMPEG), new BMessage(eMsgMenuProjectExportFfmpeg), 'E'));
	menu_project->AddItem(fMenuItemExportMediaKit = new BMenuItem(GetText(TXT_MENU_PROJECT_EXPORT_MEDIA_KIT), new BMessage(eMsgMenuProjectExportMediaKit)));
	fMenuItemExportMediaKit->SetEnabled(GetSettings()->export_enable_media_kit);
	menu_project->AddItem(new BMenuItem(GetText(TXT_MENU_PROJECT_EXPORT_FRAME), new BMessage(eMsgMenuProjectExportFrame)));

	BMenu *menu_edit = new BMenu(GetText(TXT_MENU_EDIT));
	fMenuBar->AddItem(menu_edit);
	fMenuItemEditUndo = new BMenuItem(GetText(TXT_MENU_EDIT_UNDO), new BMessage(eMsgMenuEditUndo), 'Z');
	fMenuItemEditUndo->SetEnabled(false);
	menu_edit->AddItem(fMenuItemEditUndo);
	fMenuItemEditRedo = new BMenuItem(GetText(TXT_MENU_EDIT_REDO), new BMessage(eMsgMenuEditRedo), 'Y');
	fMenuItemEditRedo->SetEnabled(false);
	menu_edit->AddItem(fMenuItemEditRedo);
	
	BMenu *menu_view = new BMenu(GetText(TXT_MENU_VIEW));
	fMenuBar->AddItem(menu_view);
	BMenu *submenu_layout = new BMenu(GetText(TXT_MENU_VIEW_LAYOUT));
	submenu_layout->AddItem(new BMenuItem(GetText(TXT_MENU_VIEW_LAYOUT_LARGE_PREVIEW), new BMessage(eMsgMenuViewLayout_1), '1'));
	submenu_layout->AddItem(new BMenuItem(GetText(TXT_MENU_VIEW_LAYOUT_LARGE_TIMELINE), new BMessage(eMsgMenuViewLayout_2), '2'));
	submenu_layout->AddItem(new BMenuItem(GetText(TXT_MENU_VIEW_LAYOUT_COLOUR_GRADING), new BMessage(eMsgMenuViewLayout_3), '3'));
	submenu_layout->AddItem(new BMenuItem(GetText(TXT_MENU_VIEW_LAYOUT_AUDIO_EDIT), new BMessage(eMsgMenuViewLayout_4), '4'));
	menu_view->AddItem(submenu_layout);
	fMenuItemViewShowClipTags = new BMenuItem(GetText(TXT_MENU_VIEW_SHOW_CLIP_TAGS), new BMessage(eMsgMenuViewShowClipTags));
	fMenuItemViewShowClipTags->SetMarked(true);
	menu_view->AddItem(fMenuItemViewShowClipTags);
	fMenuItemViewShowNotes = new BMenuItem(GetText(TXT_MENU_VIEW_SHOW_NOTES), new BMessage(eMsgMenuViewShowNotes));
	fMenuItemViewShowNotes->SetMarked(true);
	menu_view->AddItem(fMenuItemViewShowNotes);
	fMenuItemViewShowThumbnails = new BMenuItem(GetText(TXT_MENU_VIEW_SHOW_THUMBNAILS), new BMessage(eMsgMenuViewShowThumbnails));
	fMenuItemViewShowThumbnails->SetMarked(true);
	menu_view->AddItem(fMenuItemViewShowThumbnails);

	BMenu *menu_tools = new BMenu(GetText(TXT_MENU_TOOLS));
	fMenuBar->AddItem(menu_tools);
	menu_tools->AddItem(new BMenuItem(GetText(TXT_MENU_TOOLS_MONITOR), new BMessage(eMsgMenuToolsMonitor), 'F'));
	menu_tools->AddItem(new BMenuItem(GetText(TXT_MENU_TOOLS_COLOUR_SCOPE), new BMessage(eMsgMenuToolsColourScope)));
	menu_tools->AddItem(new BMenuItem(GetText(TXT_MENU_TOOLS_AUDIO_MIXER), new BMessage(eMsgMenuToolsAudioMixer)));
	menu_tools->AddItem(new BMenuItem(GetText(TXT_MENU_TOOLS_SOUND_RECORDER), new BMessage(eMsgMenuToolsSoundRecorder)));
	
    const float kFontFactor = be_plain_font->Size()/20.0f;
    BRect control_rect(kTabViewWidth*kFontFactor, menu_height, kTabViewWidth*kFontFactor + kControlViewWidth, kControlViewHeight+menu_height);

	//	Tab Main view
    fTabMainView = new TabMainView(BRect(0, menu_height, kTabViewWidth*kFontFactor, kTabViewHeight+menu_height));
	AddChild(fTabMainView);

	//	Control view
	fControlViews[CONTROL_SOURCE] = new ControlSource(control_rect);
	fControlViews[CONTROL_OUTPUT] = new OutputView(control_rect);

	fControlMode = CONTROL_OUTPUT;
	AddChild(fControlViews[fControlMode]);

	//	Timeline view
	BRect frame = Bounds();
	fTimelineView = new TimelineView(BRect(frame.left, frame.bottom - kTimeViewHeight , frame.right, frame.bottom), this);
	((OutputView *)fControlViews[CONTROL_OUTPUT])->SetTimelineView(fTimelineView);
	AddChild(fTimelineView);

	//	Divider View
	fDividerMessage = new BMessage(eMsgActionDividerResize);
	fDividerPositionY = frame.bottom - kTimeViewHeight - kDividerViewHeight;
	fDividerAspectY = fDividerPositionY/frame.Height();
	fDividerView = new DividerView(BRect(frame.left, fDividerPositionY, frame.right, fDividerPositionY + kDividerViewHeight), fDividerMessage);
	AddChild(fDividerView);
	
	//	Status view
	fStatusView = new StatusView(BRect(frame.left, frame.bottom - (B_H_SCROLL_BAR_HEIGHT+4), kStatusViewWidth, frame.bottom));
	AddChild(fStatusView);
	
	FrameResized(frame.Width(), frame.Height());

	//	Caution - if audio construction happens earlier, BSoundPlayer/BBufferGroup crash on exit (reported on Haiku bug tracker)
	assert(gAudioManager == nullptr);
	gAudioManager = new AudioManager;
}

/*	FUNCTION:		MedoWindow :: ~MedoWindow
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
MedoWindow :: ~MedoWindow()
{
	delete gAudioManager;
	gAudioManager = nullptr;

	delete gRenderActor;

	DestroyFilePanel();

	if (fAboutWindow)
		fAboutWindow->Terminate();
	if (fAudioMixer)
		fAudioMixer->Terminate();
	if (fColourScope)
		fColourScope->Terminate();
	if (fExportMediaWindow)
		fExportMediaWindow->PostMessage(B_QUIT_REQUESTED);
	if (fMonitorWindow)
		fMonitorWindow->Terminate();
	if (fProjectSettings)
		fProjectSettings->Terminate();
	if (fSettingsWindow)
		fSettingsWindow->Terminate();
	
	for (int i=0; i < NUMBER_CONTROL_MODES; i++)
	{
		if (i != fControlMode)
			delete fControlViews[i];
	}
	delete fProject;
	fProject = nullptr;

	//	attached views cleaned up automatically
	printf("MedoWindow::~MedoWindow()\n");
}

/*	FUNCTION:		MedoWindow :: FrameResized
	ARGS:			width
					height
	RETURN:			none
	DESCRIPTION:	Hook function
*/
void MedoWindow :: FrameResized(float width, float height)
{
	fDividerPositionY = height * fDividerAspectY;
	ResizeWindow();
}

/*	FUNCTION:		MedoWindow :: ResizeWindow
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Hook function when frame resized or divider moved
*/
void MedoWindow :: ResizeWindow()
{
	BRect frame = Bounds();
	const float menu_height = fMenuBar->Frame().Height();
    const float kFontFactor = be_plain_font->Size()/20.0f;
    const float tab_view_width = (frame.Width() > 2*kTabViewWidth*kFontFactor) ? kTabViewWidth*kFontFactor : 0.5f*frame.Width();
	if (fDividerPositionY > 0.75f*frame.Height())
		fDividerPositionY = 0.75f*frame.Height();
	if (fDividerPositionY < 0.25f*frame.Height())
		fDividerPositionY = 0.25f*frame.Height();

	fTabMainView->ResizeTo(tab_view_width, fDividerPositionY - menu_height);
	fControlViews[fControlMode]->ResizeTo(frame.Width() - tab_view_width, fDividerPositionY - menu_height);
	fControlViews[fControlMode]->MoveTo(tab_view_width, menu_height);
	fDividerView->ResizeTo(frame.Width(), kDividerViewHeight);
	fDividerView->MoveTo(0, fDividerPositionY);
	fTimelineView->ResizeTo(frame.Width(), frame.Height() - (fDividerPositionY+kDividerViewHeight));
	fTimelineView->MoveTo(0, fDividerPositionY + kDividerViewHeight);
	fDividerAspectY = fDividerPositionY / frame.Height();
}

/*	FUNCTION:		MedoWindow :: QuitRequested
	ARGS:			msg
	RETURN:			none
	DESCRIPTION:	Hook function
*/
bool MedoWindow :: QuitRequested()
{
	fTimelineView->PlayComplete();
	if (fExportMediaWindow && !fExportMediaWindow->QuitRequested())
	{
		BAlert *alert = new BAlert("Quit Requested", "Export Media Window is busy", "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return false;
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

/*	FUNCTION:		MedoWindow :: MessageReceived
	ARGS:			msg
	RETURN:			none
	DESCRIPTION:	Hook function
*/
void MedoWindow :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case eMsgActionAsyncPreviewReady:
		{
			BBitmap *bitmap;
			if (msg->FindPointer("BBitmap", (void **)&bitmap) == B_OK)
			{
				if (fMonitorWindow && !fMonitorWindow->IsHidden())
				{
					fMonitorWindow->PostMessage(msg);
					if (fMonitorWindow->IsFullscreen())
						break;
				}

				((OutputView *)fControlViews[CONTROL_OUTPUT])->SetBitmap(bitmap);
				fControlViews[CONTROL_OUTPUT]->Invalidate();

				if (fColourScope && !fColourScope->IsHidden())
					fColourScope->PostMessage(msg);
			}
			else
				printf("MedoWindow::MessageReceived(eMsgActionAsyncPreviewReady) - error pointer not found\n");
			break;
		}
		case eMsgActionAsyncTimelinePlayerUpdate:
		{
			int64 pos;
			if (msg->FindInt64("Position", &pos) == B_OK)
				fTimelineView->PositionUpdate(pos, false);
			else
				printf("MedoWindow::MessageReceived(eMsgActionAsyncTimelinePlayerUpdate) - error \"Position\" not found\n");

			bool complete;
			if (msg->FindBool("Complete", &complete) == B_OK)
			{
				if (complete)
					fTimelineView->PlayComplete();
			}
			else
				printf("MedoWindow::MessageReceived(eMsgActionAsyncTimelinePlayerUpdate) - error \'Complete\' not found\n");
			break;
		}
		case eMsgActionAsyncThumbnailReady:
			fTimelineView->InvalidateItems(TimelineView::INVALIDATE_EDIT_TRACKS);		//	TimelineEdit
			if (fControlMode == CONTROL_SOURCE)
				((ControlSource *)fControlViews[CONTROL_SOURCE])->ShowPreview(fTimelineView->GetCurrrentFrame());
			break;

		//	Menu Medo
		case eMsgMenuMedoAbout:
		{
			if (fAboutWindow == nullptr)
				fAboutWindow = new AboutWindow(BRect(64, 64, 800, 640), GetText(TXT_MENU_MEDO_ABOUT));
			fAboutWindow->Show();
			break;	
		}
		case eMsgMenuMedoSettings:
		{
			if (!fSettingsWindow)
				fSettingsWindow = new SettingsWindow(BRect(40, 40, 440, 340), GetText(TXT_MENU_MEDO_SETTINGS));
			//	Make sure SettingsWindow front most window (if already open)
			if (fSettingsWindow->IsHidden())
				fSettingsWindow->Show();
			while (fSettingsWindow->IsActive())
				fSettingsWindow->Activate(false);
			fSettingsWindow->Activate(true);
			break;
		}
		
		//	Menu Project
		case eMsgMenuProjectNew:
		case eMsgMenuProjectOpen:
		case eMsgMenuProjectSave:
		case eMsgMenuProjectAddSource:
		case eMsgActionProjectSaveFilename:
		case eMsgMenuProjectExportFfmpeg:
		case eMsgMenuProjectExportMediaKit:
		case eMsgMenuProjectExportFrame:
		case eMsgActionEffectsFilePanelOpen:
		case eMsgActionFilePanelCancel:
			ProjectIOMessage(msg);
			break;

		//	Export Media Window
		case eMsgActionExportWindowClosed:
			assert(fExportMediaWindow);
			fExportMediaWindow = nullptr;
			break;

		//	Project settings window
		case eMsgMenuProjectSettings:
			if (!fProjectSettings)
				fProjectSettings = new ProjectSettings(this);
			fProjectSettings->Show();
			break;
		case eMsgActionProjectSettingsChanged:
			gRenderActor->Async(&RenderActor::AsyncInvalidateProjectSettings, gRenderActor, 0);
			break;

		//	Edit menu
		case eMsgMenuEditUndo:
			gProject->Undo();
			break;
		case eMsgMenuEditRedo:
			gProject->Redo();
			break;
		
		//	Menu View
		case eMsgMenuViewLayout_1:
		case eMsgMenuViewLayout_2:
		case eMsgMenuViewLayout_3:
		case eMsgMenuViewLayout_4:
			SetUserLayout(msg->what);
			break;
		case eMsgMenuViewShowClipTags:
		{
			fMenuItemViewShowClipTags->SetMarked(!fMenuItemViewShowClipTags->IsMarked());
			fTimelineView->GetTimelineEdit()->SetTrackShowClipTags(fMenuItemViewShowClipTags->IsMarked());
			break;
		}
		case eMsgMenuViewShowNotes:
		{
			fMenuItemViewShowNotes->SetMarked(!fMenuItemViewShowNotes->IsMarked());
			fTimelineView->GetTimelineEdit()->SetTrackShowNotes(fMenuItemViewShowNotes->IsMarked());
			break;
		}
		case eMsgMenuViewShowThumbnails:
		{
			fMenuItemViewShowThumbnails->SetMarked(!fMenuItemViewShowThumbnails->IsMarked());
			fTimelineView->GetTimelineEdit()->SetShowAllVideoThumbnails(fMenuItemViewShowThumbnails->IsMarked());
			break;
		}

		//	Divider
		case eMsgActionDividerResize:
		{
			BPoint where;
			if (msg->FindPoint("point", &where) == B_OK)
			{
				fDividerPositionY += where.y;
				ResizeWindow();
			}
			break;
		}
		
		//	Tabs
		case eMsgActionTabSourceSelected:
		{
			MediaSource *src;
			SetActiveControl(CONTROL_SOURCE);
			if (msg->FindPointer("MediaSource", (void **)&src) == B_OK)
				((ControlSource *)fControlViews[CONTROL_SOURCE])->SetMediaSource(src);
			break;
		}
		case eMsgActionTabEffectSelected:
		{
			fTabMainView->GetEffectsTab()->SelectionChanged();
			return;
		}
		case eMsgActionTabTextSelected:
		{
			fTabMainView->GetTextTab()->SelectionChanged();
			return;
		}
		case eMsgActionTimelineEffectSelected:
		{
			MediaEffect *effect;
			if (msg->FindPointer("MediaEffect", (void **)&effect) == B_OK)
				fTabMainView->SelectEffect(effect);
			return;
		}
		case eMsgActionMedoSettingsChanged:
			fMenuItemExportMediaKit->SetEnabled(GetSettings()->export_enable_media_kit);
			break;

		case eMsgMenuToolsMonitor:
			if (fMonitorWindow == nullptr)
			{
				BScreen screen;
				BRect frame = screen.Frame();
				if (frame.Width() > gProject->mResolution.width + 100)
				{
					frame.left = 100.0f;
					frame.right = gProject->mResolution.width + 100;
				}
				if (frame.Height() > gProject->mResolution.height + 100)
				{
					frame.top = 100;
					frame.bottom = gProject->mResolution.height + 100;
				}
				else
					frame.top = 32;

				fMonitorWindow = new MonitorWindow(frame, GetText(TXT_MENU_TOOLS_MONITOR), fTimelineView->GetTimelinePlayer());
			}
			//	Make sure MonitorWindow front most window (if already open)
			if (fMonitorWindow->IsHidden())
				fMonitorWindow->Show();
			while (fMonitorWindow->IsActive())
				fMonitorWindow->Activate(false);
			fMonitorWindow->Activate(true);

			gProject->InvalidatePreview();
			break;

		case eMsgMenuToolsColourScope:
			if (fColourScope == nullptr)
			{
				BScreen screen;
				BRect frame = screen.Frame();
				fColourScope = new ColourScope(BRect(frame.right - 1000, frame.bottom - 1000, frame.right, frame.bottom), GetText(TXT_MENU_TOOLS_COLOUR_SCOPE));
			}
			//	Make sure ColourScope front most window (if already open)
			if (fColourScope->IsHidden())
				fColourScope->Show();
			while (fColourScope->IsActive())
				fColourScope->Activate(false);
			fColourScope->Activate(true);

			gProject->InvalidatePreview();
			break;

		case eMsgMenuToolsAudioMixer:
			if (fAudioMixer == nullptr)
				fAudioMixer = new AudioMixer(BRect(32, 32, 32+640, 32+480), GetText(TXT_MENU_TOOLS_AUDIO_MIXER));

			//	Make sure AudioMixer front most window (if already open)
			if (fAudioMixer->IsHidden())
				fAudioMixer->Show();
			while (fAudioMixer->IsActive())
				fAudioMixer->Activate(false);
			fAudioMixer->Activate(true);
			break;

		case eMsgMenuToolsSoundRecorder:
		{
			BEntry entry("/system/apps/SoundRecorder");
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK)
			{
				if (be_roster->Launch((const entry_ref *)&ref, (BMessage *)nullptr, (team_id *)nullptr) != B_OK)
				{
					BAlert *alert = new BAlert("Alert", "/system/apps/SoundRecorder (already open)", "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_STOP_ALERT);
					alert->Go();
				}
			}
			else
			{
				BAlert *alert = new BAlert("Alert", "Cannot find /system/apps/SoundRecorder", "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->Go();
			}
			break;
		}

		case B_REFS_RECEIVED:
			RefsReceived(msg);
			break;
		
		case B_SIMPLE_DATA:
			if (msg->HasRef("refs"))
				RefsReceived(msg);
			break;
			
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
			break;

		case B_KEY_DOWN:
			if (!fTimelineView->KeyDownMessage(msg))
				BWindow::MessageReceived(msg);
			break;

		case B_MODIFIERS_CHANGED:
		case B_MOUSE_IDLE:
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

/*	FUNCTION:		MedoWindow :: GetKeyModifiers
	ARGS:			none
	RETURN:			modifier key
	DESCRIPTION:	Get modifier keys (B_SHIFT_KEY, B_CONTROL_KEY etc)
*/
const uint32 MedoWindow :: GetKeyModifiers()
{
	return modifiers();
}

/*	FUNCTION:		MedoWindow :: SnapshotUpdate
	ARGS:			undo_available
					redo_available
	RETURN:			none
	DESCRIPTION:	Update edit menu items
*/
void MedoWindow :: SnapshotUpdate(bool undo_available, bool redo_available)
{
	fMenuItemEditUndo->SetEnabled(undo_available);
	fMenuItemEditRedo->SetEnabled(redo_available);
}

/*	FUNCTION:		MedoWindow :: SetActiveControl
	ARGS:			mode
	RETURN:			none
	DESCRIPTION:	Set control mode
*/
void MedoWindow :: SetActiveControl(CONTROL_MODE mode)
{
	if (fControlMode == mode)
		return;

	RemoveChild(fControlViews[fControlMode]);
	fControlMode = mode;
	AddChild(fControlViews[fControlMode]);
	ResizeWindow();
}

/*	FUNCTION:		MedoWindow :: InvalidatePreview
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Invalidate preview
*/
void MedoWindow :: InvalidatePreview()
{
	if (fControlMode != CONTROL_OUTPUT)
		SetActiveControl(CONTROL_OUTPUT);

	fTimelineView->InvalidateItems(-1);
	fTimelineView->PositionUpdate(fTimelineView->GetCurrrentFrame(), true);
}

/*	FUNCTION:		MedoWindow :: SetLayout
	ARGS:			layout
	RETURN:			n/a
	DESCRIPTION:	Set user layout
*/
void MedoWindow :: SetUserLayout(int layout)
{
	const float font_size = be_plain_font->Size();
	const float menu_height = fMenuBar->Frame().Height();
    const float kFontFactor = font_size/20.0f;

	BScreen screen;
	const float screen_width = screen.Frame().Width();
	const float screen_height = screen.Frame().Height();
    const float window_width = screen_width - 740*kFontFactor - 2*4;
	const float window_height = screen_height - (2*font_size + 4);	//	title bar + window_frame

	EffectsWindow *effects_window = EffectsWindow::GetInstance();

	switch (layout)
	{
		case eMsgMenuViewLayout_1:		//	Large Preview
		{
			//	Reverse fDividerAspectY to end up with 16:9 preview
            float preview_width = window_width - kTabViewWidth*kFontFactor;
			float preview_height = preview_width * 9.0f / 16.0f;
			//	fDividerPositionY = height * fDividerAspectY;
			fDividerAspectY = preview_height/(window_height - menu_height - 2*kDividerViewHeight);

			MoveTo(2, 2*font_size);
			ResizeTo(window_width, window_height);
			FrameResized(window_width, window_height);

			effects_window->MoveTo(2 + window_width + 8, 2*font_size);
            effects_window->ResizeTo(740*kFontFactor-12, 700);
			if (effects_window->IsHidden())
				effects_window->Show();

			if (fAudioMixer && !fAudioMixer->IsHidden())
				fAudioMixer->Hide();
			if (fColourScope && !fColourScope->IsHidden())
				fColourScope->Hide();
			break;
		}
		case eMsgMenuViewLayout_2:		//	Large Timeline
		{
			//	Reverse fDividerAspectY to end up with 0.25*window_height
			float preview_height = 0.25f*window_height;
			//	fDividerPositionY = height * fDividerAspectY;
			fDividerAspectY = preview_height/(window_height - menu_height - 2*kDividerViewHeight);

			MoveTo(2, 2*font_size);
			ResizeTo(window_width, window_height);
			FrameResized(window_width, window_height);

			effects_window->MoveTo(2 + window_width + 8, 2*font_size);
            effects_window->ResizeTo(740*kFontFactor-12, 700);
			while (effects_window->IsHidden())
				effects_window->Show();

			if (fAudioMixer && !fAudioMixer->IsHidden())
				fAudioMixer->Hide();
			if (fColourScope && !fColourScope->IsHidden())
				fColourScope->Hide();
			break;
		}
			break;
		case eMsgMenuViewLayout_3:		//	Colour grading
		{
			//	Reverse fDividerAspectY to end up with 16:9 preview
            float preview_width = window_width - kTabViewWidth*kFontFactor;
			float preview_height = preview_width * 9.0f / 16.0f;
			//	fDividerPositionY = height * fDividerAspectY;
			fDividerAspectY = preview_height/(window_height - menu_height - 2*kDividerViewHeight);

			MoveTo(2, 2*font_size);
			ResizeTo(window_width, window_height);
			FrameResized(window_width, window_height);

			effects_window->MoveTo(2 + window_width + 8, 2*font_size);
            effects_window->ResizeTo(740*kFontFactor-12, 700);
			while (effects_window->IsHidden())
				effects_window->Show();

			float scope_height = (screen_height > 700 + 2*font_size + 700) ? 700 : screen_height - (700 + 2*font_size);
			if (!fColourScope)
				fColourScope = new ColourScope(BRect(2 + window_width + 8, screen_height - scope_height, 740-12, scope_height), GetText(TXT_MENU_TOOLS_COLOUR_SCOPE));

			fColourScope->MoveTo(2 + window_width + 8, screen_height - scope_height);
            fColourScope->ResizeTo(740*kFontFactor-12, scope_height);
			while (fColourScope->IsHidden())
				fColourScope->Show();

			if (fAudioMixer && !fAudioMixer->IsHidden())
				fAudioMixer->Hide();
			InvalidatePreview();
			break;
		}
		case eMsgMenuViewLayout_4:		//	Audio editing
		{
			//	Reverse fDividerAspectY to end up with 16:9 preview
            float preview_width = window_width - kTabViewWidth*kFontFactor;
			float preview_height = preview_width * 9.0f / 16.0f;
			//	fDividerPositionY = height * fDividerAspectY;
			fDividerAspectY = preview_height/(window_height - menu_height - 2*kDividerViewHeight);

			MoveTo(2, 2*font_size);
			ResizeTo(window_width, window_height);
			FrameResized(window_width, window_height);

			effects_window->MoveTo(2 + window_width + 8, 2*font_size);
            effects_window->ResizeTo(740*kFontFactor-12, 700);
			while (effects_window->IsHidden())
				effects_window->Show();

			float mixer_height = (screen_height > 700 + 2*font_size + 480) ? 480 : screen_height - (700 + 2*font_size);
			if (!fAudioMixer)
				fAudioMixer = new AudioMixer(BRect(2 + window_width + 8, screen_height - mixer_height, 740 - 12, mixer_height), GetText(TXT_MENU_TOOLS_AUDIO_MIXER));

			fAudioMixer->MoveTo(2 + window_width + 8, screen_height - mixer_height);
            fAudioMixer->ResizeTo(740*kFontFactor-12, mixer_height);
			while (fAudioMixer->IsHidden())
				fAudioMixer->Show();

			if (fColourScope && !fColourScope->IsHidden())
				fColourScope->Hide();
			break;
		}
		default:
			assert(0);
	}

	//	Workaround for lost focus when new window shown (send msg to EffectsWindow which will call MedoWindow::Activate())
	effects_window->PostMessage(EffectsWindow::eMsgActivateMedoWindow);
}

