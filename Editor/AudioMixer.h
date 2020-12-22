/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Audio mixer
 */

#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#ifndef PERSISTANT_WINDOW_H
#include "PersistantWindow.h"
#endif

class AudioMixerView;
class BScrollView;
class BMessage;

class AudioMixer : public PersistantWindow
{
public:
			AudioMixer(BRect frame, const char *title);
			~AudioMixer()					override;
	void	FrameResized(float width, float height) override;
	void	MessageReceived(BMessage *msg)	override;

	enum	{kMsgProjectInvalidated = 'proj', kMsgVisualiseLevels};

	BMessage	*mMsgVisualiseLevels;

private:
	AudioMixerView			*fMixerView;
	BScrollView				*fScrollView;
};


#endif	//#ifndef AUDIO_MIXER_H
