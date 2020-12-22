/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Theme
 */

#ifndef THEME_H
#define THEME_H

#ifndef _GRAPHICS_DEFS_H
#include <interface/GraphicsDefs.h>
#endif

enum class UiColour
{
	eTimelineView,
	eTimelineTrack,
	eTimelinePosition,
	eListBackground,
	eListText,
	eListSelection,
	eListOutlineTriangle,
	ePanelBackground,
	ePanelText,
};

//=======================
namespace Theme
{
	enum class Preset
	{
		eDark,
		eBright,
		eSystem,
	};
	rgb_color	GetUiColour(UiColour colour);
	void		SetTheme(Preset preset);
	Preset		GetTheme();
};

#endif	//#ifndef THEME_H
