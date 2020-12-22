/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2020
 *	DESCRIPTION:	Timeline player
 */

#ifndef _TIMELINE_PLAYER_H_
#define _TIMELINE_PLAYER_H_

#ifndef _YARRA_ACTOR_H_
#include "Actor/Actor.h"
#endif

class MedoWindow;
class BMessage;

class TimelinePlayer : public yarra::Actor
{
public:
				TimelinePlayer(MedoWindow	*parent);
				~TimelinePlayer()	override;

	void		AsyncPlay(bigtime_t start, bigtime_t end, const bool repeat);
	void		AsyncSetFrame(bigtime_t frame_idx);
	void		AsyncStop();

	const bool	IsPlaying() const	{return fPlaying;}
	const bool	IsRepeat() const	{return fRepeat;}

private:
	bigtime_t		fCurrentPosition;
	bigtime_t		fStartPosition;
	bigtime_t		fEndPosition;
	bigtime_t		fTimestamp;
	bool			fRepeat;
	bool			fPlaying;

	MedoWindow		*fParentWindow;
	BMessage		*fTimelinePositionMessage;

	void			AsyncOutputComplete();
};


#endif // #ifndef _TIMELINE_PLAYER_H_
