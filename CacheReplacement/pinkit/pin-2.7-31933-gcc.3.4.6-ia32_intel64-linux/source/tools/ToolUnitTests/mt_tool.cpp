/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2009 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*! @file
 * A tool that creates its own native threads and verifies that pin 
 * does not instrument them. Also, the tool verifies that PIN_SafeCopy
 * works correctly in native threads.
 */

#include "pin.H"
#include <string>
#include <iostream>

using namespace std;

#if defined(TARGET_WINDOWS)

namespace WINDOWS
{
#include <windows.h>
};

#else

#include <sched.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#endif

void DelayThread(unsigned int millisec)
{
#if defined(TARGET_WINDOWS)
    WINDOWS::Sleep(millisec);
#else
    usleep(millisec*1000);
#endif
}

PIN_LOCK lock;
int appThreadsStarted = 0;
int appThreadsFinished = 0;
int toolThreadsCreated = 0;
int toolThreadsFinished = 0;
int toolThreadsStarted = 0;

/*!
 * Convert a (function) pointer to ADDRINT.
 */
template <typename PF> ADDRINT Ptr2Addrint(PF pf)
{
    union CAST
    {
        PF pf;
        ADDRINT addr;
    } cast;
    cast.pf = pf;
    return cast.addr;
}

/*!
 * Starting procedure (OS_THREAD_FUNC) of the tool native thread.
 */
static VOID ThreadProc(VOID * arg)
{
    OS_THREAD_ID myTid = PIN_GetTid();

    GetLock(&lock, myTid);
    toolThreadsStarted++;
    cerr << "Native thread started running, tid = " << myTid << endl;
    ReleaseLock(&lock);


    // Exercise PIN_SafeCopy
    char buffer[16];
    size_t size = PIN_SafeCopy(buffer, NULL, sizeof(buffer));
    if (size != 0)
    {
        cerr << "PIN_SafeCopy failed, tid = " << myTid << endl;
    }

    GetLock(&lock, myTid);
    toolThreadsFinished++;
    cerr << "Native thread finished, tid = " << myTid << endl;
    ReleaseLock(&lock);
    return; 
}


/*!
 * Trace instrumentation routine.
 */
static VOID Trace(TRACE trace, VOID *v)
{
    OS_THREAD_ID myTid = PIN_GetTid();
    if (TRACE_Address(trace) == Ptr2Addrint(ThreadProc))
    {
        GetLock(&lock, myTid);
        cerr << "Pin attempts to instrument tool thread, tid = " << myTid << endl;
        ReleaseLock(&lock);
        exit(1);
    }
}

/*!
 * Callbacks.
 */
static VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    OS_THREAD_ID myTid = PIN_GetTid();

    GetLock(&lock, myTid);
    appThreadsStarted++;
    cerr << "Application thread started running, tid = " << myTid << endl;
    ReleaseLock(&lock);
}

static VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    OS_THREAD_ID myTid = PIN_GetTid();

    GetLock(&lock, myTid);
    appThreadsFinished++;
    cerr << "Application thread finished, tid = " << myTid << endl;
    ReleaseLock(&lock);
}

static VOID Fini(INT32 code, VOID *v)
{
    cerr << "Number of application threads started: "  << appThreadsStarted  << endl;
    cerr << "Number of application threads finished: " << appThreadsFinished << endl;
    cerr << "Number of tool threads created: "  << toolThreadsCreated << endl;
    cerr << "Number of tool threads started: " << toolThreadsStarted << endl;
    cerr << "Number of tool threads finished: " << toolThreadsFinished << endl;
    if (toolThreadsCreated != toolThreadsStarted)
    {
        cerr << (toolThreadsCreated - toolThreadsStarted) << " tool threads has not started" << endl;
        exit(1);
    }
    if (toolThreadsCreated != toolThreadsFinished)
    {
        cerr << (toolThreadsCreated - toolThreadsFinished) << " tool threads has not finished" << endl;
        exit(1);
    }
}

/*!
 * The main procedure of the tool.
 */
int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);
    InitLock(&lock);

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
    TRACE_AddInstrumentFunction(Trace, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Spawn native threads
    OS_THREAD_ID myTid = PIN_GetTid();
    const int numThreads = 4;
    for (int i = 0; i < numThreads; i++)
    {
        OS_THREAD_ID sysId = PIN_SpawnNativeThread(ThreadProc, 0, 0);
        if (sysId == INVALID_OS_THREAD_ID)
        {
            GetLock(&lock, myTid);
            cerr << "PIN_SpawnNativeThread failed" << endl;
            ReleaseLock(&lock);
            exit(1);
        }

        GetLock(&lock, myTid);
        toolThreadsCreated++;
        cerr << "Tool spawned a native thread, tid = " << sysId << endl;
        ReleaseLock(&lock);
    }

    // Never returns
    PIN_StartProgram();
    return 0;
}
