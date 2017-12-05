#include "tests.h"

#include "zthread.h"
#include "zmutex.h"
#include "zlock.h"

#if LIBCHAOS_PLATFORM == _PLATFORM_WINDOWS
    #include <windows.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <assert.h>
    #include <iostream>
#endif

#define RET_MAGIC 0x5a5a5a5a

namespace LibChaosTest {

void *thread_func(void * /*zarg*/){
    //ZThreadArg *arg = (ZThreadArg*)zarg;
    LOG("running " << ZThread::thisTid());
    ZThread::sleep(2);
    LOG("waited 2 " << ZThread::thisTid());
    return NULL;
}

void *thread_func2(ZThread::ZThreadArg zarg){
    void *arg = zarg.arg;
    LOG("running " << ZThread::thisTid());
    LOG((const char *)arg << ", " << zarg.stop());
    int i = 0;
    while(!zarg.stop()){
        LOG("loop" << ++i << " in " << ZThread::thisTid());
        ZThread::usleep(1000000);
    }
    LOG("broke loop " << ZThread::thisTid());
    return (void *)RET_MAGIC;
}

void thread(){
    LOG("=== Thread Test...");
    /*
    LOG("this text " << ZThread::thisTid());
    ZThread thr(thread_func);
    LOG("thread " << thr.tid() << " created");
    sleep(1);
    LOG("waited 1 " << ZThread::thisTid());
    thr.kill();
    LOG("killed " << thr.tid());
    */

    ZString txt = "hello there from here";
    ZThread thr2(thread_func2);
    thr2.exec(txt.c());
    LOG("thread " << thr2.tid() << " created");
    ZThread::sleep(5);
    LOG("waited 5 " << ZThread::thisTid());
    thr2.stop();
    LOG("stopped " << thr2.tid());
    void *ret = thr2.join();
    LOG("joined " << thr2.tid());
    TASSERT((zu64)ret == RET_MAGIC);
}

#if LIBCHAOS_PLATFORM == _PLATFORM_WINDOWS

ZMutex gmutex;
CRITICAL_SECTION gCS; // shared structure

const int gcMaxCount = 10;
volatile int gCount = 0;

DWORD __attribute__((__stdcall__)) threadLoop(void *name){
    while(true){
        TLOG((char *)name << " entering critical Section...");
//        EnterCriticalSection(&gCS);
//        mutex.lock();
        ZLock lock(gmutex);
        if(gCount < gcMaxCount){
            TLOG((char *)name << " in critical Section");
            gCount++;
        } else {
//            LeaveCriticalSection(&gCS);
//            mutex.unlock();
            break;
        }
//        LeaveCriticalSection(&gCS);
//        mutex.unlock();
        TLOG((char *)name << " left critical Section");
    }
    return 0;
}

HANDLE CreateChild(const char *name){
    HANDLE hThread; DWORD dwId;
    hThread = CreateThread(NULL, 0, threadLoop, (LPVOID)const_cast<char*>(name), 0, &dwId);
    assert(hThread != NULL);
    return hThread;
}

void mutex(){
    HANDLE hT[4];
    InitializeCriticalSection(&gCS);

    TLOG("Starting...");
//    Sleep(200);

    // Create multiple child threads
    hT[0] = CreateChild("Evelyn");
    hT[1] = CreateChild("Bodie");
    hT[2] = CreateChild("Rebecca");
    hT[3] = CreateChild("Jeff");

    WaitForMultipleObjects(4, hT, TRUE, INFINITE);
    TLOG("Completed!");

    CloseHandle(hT[0]);
    CloseHandle(hT[1]);
    CloseHandle(hT[2]);
    CloseHandle(hT[3]);

    DeleteCriticalSection(&gCS);
}

#else

void mutex(){

}

#endif

ZArray<Test> thread_tests(){
    return {
        { "thread", thread, false, {} },
        { "mutex",  mutex,  false, {} },
    };
}

}
