/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2017-2018, ZenYes Pty Ltd
	DESCRIPTION:	Platform specific multiprocessing support
*/

#ifndef _YARRA_PLATFORM_H_
#define _YARRA_PLATFORM_H_

#if defined WIN32
	#include <windows.h>
	#include <atomic>
#elif defined __APPLE__
	#include <pthread.h>
	
	#define OSX_DISPATCH_SEMAPHORE		1
	#if OSX_DISPATCH_SEMAPHORE
		#include <dispatch/dispatch.h>
	#else
		#include <mach/semaphore.h>
	#endif
#elif defined __HAIKU__
	#include <kernel/OS.h>
	#include <atomic>
#elif defined __LINUX__
	#include <semaphore.h>
	#include <atomic>
#else
	#error Unknown platform
#endif

#include <string>

namespace yarra
{
	namespace Platform
	{

		/*********************************************
			Platform API
		*********************************************/
		void			ExitThread();
		const uint64_t	GetThreadId();
		void			Sleep(const uint64_t milliseconds);
		const double	GetElapsedTime();
		void *			AlignedAlloc(size_t alignment, size_t size);
		void			AlignedFree(void *ptr);

		/*********************************************
		******		Semaphore					******
		**********************************************/
		class	Semaphore
		{
		public:
			Semaphore(const int initial = 1);
			~Semaphore();

			//	Acquire semaphore
			const bool		Lock();
			const bool		TryLock(const uint64_t milliseconds = 1, const bool exit_locked = true);
			//	Release semaphore
			const bool		Unlock(const bool reschudule = false);

			inline const bool IsLocked() const { return (fAvailable <= 0);}	//	mostly accurate, used to reduce contention in StealWork

			//	Convience functions to aid readability
			inline const bool		Wait() { return Lock(); }
			inline const bool		Signal() { return Unlock(); }

			//	Convenience functions to work with lock_guard<Semaphore>
			inline void		lock() { Lock(); }
			inline void		unlock() { Unlock(); }

		private:
			std::atomic<int32_t>	fAvailable;

#if defined WIN32
			HANDLE					fSemaphore;
#elif defined (__LINUX__)
			sem_t					fSemaphore;
#elif defined (__APPLE__)
	#if OSX_DISPATCH_SEMAPHORE
			dispatch_semaphore_t	fSemaphore;
	#else
			semaphore_t				fSemaphore;
	#endif
#elif defined (__HAIKU__)
			sem_id					fSemaphore;
#endif
		};

		/*********************************************
		 SpinLock
		**********************************************/
		class SpinLock
		{
			std::atomic_flag fLocked = ATOMIC_FLAG_INIT;
			bool fIsLocked = false;
		public:
			const bool			Lock();
			const bool			TryLock(const int num_cycles = 256);
			const bool			Unlock();
			
			inline const bool	IsLocked() const { return fIsLocked; }	//	mostly accurate, used to reduce contention in StealWork
			
			//	Convenience functions to work with lock_guard<SpinLock>
			inline void		lock() { Lock(); }
			inline void		unlock() { Unlock(); }
		};

		/*********************************************
			Thread is a wrapper for cross platform thread support.
		**********************************************/
		class Thread
		{
		public:
			Thread(int(*thread_function)(void *), void *data, const char *thread_name);
			~Thread();

			const bool	Suspend();
			const bool	Resume();
			const bool	IsCurrentCallingThread() const;

			/*	pthreads don't properly support suspend/resume.
				Use the following workaround to actually start the thread.
			*/
			const bool	Start();

		private:
			std::string		fThreadName;
#ifdef WIN32
			HANDLE			fThreadHandle;
			DWORD			fThreadId;
#endif
#ifdef __HAIKU__
			int32			fThreadHandle;
#endif
#if defined (__APPLE__) || defined (__LINUX__)
			pthread_t		fThreadHandle;
			int(*fFunction)(void *);
			void			*fFunctionData;
#endif
		};

	};	//	namespace Platform
	
};	//	namespace yarra

#endif	//#ifndef _YARRA_PLATFORM_H_

