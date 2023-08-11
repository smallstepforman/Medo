/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2017-2018, ZenYes Pty Ltd
	DESCRIPTION:	Work thread
*/

#include <cstdio>
#include <cassert>
#include <thread>

#include "Platform.h"
#include "Actor.h"
#include "ActorManager.h"
#include "WorkThread.h"

namespace yarra
{

/*	FUNCTION:		WorkThread :: operator new
	ARGUMENTS:		size
	RETURN:			aligned memory
	DESCRIPTION:	Aligned alloc
*/
void * WorkThread :: operator new(size_t size)
{
	void *p = yplatform::AlignedAlloc(std::hardware_destructive_interference_size, size);
	if (p == nullptr)
		throw std::bad_alloc();
	return p;
}

/*	FUNCTION:		WorkThread :: operator delete
	ARGUMENTS:		p
	RETURN:			n/a
	DESCRIPTION:	Aligned alloc
*/
void WorkThread :: operator delete(void *p)
{
	WorkThread *pc = static_cast<WorkThread *>(p);
	yplatform::AlignedFree(pc);
}

/*	FUNCTION:		WorkThread :: WorkThread
	ARGUMENTS:		index
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
WorkThread :: WorkThread(const int index, const bool spawn_thread)
: fThreadIndex(index), fLastActor(nullptr), fWorkThreadState(0), fRequestedMessageCount(0), fProcessedMessageCount(0)
{
	fThreadSemaphore.Lock();
	
	if (spawn_thread)
	{
		char buffer[32];
		sprintf(buffer, "WorkThread_%02d", index);
		fThread = new yplatform::Thread(&WorkThread::work_thread, this, buffer);
	}
	else
	{
		fThread = nullptr;
		fThreadId = std::this_thread::get_id();
	}

#if ACTOR_DEBUG
	fMigratedFromCount = 0;
	fMigratedToCount = 0;
#endif
}

/*	FUNCTION:		WorkThread :: ~WorkThread
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
					TODO - dont exit until all messages processed
*/
WorkThread :: ~WorkThread()
{
	fWorkQueueLock.Lock();

	delete fThread;
}

/*	FUNCTION:		WorkThread :: Start
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Start thread
*/
void WorkThread :: Start()
{
	if (fThread)
		fThread->Start();
}

/*	FUNCTION:		WorkThread :: AddAsyncWork
	ARGUMENTS:		actor
	RETURN:			n/a
	DESCRIPTION:	Called by Actor::EndAsyncMessage()
*/
void WorkThread :: AddAsyncWork(Actor *actor) noexcept
{
	fRequestedMessageCount++;

	bool signal = false;
	if ((actor->fMessageQueue.size() == 1) && !(actor->fState & Actor::State::eExecuting))
	{
		fWorkQueue.push_back(actor);
		signal = true;
	}
	
	const bool can_migrate = (fLastActor &&
							(fLastActor->fState & (Actor::State::eExecuting | Actor::State::eSchedularLock)) &&
							(fLastActor != actor) &&
							!(actor->fState & Actor::State::eLockedToThread));
	fWorkQueueLock.Unlock();
	
	if (can_migrate && ActorManager::GetInstance()->StealWork(nullptr, this))
		signal = false;
	
	if (signal)
		fThreadSemaphore.Signal();
}

/*	FUNCTION:		WorkThread :: SyncWorkComplete
	ARGUMENTS:		actor
	RETURN:			n/a
	DESCRIPTION:	Called by Actor::EndSyncMessage()
*/
void WorkThread :: SyncWorkComplete(Actor *actor) noexcept
{
	fWorkQueueLock.Lock();
	assert(actor->fState & Actor::State::eSchedularLock);
	bool signal = actor->fState & Actor::State::ePendingSyncSignal;
	actor->fState &= ~(Actor::State::eSchedularLock | Actor::State::ePendingSyncSignal);
	if (signal)
	{
		fWorkQueue.push_front(actor);
	}
	fWorkQueueLock.Unlock();
	
	//	For the scenario where we delayed executing an AsyncMessage, signal the WorkThread
	if (signal)
		fThreadSemaphore.Signal();
}

/*	FUNCTION:		WorkThread :: work_thread
	ARGUMENTS:		arg
	RETURN:			thread exit status
	DESCRIPTION:	Work thread.  Wait for work, then execute Actors's work
*/
int WorkThread :: work_thread(void *arg)
{
	WorkThread *wt = (WorkThread *) arg;
	wt->fThreadId = std::this_thread::get_id();
	ActorManager *actor_manager = ActorManager::GetInstance();
	int tick = 0;	//	prefer to use hot cache
	
	while (1)
	{
		//	steal work ?
		if (wt->fWorkQueue.empty())
		{
			if (!actor_manager->StealWork(wt, nullptr))
				actor_manager->WorkThreadIdle();
		}

		//	Wait for some work
		if (!wt->fThreadSemaphore.Wait())
			goto exit_thread;
		
		bool work_available = true;		//	process a couple of messages from next actor
		while (work_available)
		{
			work_available = false;
			if (!wt->fWorkQueueLock.Lock())
				goto exit_thread;

			if (!wt->fWorkQueue.empty())
			{
				//	Pop Actor from work queue
				wt->fLastActor = std::move(wt->fWorkQueue.front());
				wt->fWorkQueue.pop_front();

				//	Check if Schedular lock active, if so then reinsert work to end of queue
				if (wt->fLastActor->fState & Actor::State::eSchedularLock)
				{
					wt->fLastActor->fState |= Actor::State::ePendingSyncSignal;
					wt->fLastActor = nullptr;
					wt->fWorkQueueLock.Unlock();
					continue;
				}

				//	Fetch work from Actor's message queue
				if (!wt->fLastActor->fMessageQueue.empty())
				{
					//	Pop message from Actor's message queue
					auto msg = std::move(wt->fLastActor->fMessageQueue.front());
					wt->fLastActor->fMessageQueue.pop_front();
					
					wt->fLastActor->fState |= Actor::State::eExecuting;
					wt->fWorkThreadState |= ThreadState::eBusy;
					wt->fWorkThreadState &= ~ThreadState::eStoleWork;
					wt->fWorkQueueLock.Unlock();

					//	Execute message
					msg();

					//	Reset state
					wt->fWorkQueueLock.Lock();
					wt->fWorkThreadState &= ~ThreadState::eBusy;
					++wt->fProcessedMessageCount;
					wt->fLastActor->fState &= ~Actor::State::eExecuting;

					//	More queued messages?
					if (!wt->fLastActor->fMessageQueue.empty())
					{
						//	Add work to WorkThread
						if (++tick & 0x01)	//	50% chance to reschudule same actor
							wt->fWorkQueue.push_front(wt->fLastActor);
						else
							wt->fWorkQueue.push_back(wt->fLastActor);
						work_available = true;
					}
					wt->fLastActor = nullptr;
					wt->fWorkQueueLock.Unlock();
				}
				else
				{
#if ACTOR_DEBUG
					printf("[%d] WorkThread() - No Actor::Message(%p)\n", wt->fThreadIndex, wt->fLastActor);
#endif
					wt->fLastActor = nullptr;
					wt->fWorkQueueLock.Unlock();
				}
			}
			else
			{
#if ACTOR_DEBUG
				printf("[%d] WorkThread() - Unprocessed signal\n", wt->fThreadIndex);
#endif
				wt->fWorkQueueLock.Unlock();
			}
		}
	}

exit_thread:
#if ACTOR_DEBUG
	printf("WorkThread() - Exiting (Thread %d)\n", wt->fThreadIndex);
#endif
	yplatform::ExitThread();
	return 0;
}

const bool WorkThread :: IsCurrentCallingThread() const
{
	return (fThreadId == std::this_thread::get_id());
}

/*************************************************************************************
	OS Looper allows integration of native platform GUI event loops and Actors
	Most native GUI's are single threaded, and typically control OpenGL rendering context.
	The platform event loop is responsible for draining queued messages (eg. after render cycle)
***********************************************************/

/*	FUNCTION:		OsLooper :: OsLooper
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
OsLooper :: OsLooper()
	: WorkThread(-1, false)
{ }

/*	FUNCTION:		OsLooper :: AddAsyncWork
	ARGUMENTS:		actor
	RETURN:			none
	DESCRIPTION:	Add actor to work queue
*/
void OsLooper :: AddAsyncWork(Actor *actor) noexcept
{
	fRequestedMessageCount++;
	if ((actor->fMessageQueue.size() == 1) && !(actor->fState & Actor::State::eExecuting))
	{
		fWorkQueue.push_back(actor);
	}
	fWorkQueueLock.Unlock();
}

/*	FUNCTION:		OsLooper :: SyncWorkComplete
	ARGUMENTS:		actor
	RETURN:			none
	DESCRIPTION:	Check if delayed addition to work queue
*/
void OsLooper :: SyncWorkComplete(Actor *actor) noexcept
{
	fWorkQueueLock.Lock();
	assert(actor->fState & Actor::State::eSchedularLock);
	bool queue = actor->fState & Actor::State::ePendingSyncSignal;
	actor->fState &= ~(Actor::State::eSchedularLock | Actor::State::ePendingSyncSignal);
	if (queue)
		fWorkQueue.push_front(actor);
	fWorkQueueLock.Unlock();
}

/*	FUNCTION:		OsLooper :: ProcessPendingMessages
	ARGUMENTS:		none
	RETURN:			none
	DESCRIPTION:	Called by native platform GUI event loop
*/
void OsLooper :: ProcessPendingMessages()
{
	int tick = 0;	//	prefer to use hot cache

	while (!fWorkQueue.empty() || !fMessageQueue.empty())
	{
		fWorkQueueLock.Lock();
		if (!fMessageQueue.empty())
		{
			auto msg = std::move(fMessageQueue.front());
			fMessageQueue.pop_front();
			fWorkQueueLock.Unlock();

			//	Execute message
			msg();
			continue;
		}
		else if (!fWorkQueue.empty())
		{
			//	Pop Actor from work queue
			fLastActor = std::move(fWorkQueue.front());
			fWorkQueue.pop_front();

			//	Check if Schedular lock active, if so then reinsert work to end of queue
			if (fLastActor->fState & Actor::State::eSchedularLock)
			{
				fLastActor->fState |= Actor::State::ePendingSyncSignal;
				fLastActor = nullptr;
				fWorkQueueLock.Unlock();
				continue;
			}

			//	Fetch work from Actor's message queue
			if (!fLastActor->fMessageQueue.empty())
			{
				//	Pop message from Actor's message queue
				auto msg = std::move(fLastActor->fMessageQueue.front());
				fLastActor->fMessageQueue.pop_front();

				fLastActor->fState |= Actor::State::eExecuting;
				fWorkQueueLock.Unlock();

				//	Execute message
				msg();

				//	Reset state
				fWorkQueueLock.Lock();
				++fProcessedMessageCount;
				fLastActor->fState &= ~Actor::State::eExecuting;

				//	More queued messages?
				if (!fLastActor->fMessageQueue.empty())
				{
					//	Add work to WorkThread
					if (++tick & 0x01)	//	50% chance to reschudule same actor
						fWorkQueue.push_front(fLastActor);
					else
						fWorkQueue.push_back(fLastActor);
				}
			}
			fLastActor = nullptr;
		}
		fWorkQueueLock.Unlock();
	}
}

/*	FUNCTION:		OsLooper :: AsyncValidityCheck
	ARGUMENTS:		none
	RETURN:			true if valid
	DESCRIPTION:	Check if called from OS platform event loop
*/
const bool OsLooper:: AsyncValidityCheck() const
{
	const bool is_looper_thread = (fThreadId == std::this_thread::get_id());
	if (!is_looper_thread)
		assert(0);
	return is_looper_thread;
}

};	//	namespace yarra
