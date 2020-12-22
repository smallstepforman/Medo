
#ifndef _YARRA_PLATFORM_H_
#include "Yarra/Platform.h"
#endif

#if defined(WIN32)

	#ifdef APIENTRY
		# ifndef GLAPIENTRY
			#define GLAPIENTRY APIENTRY
		#endif
	#else
		#if defined(__MINGW32__) || defined(__CYGWIN__) || (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__)
			#define APIENTRY __stdcall
			#ifndef GLAPIENTRY
				#define GLAPIENTRY __stdcall
			#endif
		#else
			#define APIENTRY
		#endif
	#endif

	#ifndef CALLBACK
		#if defined(__MINGW32__) || defined(__CYGWIN__)
			#define CALLBACK __attribute__ ((__stdcall__))
		#elif (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
			#define CALLBACK __stdcall
		#else
			#define CALLBACK
		#endif
	#endif

	#ifndef GLAPI
		#define GLAPI 
	#endif

#else //#if defined(WIN32)
	#ifndef APIENTRY
		#define APIENTRY
	#endif
	#ifndef GLAPI
		#define GLAPI extern
	#endif
#endif	//#if defined(WIN32)

#ifndef GLAPIENTRY
	#define GLAPIENTRY
#endif
