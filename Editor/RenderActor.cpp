/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	OpenGL view
 */

#include <cstdio>
#include <cstring>

#include <opengl/GLView.h>
#include <interface/Bitmap.h>
#include <interface/Window.h>
#include <translation/TranslationUtils.h>

#include "Actor/Actor.h"

#include "Yarra/FileManager.h"
#include "Yarra/Math/Math.h"
#include "Yarra/Render/Camera.h"
#include "Yarra/Render/RenderTarget.h"
#include "Yarra/Render/Picture.h"
#include "Yarra/Render/Texture.h"

#include "EffectsManager.h"
#include "EffectNode.h"
#include "Effects/Effect_None.h"
#include "Effects/Effect_Speed.h"

#include "RenderActor.h"
#include "MedoWindow.h"
#include "Project.h"
#include "VideoManager.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

using namespace yrender;

RenderActor *gRenderActor = nullptr;

/**************************************
	RenderView
***************************************/
class RenderView : public BGLView
{
public:
				RenderView(BRect frame);
				~RenderView();
	void		ErrorCallback(unsigned long errorCode);
	void		ResetViewport();

	enum FRAME_BUFFER {PRIMARY_FRAME_BUFFER, SECONDARY_FRAME_BUFFER};
	void		ActivateFrameBuffer(FRAME_BUFFER target, const bool clear, const bool is_alpha_clear = false);
	void		DeactivateFrameBuffer(FRAME_BUFFER target);
	BBitmap		*GetFrameBufferBitmap(FRAME_BUFFER target, GLenum format = GL_RGBA);

private:
	yrender::YCamera 		*fCamera;
	enum {kNumberBitmapBuffers = 2};
	yrender::YRenderTarget	*fRenderTarget[2];
	BBitmap 				*fOpenGlBitmap[kNumberBitmapBuffers];
	int						fBitmapIndex;
};

/**********************************
	PictureCache
***********************************/
class PictureCache
{
	struct PICTURE_ITEM
	{
		unsigned int		width;
		unsigned int		height;
		BBitmap				*source;
		yrender::YPicture	*picture;
	};
	std::vector<PICTURE_ITEM>		fPictures;

public:
	~PictureCache()
	{
		for (auto i : fPictures)
			delete i.picture;
	}

	yrender::YPicture	*GetPicture(const unsigned int width, const unsigned int height, BBitmap *source)
	{
		for (auto i : fPictures)
		{
			if ((width == i.width) && (height == i.height))
			{
				if (i.source != source)
					i.picture->mTexture->Upload(source);
				return i.picture;
			}
		}

		PICTURE_ITEM aItem;
		aItem.width = width;
		aItem.height = height;
		aItem.picture = new yrender::YPicture(width, height, true, true);
		aItem.source - source;
		fPictures.push_back(aItem);
		aItem.picture->mTexture->Upload(source);
		return aItem.picture;
	}
};

/**************************************
	RenderActor
***************************************/

/*	FUNCTION:		RenderActor :: RenderActor
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
RenderActor :: RenderActor(BRect frame)
	: yarra::Actor(yarra::Actor::CONFIGURATION_LOCK_TO_THREAD)
{
	assert(gRenderActor == nullptr);
	gRenderActor = this;

	fRenderView = nullptr;
	fBackgroundBitmap = BTranslationUtils::GetBitmap("Resources/black.png");
	fPictureCache = new PictureCache;

	fPreviewMessage = new BMessage(MedoWindow::eMsgActionAsyncPreviewReady);
	fPreviewMessage->AddPointer("BBitmap", nullptr);
	fPreviewMessage->AddInt64("frame", 0);

	fMsgInvalidateTimelineEdit = new BMessage(MedoWindow::eMsgActionAsyncThumbnailReady);

	Async(&RenderActor::AsyncInitOpenGlView, this, frame);

}

/*	FUNCTION:		RenderActor :: ~RenderActor
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
RenderActor :: ~RenderActor()
{
	delete fRenderView;		//	TODO destructor must be run from same thread
	delete fBackgroundBitmap;
	delete fPictureCache;
	delete fPreviewMessage;
	delete fMsgInvalidateTimelineEdit;
}

/*	FUNCTION:		RenderActor ::AsyncInitOpenGlView
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Init OpenGL context (actor thread)
*/
void RenderActor :: AsyncInitOpenGlView(BRect frame)
{
	fRenderView = new RenderView(frame);
}

/*	FUNCTION:		RenderActor ::AsyncCreateEffectNode
	ARGS:			node
	RETURN:			n/a
	DESCRIPTION:	Create OpenGL effect node  (actor thread)
*/
void RenderActor :: AsyncCreateEffectNode(EffectNode *node)
{
	fRenderView->LockGL();
	node->InitRenderObjects();
	fRenderView->UnlockGL();
}

/*	FUNCTION:		RenderActor ::AsyncPrepareFrame
	ARGS:			frame_idx
	RETURN:			n/a
	DESCRIPTION:	Prepare rendered frame  (actor thread)
					Note - Filter older messages
*/
void RenderActor :: AsyncPrepareFrame(bigtime_t frame_idx)
{
	//	TODO ExportMedia will deadlock if AsyncPrepareExportFrame() messages destroyed
	ClearAllMessages();

	BBitmap *bitmap = GetOutputFrame(frame_idx);
	fPreviewMessage->ReplacePointer("BBitmap", bitmap);
	fPreviewMessage->ReplaceInt64("frame", frame_idx);
	MedoWindow::GetInstance()->PostMessage(fPreviewMessage);
}

/*	FUNCTION:		RenderActor ::AsyncPlayFrame
	ARGS:			frame_idx
					completion
					behaviour
	RETURN:			n/a
	DESCRIPTION:	Called by TimelinePlayer, prepare frame, display it, send completion message
*/
void RenderActor :: AsyncPlayFrame(bigtime_t frame_idx, Actor *completion, std::function<void()> behaviour)
{
	AsyncPrepareFrame(frame_idx);	//	Sync call
	completion->Async(behaviour);
}
/*	FUNCTION:		RenderActor ::AsyncPrepareExportFrame
	ARGS:			frame_idx
	RETURN:			n/a
	DESCRIPTION:	Prepare export frame (actor thread)
					Caller blocks until sem_signal
*/
void RenderActor :: AsyncPrepareExportFrame(bigtime_t frame_idx, sem_id sem_signal, BBitmap **pbitmap)
{
	*pbitmap = GetOutputFrame(frame_idx);
	release_sem(sem_signal);
}

/*	FUNCTION:		RenderActor :: GetPicture
	ARGS:			width
					height
	RETURN:			YPicture
	DESCRIPTION:	Get Picture from cache, maintain ownership
*/
yrender::YPicture * RenderActor :: GetPicture(const unsigned int width, unsigned int height, BBitmap *source)
{
	AsyncValidityCheck();
	return fPictureCache->GetPicture(width, height, source);
}

/*	FUNCTION:		RenderActor :: GetOutputFrame
	ARGS:			frame_idx
	RETURN:			Output frame
	DESCRIPTION:	Create output frame
*/
BBitmap * RenderActor :: GetOutputFrame(int64 frame_idx)
{
	DEBUG("RenderActor::GetOutputFrame(%ld)\n", frame_idx);

	std::deque<FRAME_ITEM>		frame_items;
	std::deque<int64>			track_timelines;

	//	Reverse iterate each track, find clips and effects
	for (std::vector<TimelineTrack *>::const_reverse_iterator t = gProject->mTimelineTracks.rbegin(); t < gProject->mTimelineTracks.rend(); ++t)
	{
		if (!(*t)->mVideoEnabled)
			continue;

		//	Find clips (TODO binary search)
		for (auto &clip : (*t)->mClips)
		{
			if ((frame_idx >= clip.mTimelineFrameStart) && (frame_idx < clip.GetTimelineEndFrame()) && clip.mVideoEnabled &&
				((clip.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO) || (clip.mMediaSourceType == MediaSource::MEDIA_PICTURE)))
			{
				frame_items.emplace_back(FRAME_ITEM(*t, &clip, nullptr, false));
				break;
			}
			else if (clip.mTimelineFrameStart > frame_idx)
				break;
		}

		//	Find effects (TODO binary search, TODO early exit)
		std::vector<FRAME_ITEM> effects;
		bool use_secondary_buffer = false;
		int64 timeline_frame_idx = frame_idx;
		for (auto e : (*t)->mEffects)
		{
			if ((frame_idx >= e->mTimelineFrameStart) && (frame_idx < e->mTimelineFrameEnd) && e->mEnabled)
			{
				if (e->Type() == MediaEffect::MEDIA_EFFECT_IMAGE)
				{
					effects.emplace_back(FRAME_ITEM(*t, nullptr, e, e->mEffectNode->UseSecondaryFrameBuffer()));
					if (e->mEffectNode->UseSecondaryFrameBuffer())
						use_secondary_buffer = true;
				}

				//	timeline - warning not cumulative
				if (e->mEffectNode->IsSpeedEffect())
				{
					timeline_frame_idx = ((Effect_Speed *)e->mEffectNode)->GetSpeedTime(frame_idx, e);
					track_timelines.push_back(timeline_frame_idx);
				}
			}
		}
		if (!effects.empty())
		{
			if (effects.size() > 1)
			{
				std::sort(effects.begin(), effects.end(), [](const FRAME_ITEM &a, const FRAME_ITEM &b){return a.effect->mPriority < b.effect->mPriority;});

				//	If one effect requires secondary buffer, all effects + clip render to secondary frame buffer
				if (use_secondary_buffer && !frame_items.empty() && frame_items[frame_items.size()-1].clip)
				{
					frame_items[frame_items.size() - 1].secondary_framebuffer = true;
					for (auto &e : effects)
						e.secondary_framebuffer = true;
				}
			}
			for (auto &e : effects)
				frame_items.emplace_back(e);
		}
		effects.clear();
	}

	if (frame_items.empty())
		return fBackgroundBitmap;

	//	Add 25% grace to frame time to avoid touching clips (end/begin)
	const int64 kFrameReadGrace = kFramesSecond / (4.0*gProject->mResolution.frame_rate);

	BBitmap *bitmap = fBackgroundBitmap;

	//	Early exit if single (or final) full screen frame
	const FRAME_ITEM &last = frame_items[frame_items.size() - 1];
	if (last.clip)
	{
		const MediaClip &clip = *last.clip;
		bool last_fullscreen = (clip.mMediaSource->GetVideoWidth() == gProject->mResolution.width) &&
							   (clip.mMediaSource->GetVideoHeight() == gProject->mResolution.height);
		if (last_fullscreen)
		{
			int64 requested_frame = (frame_idx - clip.mTimelineFrameStart) + clip.mSourceFrameStart;
			if ((clip.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO))
				bitmap = gVideoManager->GetFrameBitmap(clip.mMediaSource, requested_frame + kFrameReadGrace);
			else
				bitmap = clip.mMediaSource->GetBitmap();
			return bitmap;
		}
	}

	DEBUG("*** Output ***\n");
	for (auto i : frame_items)
	{
		if (i.clip)
		{
			DEBUG("   %s (Framebuffer=%d)\n", i.clip->mMediaSource->GetFilename().String(), i.secondary_framebuffer);
		}
		else
		{
			DEBUG("   %s (Framebuffer=%d) (prio=%d)\n", i.effect->mEffectNode->GetEffectName(), i.secondary_framebuffer, i.effect->mPriority);
		}
	}

	BBitmap *frame_bitmap = bitmap;
	bool initial_primary = true;
	bool initial_secondary = true;
	BBitmap *secondary_bitmap = nullptr;
	bool secondary_transfer_pending = false;
	double ts = yplatform::GetElapsedTime();
	fRenderView->LockGL();
	TimelineTrack *timeline_track = nullptr;
	int64 timeline_frame_idx = frame_idx;
	while (!frame_items.empty())
	{
		const FRAME_ITEM &item = frame_items.front();
		frame_items.pop_front();

		if ((track_timelines.size() > 0) && (item.track != timeline_track))
		{
			timeline_frame_idx = track_timelines.front();
			timeline_track = item.track;
			track_timelines.pop_front();
		}

		DEBUG("   >>> %s\n", item.clip ? item.clip->mMediaSource->GetFilename().String() : item.effect->mEffectNode->GetEffectName());

		if (item.clip)
		{
			const MediaClip &clip = *item.clip;
			int64 requested_frame = (timeline_frame_idx - clip.mTimelineFrameStart) + clip.mSourceFrameStart;
			if ((clip.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO))
				frame_bitmap = gVideoManager->GetFrameBitmap(clip.mMediaSource, requested_frame + kFrameReadGrace);
			else
				frame_bitmap = clip.mMediaSource->GetBitmap();

			if (secondary_transfer_pending)
			{
				fRenderView->ActivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER, initial_primary, false);
				gEffectsManager->GetEffectNone()->RenderEffect(secondary_bitmap, nullptr, frame_idx, frame_items);
				fRenderView->DeactivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER);
				bitmap = fRenderView->GetFrameBufferBitmap(RenderView::PRIMARY_FRAME_BUFFER, GL_RGBA);
				secondary_transfer_pending = false;
			}

			if (frame_bitmap)
			{
				if (!item.secondary_framebuffer)
				{
					fRenderView->ActivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER, initial_primary, false);
					gEffectsManager->GetEffectNone()->RenderEffect(frame_bitmap, nullptr, frame_idx, frame_items);
					fRenderView->DeactivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER);
					bitmap = fRenderView->GetFrameBufferBitmap(RenderView::PRIMARY_FRAME_BUFFER, GL_RGBA);
					initial_primary = false;
				}
				else
				{
					fRenderView->ActivateFrameBuffer(RenderView::SECONDARY_FRAME_BUFFER, initial_secondary, true);
					gEffectsManager->GetEffectNone()->RenderEffect(frame_bitmap, nullptr, frame_idx, frame_items);
					fRenderView->DeactivateFrameBuffer(RenderView::SECONDARY_FRAME_BUFFER);
					secondary_bitmap = fRenderView->GetFrameBufferBitmap(RenderView::SECONDARY_FRAME_BUFFER, GL_RGBA);
					initial_secondary = false;
					secondary_transfer_pending = true;
				}

			}
			else
				printf("RenderActor::GetOutputFrame(%ld) - cannot retrieve frame File: %s\n", frame_idx, item.clip->mMediaSource->GetFilename().String());
		}
		else
		{
			//	effect
			if (item.effect->Type() == MediaEffect::MEDIA_EFFECT_IMAGE)
			{
				if (item.effect->mEffectNode->IsSpatialTransform())
				{
					//	Scenario where Spatial transform needs to clear frame buffer
					initial_primary = true;
				}

				if (!item.secondary_framebuffer)
				{
					fRenderView->ActivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER, initial_primary, false);
					item.effect->mEffectNode->RenderEffect(bitmap, item.effect, frame_idx, frame_items);
					fRenderView->DeactivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER);
					bitmap = fRenderView->GetFrameBufferBitmap(RenderView::PRIMARY_FRAME_BUFFER, GL_RGBA);
					initial_primary = false;
				}
				else
				{
					fRenderView->ActivateFrameBuffer(RenderView::SECONDARY_FRAME_BUFFER, initial_secondary, false);
					item.effect->mEffectNode->RenderEffect(secondary_bitmap == nullptr ? bitmap : secondary_bitmap, item.effect, frame_idx, frame_items);
					fRenderView->DeactivateFrameBuffer(RenderView::SECONDARY_FRAME_BUFFER);
					secondary_bitmap = fRenderView->GetFrameBufferBitmap(RenderView::SECONDARY_FRAME_BUFFER, GL_RGBA);
					initial_secondary = false;
					secondary_transfer_pending = true;
				}
			}
		}
	}

	if (secondary_transfer_pending)
	{
		fRenderView->ActivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER, initial_primary, false);
		gEffectsManager->GetEffectNone()->RenderEffect(secondary_bitmap, nullptr, frame_idx, frame_items);
		fRenderView->DeactivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER);
		bitmap = fRenderView->GetFrameBufferBitmap(RenderView::PRIMARY_FRAME_BUFFER, GL_RGBA);
		secondary_transfer_pending = false;
	}

	fRenderView->UnlockGL();
	DEBUG("RenderTime[3] = %fms\n", 1000.0 * (yplatform::GetElapsedTime() - ts));
	return bitmap;
}

BBitmap * RenderActor :: GetCurrentFrameBufferTexture(GLenum format)
{
	return fRenderView->GetFrameBufferBitmap(RenderView::PRIMARY_FRAME_BUFFER, format);
}

void RenderActor :: ActivateSecondaryRenderBuffer(const bool is_alpha_clear)
{
	fRenderView->ActivateFrameBuffer(RenderView::SECONDARY_FRAME_BUFFER, true, is_alpha_clear);
}

void RenderActor :: DeactivateSecondaryRenderBuffer()
{
	fRenderView->DeactivateFrameBuffer(RenderView::SECONDARY_FRAME_BUFFER);
}

BBitmap *RenderActor :: GetSecondaryFrameBufferTexture(GLenum format)
{
	return fRenderView->GetFrameBufferBitmap(RenderView::SECONDARY_FRAME_BUFFER, format);
}

void RenderActor :: EffectResetPrimaryRenderBuffer()
{
	fRenderView->DeactivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER);
	fRenderView->ActivateFrameBuffer(RenderView::PRIMARY_FRAME_BUFFER, true, false);
}

/*	FUNCTION:		RenderView :: AsyncPreloadFrame
	ARGUMENTS:		frame_idx
	RETURN:			n/a
	DESCRIPTION:	Called by TimelinePlayer, decode next frame for smoother playback
*/
void RenderActor :: AsyncPreloadFrame(bigtime_t frame_idx)
{
	//	Reverse iterate each track, find clips
	for (std::vector<TimelineTrack *>::const_reverse_iterator t = gProject->mTimelineTracks.rbegin(); t < gProject->mTimelineTracks.rend(); ++t)
	{
		//	Find clips (TODO binary search)
		for (auto &clip : (*t)->mClips)
		{
			if ((frame_idx >= clip.mTimelineFrameStart) && (frame_idx < clip.GetTimelineEndFrame()) &&
				((clip.mMediaSourceType == MediaSource::MEDIA_VIDEO) || (clip.mMediaSourceType == MediaSource::MEDIA_VIDEO_AND_AUDIO)))
			{
				int64 requested_frame = (frame_idx - clip.mTimelineFrameStart) + clip.mSourceFrameStart;
				gVideoManager->GetFrameBitmap(clip.mMediaSource, requested_frame);
			}
			else if (clip.mTimelineFrameStart > frame_idx)
				break;
		}
	}
}

/*	FUNCTION:		RenderView :: AsyncInvalidateTimelineEdit
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	HACK.  TimelineEdit context menu will close AFTER an invalidate, causing corruption.
					Generate a redraw message after some time.  Better than a timer ...
*/
void RenderActor :: AsyncInvalidateTimelineEdit()
{
	MedoWindow::GetInstance()->PostMessage(fMsgInvalidateTimelineEdit);
}

/*	FUNCTION:		RenderView :: AsyncInvalidateProjectSettings
	ARGUMENTS:		sem_id
	RETURN:			n/a
	DESCRIPTION:	Project settings changes, recreate Effect Nodes from RenderThread
*/
void RenderActor :: AsyncInvalidateProjectSettings(int32 sem_id)
{
#if 0
	fRenderView->LockGL();
	fRenderView->ResetViewport();
	gEffectsManager->ProjectSettingsChanged();
	fRenderView->UnlockGL();
#else
	delete fRenderView;
	fRenderView = new RenderView(BRect(0, 0, gProject->mResolution.width, gProject->mResolution.height));
	fRenderView->LockGL();
	gEffectsManager->ProjectSettingsChanged();
	fRenderView->UnlockGL();
#endif

	if (sem_id > 0)
		release_sem(sem_id);
	else
		gProject->InvalidatePreview();
}

/*	FUNCTION:		RenderView :: Reset
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Wait until render thread idle
*/
void RenderActor :: WaitIdle()
{
	while (!IsIdle())
		usleep(100);
	return;
}

/********************************************
	RenderView is a BGLView
*********************************************/

/*	FUNCTION:		RenderView :: RenderView
	ARGUMENTS:		frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
RenderView :: RenderView(BRect frame)
	: BGLView(frame, "RenderView", B_FOLLOW_NONE, 0, BGL_RGB | BGL_DOUBLE | BGL_DEPTH)
{
	printf("[OpenGL Renderer]          %s\n", glGetString(GL_RENDERER));
	printf("[OpenGL Vendor]            %s\n", glGetString(GL_VENDOR));
	printf("[OpenGL Version]           %s\n", glGetString(GL_VERSION));
	GLint profile;	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
	printf("[OpenGL Profile]           %s\n", profile ? "Core" : "Compatibility");
	printf("[OpenGL Shading Language]  %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	
	SetViewColor(B_TRANSPARENT_COLOR);
	
	YInitFileManager("~/development/Medo");
	
	LockGL();
	//	init OpenGL state
	glClearColor(0, 0, 0, 0);	//	BGRA

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glEnable(GL_DEPTH_TEST);
	//glDepthMask(GL_TRUE);
	//glDepthFunc(GL_LEQUAL);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	const float width = gProject->mResolution.width;
	const float height = gProject->mResolution.height;
	fCamera = new yrender::YCamera(yrender::YCamera::CAMERA_PERSPECTIVE, width, height);
	fCamera->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, width));
	fCamera->SetDirection(ymath::YVector3(0, 0, -1));

	BRect glFrame(0, 0, width-1, height-1);
	for (int i=0; i < kNumberBitmapBuffers; i++)
		fOpenGlBitmap[i] = new BBitmap(glFrame, B_RGB32);
	fBitmapIndex = 0;
	
	fRenderTarget[PRIMARY_FRAME_BUFFER] = new YRenderTarget(GL_RGBA, width, height);
	fRenderTarget[SECONDARY_FRAME_BUFFER] = new YRenderTarget(GL_RGBA, width, height);
	
	UnlockGL();
}

/*	FUNCTION:		RenderView :: ~RenderView
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
RenderView :: ~RenderView()
{
	LockGL();
	delete fCamera;
	for (int i=0; i < kNumberBitmapBuffers; i++)
		delete fOpenGlBitmap[i];
	delete fRenderTarget[PRIMARY_FRAME_BUFFER];
	delete fRenderTarget[SECONDARY_FRAME_BUFFER];
	UnlockGL();
	YDestroyFileManager();	
}

/*	FUNCTION:		RenderView :: ResetViewport
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Recreate OpenGL frame buffers at new size
*/
void RenderView :: ResetViewport()
{
	delete fCamera;
	for (int i=0; i < kNumberBitmapBuffers; i++)
		delete fOpenGlBitmap[i];
	delete fRenderTarget[PRIMARY_FRAME_BUFFER];
	delete fRenderTarget[SECONDARY_FRAME_BUFFER];

	const float width = gProject->mResolution.width;
	const float height = gProject->mResolution.height;
	FrameResized(width, height);
	fCamera = new yrender::YCamera(yrender::YCamera::CAMERA_PERSPECTIVE, width, height);
	fCamera->mSpatial.SetPosition(ymath::YVector3(0.5f*width, 0.5f*height, width));
	fCamera->SetDirection(ymath::YVector3(0, 0, -1));

	BRect glFrame(0, 0, width-1, height-1);
	for (int i=0; i < kNumberBitmapBuffers; i++)
		fOpenGlBitmap[i] = new BBitmap(glFrame, B_RGB32);
	fBitmapIndex = 0;

	fRenderTarget[PRIMARY_FRAME_BUFFER] = new YRenderTarget(GL_RGBA, width, height);
	fRenderTarget[SECONDARY_FRAME_BUFFER] = new YRenderTarget(GL_RGBA, width, height);
}

/*	FUNCTION:		MedoOpenGlView :: ErrorCallback
	ARGUMENTS:		errorCode
	RETURN:			n/a
	DESCRIPTION:	Hook function when an OpenGL error occurs
*/
void RenderView :: ErrorCallback(unsigned long errorCode)
{
	switch (errorCode)
	{
		case GL_INVALID_ENUM:		printf("GL_INVALID_ENUM\n");		break;
		case GL_INVALID_VALUE:		printf("GL_INVALID_VALUE\n");		break;
		case GL_INVALID_OPERATION:	printf("GL_INVALID_OPERATION\n");	break;
		case GL_STACK_OVERFLOW:		printf("GL_STACK_OVERFLOW\n");		break;
		case GL_STACK_UNDERFLOW:	printf("GL_STACK_UNDERFLOW\n");		break;
		case GL_OUT_OF_MEMORY:		printf("GL_OUT_OF_MEMORY\n");		break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:	printf("GL_INVALID_FRAMEBUFFER_OPERATION\n");	break;
		default:					printf("GL_ERROR(%ld)\n", errorCode);break;
	}	
}

/*	FUNCTION:		RenderView :: ActivateFrameBuffer
	ARGUMENTS:		target
					clear
	RETURN:			n/a
	DESCRIPTION:	Activate frame buffer
*/
void RenderView :: ActivateFrameBuffer(FRAME_BUFFER target, const bool clear, const bool is_alpha_clear)
{
	if (++fBitmapIndex > 1)
		fBitmapIndex = 0;

	if (is_alpha_clear)
		fRenderTarget[target]->ActivateTransparentBuffer();
	else
		fRenderTarget[target]->Activate(clear);
	if (target == PRIMARY_FRAME_BUFFER)
		fCamera->Render(0.0f);
}

/*	FUNCTION:		RenderView :: DeactivateFrameBuffer
	ARGUMENTS:		target
	RETURN:			n/a
	DESCRIPTION:	De-activate frame buffer
*/
void RenderView :: DeactivateFrameBuffer(FRAME_BUFFER target)
{
	fRenderTarget[target]->Deactivate();	
}

/*	FUNCTION:		RenderView :: GetFrameBufferBitmap
	ARGUMENTS:		target
	RETURN:			BBitmap
	DESCRIPTION:	GetFrameBuffer BBitmap
*/
BBitmap * RenderView :: GetFrameBufferBitmap(FRAME_BUFFER target, GLenum format)
{
	fRenderTarget[target]->BindTexture();
	glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, fOpenGlBitmap[fBitmapIndex]->Bits());
	return fOpenGlBitmap[fBitmapIndex];
}
