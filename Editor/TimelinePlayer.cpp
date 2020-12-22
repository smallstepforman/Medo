/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2020
 *	DESCRIPTION:	Timeline player
 */

#include <cassert>

#include "Actor/Actor.h"
#include "Actor/ActorManager.h"

#include "MedoWindow.h"
#include "RenderActor.h"
#include "Project.h"
#include "TimelinePlayer.h"

#if 0
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(...) {}
#endif

/*	FUNCTION:		TimelinePlayer :: TimelinePlayer
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
TimelinePlayer :: TimelinePlayer(MedoWindow *parent)
	: yarra::Actor(), fParentWindow(parent)
{
	fTimelinePositionMessage = new BMessage(MedoWindow::eMsgActionAsyncTimelinePlayerUpdate);
	fTimelinePositionMessage->AddInt64("Position", 0);
	fTimelinePositionMessage->AddBool("Complete", false);

	fPlaying = false;
}

/*	FUNCTION:		TimelinePlayer :: ~TimelinePlayer
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
TimelinePlayer :: ~TimelinePlayer()
{
	yarra::ActorManager::GetInstance()->CancelTimers(this);
	delete fTimelinePositionMessage;
}

/*	FUNCTION:		TimelinePlayer :: AsyncPlay
	ARGS:			start
					end
					repeat
	RETURN:			n/a
	DESCRIPTION:	Start issueing prepare frame commands
*/
void TimelinePlayer :: AsyncPlay(bigtime_t start, bigtime_t end, const bool repeat)
{
	DEBUG("TimelinePlayer::AsyncPlay() start=%ld,end=%ld, repeat=%d\n", start, end, repeat);

	if (end < 0)
	{
		end = gProject->mTotalDuration;
		if ((start > end) || (start + kFramesSecond/10 > end))
			start = 0;
	}
	fStartPosition = start;
	fCurrentPosition = start;
	fEndPosition = end;
	fRepeat = repeat;
	fPlaying = true;

	fTimestamp = system_time();
	std::function<void()> render_complete = std::bind(&TimelinePlayer::AsyncOutputComplete, this);
	gRenderActor->AsyncPriority(&RenderActor::AsyncPlayFrame, gRenderActor, fCurrentPosition, this, render_complete);
}

/*	FUNCTION:		TimelinePlayer :: AsyncSetFrame
	ARGS:			frame_idx
	RETURN:			n/a
	DESCRIPTION:	Set current frame
*/
void TimelinePlayer :: AsyncSetFrame(bigtime_t frame_idx)
{
	fCurrentPosition = frame_idx;
	if (fPlaying)
	{
		if ((fCurrentPosition > fEndPosition) && fRepeat)
			fPlaying = false;
		if ((fCurrentPosition < fStartPosition) && fRepeat)
			fStartPosition = fCurrentPosition;
	}
	else
		gRenderActor->AsyncPriority(&RenderActor::AsyncPrepareFrame, gRenderActor, fCurrentPosition);
}

/*	FUNCTION:		TimelinePlayer :: AsyncStop
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Stop sending prepare frame commands
*/
void TimelinePlayer :: AsyncStop()
{
	DEBUG("TimelinePlayer::AsyncStop()\n");
	fPlaying = false;
}

/*	FUNCTION:		TimelinePlayer :: AsyncOutputComplete
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Called when RenderActor finished processing frame
					Typically sleep for a couple of milliseconds
*/
void TimelinePlayer :: AsyncOutputComplete()
{
	DEBUG("TimelinePlayer::AsyncOutputComplete()\n");

	const bigtime_t frame_time = kFramesSecond / gProject->mResolution.frame_rate;
	bigtime_t ts = system_time();
	bigtime_t duration = ts - fTimestamp;
	if (duration > frame_time)
		duration = frame_time;
	if (duration < frame_time)
	{
		yarra::ActorManager::GetInstance()->AddTimer((frame_time - duration)/1000, this, std::bind(&TimelinePlayer::AsyncOutputComplete, this));
		//	Preload next frame
		gRenderActor->Async(&RenderActor::AsyncPreloadFrame, gRenderActor, fCurrentPosition + frame_time);
		return;
	}

//	printf("%ld\n", ts - fTimestamp);

	fCurrentPosition += frame_time;
	if ((fCurrentPosition >= fEndPosition) && fRepeat)
		fCurrentPosition = fStartPosition;

	if (fPlaying && (fCurrentPosition < fEndPosition))
	{
		fTimestamp = system_time();
		std::function<void()> render_complete = std::bind(&TimelinePlayer::AsyncOutputComplete, this);
		gRenderActor->AsyncPriority(&RenderActor::AsyncPlayFrame, gRenderActor, fCurrentPosition, this, render_complete);
		fTimelinePositionMessage->ReplaceBool("Complete", false);
	}
	else
	{
		fPlaying = false;
		fTimelinePositionMessage->ReplaceBool("Complete", true);
	}
	fTimelinePositionMessage->ReplaceInt64("Position", fCurrentPosition);
	fParentWindow->PostMessage(fTimelinePositionMessage);
}
