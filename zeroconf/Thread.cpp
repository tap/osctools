/*
	Copyright (c) 2005 RÈmy Muller. 
	
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Thread.h"
#include <cassert>

#ifdef WIN32

static void *threadEntryPoint(void *pUser)
{
	assert(0);
	return NULL;
}

static void *createThread(void *pUser)
{
	assert(0);
	return NULL;
}

static void killThread(void *pHandle)
{
	assert(0);
}

static void threadSleep(int ms)
{
	assert(0);
}

static void closeThread(void *pHandle)
{
	assert(0);
}

#else

#include "pthread.h"

static void *threadEntryPoint(void *pUser)
{
	//const ScopedAutoReleasePool pool;
	Thread::entryPoint((Thread*)pUser);
	return NULL;
}

static void *createThread(void *pUser)
{
    pthread_t handle = 0;
	
    if (pthread_create (&handle, 0, threadEntryPoint, pUser) == 0)
    {
        pthread_detach (handle);
        return (void*) handle;
    }
	
    return 0;
}

static void killThread(void *pHandle)
{
    if (pHandle != NULL)
        pthread_cancel ((pthread_t) pHandle);
}

static void threadSleep(int ms)
{
    struct timespec time;
    time.tv_sec = ms / 1000;
    time.tv_nsec = (ms % 1000) * 1000000;
    nanosleep (&time, 0);
}

static void closeThread(void *pHandle)
{
	// nothing to do on posix
}

#endif

Thread::Thread()
: mpThreadHandle(NULL)
, mShouldExit(false)
{
}

Thread::~Thread()
{
	stop(100);
}

void Thread::start()
{
	const ScopedLock lock(mCriticalSection);
	
	mShouldExit = false;
	if(mpThreadHandle == NULL)
	{
		mpThreadHandle = createThread(this);
	}
}

void Thread::stop(const int timeOut)
{
	const ScopedLock lock(mCriticalSection);
	
	if(isRunning())
	{
		setShouldExit();
		// notify
		
		if(timeOut != 0)
			waitForThreadToExit(timeOut);
		
		if(isRunning())
		{
			killThread(mpThreadHandle);
			mpThreadHandle = NULL;
		}
	}
}


bool Thread::isRunning() const
{
	return mpThreadHandle != NULL;
}

bool Thread::shouldExit() const
{
	return mShouldExit;
}

void Thread::setShouldExit()
{
	mShouldExit = true;
}

bool Thread::waitForThreadToExit(const int timeOut)
{
	int count = timeOut;
	
	while (isRunning()) {
		if(timeOut >0 && --count < 0)
			return false;
			
		sleep(1);
	}
	
	return true;
}

void Thread::sleep(int ms)
{
	threadSleep(ms);
}


void Thread::entryPoint(Thread *pThread)
{
	pThread->run();
	
	closeThread(pThread->mpThreadHandle);
	
	pThread->mpThreadHandle = NULL;
}