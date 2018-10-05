// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifdef __sun
#    define _POSIX_PTHREAD_SEMANTICS
#endif

#include <DataStorm/CtrlCHandler.h>
#include <IceUtil/Mutex.h>

#ifndef _WIN32
#   include <signal.h>
#else
#   include <IceUtil/Cond.h>
#endif

using namespace std;
using namespace DataStorm;

namespace
{

CtrlCHandlerCallback _callback = ICE_NULLPTR;

const CtrlCHandler* _handler = 0;

IceUtil::Mutex* globalMutex = 0;

class Init
{
public:

    Init()
    {
        globalMutex = new IceUtil::Mutex;
    }

    ~Init()
    {
        delete globalMutex;
        globalMutex = 0;
    }
};

Init init;

}

CtrlCHandlerException::CtrlCHandlerException(const char* file, int line) :
    ExceptionHelper<CtrlCHandlerException>(file, line)
{
}

string
CtrlCHandlerException::ice_id() const
{
    return "::IceUtil::CtrlCHandlerException";
}

#ifndef ICE_CPP11_MAPPING
CtrlCHandlerException*
CtrlCHandlerException::ice_clone() const
{
    return new CtrlCHandlerException(*this);
}
#endif

CtrlCHandlerCallback
CtrlCHandler::setCallback(CtrlCHandlerCallback callback)
{
    IceUtil::Mutex::Lock lock(*globalMutex);
    CtrlCHandlerCallback oldCallback = _callback;
    _callback = callback;
    return oldCallback;
}

CtrlCHandlerCallback
CtrlCHandler::getCallback() const
{
    IceUtil::Mutex::Lock lock(*globalMutex);
    return _callback;
}

#ifdef _WIN32

namespace
{

IceUtil::Cond* globalCond = 0;
int inProgress = 0;

}

static BOOL WINAPI handlerRoutine(DWORD dwCtrlType)
{
    CtrlCHandlerCallback callback;
    {
        IceUtil::Mutex::Lock lock(*globalMutex);
        if(!_handler) // The handler is destroyed.
        {
            return FALSE;
        }
        callback = _callback;
        ++inProgress;
    }
    if(callback)
    {
        callback(dwCtrlType);
    }
    {
        IceUtil::Mutex::Lock lock(*globalMutex);
        --inProgress;
        if(globalCond)
        {
            globalCond->signal();
        }
    }
    return TRUE;
}

void
CtrlCHandler::init()
{
    // Nothinkg to do
}

CtrlCHandler::CtrlCHandler(CtrlCHandlerCallback callback)
{
    IceUtil::Mutex::Lock lock(*globalMutex);
    bool handler = _handler != 0;

    if(handler)
    {
        throw CtrlCHandlerException(__FILE__, __LINE__);
    }
    else
    {
        _callback = callback;
        _handler = this;
        lock.release();

        SetConsoleCtrlHandler(handlerRoutine, TRUE);
    }
}

CtrlCHandler::~CtrlCHandler()
{
    SetConsoleCtrlHandler(handlerRoutine, FALSE);
    {
        IceUtil::Mutex::Lock lock(*globalMutex);
        _handler = 0;
        _callback = ICE_NULLPTR;
        if(inProgress > 0)
        {
            globalCond = new IceUtil::Cond();
            while(inProgress > 0)
            {
                globalCond->wait(lock);
            }
            delete globalCond;
            globalCond = 0;
        }
    }
}

#else

extern "C"
{

static void*
sigwaitThread(void*)
{
    sigset_t ctrlCLikeSignals;
    sigemptyset(&ctrlCLikeSignals);
    sigaddset(&ctrlCLikeSignals, SIGHUP);
    sigaddset(&ctrlCLikeSignals, SIGINT);
    sigaddset(&ctrlCLikeSignals, SIGTERM);

    //
    // Run until the handler is destroyed (_handler == 0)
    //
    for(;;)
    {
        int signal = 0;
        int rc = sigwait(&ctrlCLikeSignals, &signal);
        if(rc == EINTR)
        {
            //
            // Some sigwait() implementations incorrectly return EINTR
            // when interrupted by an unblocked caught signal
            //
            continue;
        }
        assert(rc == 0);

        CtrlCHandlerCallback callback;
        {
            IceUtil::Mutex::Lock lock(*globalMutex);
            if(!_handler) // The handler is destroyed.
            {
                return 0;
            }
            callback = _callback;
        }

        if(callback)
        {
            callback(signal);
        }
    }
    return 0;
}

}

namespace
{

pthread_t _tid;

}

void
CtrlCHandler::init()
{
    // We block these CTRL+C like signals in the main thread,
    // and by default all other threads will inherit this signal
    // mask.

    sigset_t ctrlCLikeSignals;
    sigemptyset(&ctrlCLikeSignals);
    sigaddset(&ctrlCLikeSignals, SIGHUP);
    sigaddset(&ctrlCLikeSignals, SIGINT);
    sigaddset(&ctrlCLikeSignals, SIGTERM);

#ifndef NDEBUG
    int rc = pthread_sigmask(SIG_BLOCK, &ctrlCLikeSignals, 0);
    assert(rc == 0);
#else
    pthread_sigmask(SIG_BLOCK, &ctrlCLikeSignals, 0);
#endif
}

CtrlCHandler::CtrlCHandler(CtrlCHandlerCallback callback)
{
    IceUtil::Mutex::Lock lock(*globalMutex);
    bool handler = _handler != 0;

    if(handler)
    {
        throw CtrlCHandlerException(__FILE__, __LINE__);
    }
    else
    {
        _callback = callback;
        _handler = this;

        lock.release();

        // Joinable thread
#ifndef NDEBUG
        int rc = pthread_create(&_tid, 0, sigwaitThread, 0);
        assert(rc == 0);
#else
        pthread_create(&_tid, 0, sigwaitThread, 0);
#endif
    }
}

CtrlCHandler::~CtrlCHandler()
{
    //
    // Clear the handler, the sigwaitThread will exit if _handler is null
    //
    {
        IceUtil::Mutex::Lock lock(*globalMutex);
        _handler = 0;
        _callback = ICE_NULLPTR;
    }

    //
    // Signal the sigwaitThread and join it.
    //
    void* status = 0;
#ifndef NDEBUG
    int rc = pthread_kill(_tid, SIGTERM);
    assert(rc == 0);
    rc = pthread_join(_tid, &status);
    assert(rc == 0);
#else
    pthread_kill(_tid, SIGTERM);
    pthread_join(_tid, &status);
#endif
}

#endif