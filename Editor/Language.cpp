/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Language text
 */

#include <cassert>
#include <cstdio>
#include <vector>
#include <cstring>

#include <StorageKit.h>
#include <AppKit.h>

#include "FileUtility.h"
#include "Language.h"

//	TODO only load current language

LanguageManager *gLanguageManager = nullptr;

/*	FUNCTION:		LanguageManager :: LanguageManager
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
LanguageManager :: LanguageManager()
{
	assert(gLanguageManager == nullptr);
	gLanguageManager = this;
	fCurrentLanguageIndex = -1;

	//	Load languages from application path
	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	BPath executable_path(&appInfo.ref);
	BString file_path(executable_path.Path());
	int last_dir = file_path.FindLast('/');
	file_path.Truncate(last_dir+1);
	file_path.Append("Languages");
	ParseLanguageDirectory(file_path);

	// Load languages from Config path
	BPath config_path;
	find_directory(B_USER_CONFIG_DIRECTORY, &config_path);
	file_path.SetTo(config_path.Path());
	file_path.Append("/settings/Medo/Languages");
	ParseLanguageDirectory(file_path);

	if (fLanguages.empty())
	{
		printf("LanguageManager() error - no languages found\n");
		exit(1);
	}
	//	Available languages
	int idx = 0;
	for (auto &i : fLanguages)
	{
		printf("Available Language: %s\n", i->mName.String());
		if ((fCurrentLanguageIndex < 0) && (i->mName.Compare("English (Britain)") == 0))
			fCurrentLanguageIndex = idx;
		++idx;
	}
	if (fCurrentLanguageIndex >= 0)
		printf("Default Language: %s\n", fLanguages[fCurrentLanguageIndex]->mName.String());
	else
	{
		printf("Error: no default language\n");
		fCurrentLanguageIndex = 0;
	}
}

/*	FUNCTION:		LanguageManager :: ~LanguageManager
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
LanguageManager :: ~LanguageManager()
{
	printf("~LanguageManager()\n");
	for (auto &i : fLanguages)
		delete i;
}

/*	FUNCTION:		LanguageManager :: ParseLanguageDirectory
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Detect and load all languages
*/
void LanguageManager :: ParseLanguageDirectory(const BString &dir_path)
{
	printf("LoadLanguages: %s\n", dir_path.String());
	BEntry entry;
	BDirectory dir(dir_path);
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		if (!entry.IsDirectory())
		{
			BPath path(&entry);
			if (strstr(path.Path(), ".lang"))
			{
				ParseLanguageFile(path);
			}
		}
	}
}

/*	FUNCTION:		LanguageManager :: ParseLanguageFile
	ARGUMENTS:		path
	RETURN:			n/a
	DESCRIPTION:	Parse language file
*/
void LanguageManager :: ParseLanguageFile(const BPath &path)
{
	//printf("FParseLanguageFile: %s\n", path.Path());
	LanguageFile *aLanguage = new LanguageFile;
	aLanguage->mFilename.SetTo(path.Path());

	char *data = ReadFileToBuffer(path.Path());
	if (!data)
	{
		printf("ParseLanguageFile(%s) - error, cannot open file\n", path.Path());
		delete aLanguage;
		return;
	}

	char *itr = data;
	while (*itr)
	{
		while (*itr && (*itr != '"'))
			++itr;
		if (*itr == 0)
			break;

		itr++;
		char *start = itr;
		while (*itr && (*itr != '"'))
			++itr;
		if (*itr == 0)
		{
			printf("ParseLanguageFiles(%s) - mismatched quotes\n", path.Path());
			delete [] data;
			delete aLanguage;
			return;
		}
		if (itr != start)
		{
			aLanguage->mText.emplace_back(BString(start, itr-start));
		}
		else
			aLanguage->mText.emplace_back("<unknown>");
		++itr;
	}

	if (aLanguage->mText.size() == NUMBER_TXT_DEFINITIONS)
	{
		aLanguage->mName = path.Path();
		aLanguage->mName.RemoveLast(".lang");
		int32 last = aLanguage->mName.FindLast('/');
		aLanguage->mName.Remove(0, last + 1);
		fLanguages.push_back(aLanguage);
	}
	else
	{
		printf("*** ParseLanguageFile: %s ***\n", path.Path());
		printf("Invalid number of strings(%ld), expected(%d)\n", aLanguage->mText.size(), NUMBER_TXT_DEFINITIONS);
		delete aLanguage;
	}
	delete [] data;
}

/*	FUNCTION:		LanguageManager :: SetLanguage
	ARGUMENTS:		language_idx
	RETURN:			n/a
	DESCRIPTION:	Set current language index
*/
void LanguageManager :: SetLanguage(const BString &language)
{
	for (uint32 i=0; i < fLanguages.size(); i++)
	{
		if (fLanguages[i]->mName.Compare(language) == 0)
			fCurrentLanguageIndex = i;
	}
}

/*	FUNCTION:		LanguageManager :: GetNumberAvailableLanguages
	ARGUMENTS:		none
	RETURN:			count of available languages
	DESCRIPTION:	Get count of available languages
*/
uint32 LanguageManager :: GetNumberAvailableLanguages() const
{
	return (uint32) fLanguages.size();
}

/*	FUNCTION:		LanguageManager :: GetAvailableLanguages
	ARGUMENTS:		languages
	RETURN:			via argument
	DESCRIPTION:	Get list of available languages (used for SettingsWindow)
*/
void LanguageManager :: GetAvailableLanguages(std::vector<BString *> &languages)
{
	languages.clear();
	languages.reserve(fLanguages.size());
	for (auto &i : fLanguages)
		languages.push_back(&i->mName);
}

/*	FUNCTION:		LanguageManager :: GetCurrentLanguageName
	ARGUMENTS:		none
	RETURN:			Current language name
	DESCRIPTION:	Used by SettingsWindow
*/
const BString & LanguageManager :: GetCurrentLanguageName() const
{
	return fLanguages[fCurrentLanguageIndex]->mName;
}

/*	FUNCTION:		LanguageManager :: GetText
	ARGUMENTS:		text
	RETURN:			text in current language
	DESCRIPTION:	Get text in current language
*/
const char * LanguageManager :: GetText(LANGUAGE_TEXT text)
{
	return fLanguages[fCurrentLanguageIndex]->mText[text];
}

/*	FUNCTION:		GetText
	ARGUMENTS:		text
	RETURN:			text in current language
	DESCRIPTION:	Get text in current language
*/
const char *GetText(LANGUAGE_TEXT text)
{
	assert((text >= 0) && (text < NUMBER_TXT_DEFINITIONS));
	return gLanguageManager->GetText(text);

}
