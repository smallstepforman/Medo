/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Effect None (simple render)
 */

#include <cstdio>
#include <cassert>

#include <interface/Bitmap.h>

#include "Yarra/Render/Picture.h"
#include "Yarra/Render/Texture.h"
#include "Yarra/Render/MatrixStack.h"

#include "Editor/EffectNode.h"
#include "Editor/Project.h"
#include "Editor/RenderActor.h"
#include "Editor/ImageUtility.h"

#include "Effect_None.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

using namespace yrender;

/*	FUNCTION:		Effect_None :: Effect_None
	ARGS:			frame
					filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Effect_None :: Effect_None(BRect frame, const char *filename)
	: EffectNode(frame, filename)
{
}

/*	FUNCTION:		Effect_None :: ~Effect_None
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Effect_None :: ~Effect_None()
{
}

/*	FUNCTION:		Effect_None :: RenderEffect
	ARGS:			source
					data
					frame_idx
					chained_effects
	RETURN:			applied effect
	DESCRIPTION:	Apply media effect
*/
void Effect_None :: RenderEffect(BBitmap *source, MediaEffect *data, int64 frame_idx, std::deque<FRAME_ITEM> & chained_effects)
{
	if (!source)
		return;

	unsigned int w = source->Bounds().IntegerWidth() + 1;
	unsigned int h = source->Bounds().IntegerHeight() + 1;
	yrender::YPicture *picture = gRenderActor->GetPicture(w, h, source);

	//	Chained spatial transform
	yMatrixStack.Push();
	bool chained_spatial = false;
	bool chained_effect = false;
	if (!chained_effects.empty())
	{
		std::deque<FRAME_ITEM>::iterator i = chained_effects.begin();
		while ((i != chained_effects.end()) && (chained_effects[0].track == i->track))
		{
			if (!(*i).effect)
				break;
			if ((*i).effect->mEffectNode->IsSpatialTransform())
			{
				chained_spatial = true;
				(*i).effect->mEffectNode->ChainedSpatialTransform((*i).effect, frame_idx);
				chained_effects.erase(i);
				break;
			}
			else if (((*i).effect->mEffectNode->GetEffectGroup() == EffectNode::EFFECT_COLOUR) ||
					 ((*i).effect->mEffectNode->GetEffectGroup() == EffectNode::EFFECT_IMAGE) ||
					 ((*i).effect->mEffectNode->GetEffectGroup() == EffectNode::EFFECT_TRANSITION) ||
					 ((*i).effect->mEffectNode->GetEffectGroup() == EffectNode::EFFECT_SPECIAL))
			{
				((*i).effect->mEffectNode->RenderEffect(source, (*i).effect, frame_idx, chained_effects));
				chained_effects.erase(i);
				chained_effect = true;
				break;
			}
			else
				break;
			i++;
		}
	}

	if (!chained_spatial)
	{
		picture->mSpatial.SetPosition(ymath::YVector3(0.5f*gProject->mResolution.width, 0.5f*gProject->mResolution.height, 0));
		picture->mSpatial.SetRotation(ymath::YVector3(0, 0, 0));
		picture->mSpatial.SetScale(ymath::YVector3(0.5f*w, 0.5f*h, 1));
	}

	if (!chained_effect)
		picture->Render(0.0f);

	yMatrixStack.Pop();

	if (picture->mSpatial.GetPosition() != ymath::YVector3(0, 0, 0))
	{
#if 0
		printf("Effect_None :: RenderEffect() - spatial corrupt ");
		picture->mSpatial.GetPosition().PrintToStream();
#endif
		picture->mSpatial.SetPosition(ymath::YVector3(0, 0, 0));
	}
}
