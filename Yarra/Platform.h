/*	PROJECT:		Yarra
	COPYRIGHT:		2017-2018,  Zen Yes Pty Ltd, Australia
	AUTHORS:		Zenja Solaja
	DESCRIPTION:	Yarra Platform abstraction
*/

#ifndef _YARRA_PLATFORM_H_
#define _YARRA_PLATFORM_H_

#define GL_GLEXT_PROTOTYPES		1
#include <GL/gl.h>
#include <GL/glext.h>

#ifndef _STDDEF_H_
#include <stddef.h>		//	needed for size_t
#endif

namespace yplatform
{
	void			Debug(const char *text, ...);
	void			Exit(const char *error, ...);
	const double	GetElapsedTime();
	void			PrintOpenGLError();

	void			*AlignedAlloc(size_t alignment, size_t size);
	void			AlignedFree(void* ptr );
};	//	namespace yplatform

#endif	//#ifndef _YARRA_PLATFORM_H_
