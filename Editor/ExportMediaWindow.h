/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Export Media Window
 */

#ifndef _EXPORT_MEDIA_WINDOW_H_
#define _EXPORT_MEDIA_WINDOW_H_

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

class BCheckBox;
class BOptionPopUp;
class BTextControl;
class BStringView;
class BButton;
class BFilePanel;
class BMessage;

class BitmapCheckbox;
class ProgressBar;
class MedoWindow;

class ExportMediaWindow;
class Export_MediaKit;
class Export_ffmpeg;

/********************************
	Export Engine Base Class
	- Native BMediaKit
	- FFMPEG (libavformat)
*********************************/
class ExportEngine
{
protected:
	ExportMediaWindow	*mParent;

public:
						ExportEngine(ExportMediaWindow *parent) : mParent(parent) {}
	virtual				~ExportEngine() {}
	virtual void		BuildFileFormatOptions() = 0;
	virtual void		BuildVideoCodecOptions() = 0;
	virtual void		BuildAudioCodecOptions() = 0;
	virtual void		StartEncode() = 0;
	virtual void		StopEncode(const bool complete) = 0;
	virtual void		FileFormatSelectionChanged() = 0;

	virtual float		AddCustomVideoGui(float start_y) {return start_y;}
	virtual float		AddCustomAudioGui(float start_y) {return start_y;}
	virtual bool		MessageRedirect(BMessage *msg) {return false;}
};

/********************************
	Export Media Base Class
*********************************/
class ExportMediaWindow : public BWindow
{
public:
	enum EXPORT_ENGINE {EXPORT_USING_MEDIA_KIT, EXPORT_USING_FFMPEG};
			ExportMediaWindow(MedoWindow *parent, EXPORT_ENGINE engine);
			~ExportMediaWindow();
	bool	QuitRequested();
	void	MessageReceived(BMessage *msg);

private:
	ExportEngine	*fExportEngine;
	friend class	Export_MediaKit;
	friend class	Export_ffmpeg;

	MedoWindow		*fMedoWindow;
	BView			*fBackgroundView;
	bool			fHasVideo;
	bool			fHasAudio;

	BCheckBox		*fEnableVideo;
	BCheckBox		*fEnableAudio;
	BOptionPopUp	*fOptionFileFormat;

	BOptionPopUp	*fOptionVideoFrameRate;
	BOptionPopUp	*fOptionVideoCodec;
	BOptionPopUp	*fOptionVideoResolution;
	BCheckBox		*fEnableCustomVideoResolution;
	BTextControl	*fTextVideoCustomWidth;
	BTextControl	*fTextVideoCustomHeight;
	BitmapCheckbox	*fCheckboxCustomResolutionLinked;

	BOptionPopUp	*fOptionAudioSampleRate;
	BOptionPopUp	*fOptionAudioChannelCount;
	BOptionPopUp	*fOptionVideoBitrate;
	BCheckBox		*fEnableCustomVideoBitrate;
	BTextControl	*fTextVideoCustomBitrate;

	BOptionPopUp	*fOptionAudioCodec;

	BOptionPopUp	*fOptionAudioBitrate;
	BCheckBox		*fEnableCustomAudioBitrate;
	BTextControl	*fTextAudioCustomBitrate;

	BStringView		*fTextOutFile;
	BButton			*fButtonStartEncode;
	BFilePanel		*fFilePanel;

	enum STATE {STATE_INPUT, STATE_ENCODING};
	STATE			fState;
	BMessage		*fMsgExportEngine;
	BStringView		*fTextExportProgress;
	ProgressBar		*fExportProgressBar;

	void			PreprocessProject();
	float			CreateFileFormatGui(float start_y);
	float			CreateVideoGui(float start_y);
	float			CreateAudioGui(float start_y);
	float			CreateFileSaveGui(float start_y);
	void			ValidateTextField(BTextControl *control, uint32 what);
	void			UpdateCustomVideoResolution(uint32 msg);
	void			StartEncode();
	void			StopEncode(const bool complete);

	uint32			GetSelectedVideoWidth() const;
	uint32			GetSelectedVideoHeight() const;
	float			GetSelectedVideoFrameRate() const;
	uint32			GetSelectedVideoBitrate() const;
	uint32			GetSelectedAudioSampleRate() const;
	uint32			GetSelectedAudioNumberChannels() const;
	uint32			GetSelectedAudioBitrate() const;
};

#endif // #ifndef _EXPORT_MEDIA_WINDOW_H_
