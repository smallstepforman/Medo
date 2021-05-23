/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effects plug-in
 */

#include <string>
#include <vector>

#include <StorageKit.h>
#include <storage/Directory.h>
#include <support/String.h>
#include <AppKit.h>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "EffectsManager.h"
#include "EffectNode.h"
#include "FileUtility.h"
#include "Language.h"
#include "LanguageJson.h"

#include "Effects/Effect_Plugin.h"

static const std::vector<const char *>	kUniformTypes = {"sampler2D", "float", "vec2", "vec3", "vec4", "colour", "int", "timestamp", "interval", "resolution"};		//	Must match PluginUniform::UniformType
static const std::vector<const char *> kGuiWidgets = {"slider", "checkbox", "radiobutton", "vec2", "vec3", "vec4", "colour", "text"};												//	Must match PluginGuiWidget::GuiWidget

/*	FUNCTION:		EffectsManager :: LoadPlugins
	ARGS:			plugin_path
	RETURN:			n/a
	DESCRIPTION:	Parse Plugins directory for effects
*/
void EffectsManager :: LoadPlugins(const char *plugin_path)
{
	BEntry entry;
	BDirectory dir(plugin_path);

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
					if (strstr(path.Path(), ".plugin"))
					{
						if (!LoadPlugin(&path))
							printf("Error loading pluging(%s)\n", path.Path());
					}
				}
			}
		}
		else
		{
			BPath path(&entry);
			if (strstr(path.Path(), ".plugin"))
			{
				if (!LoadPlugin(&path))
					printf("Error loading pluging(%s)\n", path.Path());
			}
		}
	}
}

/*	FUNCTION:		EffectsManager :: DestroyPlugins
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Destroy all plugins
*/
void EffectsManager :: DestroyPlugins()
{
	for (auto &p : fEffectPlugins)
	{
		delete p->mLanguage;
		delete p;
	}
}

/*	FUNCTION:		EffectsManager :: LoadPlugin
	ARGS:			path
	RETURN:			n/a
	DESCRIPTION:	Load plugin
*/
bool EffectsManager :: LoadPlugin(BPath *path)
{
	char *data = ReadFileToBuffer(path->Path());
	if (!data)
		return false;

	const uint32 kCountAvailableLanguages = gLanguageManager->GetNumberAvailableLanguages();

	EffectPlugin *aPlugin = new EffectPlugin;

#define ERROR_EXIT(a) {printf("ERROR(%s) %s\n", path->Path(), a);	delete [] data; delete aPlugin; return false;}

	rapidjson::Document document;
	rapidjson::ParseResult res = document.Parse<rapidjson::kParseTrailingCommasFlag>(data);
	if (!res)
	{
		printf("JSON parse error: %s (Byte offset: %lu)\n", rapidjson::GetParseError_En(res.Code()), res.Offset());

		for (int c=-20; c < 20; c++)
			printf("%c", data[res.Offset() + c]);
		printf("\n");

		ERROR_EXIT("Invalid JSON");
	}

/****************************
	Load language file
*****************************/
	BString languages_path(path->Path());
	int32 languages_idx = languages_path.FindLast("/");
	languages_path.Remove(languages_idx, languages_path.Length() - languages_idx);
	languages_path.Append("/Languages.json");
	aPlugin->mLanguage = new LanguageJson(languages_path.String());
	if (aPlugin->mLanguage->GetTextCount() == 0)
	{
		printf("Missing file \"%s\"\n", languages_path.String());
		return false;
	}

/****************************
	"plugin" (header)
****************************/
	{
		PluginHeader &header = aPlugin->mHeader;

		if (!document.HasMember("plugin"))
			ERROR_EXIT("Missing object \"plugin\"");
		const rapidjson::Value &plugin = document["plugin"];

		if (!plugin.HasMember("version") || !plugin["version"].IsInt())
			ERROR_EXIT("Missing attribute plugin::version");
		int version = plugin["version"].GetInt();
		if (version != 1)
			ERROR_EXIT("plugin::version != 1");

		if (!plugin.HasMember("vendor") || !plugin["vendor"].IsString())
			ERROR_EXIT("Missing attribute plugin::vendor");
		header.vendor.assign(plugin["vendor"].GetString());

		if (!plugin.HasMember("type") || !plugin["type"].IsString())
			ERROR_EXIT("Missing attribute plugin::type");
		std::string type(plugin["type"].GetString());
		header.type = EffectNode::EFFECT_SPATIAL;	//	use as unassigned marker
		if (type.compare("colour") == 0)
			header.type = EffectNode::EFFECT_COLOUR;
		else if (type.compare("image") == 0)
			header.type = EffectNode::EFFECT_IMAGE;
		else if (type.compare("transition") == 0)
			header.type = EffectNode::EFFECT_TRANSITION;
		else if (type.compare("special") == 0)
			header.type = EffectNode::EFFECT_SPECIAL;

		if (header.type == EffectNode::EFFECT_SPATIAL)	//	unassigned
			ERROR_EXIT("Invalid attribute plugin::type");

		//	Name
		if (!plugin.HasMember("name") || !plugin["name"].IsString())
			ERROR_EXIT("Missing attribute plugin::name");
		header.name.assign(plugin["name"].GetString());

		//	labelA
		if (plugin.HasMember("labelA") && plugin["labelA"].IsUint())
		{
			header.txt_labelA = plugin["labelA"].GetUint();
			if (header.txt_labelA >= aPlugin->mLanguage->GetTextCount())
			{
				printf("plugin::labelA(%d) out of bounds\n", header.txt_labelA);
				ERROR_EXIT("plugin::labelA > Languages.json::text count");
			}
		}
		else
			ERROR_EXIT("Missing attribute plugin::labelA");

		//	labelB
		if (plugin.HasMember("labelB") && plugin["labelB"].IsUint())
		{
			header.txt_labelB = plugin["labelB"].GetUint();
			if (header.txt_labelB >= aPlugin->mLanguage->GetTextCount())
			{
				printf("plugin::labelB(%d) out of bounds\n", header.txt_labelB);
				ERROR_EXIT("plugin::labelB > Languages.json::text count");
			}
		}
		else
			ERROR_EXIT("Missing attribute plugin::labelB");

		//	icon
		if (!plugin.HasMember("icon") || !plugin["icon"].IsString())
			ERROR_EXIT("Missing attribute plugin::icon");
		header.icon.assign(plugin["icon"].GetString());

#if 0
		printf("Plugin(%s)\n", path->Path());
		printf("\tname=%s\n", header.name.c_str());
		printf("\tvendor=%s\n", header.vendor.c_str());
		printf("\ttype=%s\n",  type.c_str());
		printf("\tlabelA=%s\n", header.labelA.c_str());
		printf("\tlabelB=%s\n", header.labelB.c_str());
		printf("\ttooltip=%s\n", header.tooltip.c_str());
		printf("\ticon=%s\n", header.icon.c_str());
#endif
	}

/****************************
	"fragment" (shader)
****************************/
	{
		if (!document.HasMember("fragment"))
			ERROR_EXIT("Missing object \"fragment\"");
		const rapidjson::Value &fragment = document["fragment"];
		aPlugin->mFragmentShader.type = PluginShader::eFragment;

		/****************************
			"uniforms"
		*****************************/
		if (!fragment.HasMember("uniforms"))
			ERROR_EXIT("Missing object \"fragment\": \"uniforms\"");
		const rapidjson::Value &uniforms = fragment["uniforms"];
		if (!uniforms.IsArray())
			ERROR_EXIT("\"uniforms\" is not an array");
		for (auto &v : uniforms.GetArray())
		{
			PluginUniform aUniform;

			//	"type"
			if (!v.HasMember("type") || !v["type"].IsString())
				ERROR_EXIT("Missing attribute uniforms::type");
			std::string uniform_type(v["type"].GetString());
			std::size_t uniform_idx = 0;
			while (uniform_idx < kUniformTypes.size())
			{
				if (uniform_type.compare(kUniformTypes[uniform_idx]) == 0)
				{
					aUniform.type = (PluginUniform::UniformType) uniform_idx;
					break;
				}
				uniform_idx++;
			}
			if (uniform_idx >= kUniformTypes.size())
				ERROR_EXIT("Invalid attribute fragment::uniforms::type");

			//	"name"
			if (!v.HasMember("name") || !v["name"].IsString())
				ERROR_EXIT("Missing attribute fragment::uniforms::name");
			aUniform.name.assign(v["name"].GetString());

			aPlugin->mFragmentShader.uniforms.push_back(aUniform);
		}

#if 0
		printf("Plugin(%s) FragmentShader::uniforms\n", path->Path());
		for (auto &u : aPlugin->mFragmentShader.uniforms)
		{
			printf("\tUniform: %s (%s)\n", u.name.c_str(), kUniformTypes[u.type]);
		}
#endif

		/****************************
			"source"
		*****************************/
		if (!fragment.HasMember("source"))
			ERROR_EXIT("Missing object \"fragment\": \"source\"");
		const rapidjson::Value &source = fragment["source"];

		if (source.HasMember("file"))
		{
			if (!source["file"].IsString())
				ERROR_EXIT("Invalid attribute \"fragment\":\"source\":\"file\"");
			aPlugin->mFragmentShader.source_file.assign(source["file"].GetString());

			//	Attempt to locate file
			char *source_buffer = ReadFileToBuffer(aPlugin->mFragmentShader.source_file.c_str());
			if (!source_buffer)
			{
				// 2nd attempt, from application directory
				app_info appInfo;
				be_app->GetAppInfo(&appInfo);
				BPath executable_path(&appInfo.ref);
				BString shader_path(executable_path.Path());
				int last_dir = shader_path.FindLast('/');
				shader_path.Truncate(last_dir+1);
				shader_path.Append(aPlugin->mFragmentShader.source_file.c_str());
				source_buffer = ReadFileToBuffer(shader_path.String());
			}
			if (!source_buffer)
			{
				//	3rd attempt, from B_USER_CONFIG_DIRECTORY directory
				BPath config_path;
				find_directory(B_USER_CONFIG_DIRECTORY, &config_path);
				BString shader_path(config_path.Path());
				shader_path.Append("/settings/Medo/");
				shader_path.Append(aPlugin->mFragmentShader.source_file.c_str());
				source_buffer = ReadFileToBuffer(shader_path.String());
				if (source_buffer)
				{
					BString icon_path(config_path.Path());
					icon_path.Append("/settings/Medo/");
					icon_path.Append(aPlugin->mHeader.icon.c_str());
					aPlugin->mHeader.icon.assign(icon_path.String());
				}
			}

			if (source_buffer)
			{
				aPlugin->mFragmentShader.source_text.assign(source_buffer);
				delete [] source_buffer;
			}
			else
			{
				printf("Error parsing file: %s\n", aPlugin->mFragmentShader.source_file.c_str());
				ERROR_EXIT("Failed to open \"fragment\":\"source\":\"file\"");
			}

		}
		else if (source.HasMember("text"))
		{
			if (!source["text"].IsString())
				ERROR_EXIT("Invalid attribute \"fragment\":\"source\":\"text\"");
			aPlugin->mFragmentShader.source_text.assign(source["text"].GetString());
		}
		else
			ERROR_EXIT("Missing attribute \"fragment\":\"source\": \"file\" or \"text\"");

#if 0
		if (aPlugin->mFragmentShader.source_file.size() > 0)
			printf("Plugin(%s) FragmentShader::Source::File = %s\n", path->Path(), aPlugin->mFragmentShader.source_file.c_str());

		printf("Plugin(%s) FragmentShader::Source::Text\n%s\n", path->Path(), aPlugin->mFragmentShader.source_text.c_str());
#endif

		/****************************
			"gui"
		*****************************/
		if (!fragment.HasMember("gui"))
			ERROR_EXIT("Missing object \"fragment\": \"gui\"");
		const rapidjson::Value &gui = fragment["gui"];
		if (!gui.IsArray())
			ERROR_EXIT("\"gui\" is not an array");
		for (auto &v : gui.GetArray())
		{
			PluginGuiWidget aWidget;

			//	gui::type
			if (!v.HasMember("type") || !v["type"].IsString())
				ERROR_EXIT("Missing attribute fragment::gui::type");
			std::string gui_type(v["type"].GetString());
			std::size_t widget_idx = 0;
			while (widget_idx < kGuiWidgets.size())
			{
				if (gui_type.compare(kGuiWidgets[widget_idx]) == 0)
				{
					aWidget.widget_type = (PluginGuiWidget::GuiWidget) widget_idx;
					break;
				}
				widget_idx++;
			}
			if (widget_idx >= kGuiWidgets.size())
				ERROR_EXIT("Invalid attribute fragment::gui::type");

			//	gui::rect
			if (v.HasMember("rect") && v["rect"].IsArray())
			{
				const rapidjson::Value &arr = v["rect"];
				rapidjson::SizeType num_elems = arr.GetArray().Size();
				if (num_elems == 4)
				{
					aWidget.rect.left = arr.GetArray()[0].GetFloat();
					aWidget.rect.top = arr.GetArray()[1].GetFloat();
					aWidget.rect.right = arr.GetArray()[2].GetFloat();
					aWidget.rect.bottom = arr.GetArray()[3].GetFloat();
				}
				else
					ERROR_EXIT("Invalid attribute fragment::gui::rect");
			}
			else
				ERROR_EXIT("Invalid attribute fragment::gui::rect");

			//	gui::label
			if (v.HasMember("label") && v["label"].IsUint())
			{
				aWidget.txt_label = v["label"].GetUint();
				if (aWidget.txt_label >= aPlugin->mLanguage->GetTextCount())
				{
					printf("plugin::label(%d) out of bounds\n", aWidget.txt_label);
					ERROR_EXIT("fragment::gui::label > Languages.json::text count");
				}
			}
			else
				ERROR_EXIT("Corrupt attribute fragment::gui::label");

			//	gui::uniform
			if (aWidget.widget_type != PluginGuiWidget::GuiWidget::eText)
			{
				if (!v.HasMember("uniform") || !v["uniform"].IsString())
					ERROR_EXIT("Missing attribute fragment::gui::uniform");
				aWidget.uniform.assign(v["uniform"].GetString());
				int uniform_idx = 0;
				for (auto &u : aPlugin->mFragmentShader.uniforms)
				{
					if (aWidget.uniform.compare(u.name) == 0)
					{
						aWidget.uniform_idx = uniform_idx;
						break;
					}
					++uniform_idx;
				}
				if (uniform_idx >= aPlugin->mFragmentShader.uniforms.size())
					ERROR_EXIT("No matching uniform for attribute fragment::gui::uniform");
			}

			//	gui::widget::optional
			switch (aWidget.widget_type)
			{
				case PluginGuiWidget::eSlider:
				{
					//	label_min
					if (v.HasMember("label_min") && v["label_min"].IsUint())
					{
						aWidget.txt_slider_min = v["label_min"].GetUint();
						if (aWidget.txt_slider_min >= aPlugin->mLanguage->GetTextCount())
							ERROR_EXIT("fragment::gui::slider::label_min > Languages.json::text count");
					}
					else
						ERROR_EXIT("Corrupt attribute fragment::gui::slider::label_min");

					//	label_max
					if (v.HasMember("label_max") && v["label_max"].IsUint())
					{
						aWidget.txt_slider_max = v["label_max"].GetUint();
						if (aWidget.txt_slider_max >= aPlugin->mLanguage->GetTextCount())
							ERROR_EXIT("fragment::gui::slider::label_max > Languages.json::text count");
					}
					else
						ERROR_EXIT("Corrupt attribute fragment::gui::slider::label_max");

					//	default
					if (!v.HasMember("default") || !v["default"].IsFloat())
						ERROR_EXIT("Missing attribute fragment::gui::slider::default");
					aWidget.default_value[0] = v["default"].GetFloat();

					//	range
					if (!v.HasMember("range"))
						ERROR_EXIT("Missing array fragment::gui::slider::range");
					if (!v["range"].IsArray())
						ERROR_EXIT("Element not an array fragment::gui::slider::range");
					const rapidjson::Value &arr = v["range"];
					rapidjson::SizeType num_elems = arr.GetArray().Size();
					if (num_elems == 2)
					{
						aWidget.range[0] = arr.GetArray()[0].GetFloat();
						aWidget.range[1] = arr.GetArray()[1].GetFloat();
					}
					else
						ERROR_EXIT("Invalid array fragment::gui::slider::range");
					break;
				}
				case PluginGuiWidget::eCheckbox:
				{
					//	default
					if (!v.HasMember("default") || !v["default"].IsInt())
						ERROR_EXIT("Missing attribute fragment::gui::checkbox::default");
					aWidget.default_value[0] = v["default"].GetInt();
					break;
				}
				case PluginGuiWidget::eRadioButton:
				{
					//	default
					if (!v.HasMember("default") || !v["default"].IsInt())
						ERROR_EXIT("Missing attribute fragment::gui::radiobutton::default");
					aWidget.default_value[0] = v["default"].GetInt();
					break;
				}
				case PluginGuiWidget::eSpinner2:
				{
					//	mouse_down
					if (v.HasMember("mouse_down"))
					{
						if (v["mouse_down"].IsBool())
						{
							bool mouse_down = v["mouse_down"].GetBool();
							aWidget.default_value[3] = mouse_down ? 1.0f : 0.0f;
						}
					}
					else
						ERROR_EXIT("Invalid fragment::gui::eSpinner2::mouse_down");
				}
				[[fallthrough]];
				case PluginGuiWidget::eSpinner3:
				case PluginGuiWidget::eSpinner4:
				{
					//	default
					if (!v.HasMember("default"))
						ERROR_EXIT("Missing array fragment::gui::eSpinner::default");
					if (!v["default"].IsArray())
						ERROR_EXIT("Element not an array fragment::gui::eSpinner::default");
					const rapidjson::Value &arr_default = v["default"];
					rapidjson::SizeType num_elems = arr_default.GetArray().Size();
					if (num_elems == PluginGuiWidget::kVecCountElements[aWidget.widget_type])
					{
						for (unsigned int i=0; i < num_elems; i++)
							aWidget.default_value[i] = arr_default.GetArray()[i].GetFloat();
					}
					else
						ERROR_EXIT("Invalid array fragment::gui::eSpinner::default");

					//	range
					if (!v.HasMember("range"))
						ERROR_EXIT("Missing array fragment::gui::eSpinner::range");
					if (!v["range"].IsArray())
						ERROR_EXIT("Element not an array fragment::gui::eSpinner::range");
					const rapidjson::Value &arr_range = v["range"];
					num_elems = arr_range.GetArray().Size();
					if (num_elems == 2)
					{
						aWidget.range[0] = arr_range.GetArray()[0].GetFloat();
						aWidget.range[1] = arr_range.GetArray()[1].GetFloat();
					}
					else
						ERROR_EXIT("Invalid array fragment::gui::eSpinner::range");
					break;
				}

				case PluginGuiWidget::eColour:
				{
					//	default
					if (!v.HasMember("default"))
						ERROR_EXIT("Missing array fragment::gui::colour::default");
					if (!v["default"].IsArray())
						ERROR_EXIT("Element not an array fragment::gui::colour::default");
					const rapidjson::Value &arr = v["default"];
					rapidjson::SizeType num_elems = arr.GetArray().Size();
					if ((num_elems >=3) && (num_elems <= 4))
					{
						for (uint32 i=0; i < num_elems; i++)
						{
							aWidget.vec4[i] = arr.GetArray()[i].GetFloat();
							aWidget.default_value[i] = aWidget.vec4[i];
						}
					}
					else
						ERROR_EXIT("Invalid array fragment::gui::colour::default");
					break;
				}
				case PluginGuiWidget::eText:
				{
					//	font
					if (!v.HasMember("font") || !v["font"].IsString())
						ERROR_EXIT("Missing attribute fragment::gui::text::font");
					std::string font;
					font.assign(v["font"].GetString());
					if (font.compare("be_plain_font") == 0)
						aWidget.uniform_idx = 0;
					else if (font.compare("be_bold_font") == 0)
						aWidget.uniform_idx = 1;
					else
						ERROR_EXIT("Corrupt attribute fragment::gui::text::font");
					break;
				}

				default:
					assert(0);
			}

			aPlugin->mFragmentShader.gui_widgets.push_back(aWidget);
		}

#if 0
		printf("Plugin(%s) FragmentShader::gui\n", path->Path());
		for (auto &w : aPlugin->mFragmentShader.gui_widgets)
		{
			printf("%s (%s)\n", w.label.c_str(), kGuiWidgets[w.type]);
			printf("Uniform: %s, uniform_idx=%d\n", w.uniform.c_str(), w.uniform_idx);
			w.rect.PrintToStream();
			switch (w.type)
			{
				case PluginGuiWidget::eSlider:
					printf("label_min(\"%s\"), label_max(\"%s\"), range[%f, %f]\n", w.slider_label_min.c_str(), w.slider_label_max.c_str(), w.slider_range[0], w.slider_range[1]);
					break;
				case PluginGuiWidget::eCheckbox:
					printf("default: %f\n", w.default_value);
					break;
				default:
					break;
			}
		}
#endif
	}
	delete [] data;

	fEffectPlugins.push_back(aPlugin);
	printf("Plugin(%s)\n", path->Path());
	return true;
}
