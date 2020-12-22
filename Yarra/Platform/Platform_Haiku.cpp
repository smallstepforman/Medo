/*	PROJECT:		Yarra
	COPYRIGHT:		2008-2018, Zenja Solaja, Melbourne, Australia
	AUTHORS:		Zenja Solaja
	FILE:			Platform_Haiku.cpp
	DESCRIPTION:	Platform specific utility functions / objects
*/
#include <sys/time.h>
#include <kernel/OS.h>
#include <string>

#include "Yarra/Platform.h"

namespace yplatform
{
	
/*	FUNCTION:		yplatform::GetElapsedTime
	ARGUMENTS:		none
	RETURN:			time in float format
	DESCRIPTION:	Platform specific timer function.
					This function is only useful for delta_time calculations.
*/
const double GetElapsedTime()
{
	bigtime_t current_time = system_time();
	return double(current_time)/1000000.0f;
}

/*	FUNCTION:		yplatform::Debug
	ARGUMENTS:		text to display
	RETURN:			none
	DESCRIPTION:	Print debug output
 */
void Debug(const char *format, ...)
{
	static char gDebugPrintBuffer[0x400];	// OSX paths can be up to 1024 characters long
	
	va_list			ap;		//argument pointer
	*gDebugPrintBuffer = 0;
	
	// format string
	va_start(ap, format);
	vsprintf(gDebugPrintBuffer, format, ap);
	va_end(ap);
	
	printf("%s", gDebugPrintBuffer);
}

/*	FUNCTION:		yplatform::PrintOpenGLError
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Check for OpenGL errors, then print them
*/
void PrintOpenGLError()
{
	GLenum err;
	
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		Debug("[GL Error]  ");
		switch (err)
		{
			case GL_INVALID_ENUM:		Debug("GL_INVALID_ENUM\n");			break;
			case GL_INVALID_VALUE:		Debug("GL_INVALID_VALUE\n");		break;
			case GL_INVALID_OPERATION:	Debug("GL_INVALID_OPERATION\n");	break;
			case GL_STACK_OVERFLOW:		Debug("GL_STACK_OVERFLOW\n");		break;
			case GL_STACK_UNDERFLOW:	Debug("GL_STACK_UNDERFLOW\n");		break;
			case GL_OUT_OF_MEMORY:		Debug("GL_OUT_OF_MEMORY\n");		break;
			default:					Debug("%d\n", err);					break;
		}
	}
}

/*	FUNCTION:		yplatform::Exit
	ARGUMENTS:		message
	RETURN:			n/a
	DESCRIPTION:	Abnormal exit.  Print error message, then exit.
 */
void Exit(const char *message, ...)
{
	va_list			ap;		//argument pointer
	char buffer[256] = "";
	
	// format string
	va_start(ap, message);
	vsprintf(buffer, message, ap);
	va_end(ap);
	
	printf("%s\n", buffer);
	exit(1);
}

/*	FUNCTION:		yplatform::AlignedAlloc
	ARGUMENTS:		alignment
					size
	RETURN:			std::aligned_alloc
	DESCRIPTION:	C++17 std::aligned_alloc not available on some compilers 
*/
void * AlignedAlloc(size_t alignment, size_t size)
{
	void *p;
	if (posix_memalign(&p, alignment, size) != 0)
		p = nullptr;
	return p;
}

/*	FUNCTION:		yplatform::AlignedAlloc
	ARGUMENTS:		ptr
	RETURN:			n/a
	DESCRIPTION:	C++17 std::free not available on VS2015
*/
void AlignedFree(void *ptr)
{
	free(ptr);
}

};	//	namespace yarra
