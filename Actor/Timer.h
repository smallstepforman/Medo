/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2017-2018, ZenYes Pty Ltd
	DESCRIPTION:	Timer
*/

#ifndef _YARRA_TIMER_H_
#define _YARRA_TIMER_H_

#ifndef _YARRA_ACTOR_H_
#include "Actor.h"
#endif

#include <deque>

namespace yplatform
{
class Thread;
class Semaphore;
};

namespace yarra
{

	/******************************
		yarra::Timer
	*******************************/
	class Timer : public Actor
	{
	public:
				Timer();
				~Timer();

		/*	Schedule timer message:
				Prefer to use yarra::ActorManager::GetInstance()->AddTimer(milliseconds, target, std::bind(&Actor::Behaviour, target, ...));
		*/
		void		AddTimer(const int64_t milliseconds, Actor *target, std::function<void()> behaviour);
		void		CancelTimersLocked(Actor *target);

	private:
		struct TimerObject
		{
			int64_t									milliseconds;
			Actor									*target;
			std::function<void()>					behaviour;
#if ACTOR_DEBUG
			double									timestamp;
#endif

			TimerObject(int64_t ms, Actor *t, std::function<void()> b)
				: milliseconds(ms), target(t), behaviour(b)
			{
#if ACTOR_DEBUG
				timestamp = yplatform::GetElapsedTime();
#endif
			}
		};
		std::deque<TimerObject>		fTimerQueue;
		double						fTimeStamp;
		void						TimerTickLocked();
		
		yplatform::Thread			*fTimerThread;
		yplatform::Semaphore		fQueueLock;
		yplatform::Semaphore		fThreadSemaphore;
		static int					TimerThread(void *);
		bool						fKeepAlive;

		friend class ActorManager;
		const bool		IsBusy();
	};

};	//namespace yarra

#endif	//#ifndef _YARRA_TIMER_H_