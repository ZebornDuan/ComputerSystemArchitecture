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
/* ===================================================================== */
/*
  @ORIGINAL_AUTHOR: Elena Demikhovsky
*/

/* ===================================================================== */
/*! @file
 * A test for callbacks around fork in jit mode.
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "pin.H"

#include <iostream>
#include <fstream>

using namespace std;
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "fork_jit_tool.out", "specify file name");

ofstream Out;
/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This pin tool tests probe replacement.\n"
        "\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}


pid_t parent_pid;
PIN_LOCK lock;

VOID BeforeFork(THREADID threadid, const CONTEXT* ctxt, VOID * arg)
{
    GetLock(&lock, threadid+1);
    Out << "TOOL: Before fork." << endl;
    ReleaseLock(&lock);
    parent_pid = PIN_GetPid();
}
VOID AfterForkInParent(THREADID threadid, const CONTEXT* ctxt, VOID * arg)
{
    GetLock(&lock, threadid+1);
    Out << "TOOL: After fork in parent." << endl;
    ReleaseLock(&lock);
    if (PIN_GetPid() != parent_pid)
    {
    	cerr << "PIN_GetPid() fails in parent process" << endl;
		exit(-1);
    }
    else
    {
    	Out << "PIN_GetPid() is correct in parent process" << endl;
    }    
}
VOID AfterForkInChild(THREADID threadid, const CONTEXT* ctxt, VOID * arg)
{

    // After the fork, there is only one thread in the child process.  It's possible
    // that a different thread in the parent held this lock when the fork() happened.
    // Since that thread does not exist in the child, it will never release the lock.
    // Compensate by re-initializing the lock here in the child.

    InitLock(&lock);
    GetLock(&lock, threadid+1);
    ReleaseLock(&lock);

    ofstream childOut;

    char *outFileName = new char[KnobOutputFile.Value().size()+10];
    sprintf(outFileName, "%s_%d", KnobOutputFile.Value().c_str(), PIN_GetPid());
    childOut.open(outFileName);
    childOut << "TOOL: After fork in child." << endl;
    
    if ((PIN_GetPid() == parent_pid) || (getppid() != parent_pid))
    {
		cerr << "PIN_GetPid() fails in child process" << endl;
		exit(-1);
    }
    else
    {
    	childOut << "PIN_GetPid() is correct in child process" << endl;
    }    
}



int main(INT32 argc, CHAR **argv)
{
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
	
    Out.open(KnobOutputFile.Value().c_str());
    
    PIN_AddForkFunction(FPOINT_BEFORE, BeforeFork, 0);	
    PIN_AddForkFunction(FPOINT_AFTER_IN_PARENT, AfterForkInParent, 0);
	PIN_AddForkFunction(FPOINT_AFTER_IN_CHILD, AfterForkInChild, 0);
	
    // Never returns
    PIN_StartProgram();
    
    return 0;
}
