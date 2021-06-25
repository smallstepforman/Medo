/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Audio manager (accurate seeking + cache frames)
 */

#include <cassert>
#include <vector>

#include <MediaKit.h>
#include <interface/Bitmap.h>
#include <kernel/OS.h>

extern "C" {
#include <libswresample/swresample.h>
}

#include "MediaSource.h"
#include "Project.h"
#include "AudioCache.h"
#include "AudioManager.h"
#include "MedoWindow.h"
#include "Actor/Actor.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

AudioManager	*gAudioManager = nullptr;

/**********************************
	AudioThumbnailActor
***********************************/

class AudioThumbnailActor : public yarra::Actor
{
	BMessage		*fMessage;
public:
	AudioThumbnailActor()
	{
		fMessage = new BMessage(MedoWindow::eMsgActionAsyncThumbnailReady);
	}
	~AudioThumbnailActor()
	{
		delete fMessage;
	}

	/*	FUNCTION:		AudioThumbnailActor :: AsyncGenerateThumbnail
		ARGS:			none
		RETURN:			n/a
		DESCRIPTION:	Async generate audio bitmap.
	*/
	void AsyncGenerateThumbnail(MediaSource *source, const int64 audio_start, const int64 audio_end, const float width, const float height)
	{
		BBitmap *bitmap = gAudioManager->GetBitmapAudioFrame(source, audio_start, audio_end, width, height);
		if (bitmap)
			MedoWindow::GetInstance()->PostMessage(fMessage);
	}

	/*	FUNCTION:		AudioThumbnailActor :: ClearPendingThumbnails
		ARGS:			none
		RETURN:			n/a
		DESCRIPTION:	FrameResized() can cause an explosion of these messages, so discard older requests
	*/
	void ClearPendingThumbnails()
	{
		ClearAllMessages();
	}
};

/**********************************
	Audio Manager
***********************************/

/*	FUNCTION:		AudioManager :: AudioManager
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
AudioManager :: AudioManager()
{
	fAudioCache = new AudioCache;
	for (int i=0; i < NUMBER_AUDIO_PROCESSING_BUFFERS; i++)
		fProcessingBuffers[i] = new uint8 [192*1024*sizeof(float)*2];		//	192k samples/second, sizeof(float), 2 channels

	fCacheSemaphore = create_sem(1, "AudioManager::fQueueSemaphore");
	if (fCacheSemaphore < B_OK)
	{
		printf("AudioManager() - cannot create fQueueSemaphore\n");
		exit(1);
	}
	fAudioThumbnailActor = new AudioThumbnailActor;

	fPreviewStartFrame = 0;
	fPreviewEndFrame = 0;
	fPreviewSource = nullptr;

#if 1
	fSoundPlayer = new BSoundPlayer("Medo", SoundPlayerCallback, nullptr, this);
	if (fSoundPlayer->InitCheck() == B_OK)
	{
		fSoundPlayer->SetHasData(false);
		fSoundPlayer->Start();
	}
	else
	{
		delete fSoundPlayer;
		fSoundPlayer = nullptr;
	}
#else
	fSoundPlayer = nullptr;
#endif
}

/*	FUNCTION:		AudioManager :: ~AudioManager
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
AudioManager :: ~AudioManager()
{
	for (auto &i : fResamplerContext)
		swr_free(&i.context);

	delete fSoundPlayer;
	delete fAudioThumbnailActor;

	if (fCacheSemaphore >= B_OK)
		delete_sem(fCacheSemaphore);

	for (int i=0; i < NUMBER_AUDIO_PROCESSING_BUFFERS; i++)
		delete fProcessingBuffers[i];
	delete fAudioCache;
}

/*	FUNCTION:		AudioManager :: GetBitmapAudioFrame
	ARGS:			source
					audio_start, audio_end
					width,height
	RETURN:			BBitmap
	DESCRIPTION:	Get audio track bitmap (no source frame conversion)
*/
BBitmap * AudioManager :: GetBitmapAudioFrame(MediaSource *source, const int64 audio_start, const int64 audio_end, const float width, const float height)
{
	DEBUG("AudioManager::GetBitmapAudioFrame() %fx%f\n", width, height);
	assert(source != nullptr);
	BMediaTrack *track = source->GetAudioTrack();
	assert(track != nullptr);
	assert(audio_start <= audio_end);
	assert(audio_end <= source->GetAudioNumberSamples());
	assert(audio_start >= 0);

	if ((audio_start == audio_end) || (width == 0.0f) || (height == 0.0f))
		return nullptr;
	if ((width <= 0.0f) || (height < 0.0f))
		return nullptr;

	BBitmap *bitmap = nullptr;
	status_t err;
	while ((err = acquire_sem(fCacheSemaphore)) == B_INTERRUPTED) ;
	if (err == B_OK)
	{
		bitmap = fAudioCache->FindBitmapLocked(source, audio_start, audio_end, width, height);
		release_sem_etc(fCacheSemaphore, 1, B_DO_NOT_RESCHEDULE);

		if (!bitmap)
			bitmap = fAudioCache->CreateBitmapUnlocked(fCacheSemaphore, source, audio_start, audio_end, width, height);
	}
	return bitmap;
}

/*	FUNCTION:		AudioManager :: GetBitmapAsync
	ARGS:			source
					source_start, source_end
					width,height
					callback
	RETURN:			BBitmap
	DESCRIPTION:	Get audio track bitmap (async)
*/
BBitmap	* AudioManager :: GetBitmapAsync(MediaSource *source, const int64 start_frame, const int64 end_frame, const float width, const float height)
{
	DEBUG("AudioManager::GetBitmapAsync() %fx%f\n", width, height);

	assert(source != nullptr);
	assert(source->GetAudioTrack() != nullptr);
	assert(start_frame <= end_frame);
	assert(start_frame >= 0);

	if ((start_frame >= end_frame) || (width == 0.0f) || (height == 0.0f))
		return nullptr;

	//	convert to source frame rate
	const float kConversionFactor = float(kFramesSecond) / source->GetAudioFrameRate();
	int64 audio_start = start_frame / kConversionFactor;
	int64 audio_end = end_frame / kConversionFactor;

	//	BMediaTrack::Duration() may incorrectly report more than available samples (https://dev.haiku-os.org/ticket/16581)
	const int64 audio_samples = source->GetAudioNumberSamples();
	if (audio_end >= audio_samples)
		audio_end = audio_samples;
	if (audio_start > audio_samples)
		return nullptr;

	//	Check if frame in cache, otherwise schedule work
	status_t err;
	while ((err = acquire_sem(fCacheSemaphore)) == B_INTERRUPTED) ;
	if (err != B_OK)
		return nullptr;

	bool generate_thumbnail = false;
	BBitmap *bitmap = fAudioCache->FindBitmapLocked(source, audio_start, audio_end, width, height);
	if (!bitmap)
	{
		//	See if we can find temporary bitmap with different size
		bitmap = fAudioCache->FindSimilarBitmapLocked(source, audio_start, audio_end, width, height);
		generate_thumbnail = true;
	}
	release_sem_etc(fCacheSemaphore, 1, B_DO_NOT_RESCHEDULE);
	
	if (generate_thumbnail)
	{
		fAudioThumbnailActor->Async(&AudioThumbnailActor::AsyncGenerateThumbnail, fAudioThumbnailActor,
									source, audio_start, audio_end, width, height);
	}
	return bitmap;
}

/*	FUNCTION:		AudioManager :: ClearPendingThumbnails
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Clear pending thumbnail generation
*/
void AudioManager :: ClearPendingThumbnails()
{
	fAudioThumbnailActor->ClearPendingThumbnails();
}
