/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simple YMatrixStack from Yarra engine
*/

#include <cassert>

#include "Yarra/Platform.h"
#include "Yarra/Math/Matrix4.h"
#include "Yarra/Math/Quaternion.h"
#include "MatrixStack.h"

namespace yrender
{
	
YMatrixStack yMatrixStack;

/***************************************
	Transform Matrix stack
	Replacement for glPushMatrix/glPopMatrix,
	as well as glTranslate/glRotate/glScale
****************************************/

#define MAX_TRANSFORM_STACK_SIZE		16

/*	FUNCTION:		YMatrixStack :: YMatrixStack
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YMatrixStack::YMatrixStack()
{
	fStack = new YMatrix4 *[MAX_TRANSFORM_STACK_SIZE];
	for (int i = 0; i < MAX_TRANSFORM_STACK_SIZE; i++)
		fStack[i] = (YMatrix4 *)yplatform::AlignedAlloc(16, sizeof(YMatrix4));

	fStackIndex = 0;
}

/*	FUNCTION:		YMatrixStack :: ~YMatrixStack
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YMatrixStack :: ~YMatrixStack()
{
	for (int i = 0; i < MAX_TRANSFORM_STACK_SIZE; i++)
		yplatform::AlignedFree(fStack[i]);
	delete[] fStack;
}

/*	FUNCTION:		YMatrixStack :: FastMultiplyMatrix
	ARGUMENTS:		m
	RETURN:			n/a
	DESCRIPTION:	fStack[fStackIndex] *= m
					We use fStack[fStackIndex + 1] as a temporary matrix
					Instead of doing a memcpy(fStackIndex, fStackIndex+1),
					all we need to do is swap stack pointers
*/
void YMatrixStack::FastMultiplyMatrix(const YMatrix4 *m)
{
	//	perform multiplication, store result in fStack[fStackIndex+1]
	YMatrixMultiply4(fStack[fStackIndex + 1], fStack[fStackIndex], m);

	//	swap pointers
	YMatrix4 *t = fStack[fStackIndex];
	fStack[fStackIndex] = fStack[fStackIndex + 1];
	fStack[fStackIndex + 1] = t;
}

/*	FUNCTION:		YMatrixStack :: Push
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Preserve original transformation matrix before modification.
					Same as glPushMatrix()
*/
void YMatrixStack::Push()
{
	fStackIndex++;
	assert(fStackIndex < MAX_TRANSFORM_STACK_SIZE - 1);	// reserve one for working stack

	//	Copy existing transform 
	*fStack[fStackIndex] = *fStack[fStackIndex - 1];
}

/*	FUNCTION:		YMatrixStack :: Pop
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Restore previous transformation matrix (cancel modification effects)
					Same as glPopMatrix()
*/
void YMatrixStack::Pop()
{
	if (fStackIndex > 0)
		fStackIndex--;
	else
		assert(0);
}

/*	FUNCTION:		YMatrixStack :: Reset
	ARGUMENTS:		initial
	RETURN:			n/a
	DESCRIPTION:	Set up initial transformation matrix
					If the stack contains more than one element,
					we have a Push/Pop mismatch
*/
void YMatrixStack::Reset(YMatrix4 *projection, YMatrix4 *initial)
{
	if (fStackIndex != 0)
		yplatform::Debug("YMatrixStack::Reset() - YPushMatrix/YPopMatrix stack error\n");
		
	fProjectionMatrix = projection;
	fStackIndex = 0;
	*fStack[fStackIndex] = (*initial);
}

/*	FUNCTION:		YMatrixStack :: MultiplyMatrix
	ARGUMENTS:		transform
	RETURN:			n/a
	DESCRIPTION:	Modify current transform matrix
					Same as glMultMatrixf(transform)
*/
void YMatrixStack::MultiplyMatrix(const YMatrix4 *transform)
{
	FastMultiplyMatrix(transform);
}

/*	FUNCTION:		YMatrixStack :: LoadMatrix
	ARGUMENTS:		transform
	RETURN:			n/a
	DESCRIPTION:	Replace current transform with argument
					Same as glLoadMatrix(transform)
*/
void YMatrixStack::LoadMatrix(const YMatrix4 *transform)
{
	*fStack[fStackIndex] = *transform;
}

/*	FUNCTION:		YMatrixStack :: Translate
	ARGUMENTS:		x, y, z
	RETURN:			n/a
	DESCRIPTION:	Modify current transform matrix
					Same as glTranslatef(x, y, z)
*/
void YMatrixStack::Translate(const float x, const float y, const float z)
{
	YMatrix4 t;
	t.LoadIdentity();
	t.Translate(YVector3(x, y, z));
	FastMultiplyMatrix(&t);
}

/*	FUNCTION:		YMatrixStack :: Scale
	ARGUMENTS:		x, y, z
	RETURN:			n/a
	DESCRIPTION:	Modify current transform matrix
					Same as glScalef(x, y, z)
*/
void YMatrixStack::Scale(const float x, const float y, const float z)
{
	YMatrix4 s;
	s.LoadIdentity();
	s.Scale(YVector3(x, y, z));
	FastMultiplyMatrix(&s);
}

/*	FUNCTION:		YMatrixStack :: Rotate
	ARGUMENTS:		angle
					x, y, z
	RETURN:			n/a
	DESCRIPTION:	Modify current transform matrix
					Same as glRotatef(angle, x, y, z)
					ASSUMPTION. x/y/z indicate a single axis
*/
void YMatrixStack::Rotate(const float angle, const float x, const float y, const float z)
{
	YMatrix4 r;
	r.LoadIdentity();
	r.Rotate(YQuaternion(angle, YVector3(x, y, z)));
	FastMultiplyMatrix(&r);
}

/*	FUNCTION:		YMatrixStack :: GetMVPMatrix
	ARGUMENTS:		none
	RETURN:			projection * modelview matrix
	DESCRIPTION:	Convenience function to get projection * modelview matrix
*/
const YMatrix4 
YMatrixStack :: GetMVPMatrix()
{
	return *fProjectionMatrix * *fStack[fStackIndex];
}

};	//	namespace yrender

