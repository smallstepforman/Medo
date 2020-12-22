/*	PROJECT:		Yarra Actor Model
	COPYRIGHT:		2017-2018, Zen Yes Pty Ltd, Melbourne, Australia
	AUTHORS:		Zenja Solaja
	DESCRIPTION:	Platform specific multiprocessing support
*/
#include <sys/time.h>
#include <cassert>
#include <immintrin.h>
#include <thread>

#include <Alert.h>

#include "Platform.h"

namespace yarra
{
namespace Platform
{
	
/*************************************************
	yarra::Platform::Platform specific functions
 **************************************************/

/*	FUNCTION:		yarra::Platform::Exit
	ARGUMENTS:		message
	RETURN:			n/a
	DESCRIPTION:	Abnormal exit.  Print error message, then exit.
 */
void Exit(const char *message, ...)
{
	va_list			ap;		//argument pointer
	char buffer[256] = "";
	
	// format string
	va_start(ap, message);
	vsprintf(buffer, message, ap);
	va_end(ap);
	
	printf("%s", buffer);
	exit(1);
}

/*	FUNCTION:		yarra::Platform::Debug
	ARGUMENTS:		text to display
	RETURN:			none
	DESCRIPTION:	Print debug output
 */
void Debug(const char *format, ...)
{
	static char gDebugPrintBuffer[0x200];	// OSX paths can be up to 1024 characters long
	
	va_list			ap;		//argument pointer
	*gDebugPrintBuffer = 0;
	
	// format string
	va_start(ap, format);
	vsprintf(gDebugPrintBuffer, format, ap);
	va_end(ap);
	
	printf("%s", gDebugPrintBuffer);
}

/*	FUNCTION:		yarra::Platform::Sleep
	ARGUMENTS:		milliseconds
	RETURN:			n/a
	DESCRIPTION:	Sleep for set number of milliseconds
*/
void Sleep(const uint64_t milliseconds)
{
	usleep(milliseconds*1000);
}

/*	FUNCTION:		yarra::Platform::GetElapsedTime
	ARGUMENTS:		none
	RETURN:			time in float format
	DESCRIPTION:	Platform specific timer function.
					This function is only useful for delta_time calculations.
*/
const double GetElapsedTime()
{
	bigtime_t current_time = system_time();
	return double(current_time)/1000000.0;
}

/*	FUNCTION:		yarra::Platform::GetNumberCpuCores
	ARGUMENTS:		none
	RETURN:			count available cores
	DESCRIPTION:	Get number of available CPU cores
 */
const int	GetNumberCpuCores()
{
	//return std::thread::hardware_concurrency();
	system_info info;
	get_system_info(&info);
	return info.cpu_count;
}


/*************************************************
	yarra::Platform::Semaphore is a platform locking primitive
**************************************************/

#define USE_BENAPHORE	1		//	Benchmark shows 14s vs 22s improvement with Benaphores

/*	FUNCTION:		Semaphore :: Semaphore
	ARGUMENTS:		initial
	RETURN:			n/a
	DESCRIPTION:	Constructor
					Cannot use Benaphores since TryLock corrupts count on timeout
*/
Semaphore :: Semaphore(const int initial)
{
#if USE_BENAPHORE
	if ((fSemaphore = create_sem(initial - 1, "aSemaphore")) < B_OK)
#else
	if ((fSemaphore = create_sem(initial, "aSemaphore")) < B_OK)
#endif		
		Exit("YSemaphore::YSemaphore() error");
	fAvailable.store(initial);
}

/*	FUNCTION:		yarra::Platform::Semaphore::~Semaphore
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Semaphore :: ~Semaphore()
{
	delete_sem(fSemaphore);
}

/*	FUNCTION:		Semaphore::Lock
	ARGUMENTS:		none
	RETURN:			true if success
	DESCRIPTION:	Enter critical region
*/
const bool Semaphore :: Lock()
{
#if USE_BENAPHORE
	int previous = fAvailable.fetch_sub(1);
	if (previous > 0)
		return true;
#endif		

	status_t err = B_NO_ERROR;
	do
	{
		err = acquire_sem(fSemaphore);
	} while (err == B_INTERRUPTED);
	if (err == B_NO_ERROR)
	{
#if !USE_BENAPHORE
		fAvailable.fetch_sub(1);
#endif
		return true;
	}
	else
		return false;
}

/*	FUNCTION:		Semaphore::TryLock
	ARGUMENTS:		milliseconds
					exit_locked
	RETURN:			true if success
	DESCRIPTION:	Try to lock with timeout.
					Cannot use Benaphores since TryLock corrupts count on timeout
*/
const bool Semaphore :: TryLock(const uint64_t milliseconds, const bool exit_locked)
{
	if (exit_locked && (fAvailable <= 0))
		return false;
		
#if USE_BENAPHORE
	int previous = fAvailable.fetch_sub(1);
	if (previous > 0)
		return true;
#endif

	status_t err = acquire_sem_etc(fSemaphore, 1, B_RELATIVE_TIMEOUT, 1000*milliseconds);
	switch (err)
	{
		case B_NO_ERROR:
#if !USE_BENAPHORE
			fAvailable.fetch_sub(1);
#endif
			return true;
		case B_TIMED_OUT:
		case B_INTERRUPTED:
#if USE_BENAPHORE
			fAvailable.fetch_add(1);
#endif
			break;
		default:
			Exit("Zen::Platform::Semaphore::TryLock() error");
	}
	return false;
}

/*	FUNCTION:		Semaphore::Unlock
	ARGUMENTS:		reschudule
	RETURN:			true if success
	DESCRIPTION:	Release semaphore
*/
const bool	Semaphore :: Unlock(const bool reschudule)
{
#if USE_BENAPHORE
	int previous = fAvailable.fetch_add(1);
	if (previous >= 0)
		return true;
#endif

	status_t err;
	if (!reschudule)
		err = release_sem_etc(fSemaphore, 1, B_DO_NOT_RESCHEDULE);
	else
		err = release_sem(fSemaphore);
	if (err == B_NO_ERROR)
	{
#if !USE_BENAPHORE
		fAvailable.fetch_add(1);
#endif
		return true;
	}
	else
	{
		Exit("Zen::Platform::Semaphore::Unlock() error");
		return false;
	}
}

#undef USE_BENAPHORE

/*************************************************
	yarra::Platform::SpinLock
**************************************************/

#define USE_STD_THREAD_YIELD	1		//	benchmarks prefer std::this_thread::yield() compared to _mm_pause()

/*	FUNCTION:		SpinLock :: Lock
	ARGUMENTS:		none
	RETURN:			true if success
	DESCRIPTION:	Acquire spinlock
*/
const bool SpinLock :: Lock()
{
	while (fLocked.test_and_set(std::memory_order_acquire))
	{
#if USE_STD_THREAD_YIELD
		std::this_thread::yield();
#else
		_mm_pause();
#endif
		
	}
	fIsLocked = true;
	return true;
}

/*	FUNCTION:		SpinLock :: TryLock
	ARGUMENTS:		num_cycles
	RETURN:			true if success
	DESCRIPTION:	Acquire spinlock
*/
const bool SpinLock :: TryLock(const int num_cycles)
{
	if (fIsLocked)
		return false;

	int c = 0;
	while (fLocked.test_and_set(std::memory_order_acquire))
	{
		if (++c >= num_cycles)
			return false;
#if USE_STD_THREAD_YIELD
		std::this_thread::yield();
#else
		_mm_pause();
#endif
	}
	fIsLocked = true;
	return true;
}

/*	FUNCTION:		SpinLock :: Unlock
	ARGUMENTS:		none
	RETURN:			true if success
	DESCRIPTION:	Release spinlock
*/
const bool SpinLock :: Unlock()
{
	fLocked.clear(std::memory_order_release);
	fIsLocked = false;
	return true;
}

#undef USE_STD_THREAD_YIELD

/*************************************************
	Zen::Platform::Thread supports platform specific threads
**************************************************/

/*	FUNCTION:		Thread::Thread
	ARGUMENTS:		function
					data
                    thread_name
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Thread :: Thread(int (*thread_function)(void *), void *data, const char *thread_name)
{
	fThreadHandle = spawn_thread((status_t (*)(void *))thread_function, thread_name, B_NORMAL_PRIORITY, data);
}

/*	FUNCTION:		Zen::Platform::Thread::~Thread
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor - kill thread
*/
Thread :: ~Thread()
{
	if (fThreadHandle)
		kill_thread(fThreadHandle);
}

/*	FUNCTION:		Zen::Platform::Thread::Suspend
	ARGUMENTS:		none
	RETURN:			true if successful
	DESCRIPTION:	Suspend a thread
*/
const bool Thread :: Suspend()
{
	suspend_thread(fThreadHandle);
	return true;
}

/*	FUNCTION:		Zen::Platform::Thread::Resume
	ARGUMENTS:		none
	RETURN:			true if successful
	DESCRIPTION:	Resume a thread
*/
const bool Thread :: Resume()
{
	resume_thread(fThreadHandle);
	return true;
}

/*	FUNCTION:		Zen::Platform::Thread::Start
	ARGUMENTS:		none
	RETURN:			true if successful
	DESCRIPTION:	Since APPLE uses pthreads which doesn't properly support
					suspend/resume, we actually create the thread here.
 */
const bool Thread :: Start()
{
	return Resume();
}

/*	FUNCTION:		Zen::Platform::Thread::IsCurrentCallingThread
	ARGUMENTS:		none
	RETURN:			true if calling thread id same as fThreadHandle
	DESCRIPTION:	Check if calling thread id matches fThreadHandle
 */
const bool Thread :: IsCurrentCallingThread() const
{
	return (find_thread(NULL) == fThreadHandle);	
}

/*	FUNCTION:		Zen::Platform::ExitThread
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Platform specific function which should be called before thread exits itself
*/
void ExitThread()
{
}

/*************************************************
	Utility functions
**************************************************/

/*	FUNCTION:		Zen::Platform::AlignedAlloc
	ARGUMENTS:		alignment
					size
	RETURN:			std::aligned_alloc
	DESCRIPTION:	C++17 std::aligned_alloc not available with current CLANG 
*/
void * AlignedAlloc(size_t alignment, size_t size)
{
	void *p;
	if (posix_memalign(&p, alignment, size) != 0)
		p = nullptr;
	return p;
}

/*	FUNCTION:		Zen::Platform::AlignedAlloc
	ARGUMENTS:		ptr
	RETURN:			n/a
	DESCRIPTION:	C++17 std::free not available on VS2015
*/
void AlignedFree(void *ptr)
{
	free(ptr);
}



};	//	namespace Platform
};	//	namespace yarra
