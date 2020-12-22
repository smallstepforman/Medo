/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Theme
 */

#include <cstdio>
#include <cassert>

#include <interface/InterfaceDefs.h>

#include "Theme.h"

namespace Theme
{

static Preset sPreset = Preset::eDark;
//static Preset sPreset = Preset::eBright;
//static Preset sPreset = Preset::eSystem;

static rgb_color	kDarkTheme[] =
{
	{80, 80, 80, 255},			//	eTimelineView
	{64, 64, 64, 255},			//	eTimelineTrack
	{144, 32, 160, 255},		//	eTimelinePosition
	{48, 48, 48, 255},			//	eListBackground
	{255, 176, 0, 255},			//	eListText
	{80, 80, 80, 255},			//	eListSelection
	{255, 0, 0, 255},			//	eListOutlineTriangle
	{64, 64, 64, 255},			//	ePanelBackground,
	{255, 255, 255, 255},		//	ePanelText,
};

static rgb_color	kBrightTheme[] =
{
	{192, 192, 192, 255},		//	eTimelineView
	{176, 176, 176, 176},		//	eTimelineTrack
	{192, 192, 255, 255},		//	eTimelinePosition
	{208, 208, 208, 208},		//	eListBackground
	{0, 0, 0, 255},				//	eListText
	{192, 192, 255, 255},		//	eListSelection
	{0, 0, 255, 255},			//	eListOutlineTriangle
	{176, 176, 176, 176},		//	ePanelBackground,
	{0, 0, 0, 255},				//	ePanelText,
};

/*	FUNCTION:		GetSystemUiColour
	ARGS:			colour
	RETURN:			theme colour
	DESCRIPTION:	Get ui component colour
*/
rgb_color	GetSystemUiColour(UiColour colour)
{
	switch (colour)
	{
		case UiColour::eTimelineView:			return ui_color(B_MENU_BACKGROUND_COLOR);
		case UiColour::eTimelineTrack:			return ui_color(B_MENU_SELECTED_BACKGROUND_COLOR);
		case UiColour::eTimelinePosition:		return ui_color(B_CONTROL_HIGHLIGHT_COLOR);
		case UiColour::eListBackground:			return ui_color(B_LIST_BACKGROUND_COLOR);
		case UiColour::eListText:				return ui_color(B_LIST_ITEM_TEXT_COLOR);
		case UiColour::eListSelection:			return ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		case UiColour::eListOutlineTriangle:	return ui_color(B_CONTROL_HIGHLIGHT_COLOR);
		case UiColour::ePanelBackground:		return ui_color(B_PANEL_BACKGROUND_COLOR);
		case UiColour::ePanelText:				return ui_color(B_PANEL_TEXT_COLOR);
		default:	assert(0);					return {255, 0, 0, 255};
	};
}

/*	FUNCTION:		GetUiColour
	ARGS:			colour
	RETURN:			theme colour
	DESCRIPTION:	Get ui component colour
*/
rgb_color GetUiColour(UiColour colour)
{
	switch (sPreset)
	{
		case Preset::eDark:			return kDarkTheme[(int)colour];
		case Preset::eBright:		return kBrightTheme[(int)colour];
		case Preset::eSystem:		return GetSystemUiColour(colour);
		default:					return {255, 0, 0, 255};
	}
}

/*	FUNCTION:		SetTheme
	ARGS:			preset
	RETURN:			n/a
	DESCRIPTION:	Set current theme
*/
void SetTheme(Preset preset)
{
	sPreset = preset;
}

/*	FUNCTION:		GetTheme
	ARGS:			none
	RETURN:			preset
	DESCRIPTION:	Get current theme
*/
Preset GetTheme()
{
	return sPreset;
}

};	//	namespace Theme
