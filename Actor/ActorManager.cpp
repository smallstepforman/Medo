/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2017-2018, ZenYes Pty Ltd
	DESCRIPTION:	Actor Manager
					C++20 TODO (when compiler update)
					- use std::invocable for method pointers (Actor::Async)
					- use std::stop_token for detecting thread termination
					- use std::counting_semaphore, also   using binary_semaphore = std::counting_semaphore<1>
					- std::atomic_flag::test to spinlock
					- investigate if ticket_mutex required (fairness)
					- use std::latch for termination processing
*/

#include <cstdio>
#include <cassert>
#include <thread>
#include <mutex>

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

#if ACTOR_DEBUG
	printf("Number Physical CPU cores = %d\n", kNumberPhysicalCpuCores);
	printf("sizeof(WorkThread) = %zu\n", sizeof(WorkThread));
#endif
	
	fThreadPoolLock.Lock();
	fThreads.reserve(kNumberPhysicalCpuCores * kMaxNumberWorkThreadsFactor);
	fLoadBalancerThreadCycleCount.reserve(kNumberPhysicalCpuCores * kMaxNumberWorkThreadsFactor);

	for (unsigned int i=0; i < num_threads; i++)
	{
		fThreads.emplace_back(new WorkThread(i));
	}
	for (auto t : fThreads)
		t->Start();
	
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
	delete fTimer;

	fThreadPoolLock.Lock();
	if (fLoadBalancerThread)
	{
#if ACTOR_DEBUG
		printf("ActorManager::~ActorManager() - destroying %zu WorkThreads\n", fThreads.size());
#endif
		delete fLoadBalancerThread;
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
		return;
	while (a->fState & Actor::STATE_EXECUTING)
	{
		a->fWorkThread->fWorkQueueLock.Unlock();
		std::this_thread::yield();
		if (!a->fWorkThread->fWorkQueueLock.Lock())
			return;
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
				(t->fWorkThreadState & WorkThread::WTS_BUSY) &&
				!(t->fWorkThreadState & WorkThread::WTS_STOLE_WORK))
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
				!((*it)->fState & (Actor::STATE_LOCKED_TO_THREAD | Actor::STATE_EXECUTING | Actor::STATE_SCHEDULAR_LOCK)))
			{
				//	remove reference to selected Actor from work queue
				actor = *it;
				actor->fWorkThread = destination_thread;
				source_thread->fRequestedMessageCount -= (uint32_t)actor->fMessageQueue.size();
#if ACTOR_DEBUG
				source_thread->fMigratedFromCount += (uint32_t)actor->fMessageQueue.size();
#endif
				source_thread->fWorkQueue.erase(it);
				break;
			}
		}
	}

	//	Push selected Actor's work to destination_thread
	if (actor)
	{
#if ACTOR_DEBUG
		printf("*** TransferWork(%p from thread %d to %d).  State=%04x\n", actor, source_thread->fThreadIndex, destination_thread->fThreadIndex, actor->fState);
#endif
		destination_thread->fWorkQueue.push_back(actor);
		destination_thread->fRequestedMessageCount += (uint32_t)actor->fMessageQueue.size();
		destination_thread->fWorkThreadState |= WorkThread::WTS_STOLE_WORK;
#if ACTOR_DEBUG
		destination_thread->fMigratedToCount += (uint32_t)actor->fMessageQueue.size();
#endif
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
			std::lock_guard<Platform::Semaphore> aLock(fThreadPoolLock);
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
		{
#if ACTOR_DEBUG		
			printf("ActorManager::Run() count_busy == 0\n");
			uint64_t requested_count = 0;
			uint64_t process_count = 0;
			uint64_t total_from = 0;
			uint64_t total_to = 0;
			for (auto i : fThreads)
			{
				printf("%d %04x fCompleted = %u, fRequestedCount = %u, migrate from=%u, to=%u\n",
					i->fThreadIndex, i->fWorkThreadState, i->fProcessedMessageCount, i->fRequestedMessageCount, i->fMigratedFromCount, i->fMigratedToCount);

				requested_count += i->fRequestedMessageCount;
				process_count += i->fProcessedMessageCount;
				total_from += i->fMigratedFromCount;
				total_to += i->fMigratedToCount;
			}
			printf("Total process count = %lu, total requested count = %lu, total_from=%lu, total_to=%lu\n", process_count, requested_count, total_from, total_to);
#endif			
			return;
		}
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
					target
					callback_complete
	RETURN:			none
	DESCRIPTION:	Helper function to add timer (async)
*/
void ActorManager :: AddTimer(const int64_t milliseconds, Actor *target, const std::function<void ()> &callback_complete)
{
	assert(target);
	assert(callback_complete);

	if (fTimer == nullptr)
		fTimer = new Timer;
	fTimer->Async(&yarra::Timer::AddTimer, fTimer, milliseconds, target, callback_complete);
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
		fLoadBalancerThread = new Platform::Thread(&ActorManager::LoadBalancerThread, this, "Load Balancer");
		fLoadBalancerPeriod = milliseconds;
		for (auto i : fThreads)
			fLoadBalancerThreadCycleCount.push_back(i->fProcessedMessageCount);
		fLoadBalancerThread->Start();
	}
	else
	{
		delete fLoadBalancerThread;
		fLoadBalancerThread = nullptr;
		fLoadBalancerThreadCycleCount.clear();
	}
}

/*	FUNCTION:		ActorManager :: LoadBalancerThread
	ARGUMENTS:		arg
	RETURN:			thread exit status
	DESCRIPTION:	Load balancer thread periodically determines if system 'busy' and adds new worker threads to prevent Actor starvation.
					The system is considerd 'busy' when no new actor messages are processed within the fLoadBalancerPeriod.
*/
int ActorManager :: LoadBalancerThread(void *arg)
{
	ActorManager *manager = (ActorManager *)arg;
	assert(manager != nullptr);

	while (1)
	{
		//	Sleep for fLoadBalancerPeriod
		Platform::Sleep(manager->fLoadBalancerPeriod);

		//	Check if system 'busy' (no new actor messages are processed within the fLoadBalancerPeriod)   
		size_t count_busy = 0;
		const size_t num_threads = manager->fThreads.size();
		for (size_t i=0; i < num_threads; i++)
		{
			WorkThread *t = manager->fThreads[i];
			if ((t->fWorkThreadState & WorkThread::WTS_BUSY) &&
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
#if ACTOR_DEBUG
			printf("ActorManager::LoadBalancerThread() - spawning new WorkThread\n");
#endif

			std::lock_guard<Platform::Semaphore> aLock(manager->fThreadPoolLock);
			manager->fThreads.emplace_back(new WorkThread((int)num_threads));
			manager->fLoadBalancerThreadCycleCount.push_back(0);
			manager->fThreads[num_threads]->Start();
		}
	}
	Platform::ExitThread();
	return 0;
}


};	//	namespace yarra
