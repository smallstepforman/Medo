/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	Render Actor (OpenGL)
 */

#ifndef _RENDER_ACTOR_H_
#define _RENDER_ACTOR_H_

#ifndef __gl_h_
#include <GL/gl.h>
#endif

#ifndef _YARRA_ACTOR_H_
#include "Actor/Actor.h"
#endif

class BBitmap;
class BMessage;

class EffectNode;
class RenderView;

namespace yrender
{
	class YPicture;
};
class PictureCache;

class RenderActor : public yarra::Actor
{
public:
				RenderActor(BRect frame);
				~RenderActor();

	void		AsyncInitOpenGlView(BRect frame);
	void		AsyncCreateEffectNode(EffectNode *node);
	void		AsyncPrepareFrame(bigtime_t frame_idx);
	void		AsyncPlayFrame(bigtime_t frame_idx, Actor *completion, std::function<void()> behaviour);
	void		AsyncPreloadFrame(bigtime_t frame_idx);
	void		AsyncPrepareExportFrame(bigtime_t frame_idx, sem_id sem_signal, BBitmap **bitmap);
	void		AsyncInvalidateTimelineEdit();
	void		AsyncInvalidateProjectSettings(int32 sem_id);
	void		WaitIdle();

	yrender::YPicture	*GetPicture(const unsigned width, unsigned int height, BBitmap *source);

	//	RenderTargets for EffectNodes (eg. Blur)
	void		ActivateSecondaryRenderBuffer(const bool is_alpha_clear = false);
	void		DeactivateSecondaryRenderBuffer();
	BBitmap		*GetSecondaryFrameBufferTexture(GLenum format = GL_RGBA);
	BBitmap		*GetBackgroundBitmap() {return fBackgroundBitmap;}
	BBitmap		*GetCurrentFrameBufferTexture(GLenum format = GL_RGBA);
	void		EffectResetPrimaryRenderBuffer();		//	caution, will reset compositing

private:
	BBitmap			*GetOutputFrame(int64 frame_idx);

	RenderView		*fRenderView;
	BBitmap			*fBackgroundBitmap;
	PictureCache	*fPictureCache;

	//	Messaging support
	BMessage		*fPreviewMessage;
	BMessage		*fMsgInvalidateTimelineEdit;
};
extern RenderActor	*gRenderActor;



#endif	//#ifndef _RENDER_ACTOR_H_

