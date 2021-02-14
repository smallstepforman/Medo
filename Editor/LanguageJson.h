/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Language Json parser
 *					Only the current language is stored in memory
 */

#ifndef LANGUAGE_JSON_H
#define LANGUAGE_JSON_H

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _B_STRING_H
#include <support/String.h>
#endif

class LanguageJson
{
public:
					LanguageJson(const char *filename);
					~LanguageJson();
	const char		*GetText(uint32 index);
	const uint32	GetTextCount() const;

private:
	std::vector<BString>	fText;
};

#endif //#ifndef LANGUAGE_JSON_H
