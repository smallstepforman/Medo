/*	PROJECT:		Yarra Actor Model
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Zen Yes Pty Ltd, 2017-2026, MIT license
	DESCRIPTION:	Actor Manager
*/

#ifndef _YARRA_ACTOR_MANAGER_H_
#define _YARRA_ACTOR_MANAGER_H_

#include <vector>

#ifndef _YARRA_PLATFORM_H_
#include "Platform.h"
#endif

#ifndef _YARRA_ACTOR_H_
#include "Actor.h"
#endif

namespace std {class jthread;}

namespace yarra
{

class WorkThread;
class Timer;

//================
class ActorManager
{
public:
			ActorManager(const unsigned int number_threads = 0,
						 const bool enable_load_balancer = true, const uint64_t load_balancer_period_milliseconds = 500);
			~ActorManager();
	void	Run(const bool idle_exit = true);
	void	Quit(const bool wait_for_unfinished_jobs = true);
	static	ActorManager inline	*GetInstance() {return sInstance;}

private:
	friend class Actor;
	friend class WorkThread;
	static	ActorManager		*sInstance;
	
	std::vector<WorkThread *>	fThreads;
	void		AddActor(Actor *a);
	void		RemoveActor(Actor *a);
	const bool	StealWork(WorkThread *destination_thread, WorkThread *source_thread);
		
	void					WorkThreadIdle();
	bool					fIdleExit;
	yplatform::Semaphore	fIdleSemaphore;
	yplatform::Semaphore	fThreadPoolLock;

	/************************************
		Timer sends Async complete messages (shared instance)
	*************************************/
public:
	void					AddTimer(const int64_t milliseconds, yarra::ActorMessage<> message);
	void					CancelTimers(Actor *target);

private:
	Timer					*fTimer;

	/********************************
		LoadBalancer (when enabled) will monitor the WorkThreads to determine whether the system is 'busy'.
		If the system is 'busy', additional WorkThreads will be spawned (capped to 3*kNumberPhysicalCpuCores)
	*********************************/
public:
	void				EnableLoadBalancer(const bool enable, const uint64_t period_milliseconds = 500);
private:
	std::jthread		*fLoadBalancerThread;
	bool				fTerminateLoadBalancerThread;
	uint64_t			fLoadBalancerPeriod;
	std::vector<int>	fLoadBalancerThreadCycleCount;
	static void			LoadBalancerThread(void *);
};

};	//	namespace yarra

#endif	//#ifndef _YARRA_ACTOR_MANAGER_H_
