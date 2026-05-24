/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Zen Yes Pty Ltd, 2017-2026, MIT license
	DESCRIPTION:	Timer
*/

#ifndef _YARRA_TIMER_H_
#define _YARRA_TIMER_H_

#ifndef _YARRA_ACTOR_H_
#include "Actor.h"
#endif

#include <deque>
#include <chrono>
#include <thread>

namespace yarra
{

namespace Semaphore {class Semaphore;}

	/******************************
		yarra::Timer
	*******************************/
	class Timer : public Actor
	{
	public:
				Timer();
				~Timer();

		/*	Schedule timer message:
				Prefer to use yarra::ActorManager::GetInstance()->AddTimer(milliseconds, message);
		*/
		void		AddTimer(const int64_t milliseconds, ActorMessage<> message);
		void		CancelTimersLocked(Actor *target);

	private:
		struct TimerObject
		{
			std::chrono::steady_clock::time_point   fExpiry;
			ActorMessage<>							fMessage;

			TimerObject(std::chrono::steady_clock::time_point t, ActorMessage<> msg)
				: fExpiry(t), fMessage(std::move(msg))
			{ }
		};
		std::deque<TimerObject>					fTimerQueue;
		std::jthread							fTimerThread;
		yplatform::Semaphore					fQueueLock;
		yplatform::Semaphore					fThreadSemaphore;
		static void								TimerThread(std::stop_token stop_token, void *);

		friend class ActorManager;
		const bool		IsBusy();
	};

};	//namespace yarra

#endif	//#ifndef _YARRA_TIMER_H_