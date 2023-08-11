/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2017-2018, ZenYes Pty Ltd
	DESCRIPTION:	Actor
*/

#include <cassert>
#include <thread>

#include "Platform.h"
#include "ActorManager.h"
#include "WorkThread.h"
#include "Actor.h"

namespace yarra
{

/*	FUNCTION:		Actor :: Actor
	ARGUMENTS:		config
					work_thread
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Actor :: Actor(const uint32_t config, WorkThread *work_thread)
	:fState(0)
{
	//	Locked to thread?
	if ((config & ActorConfiguration::eLockToThread) || work_thread)
		fState |= State::eLockedToThread;

	//	Attach to scheduler
	ActorManager *manager = ActorManager::GetInstance();
	assert(manager != nullptr);

	if (!work_thread)
		manager->AddActor(this);
	else
		fWorkThread = work_thread;
	
#if ACTOR_DEBUG
	printf("Actor::Constructor(%p) Thread %d\n", this, fWorkThread->fThreadIndex);
#endif
}

/*	FUNCTION:		Actor :: ~Actor
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
					TODO - dont exit until all messages processed
*/
Actor :: ~Actor()
{
#if ACTOR_DEBUG
	printf("Actor::Destructor() %p\n", this);
#endif
	ActorManager::GetInstance()->RemoveActor(this);
}

/*	FUNCTION:		Actor :: new
	ARGUMENTS:		sz
	RETURN:			memory
	DESCRIPTION:	Override new operator to ensure alignment
*/
void * Actor :: operator new(size_t sz)
{
    void *p = yplatform::AlignedAlloc(std::hardware_destructive_interference_size, sz);
    if (p == nullptr)
        throw std::bad_alloc();
    return p;
}

/*	FUNCTION:		Actor :: delete
	ARGUMENTS:		p
	RETURN:			n/a
	DESCRIPTION:	Override delete operator to ensure alignment
*/
void Actor :: operator delete(void *p)
{
    Actor *pc = static_cast<Actor *>(p);
    yplatform::AlignedFree(pc);
}

/*	FUNCTION:		Actor :: GetWorkThreadIndex
	ARGUMENTS:		none
	RETURN:			thread index
	DESCRIPTION:	Get thread index
*/
const int Actor :: GetWorkThreadIndex() const
{
	return fWorkThread->GetWorkThreadIndex();
}

/*	FUNCTION:		Actor :: BeginAsyncMessage
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Called from AsyncMessage()
					Caution - When stealing work, the actor can migrate to a different thread which invalidates it's fWorkQueueLock
*/
void Actor :: BeginAsyncMessage() noexcept
{
	bool repeat;
	do
	{
		repeat = false;
		WorkThread *t = fWorkThread;
		t->fWorkQueueLock.Lock();
		if (t != fWorkThread)
		{
			t->fWorkQueueLock.Unlock();
			std::this_thread::yield();	//	relieve pressure from cache line
			repeat = true;
		}
	} while (repeat);
}

/*	FUNCTION:		Actor :: EndAsyncMessage
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Called from AsyncMessage()
					Dont expose unlocking/queue operation via header file
*/
void Actor :: EndAsyncMessage() noexcept
{
	fWorkThread->AddAsyncWork(this);
}

/*	FUNCTION:		Actor :: Lock
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Manual synchronisation (use extreme caution)
					Used to access derived Actor methods
					Caller responsible for deadlock prevention
*/
void Actor :: Lock() noexcept
{
	bool repeat;
	do
	{
		repeat = false;
		WorkThread *t = fWorkThread;
		t->fWorkQueueLock.Lock();
		if ((t != fWorkThread) || (fState & (State::eExecuting | State::eSchedularLock)))
		{
			t->fWorkQueueLock.Unlock();
			std::this_thread::yield();	//	relieve pressure from cache line
			repeat = true;
		}
	} while (repeat);

	//	Validate that Actor allowed to run on this WorkThread
	if (fState & State::eLockedToThread)
		assert(fWorkThread->IsCurrentCallingThread());

	fState |= State::eSchedularLock;
	fWorkThread->fWorkQueueLock.Unlock();
}

/*	FUNCTION:		Actor :: Unlock
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Manual synchronisation (use extreme caution)
					Caller responsible for deadlock prevention
*/
void Actor :: Unlock() noexcept
{
	fWorkThread->SyncWorkComplete(this);
}

/*	FUNCTION:		Actor :: IsLocked
	ARGUMENTS:		none
	RETURN:			true if locked
	DESCRIPTION:	Check if Actor work queue locked
*/
const bool Actor :: IsLocked() const
{
	return (fState & State::eSchedularLock);
}

/*	FUNCTION:		Actor :: AsyncValidityCheck
	ARGUMENTS:		none
	RETURN:			true if valid
	DESCRIPTION:	Check if Actor called from correct thread.
					Call from Behaviour to ensure caller isn't calling function directly (bypassing Actor restrictions)
*/
const bool Actor :: AsyncValidityCheck() const
{
	if (fState & (State::eLockedToThread | State::eExecuting))
	{
		if (std::this_thread::get_id() == fWorkThread->fThreadId)
			return true;
	}

	if (fState & State::eSchedularLock)
		return true;

	assert(0);
	return false;
}

/**************************************************************
	Caution - the following methods must be used with care
	Unsafe, dangerous, know what you're doing
***************************************************************/

/*	FUNCTION:		Actor :: ClearAllMessages
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Clear pending messages
					Caution - a new message may arrive as we're returning
*/
void Actor :: ClearAllMessages()
{
	BeginAsyncMessage();
	fMessageQueue.clear();
	fWorkThread->fWorkQueueLock.Unlock();
}

/*	FUNCTION:		Actor :: IsIdle
	ARGUMENTS:		none
	RETURN:			true if idle
	DESCRIPTION:	Check if actor executing work.
					Caution - a new message/work may start as we/re returning ...
*/
const bool Actor :: IsIdle()
{
	BeginAsyncMessage();
	bool idle = ((fState & State::eExecuting) == 0) && (fMessageQueue.empty());
	fWorkThread->fWorkQueueLock.Unlock();
	return idle;
}

};	//	namespace yarra
