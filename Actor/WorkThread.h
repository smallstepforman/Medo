/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Zen Yes Pty Ltd, 2017-2026, MIT license
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
	std::jthread			*fThread;
	yplatform::Semaphore	fThreadSemaphore;
	std::thread::id			fThreadId;

	const int	GetWorkThreadIndex() const	{return fThreadIndex;}
	const bool	IsCurrentCallingThread() const;

	const int	fThreadIndex;
	static void	work_thread(std::stop_token st, void *);

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
	friend class Actor;
	void		AddAsyncWork(Actor *actor)		noexcept	override;
	void		SyncWorkComplete(Actor *actor)	noexcept	override;

/****************************************
	Messages to OsLooper
*****************************************/
private:
#if defined(__cpp_lib_move_only_function)
	std::deque<std::move_only_function<void ()>>	fMessageQueue;
#else
	std::deque<std::function<void ()>>				fMessageQueue;
#endif
protected:
	const bool AsyncValidityCheck() const;
public:
	template <class F, class ... Args>
	void Async(F&& fn, Args&& ... args) noexcept
	{
		fWorkQueueLock.Lock();
		fMessageQueue.emplace_back([this, f = std::forward<F>(fn), ...capturedArgs = std::forward<Args>(args)]() mutable
			{
				std::invoke(f, std::move(capturedArgs)...);
			});
		fWorkQueueLock.Unlock();
	}

	/****************************************************************
		SBO-Optimized Async for Member Function Pointers
		Intercepts target->Async<&Class::Method>(args)
	*****************************************************************/
	template <auto Method, class ... Args>
	void Async(Args&& ... args) noexcept
	{
		fWorkQueueLock.Lock();
		fMessageQueue.emplace_back([this, ...capturedArgs = std::forward<Args>(args)]() mutable 
			{
				[&]<typename C, typename R, typename... MArgs>(R (C::*m)(MArgs...)) {(static_cast<C*>(this)->*m)(std::move(capturedArgs)...);}(Method);
			});
		fWorkQueueLock.Unlock();
	}
};

};	//	namespace yarra

#endif	//#ifndef _YARRA_WORK_THREAD_H_
