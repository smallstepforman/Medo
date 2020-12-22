/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified YRenderNode nodes (from Yarra engine)
*/

#include "SceneNode.h"
#include "Shader.h"
#include "Texture.h"
#include "MatrixStack.h"

namespace yrender
{
	
/*	FUNCTION:		YRenderNode :: YRenderNode
	ARGUMENTS:		autodestruct
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YRenderNode :: YRenderNode(const bool autodestruct)
	: fAutoDestruct(autodestruct), mGeometryNode(nullptr), mShaderNode(nullptr), mTexture(nullptr)
{ }

/*	FUNCTION:		YRenderNode :: ~YRenderNode
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YRenderNode :: ~YRenderNode()
{
	if (fAutoDestruct)
	{
		delete mGeometryNode;
		delete mShaderNode;
		delete mTexture;
	}
}

/*	FUNCTION:		YRenderNode :: Render
	ARGUMENTS:		delta_time
	RETURN:			n/a
	DESCRIPTION:	Render node
*/
void YRenderNode :: Render(float delta_time)
{
	yMatrixStack.Push();
	mSpatial.Transform();
	
	if (mTexture)
		mTexture->Render(delta_time);
	
	if (mShaderNode)
		mShaderNode->Render(delta_time);
	
	if (mGeometryNode)
		mGeometryNode->Render(delta_time);
	
	yMatrixStack.Pop();		
}

};	//	namespace yrender
