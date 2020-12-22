/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified YPicture node (from Yarra engine)
*/

#ifndef __YARRA_PICTURE_H__
#define __YARRA_PICTURE_H__

#ifndef __YARRA_SCENE_NODE_H__
#include "SceneNode.h"
#endif

namespace yrender
{
	
class YPicture : public YRenderNode
{
public:
			YPicture(const char *filename, const bool double_sided = false, const bool inverse_texture_y = false);
			YPicture(unsigned int width, unsigned int height, const bool double_sided = false, const bool inverse_texture_y = false);
private:
	void	CreateGeometry(const bool double_sided, const bool inverse_texture);
};

};	//	namespace yrender

#endif	//#ifndef __YARRA_PICTURE_H__
