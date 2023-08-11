/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2017-2018, ZenYes Pty Ltd
	DESCRIPTION:	Work thread
*/

#ifndef _YARRA_WORK_THREAD_H_
#define _YARRA_WORK_THREAD_H_

#include <deque>
#include <thread>
#include <new>

#ifndef _YARRA_PLATFORM_H_
#include "Platform.h"
#endif

#ifndef _YARRA_ACTOR_H_
#include "Actor.h"
#endif


namespace yarra
{
class Thread;
class ActorManager;

class alignas(std::hardware_destructive_interference_size) WorkThread
{
public:
				WorkThread(const int index, const bool spawn_thread = true);
	virtual 	~WorkThread();

	void * operator		new(size_t size);
	void operator		delete(void *p);

protected:
	//	Actual work thread
	yplatform::Thread		*fThread;
	yplatform::Semaphore	fThreadSemaphore;
	std::thread::id			fThreadId;

	const int	GetWorkThreadIndex() const	{return fThreadIndex;}
	const bool	IsCurrentCallingThread() const;

	const int	fThreadIndex;
	static int	work_thread(void *);
	void		Start();

	//	Work queue (actors with pending commands)
	friend ActorManager;
	friend Actor;
	std::deque<Actor *>		fWorkQueue;

	enum ThreadState : uint32_t
	{
		eBusy			= 1 << 0,	//	Executing work
		eStoleWork		= 1 << 1,	//	prevent StealWork() from snatching freshly stolen actor
	};
	uint32_t				fWorkThreadState;

#if 1
	yplatform::Semaphore	fWorkQueueLock;
#else
	yplatform::SpinLock		fWorkQueueLock;
#endif
	Actor					*fLastActor;
	
	virtual void			AddAsyncWork(Actor *actor) noexcept;
	virtual void			SyncWorkComplete(Actor *actor) noexcept;

	uint32_t				fRequestedMessageCount;
	uint32_t				fProcessedMessageCount;

#if ACTOR_DEBUG
	uint32_t				fMigratedFromCount;
	uint32_t				fMigratedToCount;
#endif
};

/*************************************************************************************
	OS Looper allows integration of native platform GUI event loops and Actors
	Most native GUI's are single threaded, and typically control OpenGL rendering context.
	The platform event loop is responsible for draining queued messages (eg. after render cycle)
	The OsLooper is not added to ActorManager and doesn't participate in Work Stealing
***********************************************************/
class alignas(std::hardware_destructive_interference_size) OsLooper : public WorkThread
{
public:
				OsLooper();
	virtual		~OsLooper() {}
	void		ProcessPendingMessages();

private:
	friend Actor;
	void		AddAsyncWork(Actor *actor)		noexcept	override;
	void		SyncWorkComplete(Actor *actor)	noexcept	override;

/****************************************
	Messages to OsLooper
*****************************************/
private:
	std::deque<std::function<void()> >	fMessageQueue;
protected:
	const bool AsyncValidityCheck() const;
public:
	template <class F, class ... Args>
	void Async(F&& fn, Args && ... args) noexcept
	{
		fWorkQueueLock.Lock();
		fMessageQueue.emplace_back(std::bind(std::forward<F>(fn), std::forward<Args>(args)...));
		fWorkQueueLock.Unlock();
	}
};

};	//	namespace yarra

#endif	//#ifndef _YARRA_WORK_THREAD_H_
