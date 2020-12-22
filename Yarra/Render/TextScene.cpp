/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2011, ZenYes Pty Ltd
	DESCRIPTION:	Text scene node
*/

#include <cassert>

#include "Yarra/Render/MatrixStack.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Font.h"

namespace yrender
{
	
/*	FUNCTION:		YTextScene :: YTextScene
	ARGUMENTS:		font
					acquire_font_ownership
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YTextScene :: YTextScene(YFont *font, bool acquire_font_ownership)
	: fFont(font), fAcquireFontOwnership(acquire_font_ownership), fGeometryNode(nullptr),
	fHorizontalAlignment(ALIGN_HCENTER), fVerticalAlignment(ALIGN_VCENTER) 
{
	assert(font != nullptr);
	fGeometrySize.Set(0, 0, 0);
	fColour.Set(1,1,1,1);
}

/*	FUNCTION:		YTextScene :: ~YTextScene
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YTextScene :: ~YTextScene()
{
	delete fGeometryNode;
	if (fAcquireFontOwnership)
		delete fFont;
}

/*	FUNCTION:		YTextScene :: SetText
	ARGUMENTS:		text
	RETURN:			n/a
	DESCRIPTION:	Set text
*/
void YTextScene :: SetText(const char *text)
{
	if (fText.compare(text) == 0)
		return;

	fText.assign(text);
	
	delete fGeometryNode;
	fGeometryNode = fFont->CreateGeometry(text);
	fGeometrySize = fFont->GetGeometrySize();
}

/*	FUNCTION:		YTextScene :: SetColour
	ARGUMENTS:		colour
	RETURN:			n/a
	DESCRIPTION:	Convenience function to set colour
*/
void YTextScene :: SetColour(const YVector4 &colour)
{
	fColour = colour;
}

/*	FUNCTION:		YTextScene :: Render
	ARGUMENTS:		delta_time
	RETURN:			n/a
	DESCRIPTION:	Render text
*/
void YTextScene :: Render(float delta_time)
{
	if (fGeometryNode == nullptr)
		return;

	glEnable(GL_BLEND);
	yMatrixStack.Push();
	mSpatial.Transform();

	YVector3 offset(0,0,0);
	switch (fHorizontalAlignment)
	{
		case ALIGN_HCENTER:		offset.x = -0.5f*fGeometrySize.x;		break;
		case ALIGN_RIGHT:		offset.x = -fGeometrySize.x;			break;
		case ALIGN_LEFT:		break;
	}
	switch (fVerticalAlignment)
	{
		case ALIGN_VCENTER:			offset.y = -0.5f*(fGeometrySize.y + fGeometrySize.z);	break;
		case ALIGN_ASCENT_CENTER:	offset.y = -0.5f*fGeometrySize.y;						break;
		case ALIGN_BASELINE:		break;
	}
	yMatrixStack.Translate(offset.x, offset.y, offset.z);

	fFont->PreRender(fColour);
	fGeometryNode->Render(delta_time);
	yMatrixStack.Pop();
//	glDisable(GL_BLEND);
}

};	// namespace yrender
