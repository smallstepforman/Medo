/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect Plugin
 */

#ifndef EFFECT_PLUGIN_H
#define EFFECT_PLUGIN_H

#ifndef EFFECT_NODE_H
#include "Editor/EffectNode.h"
#endif

class BBitmap;
class BScrollView;
class BitmapCheckbox;

class LanguageJson;

namespace yrender
{
	class YRenderNode;
	class YTexture;
};
namespace Magnify {class TWindow;};

struct PluginHeader
{
	std::string					vendor;
	std::string					name;
	uint32						txt_labelA;
	uint32						txt_labelB;
	std::string					icon;
	EffectNode::EFFECT_GROUP	type;
};
struct PluginUniform
{
	enum UniformType {eSampler2D, eFloat, eVec2, eVec3, eVec4, eColour, eInt, eTimestamp, eInterval, eRsolution};
	UniformType					type;
	std::string					name;
};
struct PluginGuiWidget
{
	enum GuiWidget {eSlider, eCheckbox, eSpinner2, eSpinner3, eSpinner4, eColour, eText};
	static inline constexpr unsigned int kVecCountElements[] = {0, 1, 2, 3, 4, 4, 0};	//	must match GuiWidget layout

	GuiWidget					widget_type;
	BRect						rect;
	uint32						txt_label;
	std::string					uniform;
	int							uniform_idx;

	//	Common
	float						range[2];
	float						default_value[4];
	float						vec4[4];			//	eFloat, eVec2, eVec3, eVec4, eColour
	//	Slider
	uint32						txt_slider_min;
	uint32						txt_slider_max;
};

struct PluginShader
{
	enum ShaderType {eFragment};
	ShaderType						type;
	std::vector<PluginUniform>		uniforms;
	std::string						source_file;
	std::string						source_text;
	std::vector<PluginGuiWidget>	gui_widgets;
};
struct EffectPlugin
{
	PluginHeader					mHeader;
	PluginShader					mFragmentShader;
	LanguageJson					*mLanguage;

	EffectPlugin() : mLanguage(nullptr) { }
};

/*********************************
	Effect_Plugin
**********************************/
class Effect_Plugin : public EffectNode
{
	EffectPlugin			*fPlugin;
	yrender::YRenderNode	*fRenderNode;
	BView					*fMainView;
	BScrollView				*fScrollView;
	std::vector<BView *>	fGuiWidgets;
	yrender::YTexture		*fTextureUnit1;

	Magnify::TWindow		*fColourPickerWindow;
	BitmapCheckbox			*fColourPickerButton;
	BMessage				*fColourPickerMessage;

public:
					Effect_Plugin(EffectPlugin *plugin, BRect frame, const char *filename);
					~Effect_Plugin()								override;

	void			AttachedToWindow()								override;
	void			InitRenderObjects()								override;
	void			DestroyRenderObjects()							override;
	
	EFFECT_GROUP	GetEffectGroup() const							override		{return fPlugin->mHeader.type;}
	const char		*GetVendorName() const							override		{return fPlugin->mHeader.vendor.c_str();}
	const char		*GetEffectName() const							override		{return fPlugin->mHeader.name.c_str();}
	bool			LoadParameters(const rapidjson::Value &parameters, MediaEffect *media_effect)	override;
	bool			SaveParameters(FILE *file, MediaEffect *media_effect)							override;
	
	BBitmap			*GetIcon()										override;
	const char		*GetTextEffectName(const uint32 language_idx)	override;
	const char		*GetTextA(const uint32 language_idx)			override;
	const char		*GetTextB(const uint32 language_idx)			override;
	MediaEffect		*CreateMediaEffect()							override;
	
	void			RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects) override;
	void			MediaEffectSelected(MediaEffect *effect)		override;
	void			MessageReceived(BMessage *msg)					override;
	void			OutputViewMouseDown(MediaEffect *media_effect, const BPoint &point)		override;
};

#endif	//#ifndef EFFECT_PLUGIN_H
