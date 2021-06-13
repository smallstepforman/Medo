/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effects manager (cache only used EffectsNodes)
 */

#include <cstring>
#include <StorageKit.h>
#include <AppKit.h>
 
#include "EffectNode.h"
#include "EffectsManager.h"
#include "RenderActor.h"
#include "Project.h" 

//	TODO load dynamically
#include "Effects/Effect_AudioGain.h"
#include "Effects/Effect_Blur.h"
#include "Effects/Effect_Colour.h"
#include "Effects/Effect_ColourCorrection.h"
#include "Effects/Effect_ColourGrading.h"
#include "Effects/Effect_ColourLut.h"
#include "Effects/Effect_Crop.h"
#include "Effects/Effect_Marker.h"
#include "Effects/Effect_Mask.h"
#include "Effects/Effect_Mirror.h"
#include "Effects/Effect_Move.h"
#include "Effects/Effect_None.h"
#include "Effects/Effect_ParticleTrail.h"
#include "Effects/Effect_Plugin.h"
#include "Effects/Effect_PortraitBlur.h"
#include "Effects/Effect_Rotate.h"
#include "Effects/Effect_Speed.h"
#include "Effects/Effect_Text.h"
#include "Effects/Effect_Text_3D.h"
#include "Effects/Effect_Text_Counter.h"
#include "Effects/Effect_Text_Terminal.h"
#include "Effects/Effect_Transform.h"

/*	FUNCTION:		EffectsManager :: EffectsManager
	ARGS:			preview_frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
EffectsManager :: EffectsManager(BRect preview_frame)
{
	preview_frame.OffsetTo(0, 0);
	
	fEffectNone = new Effect_None(preview_frame, nullptr);

	//	TODO Convert to AddOns
	fEffectNodes.push_back(new Effect_AudioGain(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_PortraitBlur(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Blur(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Colour(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_ColourLut(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_ColourCorrection(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_ColourGrading(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Crop(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Mask(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Mirror(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Move(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_ParticleTrail(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Rotate(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Speed(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Transform(preview_frame, nullptr));

	//	Text Items (in appearance order)
	fEffectNodes.push_back(new Effect_Text(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Text3D(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_TextCounter(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_TextTerminal(preview_frame, nullptr));
	fEffectNodes.push_back(new Effect_Marker(preview_frame, nullptr));

	//	Load AddOns
	LoadAllAddOns(preview_frame);

	//	Load plugins from application path
	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	BPath executable_path(&appInfo.ref);
	BString plugin_path(executable_path.Path());
	int last_dir = plugin_path.FindLast('/');
	plugin_path.Truncate(last_dir+1);
	plugin_path.Append("Plugins");
	LoadPlugins(plugin_path.String());

	// Load plugins from Config path
	BPath config_path;
	find_directory(B_USER_CONFIG_DIRECTORY, &config_path);
	plugin_path.SetTo(config_path.Path());
	plugin_path.Append("/settings/Medo/Plugins");
	LoadPlugins(plugin_path);

	//	Reverse add plugins
	for (std::vector<EffectPlugin *>::const_reverse_iterator ri = fEffectPlugins.rbegin(); ri != fEffectPlugins.rend(); ri++)
	{
		Effect_Plugin *node_plugin = new Effect_Plugin((*ri), preview_frame, nullptr);
		fEffectNodes.push_back(node_plugin);
	}
}
 
/*	FUNCTION:		EffectsManager :: ~EffectsManager
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
EffectsManager :: ~EffectsManager()
{
	for (auto &i : fEffectNodes)
		delete i;
	delete fEffectNone;

}

/*	FUNCTION:		EffectsManager :: LoadAllAddOns
	ARGS:			preview_frame
	RETURN:			n/a
	DESCRIPTION:	Search for and load all AddOns
*/
void EffectsManager :: LoadAllAddOns(BRect preview_frame)
{
	auto load_addons = [&](const char *addons_path, BRect preview_frame)
	{
		BEntry entry;
		BDirectory dir(addons_path);
		while (dir.GetNextEntry(&entry) == B_OK)
		{
			if (entry.IsDirectory())
			{
				//	Recurse one level only
				BPath path(&entry);
				BDirectory subdir(path.Path());
				while (subdir.GetNextEntry(&entry) == B_OK)
				{
					if (!entry.IsDirectory())
					{
						BPath path(&entry);
						if (strstr(path.Path(), ".so"))
						{
							if (!LoadAddOn(&path, preview_frame))
								printf("Error loading AddOn(%s)\n", path.Path());
						}
					}
				}
			}
			else
			{
				BPath path(&entry);
				if (strstr(path.Path(), ".so"))
				{
					if (!LoadAddOn(&path, preview_frame))
						printf("Error loading AddOn(%s)\n", path.Path());
				}
			}
		}
	};

	//	Load from application path
	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	BPath executable_path(&appInfo.ref);
	BString addons_path(executable_path.Path());
	int last_dir = addons_path.FindLast('/');
	addons_path.Truncate(last_dir+1);
	addons_path.Append("AddOns");
	load_addons(addons_path.String(), preview_frame);

	//	Load from Config path
	BPath config_path;
	find_directory(B_USER_CONFIG_DIRECTORY, &config_path);
	addons_path.SetTo(config_path.Path());
	addons_path.Append("/settings/Medo/AddOns");
	load_addons(addons_path.String(), preview_frame);
}

/*	FUNCTION:		EffectsManager :: LoadAddOn
	ARGS:			path
					preview_frame
	RETURN:			true if valid
	DESCRIPTION:	Load add-ons
*/
bool EffectsManager :: LoadAddOn(BPath *path, BRect preview_frame)
{
	//	Load Addons
	image_id add_on;
	add_on = load_add_on(path->Path());
	if (add_on > 0)
	{
		EffectNode * (*init_function)(BRect);
		if (B_OK == get_image_symbol(add_on, "instantiate_effect", B_SYMBOL_TYPE_TEXT, (void **)&init_function))
		{
			EffectNode *node = (*init_function)(preview_frame);
			if (node)
			{
				printf("LoadAddOn(%s) Success\n", path->Path());
				fEffectNodes.push_back(node);
				return true;
			}
		}
	}
	return false;
}

/*	FUNCTION:		EffectsManager :: CreateMediaEffect
	ARGS:			vendor_name
					effect_name
	RETURN:			n/a
	DESCRIPTION:	Instantiate effect, init OpenGL objects from rendering context
*/
MediaEffect * EffectsManager :: CreateMediaEffect(const char *vendor_name, const char *effect_name)
{
	//printf("CreateMediaEffect(%s, %s)\n", vendor_name, effect_name);
	for (auto i : fEffectNodes)
	{
		if ((strcmp(vendor_name, i->GetVendorName()) == 0) &&
			(strcmp(effect_name, i->GetEffectName()) == 0))
		{
			MediaEffect *media_effect = i->CreateMediaEffect();
			media_effect->mEffectNode = i;

			if (!i->mRenderObjectsInitialised)
			{
				gRenderActor->Async(&RenderActor::AsyncCreateEffectNode, gRenderActor, i);
				i->mRenderObjectsInitialised = true;
			}
			return media_effect;
		}
	}
	
	//	No match found
	printf("Error - Media Effect not found\n");
	return nullptr;
}

/*	FUNCTION:		EffectsManager :: ProjectSettingsChanged
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Reinitialise rendering objects
					Called from RenderActor
*/
void EffectsManager :: ProjectSettingsChanged()
{
	printf("EffectsManager::ProjectSettingsChanged()\n");
	for (auto i : fEffectNodes)
	{
		if (i->mRenderObjectsInitialised)
		{
			printf("\tRecreating: %s\n", i->GetEffectName());
			i->DestroyRenderObjects();
			i->InitRenderObjects();
		}
	}
}

