/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2017-2018, ZenYes Pty Ltd
	DESCRIPTION:	Timer which schedules messages.
					Use a single sleeping thread shared amongst many TimerObjects
*/

#include <algorithm>
#include <cassert>
#include <thread>

#include "Platform.h"
#include "Timer.h"

namespace yarra
{

static Timer * sTimerInstance = nullptr;

/*	FUNCTION:		Timer :: Timer
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Timer :: Timer()
{
	fKeepAlive = true;
	fThreadSemaphore.Lock();
	fTimerThread = new Platform::Thread(TimerThread, this, "TimerThread");
	fTimeStamp = Platform::GetElapsedTime();
	fTimerThread->Start();

	assert(sTimerInstance == nullptr);
	sTimerInstance = this;
}

/*	FUNCTION:		Timer :: ~Timer
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Timer :: ~Timer()
{
	fKeepAlive = false;
	fThreadSemaphore.Signal();
	while (!fKeepAlive)
		std::this_thread::yield();
	delete fTimerThread;
	sTimerInstance = nullptr;
}

/*	FUNCTION:		Timer :: TimerThread
	ARGUMENTS:		cookie
	RETURN:			thread exit status
	DESCRIPTION:	Timer thread
*/
int Timer :: TimerThread(void *cookie)
{
	Timer *t = (Timer *)cookie;
	while (t->fKeepAlive)
	{
		t->fQueueLock.Lock();
		int64_t timeout = 60*1000;
		if (!t->fTimerQueue.empty())
		{
			TimerObject &top = t->fTimerQueue.front();
			timeout = top.milliseconds;
		}
		t->fQueueLock.Unlock();

		t->fThreadSemaphore.TryLock(timeout, false);
		
		t->fQueueLock.Lock();
		t->TimerTickLocked();
		t->fQueueLock.Unlock();
	}
	t->fKeepAlive = true;
	Platform::ExitThread();
	return 1;
}

/*	FUNCTION:		Timer :: AddTimer
	ARGUMENTS:		milliseconds
					target
					behaviour
	RETURN:			n/a
	DESCRIPTION:	Add timer
*/
void Timer :: AddTimer(const int64_t milliseconds, Actor *target, std::function<void()> behaviour)
{
	assert(target != nullptr);
	assert(behaviour != nullptr);
	assert(AsyncValidityCheck());

	//	Sanity check
	if (milliseconds <= 0)
	{
		target->Async(behaviour);
		return;
	}

	fQueueLock.Lock();
	//	TimerTickLocked will deduct delta time, cater for this delta 
	double ts = Platform::GetElapsedTime();
	int64_t elapsed_time = int64_t(1000.0*(ts - fTimeStamp));
	fTimerQueue.emplace_back(TimerObject(milliseconds + elapsed_time, target, behaviour));	//	TimerTickLocked() will subtract elapsed_time from all queued objects
	TimerTickLocked();
	fQueueLock.Unlock();
	fThreadSemaphore.Signal();
}

/*	FUNCTION:		Timer :: CancelTimersLocked
	ARGUMENTS:		target
	RETURN:			n/a
	DESCRIPTION:	Remove all timers to target
					Note - cannot compare behaviours, so impossible to cancel specific timer
					Sync version to avoid messaging delay
*/
void Timer :: CancelTimersLocked(Actor *target)
{
	assert(IsLocked());		//	Actor locked
	fQueueLock.Lock();
	bool repeat;
	do
	{
		repeat = false;
		for (std::deque<TimerObject>::iterator i = fTimerQueue.begin(); i != fTimerQueue.end(); i++)
		{
			if ((*i).target == target)
			{
				fTimerQueue.erase(i);
				repeat = true;
				break;
			}
		}
	} while (repeat);
	fQueueLock.Unlock();
}

/*	FUNCTION:		Timer :: TimerTickLocked
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Check fTimerQueue for expired objects
*/
void Timer :: TimerTickLocked()
{
	double ts = Platform::GetElapsedTime();
	int64_t elapsed_time = int64_t(1000.0*(ts - fTimeStamp));
	for (auto &i : fTimerQueue)
		i.milliseconds -= elapsed_time;

	bool repeat;
	do
	{
		repeat = false;
		for (std::deque<TimerObject>::iterator i = fTimerQueue.begin(); i != fTimerQueue.end(); i++)
		{
			if ((*i).milliseconds <= 0)
			{
#if ACTOR_DEBUG
				printf("Timer::TimerTickLocked() AsyncMessage(Wait time = %f)\n", Platform::GetElapsedTime() - (*i).timestamp);
#endif
				(*i).target->Async((*i).behaviour);
				fTimerQueue.erase(i);
				repeat = true;
				break;
			}
		}
	} while (repeat);

	std::sort(fTimerQueue.begin(), fTimerQueue.end(), [](const TimerObject &a, const TimerObject &b) {return a.milliseconds < b.milliseconds; });
	fTimeStamp = ts;
}

/*	FUNCTION:		Timer :: IsBusy
	ARGUMENTS:		none
	RETURN:			true if busy
	DESCRIPTION:	Called by ActorManager::WorkThreadIdle() to determine if no jobs available
*/
const bool Timer :: IsBusy()
{
	return (fTimerQueue.size() > 0);
}

};	//	namespace yarra
