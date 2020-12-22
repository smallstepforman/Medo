/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016
 *	DESCRIPTION:	Media utililty
 */

#ifndef _MEDIA_UTILITY_H_
#define _MEDIA_UTILITY_H_

#ifndef _SUPPORT_DEFS_H
#include <support/SupportDefs.h>
#endif

//=======================
class MediaDuration
{
public:
	int64		hours;
	int64		minutes;
	int64		seconds;
	int64		auxillary;		//	frames below video_fps, milliseconds for audio
	float		frame_rate;
	
			MediaDuration(bigtime_t duration, float video_fps=0 /*sound = 0, video=24/25/30*/);
	void	PrepareVideoTimestamp(char *buffer) const;
	void	PrepareAudioTimestamp(char *buffer) const;
	const char * Print() const;
};

#endif
