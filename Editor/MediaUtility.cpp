/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Media utililty
 */

#include <cstdio>

#include <support/SupportDefs.h>
#include "Project.h"
#include "MediaUtility.h"

/*	FUNCTION:		MediaDuration :: MediaDuration
	ARGS:			duration
					video_fps
	RETURN:			n/a
	DESCRIPTION:	Convert bigtime_t into hours/minutes/seconds
*/
MediaDuration :: MediaDuration(bigtime_t duration, float video_fps)
{
	frame_rate = video_fps;
	
	hours = duration / (kFramesSecond * 60 * 60);
	duration -= hours * kFramesSecond * 60 * 60;
	
	minutes = duration / (kFramesSecond * 60);
	duration -= minutes * kFramesSecond * 60;
	
	seconds = duration / kFramesSecond;
	duration -= seconds * kFramesSecond;
	
	if (video_fps > 0)
		auxillary = video_fps * duration/kFramesSecond;
	else
		auxillary = duration/1000;
}

/*	FUNCTION:		MediaDuration :: PrepareVideoTimestamp
	ARGS:			buffer
	RETURN:			n/a
	DESCRIPTION:	Create print buffer
*/
void MediaDuration :: PrepareVideoTimestamp(char *buffer) const
{
	if (frame_rate == (float)((int)frame_rate))
		sprintf(buffer, "%ldh:%02ldm:%02lds_(%02ld/%02d)", hours, minutes, seconds, auxillary, (int)frame_rate);
	else
		sprintf(buffer, "%ldh:%02ldm:%02lds_(%02ld/%4.2f)", hours, minutes, seconds, auxillary, frame_rate);
}

/*	FUNCTION:		MediaDuration :: PrepareAudioTimestamp
	ARGS:			buffer
	RETURN:			n/a
	DESCRIPTION:	Create print buffer
*/
void MediaDuration :: PrepareAudioTimestamp(char *buffer) const
{
	sprintf(buffer, "%ldh:%02ldm:%02lds:%03ldms", hours, minutes, seconds, auxillary);
}

/*	FUNCTION:		MediaDuration :: Print
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Print from local buffer
*/
const char * MediaDuration :: Print() const
{
	static char sLocalBuffer[80];
	if (frame_rate > 0)
		PrepareVideoTimestamp(sLocalBuffer);
	else
		PrepareAudioTimestamp(sLocalBuffer);
	return sLocalBuffer;	
}
