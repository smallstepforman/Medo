/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2017-2018, ZenYes Pty Ltd
	DESCRIPTION:	Actor
*/

#ifndef _YARRA_ACTOR_H_
#define _YARRA_ACTOR_H_

#include <cstdint>
#include <functional>
#include <deque>

#define ACTOR_DEBUG		0

/***********************************************
std::invoke is part of C++17 (not available in legacy compilers like C++11, C++14)
Legacy compilers must define __YARRA_STD_INVOKE__, which will add std::invoke
http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3727.html
************************************************/
#if defined (__YARRA_STD_INVOKE__)
namespace std
{
	template<typename Fn, typename... Args>
	typename std::enable_if<std::is_member_pointer<typename std::decay<Fn>::type>::value, typename std::result_of<Fn && (Args&&...)>::type>::type invoke(Fn&& f, Args&&... args)
	{
		return std::mem_fn(f)(std::forward<Args>(args)...);
	}

	template<typename Fn, typename... Args>
	typename std::enable_if<!std::is_member_pointer<typename std::decay<Fn>::type>::value, typename std::result_of<Fn && (Args&&...)>::type>::type invoke(Fn&& f, Args&&... args)
	{
		return std::forward<Fn>(f)(std::forward<Args>(args)...);
	}

	template<typename R, typename Fn, typename... Args>
	R invoke(Fn&& f, Args&&... args)
	{
		return invoke(std::forward<Fn>(f), std::forward<Args>(args)...);
	}
};
#endif	//#ifndef __YARRA_STD_INVOKE__

#if defined (__LINUX__)
	#if !defined (__cpp_lib_hardware_interference_size) || (__cpp_lib_hardware_interference_size < 201603)
		namespace std
		{
			constexpr size_t hardware_destructive_interference_size = 64;
		};
	#endif
#endif

namespace yarra
{

class WorkThread;

//============
class alignas(std::hardware_destructive_interference_size) Actor
{
public:
	enum ActorConfiguration : uint32_t
	{
		eDefault				= 0,
		eLockToThread			= 1 << 0,
	};
					Actor(const uint32_t config = ActorConfiguration::eDefault, WorkThread *work_thread = nullptr);
	virtual			~Actor();

	void * operator	new(size_t s);
	void operator	delete(void *p);

//	Message queueing
private:
	std::deque<std::function<void ()> >	fMessageQueue;

public:
	/****************************************************************
		Asynchronous Messages (schedule to be executed from WorkThread)
	
		Usage:
			target.Async(&Actor::Behaviour, target, args...);
		or
			target.Async(std::bind(&Actor::Behaviour, target, args...));
		or
			std::function<void()> msg = std::bind(&Actor::Behaviour, target, args...);
			target.Async(msg);
		or
			auto msg = std::bind(&Actor::Behaviour, target, args...);
			target.Async(msg);
		or
			auto msg = std::bind(&Actor::Behaviour, target, std::placeholders::_1);
			target.Async(msg, 42);
		or
			ASYNC(&Actor::Behaviour)(target, args...);
	*****************************************************************/
	template <class F, class ... Args>
	void Async(F &&fn, Args && ... args) noexcept
	{
		BeginAsyncMessage();
		fMessageQueue.emplace_back(std::bind(std::forward<F>(fn), std::forward<Args>(args)...));
		EndAsyncMessage();
	}

	template <class F, class ... Args>
	void AsyncPriority(F &&fn, Args && ... args) noexcept
	{
		BeginAsyncMessage();
		fMessageQueue.emplace_front(std::bind(std::forward<F>(fn), std::forward<Args>(args)...));
		EndAsyncMessage();
	}

	template <class M> struct _BehaviourAsync;
	template <class C, class ... Args> struct _BehaviourAsync<void (C::*)(Args ...)>
	{
		using M = void (C::*)(Args ...);
		template <M method> static void AsyncMessage2(C *target, Args && ... args) noexcept
		{
			target->BeginAsyncMessage();
			target->fMessageQueue.emplace_back(std::bind(method, target, args...));
			target->EndAsyncMessage();
		}
	};
	#define ASYNC(m) _BehaviourAsync<decltype(m)>::AsyncMessage2<m>

public:
	/*************************************************************
		Manual synchronisation (use extreme caution)
		Used to access derived Actor messages
		Caller responsible for deadlock prevention
	**************************************************************/
	virtual void	Lock() noexcept;
	virtual void	Unlock() noexcept;
	const bool		IsLocked() const;

	//	Convenience functions to work with lock_guard<Actor>
	inline void		lock() { Lock(); }
	inline void		unlock() { Unlock(); }
	
	/****************************************************************
		SyncMessage (executed from same thread, synchronously)
		Similar usage to AsyncMessage
	*****************************************************************/
	template <class F, class ... Args>
	void SyncMessage(F &&fn, Args && ... args)
	{
		Lock();
		std::invoke(std::forward<F>(fn), std::forward<Args>(args)...);
		Unlock();
	}

	template <class M> struct _BehaviourSync;
	template <class R, class C, class ... Args> struct _BehaviourSync<R (C::*)(Args ...)>
	{
		using M = R (C::*)(Args ...);
		template <M method> static R SyncMessage2(C *target, Args && ... args)
		{
			target->Lock();
			std::invoke(method, target, args...);
			target->Unlock();
		}
	};
	#define SYNC(m) _BehaviourSync<decltype(m)>::SyncMessage2<m>

private:
	void	BeginAsyncMessage() noexcept;		//	Dont expose locking operation via header file
	void	EndAsyncMessage() noexcept;			//	Dont expose unlock/queue operation via header file

//	Actor Manager
private:
	friend class	ActorManager;
	friend class	WorkThread;
	friend class	OsLooper;

	WorkThread		*fWorkThread;
	
	enum State : uint32_t 
	{
		eExecuting						= 1 << 0,		//	Set by fWorkThread when executing behaviour
		eSchedularLock					= 1 << 1,		//	Schedular lock active, prevent fWorkThread from executing commands
		eLockedToThread					= 1 << 2,		//	No work stealing allowed
		ePendingSyncSignal				= 1 << 3,		//	When Sync work complete, signal work thread
	};
	uint32_t		fState;

protected:
	const bool		AsyncValidityCheck() const;
	const int		GetWorkThreadIndex() const;

	//	Caution - only use the following methods if you really know what you're doing
	void			ClearAllMessages();
	const bool		IsIdle();
};

/********************************
	Message is a convenience container
*********************************/
class ActorMessage
{
public:
	Actor					*mActor;
	std::function<void ()>	mCallback;

	explicit ActorMessage() : mActor(nullptr), mCallback(0) {}
	explicit ActorMessage(Actor *actor, std::function<void ()> callback) : mActor(actor), mCallback(callback) {}
	inline const ActorMessage & operator = (const ActorMessage & actor)		{mActor = actor.mActor;	mCallback = actor.mCallback; return *this;}
	template <typename... Args>
	void Invoke(const bool reset = true, Args && ... args)
	{
		if (mActor && mCallback)
		{
			mActor->Async(mCallback, std::forward<Args>(args)...);
			if (reset)
			{
				mActor = nullptr;
				mCallback = 0;
			}
		}
	}
	const bool	IsValid() const {return mActor && mCallback;}
	void		Clear() {mActor = nullptr; mCallback = 0;}
};

};	//	namespace yarra

#endif	//#ifndef _YARRA_ACTOR_H_
