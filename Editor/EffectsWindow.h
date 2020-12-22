/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	Effects Window
 */

#ifndef EFFECTS_WINDOW_H
#define EFFECTS_WINDOW_H

#ifndef PERSISTANT_WINDOW_H
#include "PersistantWindow.h"
#endif

class EffectNode;

class EffectsWindow : public PersistantWindow
{
public:
						EffectsWindow(BRect frame);
						~EffectsWindow()	override;
	void				Terminate()			override;

	enum kWindowMessages
	{
		eMsgShowEffect = 'efm0',
		eMsgOutputViewMouseDown,
		eMsgOutputViewMouseMoved,
		eMsgActivateMedoWindow,
	};
	void				MessageReceived(BMessage *msg)	override;

	EffectNode			*GetCurrentEffectNode() {return fEffectNode;}
	const uint32		GetKeyModifiers();

private:
	EffectNode			*fEffectNode;

public:
	static EffectsWindow	*GetInstance() {return sWindowInstance;}
private:
	static EffectsWindow	*sWindowInstance;
};

#endif	//#ifndef EFFECTS_WINDOW_H
