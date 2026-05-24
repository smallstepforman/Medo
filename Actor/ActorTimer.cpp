/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Zen Yes Pty Ltd, 2017-2026, MIT license
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
	fThreadSemaphore.Lock();
	fTimerThread = std::jthread(TimerThread, this);

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
	fTimerThread.request_stop();
	fThreadSemaphore.Signal();
	fTimerThread.join();
	sTimerInstance = nullptr;
}

/*	FUNCTION:		Timer :: TimerThread
	ARGUMENTS:		cookie
	RETURN:			n/a
	DESCRIPTION:	Timer thread
*/
void Timer :: TimerThread(std::stop_token stop_token, void *cookie)
{
	Timer *t = (Timer *)cookie;
	while (!stop_token.stop_requested())
	{
		t->fQueueLock.Lock();
		int64_t timeout = 60*1000;
		if (!t->fTimerQueue.empty())
		{
			auto now = std::chrono::steady_clock::now();
            auto expiry = t->fTimerQueue.front().fExpiry;
            
            if (expiry <= now)
                timeout = 0;
            else
                timeout = std::chrono::duration_cast<std::chrono::milliseconds>(expiry - now).count();
		}
		t->fQueueLock.Unlock();

		t->fThreadSemaphore.TryLock(timeout, false);
		
		//	Notify waiting clients
		t->fQueueLock.Lock();
		auto now = std::chrono::steady_clock::now();
		while (!t->fTimerQueue.empty() && t->fTimerQueue.front().fExpiry <= now)
		{
		    TimerObject top = std::move(t->fTimerQueue.front());
		    t->fTimerQueue.pop_front();
		    top.fMessage.Send();
		}
		t->fQueueLock.Unlock();
	}
}

/*	FUNCTION:		Timer :: AddTimer
	ARGUMENTS:		milliseconds
					target
					behaviour
	RETURN:			n/a
	DESCRIPTION:	Add timer
*/
void Timer :: AddTimer(const int64_t milliseconds, yarra::ActorMessage<> message)
{
	assert(message.IsValid());
	assert(AsyncValidityCheck());

	//	Sanity check
	if (milliseconds <= 0)
	{
		message.Send();
		return;
	}

	auto now = std::chrono::steady_clock::now();
    auto expiry = now + std::chrono::milliseconds(milliseconds);

	fQueueLock.Lock();
	// Find insertion point (keep queue sorted by expiry)
    auto it = std::upper_bound(fTimerQueue.begin(), fTimerQueue.end(), expiry, [](auto val, const TimerObject& obj) {return val < obj.fExpiry;});
	bool is_front = (it == fTimerQueue.begin());
    fTimerQueue.emplace(it, expiry, message);
	fQueueLock.Unlock();
	if (is_front)
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
	assert(IsLocked());
	fQueueLock.Lock();

	// Clean up using modern erase-remove idiom for efficiency
	fTimerQueue.erase(std::remove_if(fTimerQueue.begin(), fTimerQueue.end(),
		[target](const TimerObject& obj) { return obj.fMessage.mActor == target; }), 
		fTimerQueue.end());

	fQueueLock.Unlock();
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
