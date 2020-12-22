/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Export Engine - MediaKit
 */

#ifndef _EXPORT_MEDIA_KIT_H_
#define _EXPORT_MEDIA_KIT_H_

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _EXPORT_MEDIA_WINDOW_H_
#include "ExportMediaWindow.h"
#endif

class Export_MediaKit : public ExportEngine
{
public:
				Export_MediaKit(ExportMediaWindow *parent);
				~Export_MediaKit() override;
	void		BuildFileFormatOptions() override;
	void		BuildVideoCodecOptions() override;
	void		BuildAudioCodecOptions() override;
	void		FileFormatSelectionChanged() override;
	void		StartEncode() override;
	void		StopEncode(const bool complete) override;

	float		AddCustomAudioGui(float start_y) override;

private:
	std::vector<int32>	fFileFormatCookies;

	static status_t		WorkThread(void *arg);
	int32				fThread;
	sem_id				fRenderSemaphore;
	bool				fKeepAlive;
};

#endif	//#ifndef _EXPORT_MEDIA_KIT_H_
