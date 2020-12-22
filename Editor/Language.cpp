/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Language text
 */

#include <cassert>
#include <cstdio>
#include <vector>
 
#include "Language.h"
#include "Language_English_Britain.inl"
#include "Language_English_USA.inl"
#include "Language_German.inl"
#include "Language_French.inl"
#include "Language_Italian.inl"
#include "Language_Russian.inl"
#include "Language_Serbian.inl"
#include "Language_Spanish.inl"

static const std::vector<const char *> kAvailableLanguages =
{
	"English (Britian)",
	"English (USA)",
	"Deutsch",
	"Français",
	"Italiano",
	"Русский",
	"Српски",
	"Español",
};

static_assert(sizeof(kLanguage_English_Britain)/sizeof(char *) == NUMBER_TXT_DEFINITIONS);
static_assert(sizeof(kLanguage_English_USA)/sizeof(char *) == NUMBER_TXT_DEFINITIONS);
static_assert(sizeof(kLanguage_German)/sizeof(char *) == NUMBER_TXT_DEFINITIONS);
//static_assert(sizeof(kLanguage_French)/sizeof(char *) == NUMBER_TXT_DEFINITIONS);
//static_assert(sizeof(kLanguage_Italian)/sizeof(char *) == NUMBER_TXT_DEFINITIONS);
//static_assert(sizeof(kLanguage_Russian)/sizeof(char *) == NUMBER_TXT_DEFINITIONS);
static_assert(sizeof(kLanguage_Serbian)/sizeof(char *) == NUMBER_TXT_DEFINITIONS);
static_assert(sizeof(kLanguage_Spanish)/sizeof(char *) == NUMBER_TXT_DEFINITIONS);


static const char **kLanguages[] = 
{
	kLanguage_English_Britain,
	kLanguage_English_USA,
	kLanguage_German,
	kLanguage_French,
	kLanguage_Italian,
	kLanguage_Russian,
	kLanguage_Serbian,
	kLanguage_Spanish,
};

static int sLanguageIndex = 0;

//=======================
void	SetLangauge(const uint32 index)
{
	assert(index < sizeof(kLanguages)/sizeof(char *));

	sLanguageIndex = index;
}

//=======================
int	GetLanguage()
{
	return sLanguageIndex;
}

//=======================
const std::vector<const char *>	&GetAvailableLanguages()
{
	return kAvailableLanguages;
}

//=======================
const char *GetText(LANGUAGE_TEXT text)
{
	assert((text >= 0) && (text < NUMBER_TXT_DEFINITIONS));
	return kLanguages[sLanguageIndex][text];	

}
