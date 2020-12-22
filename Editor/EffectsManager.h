/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Effects manager 
 */

#ifndef _EFFECTS_MANAGER_H_
#define _EFFECTS_MANAGER_H_

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

class BPath;

class EffectNode;
class EffectsTab;
class Effect_None;
class MediaEffect;
class TextTab;
class EffectPlugin;

class EffectsManager
{
public:
					EffectsManager(BRect preview_frame);
					~EffectsManager();
	MediaEffect		*CreateMediaEffect(const char *vendor_name, const char *effect_name);
	Effect_None		*GetEffectNone() {return fEffectNone;}
	void			ProjectSettingsChanged();
	
private:
	void			LoadAllAddOns(BRect preview_frame);
	bool			LoadAddOn(BPath *path, BRect preview_frame);

	void			LoadPlugins(const char *path);
	void			DestroyPlugins();
	bool			LoadPlugin(BPath *path);

	std::vector<EffectNode *>		fEffectNodes;
	std::vector<EffectPlugin *>		fEffectPlugins;
	Effect_None						*fEffectNone;
	friend class EffectsTab;
	friend class TextTab;
};

extern EffectsManager *gEffectsManager;

#endif	//#ifndef _EFFECTS_MANAGER_H_
