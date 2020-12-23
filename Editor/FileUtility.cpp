/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Common file utility functions
 */

#include <cstdio>

#include "FileUtility.h"

/*	FUNCTION:		ReadFile
	ARGUMENTS:		filename
	RETURN:			char * (client acquires ownership)
	DESCRIPTION:	Load file into char buffer
*/
char * ReadFileToBuffer(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	if (!file)
		return nullptr;

	size_t filesize;
	fseek (file, 0, SEEK_END);
	filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (filesize == 0)
	{
		fclose(file);
		return nullptr;
	}
	char *data = new char[filesize + 1];	// reserve space for zero terminator
	if (!data)
	{
		fclose(file);
		return nullptr;

	}
	fread(data, 1, filesize, file);
	data[filesize] = 0;			//	terminator
	fclose(file);
	return data;
}
