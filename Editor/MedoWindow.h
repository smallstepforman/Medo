/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Medo window
 */

#ifndef _MEDO_WINDOW_H_
#define _MEDO_WINDOW_H_

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#ifndef _FILE_PANEL_H
#include <storage/FilePanel.h>
#endif

class BMenuBar;
class BMenu;
class BFilePanel;

class AboutWindow;
class AudioMixer;
class ColourScope;
class DividerView;
class ExportMediaWindow;
class MediaSource;
class MonitorWindow;
class OutputView;
class Project;
class ProjectSettings;
class SettingsWindow;
class StatusView;
class TabMainView;
class TimelineView;

//====================
class MedoWindow : public BWindow, BRefFilter
{
public:
					MedoWindow();
					~MedoWindow();	
	bool			QuitRequested();
	void			MessageReceived(BMessage *msg);
	void			FrameResized(float width, float height);
	
	MediaSource		*AddMediaSource(const char *path);
	void			RemoveAllMediaSources();
	void			LoadProject(const char *path);
	void			SnapshotUpdate(bool undo_available, bool redo_available);
	const uint32	GetKeyModifiers();
	
	enum CONTROL_MODE
	{
		CONTROL_SOURCE,
		CONTROL_OUTPUT,		//	must be last item (TabMainView)
		NUMBER_CONTROL_MODES
	};
	void			SetActiveControl(CONTROL_MODE mode);
	void			InvalidatePreview();

	OutputView		*GetOutputView() {return (OutputView *)fControlViews[CONTROL_OUTPUT];}
	AudioMixer		*GetAudioMixer() {return fAudioMixer;}

private:
	TabMainView		*fTabMainView;
	TimelineView	*fTimelineView;
	StatusView		*fStatusView;

	DividerView		*fDividerView;
	BMessage		*fDividerMessage;
	float			fDividerPositionY;
	float			fDividerAspectY;
	void			ResizeWindow();

	CONTROL_MODE	fControlMode;
	BView			*fControlViews[NUMBER_CONTROL_MODES];
	
	Project			*fProject;
	
	BMenuBar		*fMenuBar;
	BMenuItem		*fMenuItemEditUndo;
	BMenuItem		*fMenuItemEditRedo;
	BMenuItem		*fMenuItemViewShowClipTags;
	BMenuItem		*fMenuItemViewShowNotes;
	BMenuItem		*fMenuItemViewShowThumbnails;
	BMenuItem		*fMenuItemExportMediaKit;

	//	Project IO
	enum FILE_PANEL_MODE {FILE_PANEL_LOAD_PROJECT, FILE_PANEL_SAVE_PROJECT, FILE_PANEL_ADD_SOURCE, FILE_PANEL_EXPORT_FRAME, FILE_PANEL_EFFECT};
	BFilePanel			*fFilePanel;
	BMessage			*fFilePanelSaveProjectMessage;
	FILE_PANEL_MODE		fFilePanelMode, fPreviousFilePanelMode;
	void				CreateFilePanel();
	void				DestroyFilePanel();
	void				ReplaceFilePanelCancelMessage();
	void				ProjectIOMessage(BMessage *msg);
	bool				Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat, const char* mimeType);
	void				RefsReceived(BMessage* message);
	void				LoadProjectSuccess(const char *status_msg);
	bool				ExportFrame(BPath *path);
	void				SetMimeType(BPath *path);
	friend class Project;

	AboutWindow			*fAboutWindow;
	AudioMixer			*fAudioMixer;
	ColourScope			*fColourScope;
	ExportMediaWindow	*fExportMediaWindow;
	MonitorWindow		*fMonitorWindow;
	ProjectSettings		*fProjectSettings;
	SettingsWindow		*fSettingsWindow;

	void				SetUserLayout(int layout);
	
public:
	enum kWindowMessages
	{
		eMsgMenuMedoAbout = 'mw00',
		eMsgMenuMedoSettings,
		eMsgMenuMedoQuit,
		eMsgMenuProjectNew,
		eMsgMenuProjectOpen,
		eMsgMenuProjectSave,
		eMsgMenuProjectSettings,
		eMsgMenuProjectAddSource,
		eMsgMenuProjectExportFfmpeg,
		eMsgMenuProjectExportMediaKit,
		eMsgMenuProjectExportFrame,
		eMsgMenuEditUndo,
		eMsgMenuEditRedo,
		eMsgMenuViewLayout_1,
		eMsgMenuViewLayout_2,
		eMsgMenuViewLayout_3,
		eMsgMenuViewLayout_4,
		eMsgMenuViewShowClipTags,
		eMsgMenuViewShowNotes,
		eMsgMenuViewShowThumbnails,
		eMsgMenuToolsMonitor,
		eMsgMenuToolsColourScope,
		eMsgMenuToolsAudioMixer,
		eMsgMenuToolsSoundRecorder,

		eMsgActionProjectSaveFilename,
		eMsgActionFilePanelCancel,
		eMsgActionTabSourceSelected,
		eMsgActionTabEffectSelected,
		eMsgActionTabTextSelected,
		eMsgActionTimelineEffectSelected,
		eMsgActionAsyncPreviewReady,
		eMsgActionAsyncTimelinePlayerUpdate,
		eMsgActionAsyncThumbnailReady,
		eMsgActionDividerResize,
		eMsgActionEffectsFilePanelOpen,
		eMsgActionExportWindowClosed,
		eMsgActionProjectSettingsChanged,
		eMsgActionMedoSettingsChanged,
		eMsgActionControlSourcePreviewReady,
	};

private:
	static MedoWindow *sMedoWindow;
public:
	static MedoWindow *GetInstance() {return sMedoWindow;}
};

#endif	//#ifndef _MEDO_WINDOW_H_


