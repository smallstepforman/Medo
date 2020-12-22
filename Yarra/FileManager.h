/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Yarra file manager allows resources to be packed / raw
*/

#ifndef _YARRA_FILE_MANAGER_H_
#define _YARRA_FILE_MANAGER_H_

#ifdef __cplusplus
#include <fstream>
extern "C" {
#else
#include <stdio.h>
#endif

typedef struct
{
	FILE			*mFile;
	void			*mItem;
	size_t			mOffset;
	int				mRawPath;	//	1 = true, 0 = false
} YFILE;

YFILE		*Yfopen(const char *filename, const char *mode);
int			Yfclose(YFILE * stream);
size_t		Yfread(void *ptr, size_t size, size_t count, YFILE *stream);
int			Yfseek(YFILE *stream, size_t offset, int origin);
size_t		Yftell(YFILE *stream);
int			Yfeof(YFILE *stream);
void		Yrewind(YFILE *stream);

void		YInitFileManager(const char *filename);
void		YAddFileSystem(const char *filename);
void		YDestroyFileManager();

#ifdef __cplusplus
}
#endif

#endif	//#ifndef __YARRA_FILE_MANAGER_H__


