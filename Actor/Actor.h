/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2017-2018, ZenYes Pty Ltd
	DESCRIPTION:	Actor
*/

#ifndef _YARRA_ACTOR_H_
#define _YARRA_ACTOR_H_

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

namespace yarra
{

static const size_t kCacheLineAlignment = 64;	//	Try to keep each object on unique cache line

class WorkThread;

//============
class alignas(kCacheLineAlignment) Actor
{
public:
	enum ACTOR_CONFIGURATION
	{
		CONFIGURATION_DEFAULT				= 0,
		CONFIGURATION_LOCK_TO_THREAD		= 1 << 0,
	};
					Actor(const uint32_t config = CONFIGURATION_DEFAULT, WorkThread *work_thread = nullptr);
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
	void			Lock() noexcept;
	void			Unlock() noexcept;
	const bool		IsLocked() const;

	//	Convenience functions to work with lock_guard<Actor>
	inline void		lock() { Lock(); }
	inline void		unlock() { Unlock(); }
	
	/****************************************************************
		SyncMessage (executed from same thread, synchronously)
		Similar usage to AsyncMessage
	*****************************************************************/
	template <class F, class ... Args>
	auto SyncMessage(F &&fn, Args && ... args)
	{
		Lock();
		auto ret = std::invoke(std::forward<F>(fn), std::forward<Args>(args)...);
		Unlock();
		return ret;
	}

	template <class M> struct _BehaviourSync;
	template <class R, class C, class ... Args> struct _BehaviourSync<R (C::*)(Args ...)>
	{
		using M = R (C::*)(Args ...);
		template <M method> static R SyncMessage2(C *target, Args && ... args)
		{
			target->Lock();
			auto ret = std::invoke(method, target, args...);
			target->Unlock();
			return ret;
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
	
	enum STATE {
		STATE_EXECUTING						= 1 << 0,		//	Set by fWorkThread when executing behaviour
		STATE_SCHEDULAR_LOCK				= 1 << 1,		//	Schedular lock active, prevent fWorkThread from executing commands
		STATE_LOCKED_TO_THREAD				= 1 << 2,		//	No work stealing allowed
		STATE_PENDING_SYNC_SIGNAL			= 1 << 3,		//	When Sync work complete, signal work thread
	};
	uint32_t		fState;

protected:
	const bool		AsyncValidityCheck() const;
	const int		GetWorkThreadIndex() const;

	//	Caution - only use the following methods if you really know what you're doing
	void			ClearAllMessages();
	bool			IsIdle();
};

};	//	namespace yarra

#endif	//#ifndef _YARRA_ACTOR_H_
