/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd 
	DESCRIPTION:	Simple YMatrixStack (from Yarra engine)
*/

#ifndef __YARRA_MATRIX_STACK_H__
#define __YARRA_MATRIX_STACK_H__

#ifndef _YARRA_MATRIX4_H_
#include "Yarra/Math/Matrix4.h"
#endif

namespace yrender
{

using namespace ymath;

/***************************************
	Transform Matrix stack
	Replacement for glPushMatrix/glPopMatrix,
	as well as glTranslate/glRotate/glScale
****************************************/
class YMatrixStack
{
private:
	YMatrix4		**fStack;
	int				fStackIndex;
	YMatrix4		*fProjectionMatrix;
	void			FastMultiplyMatrix(const YMatrix4 *matrix);

public:
	YMatrixStack();
	~YMatrixStack();

	void			Push();
	void			Pop();
	void			Reset(YMatrix4 *projection, YMatrix4 *initial);
	void			MultiplyMatrix(const YMatrix4 *transform);
	void			LoadMatrix(const YMatrix4 *transform);

	const YMatrix4	GetMVPMatrix();
	inline const YMatrix4	*GetTopMatrix() const { return fStack[fStackIndex]; }

	void			Translate(const float x, const float y, const float z);
	inline void		Translate(const YVector3 &t)
	{
		Translate(t.x, t.y, t.z);
	}
	void			Rotate(const float angle, const float x, const float y, const float z);
	inline void		Rotate(const float angle, const YVector3 &axis)
	{
		Rotate(angle, axis.x, axis.y, axis.z);
	}
	void			Scale(const float x, const float y, const float z);
	inline void		Scale(const YVector3 &t)
	{
		Scale(t.x, t.y, t.z);
	}
};

extern YMatrixStack		yMatrixStack;

};	//	namespace yrender

#endif	//#ifndef __YARRA_MATRIX_STACK_H__
