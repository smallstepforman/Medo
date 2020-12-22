/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Matrix operations, optimised for x86 processor
*/

#if defined (__GNUC__)
	#if defined(__i386__) || defined(__amd64__)
		#define Y_CPU_X86
	#endif
#elif defined (_MSC_VER)
	#define Y_CPU_X86
#endif

#ifdef Y_CPU_X86

#include <cassert>
#include <emmintrin.h>

namespace ymath
{

/*	FUNCTION:		YMatrixMultiply4f
	ARGUMENTS:		p
					a, b
	RETURN:			none
	DESCRIPTION:	Use x86 SSE instructions to perform matrix multiplication
*/
void YMatrixMultiply4f(float *p, const float *a, const float *b)
{
	assert ((p != a) && (p != b));
	__m128 a_line, b_line, p_line;
	for (int i=0; i<16; i+=4)
	{
		// unroll the first step of the loop to avoid having to initialize r_line to zero
		a_line = _mm_load_ps(a);
		b_line = _mm_set1_ps(b[i]);
		p_line = _mm_mul_ps(a_line, b_line);
		for (int j=1; j<4; j++)
		{
			a_line = _mm_load_ps(&a[j*4]);
			b_line = _mm_set1_ps(b[i+j]);
			p_line = _mm_add_ps(_mm_mul_ps(a_line, b_line), p_line);
		}
		_mm_store_ps(&p[i], p_line);     // p[i] = p_line
	}
}

};

#endif	//#ifdef Y_CPU_X86
