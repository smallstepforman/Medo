/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Zen Yes Pty Ltd, 2017-2026, MIT license
	DESCRIPTION:	Actor Manager
*/

#include <cstdio>
#include <cassert>
#include <thread>
#include <mutex>
#include <chrono>

#include "Platform.h"

#include "WorkThread.h"
#include "ActorManager.h"
#include "Actor.h"
#include "Timer.h"

namespace yarra
{

static const unsigned int kMaxNumberWorkThreadsFactor = 2;		// max number of work threads = 2 * std::thread::hardware_concurrency()
ActorManager *ActorManager::sInstance = nullptr;

/*	FUNCTION:		ActorManager :: ActorManager
	ARGUMENTS:		requested_number_threads (0=default)
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ActorManager :: ActorManager(const unsigned int requested_number_threads,
							 const bool enable_load_balancer,
							 const uint64_t load_balancer_period_milliseconds)
	: fLoadBalancerThread(nullptr), fLoadBalancerPeriod(load_balancer_period_milliseconds)
{
	assert(sInstance == nullptr);
	sInstance = this;

	fIdleExit = false;
	fIdleSemaphore.Lock();	//	Start locked

	const unsigned int kNumberPhysicalCpuCores = std::thread::hardware_concurrency();
	const unsigned int num_threads = (requested_number_threads == 0 ? kNumberPhysicalCpuCores : requested_number_threads);

	fThreadPoolLock.Lock();
	fThreads.reserve(num_threads * kMaxNumberWorkThreadsFactor);
	fLoadBalancerThreadCycleCount.reserve(num_threads * kMaxNumberWorkThreadsFactor);

	for (unsigned int i=0; i < num_threads; i++)
	{
		fThreads.emplace_back(new WorkThread(i));
	}
	
	fThreadPoolLock.Unlock();

	//	Shared Timer
	fTimer = nullptr;

	if (enable_load_balancer)
		EnableLoadBalancer(true, load_balancer_period_milliseconds);
}

/*	FUNCTION:		ActorManager :: ~ActorManager
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ActorManager :: ~ActorManager()
{
	fThreadPoolLock.Lock();
	delete fTimer;
	if (fLoadBalancerThread)
	{
		fLoadBalancerThread->request_stop();
		fLoadBalancerThread->join();
		delete fLoadBalancerThread;
		fLoadBalancerThread = nullptr;
	}

	for (auto i : fThreads)
		delete i;

	sInstance = nullptr;
}

/*	FUNCTION:		ActorManager :: AddActor
	ARGUMENTS:		a
	RETURN:			n/a
	DESCRIPTION:	Invoked from Actor constructor
*/
void ActorManager :: AddActor(Actor *a)
{
	fThreadPoolLock.Lock();
	assert(a != nullptr);
	static unsigned int sNextWorkThread = 0;
	a->fWorkThread = fThreads[sNextWorkThread];
	if (++sNextWorkThread >= (int)fThreads.size())
		sNextWorkThread = 0;
	fThreadPoolLock.Unlock();
}

/*	FUNCTION:		ActorManager :: RemoveActor
	ARGUMENTS:		a
	RETURN:			n/a
	DESCRIPTION:	Invoked from Actor destructor
					Check if thread's fWorkQueue contains actor, if so remove it
*/
void ActorManager :: RemoveActor(Actor *a)
{
	CancelTimers(a);

	if (!a->fWorkThread->fWorkQueueLock.Lock())
	{
		printf("[ActorManager] RemoveActor() - cannot acquire fWorkQueueLock #1\n");
		return;
	}
	while (a->fState & Actor::State::eExecuting)
	{
		a->fWorkThread->fWorkQueueLock.Unlock();
		std::this_thread::yield();
		if (!a->fWorkThread->fWorkQueueLock.Lock())
		{
			printf("[ActorManager] RemoveActor() - cannot acquire fWorkQueueLock #2\n");
			return;
		}
	}
	bool repeat = true;
	while (repeat)
	{
		repeat = false;
		for (std::deque<Actor *>::const_iterator it = a->fWorkThread->fWorkQueue.begin(); it != a->fWorkThread->fWorkQueue.end(); it++)
		{
			if (*it == a)
			{
				a->fWorkThread->fWorkQueue.erase(it);
				repeat = true;
				break;
			}
		}
	}
	a->fWorkThread->fWorkQueueLock.Unlock();
}

/*	FUNCTION:		ActorManager :: StealWork
	ARGUMENTS:		destination_thread (or nullptr to randomly select - called by LoadBalancerThread)
					source_thread (or nullptr to randomly select - called by idle work thread)
	RETURN:			true if success
	DESCRIPTION:	Attempt to transfer work from source_thread to destination_thread
*/
const bool ActorManager :: StealWork(WorkThread *destination_thread, WorkThread *source_thread)
{
	const size_t num_threads = fThreads.size();
	if (source_thread == nullptr)
	{
		//	Called by idle work thread, find donar (work to steal)
		int start_idx = destination_thread->fThreadIndex;
		for (size_t tidx = 1; tidx < num_threads; tidx++)
		{
			size_t actual_thread_idx = start_idx + tidx;
			if (actual_thread_idx >= num_threads) actual_thread_idx -= num_threads;
			const auto t = fThreads[actual_thread_idx];
			if (!t->fWorkQueue.empty() && !t->fWorkQueueLock.IsLocked() &&
				(t->fWorkThreadState & WorkThread::ThreadState::eBusy) &&
				!(t->fWorkThreadState & WorkThread::ThreadState::eStoleWork))
			{
				source_thread = t;
				break;
			}
		}
		if (!source_thread)
			return false;
	}

	if (destination_thread == nullptr)
	{
		//	Called by LoadBalancerThread (when system busy) / AddAsyncWork (when parent thread busy), find free thread
		int start_idx = source_thread->fThreadIndex;
		for (size_t tidx = 1; tidx < num_threads; tidx++)
		{
			size_t actual_thread_idx = start_idx + tidx;
			if (actual_thread_idx >= num_threads) actual_thread_idx -= num_threads;
			const auto t = fThreads[actual_thread_idx];
			if (t->fWorkQueue.empty() && !t->fWorkQueueLock.IsLocked())
			{
				destination_thread = t;
				break;
			}
		}

		if (!destination_thread)
		{
			//	2nd attempt, lets find a less busy thread 
			for (size_t tidx = 1; tidx < num_threads; tidx++)
			{
				size_t actual_thread_idx = start_idx + tidx;
				if (actual_thread_idx >= num_threads) actual_thread_idx -= num_threads;
				const auto t = fThreads[actual_thread_idx];
				if ((t->fProcessedMessageCount != fLoadBalancerThreadCycleCount[actual_thread_idx]) && !t->fWorkQueueLock.IsLocked())
				{
					destination_thread = t;
					break;
				}
			}

			if (!destination_thread)
				return false;
		}
	}

	//	Try to acquire both locks
	if (source_thread->fWorkQueueLock.IsLocked() || !source_thread->fWorkQueueLock.TryLock())
		return false;

	if (destination_thread->fWorkQueueLock.IsLocked() || !destination_thread->fWorkQueueLock.TryLock())
	{
		source_thread->fWorkQueueLock.Unlock();
		return false;
	}
	
	//	Select an Actor to steal from work queue
	Actor *actor = nullptr;
	if (!source_thread->fWorkQueue.empty())
	{
		for (std::deque<Actor *>::const_iterator it = source_thread->fWorkQueue.begin(); it != source_thread->fWorkQueue.end(); it++)
		{
			if ((*it != source_thread->fLastActor) &&	//	skip fLastActor (preserve thread hot cache)
				!((*it)->fState & (Actor::State::ePinnedToThread | Actor::State::eExecuting | Actor::State::eSchedularLock)))
			{
				//	remove reference to selected Actor from work queue
				actor = *it;
				actor->fWorkThread = destination_thread;
				source_thread->fRequestedMessageCount -= (uint32_t)actor->fMessageQueue.size();
				source_thread->fWorkQueue.erase(it);
				break;
			}
		}
	}

	//	Push selected Actor's work to destination_thread
	if (actor)
	{
		destination_thread->fWorkQueue.push_back(actor);
		destination_thread->fRequestedMessageCount += (uint32_t)actor->fMessageQueue.size();
		destination_thread->fWorkThreadState |= WorkThread::ThreadState::eStoleWork;
	}

	source_thread->fWorkQueueLock.Unlock();
	destination_thread->fWorkQueueLock.Unlock();
	if (actor)
	{
		destination_thread->fThreadSemaphore.Signal();
		return true;
	}
	else
		return false;
}

/*	FUNCTION:		ActorManager :: Run
	ARGUMENTS:		idle_exit
	RETURN:			n/a
	DESCRIPTION:	Wait until all jobs complete before exiting
*/
void ActorManager :: Run(const bool idle_exit)
{
	fIdleExit = idle_exit;

	while (1)
	{
		//	Block until idle semaphore signalled
		if (!fIdleSemaphore.Wait())
		{
			printf("ActorManager::Run() fIdleSemaphore error\n");
			break;
		}
		
		if (!fIdleExit)
		{
			printf("ActorManager::Run() fIdleExit\n");
			break;
		}

		//	Attempt blocking version (are we really idle?)
		int count_busy;
		bool repeat;
		do
		{
			count_busy = 0;
			repeat = false;
			int count_acquired_thread_locks = 0;
			std::lock_guard<yplatform::Semaphore> aLock(fThreadPoolLock);
			for (auto i : fThreads)
			{
				//	Use TryLock() since we need to avoid potential deadlock
				if (i->fWorkQueueLock.TryLock())
					count_acquired_thread_locks++;
				else
				{
					//	Potential dealock, lets restart the locking process
					repeat = true;
					break;
				}

				if (i->fRequestedMessageCount != i->fProcessedMessageCount)
				{
					count_busy++;
					break;
				}
			}
			if ((count_busy == 0) && (count_acquired_thread_locks == fThreads.size()) && fTimer)
			{
				if (fTimer->IsBusy())
					count_busy++;
			}
			for (int i=0; i < count_acquired_thread_locks; i++)
				fThreads[i]->fWorkQueueLock.Unlock();

		} while (repeat);

		//	Exit when no work remaining
		if (count_busy == 0)
			return;
	}
}


/*	FUNCTION:		ActorManager :: Quit
	ARGUMENTS:		wait_for_unfinished_jobs
	RETURN:			n/a
	DESCRIPTION:	ActorManager can quit when no jobs available or immediately
*/
void ActorManager :: Quit(const bool wait_for_unfinished_jobs)
{
	fIdleExit = wait_for_unfinished_jobs;
	fIdleSemaphore.Signal();
}

/*	FUNCTION:		ActorManager :: WorkThreadIdle
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Determine if no work queued and quit
					This is non-blocking version, while ActorManager::Run() will do another BLOCKING check
*/
void ActorManager :: WorkThreadIdle()
{
	if (fIdleExit)
	{
		for (auto i : fThreads)
		{
			if (i->fRequestedMessageCount != i->fProcessedMessageCount)
				return;
		}

		if (fTimer)
		{
			if (fTimer->IsBusy())
				return;
		}

		//	Possibly all work threads have no work.
		fIdleSemaphore.Signal();
	}
}

/*********************************
	Timer sends Async complete messages
	Use singleton to conserve system resources
**********************************/

/*	FUNCTION:		ActorManager :: AddTimer
	ARGUMENTS:		milliseconds
					message
	RETURN:			none
	DESCRIPTION:	Helper function to add timer (async)
*/
void ActorManager :: AddTimer(const int64_t milliseconds, yarra::ActorMessage<> message)
{
	assert(message.IsValid());

	if (fTimer == nullptr)
		fTimer = new Timer;
	fTimer->Async<&Timer::AddTimer>(milliseconds, std::move(message));
}

/*	FUNCTION:		ActorManager :: CancelTimers
	ARGUMENTS:		target
	RETURN:			none
	DESCRIPTION:	Helper function to remove all timers to target (async)
*/
void ActorManager :: CancelTimers(Actor *target)
{
	assert(target);
	if (fTimer)
	{
		fTimer->Lock();
		fTimer->CancelTimersLocked(target);
		fTimer->Unlock();
	}
}

/*********************************
	LoadBalancer (when enabled) will monitor the WorkThreads to determine whether the system is 'busy'.
	If the system is 'busy', additional work threads will be spawned (capped to kMaxNumberWorkThreadsFactor*kNumberCpuCores)
**********************************/

/*	FUNCTION:		ActorManager :: EnableLoadBalancer
	ARGUMENTS:		enable
	RETURN:			none
	DESCRIPTION:	Start load balancing thread
*/
void ActorManager :: EnableLoadBalancer(const bool enable, const uint64_t milliseconds)
{
	if (enable)
	{
		assert(fLoadBalancerThread == nullptr);
		assert(fLoadBalancerThreadCycleCount.empty());
		fLoadBalancerThread = new std::jthread(&ActorManager::LoadBalancerThread, this);
		fLoadBalancerPeriod = milliseconds;
		for (auto i : fThreads)
			fLoadBalancerThreadCycleCount.push_back(i->fProcessedMessageCount);
	}
	else
	{
		if (fLoadBalancerThread)
		{
			fLoadBalancerThread->request_stop();
			fLoadBalancerThread->join();
			delete fLoadBalancerThread;
			fLoadBalancerThread = nullptr;
		}
		fLoadBalancerThreadCycleCount.clear();
	}
}

/*	FUNCTION:		ActorManager :: LoadBalancerThread
	ARGUMENTS:		arg
	RETURN:			n/a
	DESCRIPTION:	Load balancer thread periodically determines if system 'busy' and adds new worker threads to prevent Actor starvation.
					The system is considerd 'busy' when no new actor messages are processed within the fLoadBalancerPeriod.
*/
void ActorManager :: LoadBalancerThread(void *arg)
{
	ActorManager *manager = (ActorManager *)arg;
	assert(manager != nullptr);
	while (manager->fLoadBalancerThread == nullptr)
		std::this_thread::yield();

	while (!manager->fLoadBalancerThread->get_stop_token().stop_requested())
	{
		//	Sleep for fLoadBalancerPeriod
		std::this_thread::sleep_for(std::chrono::milliseconds(manager->fLoadBalancerPeriod));

		//	Check if system 'busy' (no new actor messages are processed within the fLoadBalancerPeriod)   
		size_t count_busy = 0;
		const size_t num_threads = manager->fThreads.size();
		for (size_t i=0; i < num_threads; i++)
		{
			WorkThread *t = manager->fThreads[i];
			if ((t->fWorkThreadState & WorkThread::ThreadState::eBusy) &&
				(manager->fLoadBalancerThreadCycleCount[i] == t->fProcessedMessageCount) && !t->fWorkQueue.empty())
			{
				++count_busy;

				//	Try to move a queued Actor from the busy WorkThread to another WorkThread
				manager->StealWork(nullptr, t);
			}
			manager->fLoadBalancerThreadCycleCount[i] = t->fProcessedMessageCount;
		}

		//	Spawn new WorkThread?
		if ((count_busy == num_threads) && (num_threads < manager->fThreads.capacity()))
		{
			std::lock_guard<yplatform::Semaphore> aLock(manager->fThreadPoolLock);
			manager->fThreads.emplace_back(new WorkThread((int)num_threads));
			manager->fLoadBalancerThreadCycleCount.push_back(0);
		}
	}
}


};	//	namespace yarra
