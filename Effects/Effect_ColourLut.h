/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Colour LUT
 */

#ifndef EFFECT_COLOUR_LUT_H
#define EFFECT_COLOUR_LUT_H

#ifndef _FILE_PANEL_H
#include <storage/FilePanel.h>
#endif

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

namespace yrender
{
	class YRenderNode;
};
class BBitmap;
class BButton;
class BStringView;

class Effect_ColourLut : public EffectNode, BRefFilter
{
public:
					Effect_ColourLut(BRect frame, const char *filename);
					~Effect_ColourLut()								override;

	void			AttachedToWindow()								override;
	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const							override	{return EffectNode::EFFECT_COLOUR;}
	const char		*GetVendorName() const							override;
	const char		*GetEffectName() const							override;
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	BBitmap			*GetIcon()										override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	const char		*GetTextA(const uint32 language_idx)			override;
	const char		*GetTextB(const uint32 language_idx)			override;
	
	MediaEffect		*CreateMediaEffect()							override;
	void			MediaEffectSelected(MediaEffect *effect)		override;
	void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)	override;
	
	void			MessageReceived(BMessage *msg)					override;

	BFilePanel		*CreateFilePanel(int language_index)			override;
	void			FilePanelOpen(const char *path)					override;
	
private:
	yrender::YRenderNode 	*fRenderNode;
	int						fLutCacheIndex;

	BButton					*fLoadLutButton;
	BStringView				*fLoadLutString;

	bool					Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat, const char* mimeType)	override;
	void					LoadCubeFile(int index);
};

#endif	//#ifndef EFFECT_COLOUR_LUT_H
