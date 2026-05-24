/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Zen Yes Pty Ltd, 2017-2026, MIT license
	DESCRIPTION:	Actor

	===========================================================================
	USAGE GUIDE
	===========================================================================
	1. ASYNCHRONOUS (Standard):
	   Use target->Async<&Class::Method>( args...) for fire-and-forget logic.
	   This is the most efficient way to interact with an Actor.

	2. SYNCHRONOUS (RAII Lock):
	   Use the LockGuard template for immediate state access or multi-step logic.
	   {
	       auto lg = target->LockGuard<DerivedClass>();
	       int value = lg->GetFoo();
	       lg->SetBar(value + 1);
	   } // Lock released here

	3. DELEGATES (ActorMessage):
	   ActorMessage<int> msg = {this, &MyClass::OnEvent};
	   msg.Send(42);
	===========================================================================
*/

#ifndef _YARRA_ACTOR_H_
#define _YARRA_ACTOR_H_

#include <cstdint>
#include <functional>
#include <deque>
#include <new>

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
		ePinToThread			= 1 << 0,
	};
					Actor(const uint32_t config = ActorConfiguration::eDefault, WorkThread *work_thread = nullptr);
	virtual			~Actor();

	void * operator	new(size_t s);
	void operator	delete(void *p);

//	Message queueing
private:
#if defined(__cpp_lib_move_only_function)
	std::deque<std::move_only_function<void ()>>	fMessageQueue;
#else
	std::deque<std::function<void ()>>				fMessageQueue;
#endif
	void		BeginAsyncMessage() noexcept;		//	Dont expose locking operation via header file
	void		EndAsyncMessage() noexcept;			//	Dont expose unlock/queue operation via header file

public:
	/****************************************************************
		Asynchronous Messages (schedule to be executed from WorkThread)
	
		Usage:
			target.Async<&Actor::Behaviour>(args...);
		or
			target.Async(std::bind(&Actor::Behaviour, target, args...));
		or
			auto msg = std::bind(&Actor::Behaviour, target, args...);
			target.Async(msg);
		or
			auto msg = std::bind(&Actor::Behaviour, target, std::placeholders::_1);
			target.Async(msg, 42);
	*****************************************************************/
	template <class F, class ... Args>
	void Async(F&& fn, Args&& ... args) noexcept
	{
		BeginAsyncMessage();
		fMessageQueue.emplace_back([this, f = std::forward<F>(fn), ...capturedArgs = std::forward<Args>(args)]() mutable {std::invoke(f, std::move(capturedArgs)...);});
		EndAsyncMessage();
	}

	/****************************************************************
		SBO-Optimized Async for Member Function Pointers,
		prevents the member function pointer from bloating the lambda capture.
		Usage:
			target.Async<&Class::Method>(args) 
	*****************************************************************/
	template <auto Method, class ... Args>
	void Async(Args&& ... args) noexcept
	{
		BeginAsyncMessage();
		fMessageQueue.emplace_back([this, ...capturedArgs = std::forward<Args>(args)]() mutable
			{
				[&]<typename C, typename R, typename... MArgs>(R (C::*m)(MArgs...)) {(static_cast<C*>(this)->*m)(std::move(capturedArgs)...);}(Method);
			});
		EndAsyncMessage();
	}

	template <auto Method, class ... Args>
	void AsyncPriority(Args&& ... args) noexcept
	{
		BeginAsyncMessage();
		fMessageQueue.emplace_front([this, ...capturedArgs = std::forward<Args>(args)]() mutable 
		{
			[&]<typename C, typename R, typename... MArgs>(R (C::*m)(MArgs...)) {
				(static_cast<C*>(this)->*m)(std::move(capturedArgs)...);
			}(Method);
		});
		EndAsyncMessage();
	}

public:
	/*************************************************************
		Manual synchronisation (use extreme caution)
		Used to access derived Actor messages
		Caller responsible for deadlock prevention
	**************************************************************/
	virtual void	Lock() noexcept;
	virtual void	Unlock() noexcept;
	const bool		IsLocked() const;

public:
	/*************************************************************
		//	Scope based lock guard
		{
			auto lg = derived->LockGuard<Derived>();	//	similar to scope based lock ->  std::lock_guard<Derived> sync_access(Derived::Lock);
			lg->MethodA();								//	called while holding actor lock
			lg->MethodB();
		}												//	exit scope, release lock guard
	**************************************************************/
	template <typename T>
	struct RaiiLock
	{
		T	*mTarget;
		RaiiLock(T *a) : mTarget(a)	{static_cast<Actor*>(mTarget)->Lock();}
		~RaiiLock()					{static_cast<Actor*>(mTarget)->Unlock();}
		T *operator->()				{ return mTarget; }

		RaiiLock(const RaiiLock &) = delete;
		RaiiLock& operator=(const RaiiLock &) = delete;
	};
	template <typename T>
	[[nodiscard]] RaiiLock<T> LockGuard() 
	{
		static_assert(std::is_base_of_v<Actor, T>, "T must inherit from Actor");
		return RaiiLock<T>(static_cast<T*>(this)); 
	}

	/****************************************************************
		Synchronous access, same functionality LockGuard
		but similar to Actor::Async() function
		Usage:
			child->Sync(&Child::Method, args);
				or
			child->Sync<Child>([&](){child->Method(args);});
	*****************************************************************/
public:
	template <typename T, typename R, typename... MArgs, typename... Args>
	auto Sync(R (T::*method)(MArgs...), Args&&... args) -> R {auto lg = this->LockGuard<T>();return std::invoke(method, static_cast<T*>(this), std::forward<Args>(args)...);}
	template <typename T, typename F, typename... Args>
	auto Sync(F&& fn, Args&&... args) -> std::invoke_result_t<F, T*, Args...>
	{
		auto lg = this->LockGuard<T>();
		return std::invoke(std::forward<F>(fn), static_cast<T*>(this), std::forward<Args>(args)...);
	}
	template <typename F, typename... Args>
	auto Sync(F&& fn, Args&&... args) -> std::invoke_result_t<F, Args...> 
	{
		auto lg = this->LockGuard<Actor>();
		return std::invoke(std::forward<F>(fn), std::forward<Args>(args)...);
	}
	
private:
	friend class	ActorManager;
	friend class	WorkThread;
	friend class	OsLooper;

	WorkThread		*fWorkThread;
	
	enum State : uint32_t 
	{
		eExecuting						= 1 << 0,		//	Set by fWorkThread when executing behaviour
		eSchedularLock					= 1 << 1,		//	Schedular lock active, prevent fWorkThread from executing commands
		ePinnedToThread					= 1 << 2,		//	No work stealing allowed
		ePendingSyncSignal				= 1 << 3,		//	When Sync work complete, signal work thread
	};
	uint32_t		fState;

protected:
	const bool		AsyncValidityCheck() const;
	const bool		IsIdle();
	void			ClearAllMessages();
};

/****************************************************************
	ActorMessage<Signature...>
	Usage:
	yplatform::ActorMessage<int> msg = {this, &Child::Method};
	msg->Send(42);	// invoked asynchronously
*****************************************************************/
template <typename... Signature>
class ActorMessage
{
public:
	Actor								*mActor;
	std::function<void(Signature...)>	mCallback;

	ActorMessage() : mActor(nullptr), mCallback(nullptr) { }

	template <typename T, typename Method, typename... FixedArgs>
	ActorMessage(T* target, Method method, FixedArgs&&... fixed)
	{
		mActor = static_cast<Actor*>(target);
		mCallback = [target, method, ...fArgs = std::forward<FixedArgs>(fixed)](Signature... sigArgs) mutable 
		{
			if constexpr (std::is_member_function_pointer_v<std::decay_t<Method>>)
			{
				auto invoker = [&]<typename C, typename R, typename... MArgs>(R (C::*m)(MArgs...)) 
				{
					(static_cast<C*>(target)->*m)(fArgs..., std::forward<Signature>(sigArgs)...);
				};
				invoker(method);
			}
			else
			{
				// Handle lambdas or other callable objects
				std::invoke(method, fArgs..., std::forward<Signature>(sigArgs)...);
			}
		};
	}

	explicit ActorMessage(Actor* actor, std::function<void(Signature...)> callback) : mActor(actor), mCallback(callback) { }
	inline const ActorMessage& operator = (const ActorMessage& other)
	{
		mActor = other.mActor;	
		mCallback = other.mCallback; 
		return *this;
	}

	// Sends the message. Compiler enforces 'args' match 'Signature...'
	void Send(Signature... args)
	{
		if (mActor && mCallback)
		{
			mActor->Async(mCallback, std::forward<Signature>(args)...);
			Clear();
		}
	}

	//	Dont Clear() the message, allow clients to send repeated echo-echo-echo messages
	void Echo(Signature... args) const
	{ 
		if (mActor && mCallback)
		{
			mActor->Async(mCallback, std::forward<Signature>(args)...);
		}
	}

	const bool IsValid() const	{return mActor && (bool)mCallback;}
	operator bool() const		{return IsValid();}
	void Clear()				{mActor = nullptr; mCallback = nullptr;}
};

/********************************
	MainThreadActor locks the Actor to OsLooper() thread
*********************************/
class MainThreadActor : public Actor
{
public:
					MainThreadActor();
	virtual			~MainThreadActor() {}
	const bool		RenderThreadValidityCheck() const;
};

}	//	namespace yarra

#endif	//#ifndef _YARRA_ACTOR_H_
