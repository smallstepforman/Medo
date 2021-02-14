/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Language Json parser
 *					Only the current language is stored in memory
 */

#include <StorageKit.h>
#include <AppKit.h>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "FileUtility.h"
#include "Language.h"
#include "LanguageJson.h"

#include <InterfaceKit.h>

/*	FUNCTION:		LanguageJson :: LanguageJson
	ARGUMENTS:		filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
LanguageJson :: LanguageJson(const char *filename)
{
	char *data = ReadFileToBuffer(filename);
	if (!data)
	{
		app_info appInfo;
		be_app->GetAppInfo(&appInfo);
		BPath executable_path(&appInfo.ref);
		BString file_path(executable_path.Path());
		int last_dir = file_path.FindLast('/');
		file_path.Truncate(last_dir+1);
		file_path.Append(filename);
		data = ReadFileToBuffer(file_path.String());

		if (!data)
		{
			BAlert *alert = new BAlert("Failed to Load", file_path.String(), "OK");
			alert->Go();
		}
	}

	if (data)
	{
#define ERROR_EXIT(a) {printf("LanguageJson() Error(%s)\n", a);	delete [] data; return;}
		rapidjson::Document document;
		rapidjson::ParseResult res = document.Parse<rapidjson::kParseTrailingCommasFlag>(data);
		if (!res)
		{
			printf("LanguageJson(%s) JSON parse error: %s (Byte offset: %lu)\n", filename, rapidjson::GetParseError_En(res.Code()), res.Offset());

			for (int c=-20; c < 20; c++)
				printf("%c", data[res.Offset() + c]);
			printf("\n");
			return;
		}

		//	"languages" (header)
		if (!document.HasMember("languages") || !document["languages"].IsArray())
			ERROR_EXIT("Missing array \"languages\"");
		const rapidjson::Value &languages = document["languages"];
		bool initial = true;
		for (auto &p : languages.GetArray())
		{
			//	"language"
			if (!p.HasMember("language") || !p["language"].IsString())
				ERROR_EXIT("Corrupt field: \"language\"");

			//	Always parse first language, and potentially overwrite with currently selected language (only keep single language in memory)
			BString language(p["language"].GetString());
			if (initial || (language.Compare(gLanguageManager->GetCurrentLanguageName()) == 0))
			{
				//	"text"
				if (!p.HasMember("text") || !p["text"].IsArray())
					ERROR_EXIT("Corrupt field: \"text\"");
				const rapidjson::Value &text = p["text"];
				for (rapidjson::SizeType line=0; line < text.GetArray().Size(); line++)
				{
					if (!text.GetArray()[line].IsString())
					{
						printf("Parse error: language(%s) line(%u)\n", language.String(), line);
						ERROR_EXIT("Invalid string");
					}
					if (initial)
						fText.push_back(BString(text.GetArray()[line].GetString()));
					else
						fText[line].SetTo(text.GetArray()[line].GetString());
				}
			}
			initial = false;
		}
	}
	else
		printf("LanguagesJson(%s) - file not found\n", filename);

	delete [] data;

#if 0
	printf("LanguageJson(%s)\n", filename);
	for (auto &i : fText)
		 printf("\t%s\n", i.String());
#endif
}

/*	FUNCTION:		LanguageJson :: LanguageJson
	ARGUMENTS:		filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
LanguageJson :: ~LanguageJson()
{
	fText.clear();
}

/*	FUNCTION:		LanguageJson :: GetText
	ARGUMENTS:		index
	RETURN:			text at index
	DESCRIPTION:	Get text at index
*/
const char * LanguageJson :: GetText(uint32 index)
{
	if (index < fText.size())
		return fText[index].String();
	else
		return "???";
}

/*	FUNCTION:		LanguageJson :: GetTextCount
	ARGUMENTS:		none
	RETURN:			count text
	DESCRIPTION:	Get text count
*/
const uint32 LanguageJson :: GetTextCount() const
{
	return fText.size();
}

