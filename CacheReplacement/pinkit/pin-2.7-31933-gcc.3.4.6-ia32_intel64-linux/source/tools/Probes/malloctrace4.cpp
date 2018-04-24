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
  @ORIGINAL_AUTHOR: Robert Cohn
*/

/* ===================================================================== */
/*! @file
  Similar to Probes/malloctrace2.C.   Works in probed and jitted mode.
  Works only on the IA-64 architecture!
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <string.h>


namespace LEVEL_PINCLIENT
{
extern ADDRINT PIN_GetAppTP();
//extern AFUNPTR RTN_Replace(RTN replacedRtn, AFUNPTR replacementFun);
}

    
using namespace std;

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */


ofstream TraceFile;

// The tool thread pointer, and the application thread pointer.
ADDRINT appTP = 0x0;
ADDRINT toolTP = 0x0;


/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "malloctrace4.outfile", "specify trace file name");

/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This pin tool collects an instruction trace for debugging\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}


typedef typeof(malloc) * MallocType;
typedef typeof(free) * FreeType;
typedef typeof(dlopen) * DlopenType;
typedef typeof(dlsym) * DlsymType;

MallocType MallocWrapper = 0;
FreeType FreeWrapper = 0;
void * MallocTraceHandle = 0;

DlopenType AppDlopen = 0;
DlsymType AppDlsym = 0;

RTN freeRtn;
RTN mallocRtn;

ADDRINT mainImgEntry = 0;

/* ===================================================================== */
// Called every time a new image is loaded
// Look for routines that we want to probe
VOID ReplaceRtns(ADDRINT appTP)
{
    // Get the tool thread pointer.
    toolTP = IPF_GetTP();

    // inject mallocwrappers.so into application by executing application dlopen
    IPF_SetTP( appTP );
    MallocTraceHandle = AppDlopen("mallocwrappers.so", RTLD_LAZY);
    IPF_SetTP( toolTP );
    ASSERTX(MallocTraceHandle);
            
    // Get function pointers for the wrappers
    IPF_SetTP( appTP );
    MallocWrapper = MallocType(AppDlsym(MallocTraceHandle, "mallocWrapper"));
    FreeWrapper = FreeType(AppDlsym(MallocTraceHandle, "freeWrapper"));
    IPF_SetTP( appTP );
    
    ASSERTX(MallocWrapper && FreeWrapper);

    if ( PIN_IsProbeMode() )
    {
        if ( ! RTN_IsSafeForProbedReplacement( mallocRtn ) )
        {
            cout << "Cannot replace malloc in " << endl;
            exit(1);
        }
    }

    AFUNPTR mallocimpl =
        PIN_IsProbeMode() ?
        RTN_ReplaceProbed(mallocRtn, AFUNPTR(MallocWrapper)) :
        RTN_Replace(mallocRtn, AFUNPTR(MallocWrapper));


    // tell mallocwrappers.so how to call original code
    IPF_SetTP( appTP );
    AFUNPTR *mallocptr = (AFUNPTR *)AppDlsym(MallocTraceHandle, "mallocFun");
    IPF_SetTP( toolTP );
    ASSERTX(mallocptr);
    *mallocptr = mallocimpl;

    if ( PIN_IsProbeMode() )
    {
        if ( ! RTN_IsSafeForProbedReplacement( freeRtn ) )
        {
            cout << "Cannot replace free" << endl;
            exit(1);
        }
    }

    AFUNPTR freeimpl =
        PIN_IsProbeMode() ?
        RTN_ReplaceProbed(freeRtn, AFUNPTR(FreeWrapper)) :
        RTN_Replace(freeRtn, AFUNPTR(FreeWrapper));

    IPF_SetTP( appTP );
    AFUNPTR *freeptr = (AFUNPTR *)AppDlsym(MallocTraceHandle, "freeFun");
    IPF_SetTP( toolTP );
    ASSERTX(freeptr);
    *freeptr = freeimpl;
}

/* ===================================================================== */
// Called every time a new image is loaded
// Look for routines that we want to probe
VOID ImageLoad(IMG img, VOID *v)
{
    cout << "Processing " << IMG_Name(img) << endl;
    
    if (IMG_IsMainExecutable(img))
    {
        mainImgEntry = IMG_Entry(img);
    }
    
    if (strstr(IMG_Name(img).c_str(), "libdl.so"))
    {
        TraceFile << "Found libdl.so " << IMG_Name(img) << endl;

        // Get the function pointer for the application dlopen:
        // dlopen@@GLIBC_2.1 is the official, versioned name.
        // 
        // The exact suffix must match the ABI of the libdl header files
        // this source code gets compiled against. Makefile/configure
        // trickery would be needed to figure this suffix out, so it
        // is simply hard-coded here.
        //
        // To keep the resulting binaries compatible with future libdl.so
        // versions, this code also checks for backwards compatibility
        // versions of the calls as they would be provided in such a
        // future version.

#if defined(TARGET_IA32E)
# define DLOPEN_VERSION "GLIBC_2.2.5"
# define DLSYM_VERSION "GLIBC_2.2.5"
#elif defined(TARGET_IPF)
# define DLOPEN_VERSION "GLIBC_2.1"
# define DLSYM_VERSION "GLIBC_2.0"
#elif defined(TARGET_IA32)
# define DLOPEN_VERSION "GLIBC_2.1"
# define DLSYM_VERSION "GLIBC_2.0"
#else
# error symbol versions unknown for this target
#endif

        RTN dlopenRtn = RTN_FindByName(img, "dlopen@@" DLOPEN_VERSION);
        if (!RTN_Valid(dlopenRtn)) {
            dlopenRtn = RTN_FindByName(img, "dlopen@" DLOPEN_VERSION);
        }
        if (!RTN_Valid(dlopenRtn)) {
            // fallback because with -use_dynsym symbols do not have a version
            dlopenRtn = RTN_FindByName(img, "dlopen");
        }
        ASSERTX(RTN_Valid(dlopenRtn));
        AppDlopen = DlopenType(RTN_Funptr(dlopenRtn));
    
        // Get the function pointer for the application dlsym
        RTN dlsymRtn = RTN_FindByName(img, "dlsym@@" DLSYM_VERSION);
        if (!RTN_Valid(dlsymRtn)) {
            dlsymRtn = RTN_FindByName(img, "dlsym@" DLSYM_VERSION);
        }
        if (!RTN_Valid(dlsymRtn)) {
            // fallback because with -use_dynsym symbols do not have a version
            dlsymRtn = RTN_FindByName(img, "dlsym");
        }
        ASSERTX(RTN_Valid(dlsymRtn));
        AppDlsym = DlsymType(RTN_Funptr(dlsymRtn));
    }
    
    if (strstr(IMG_Name(img).c_str(), "libc.so"))
    {
        TraceFile << "Found libc.so " << IMG_Name(img) << endl;

        // Replace malloc and free in application libc with wrappers in mallocwrappers.so
        mallocRtn = RTN_FindByName(img, "malloc");
        ASSERTX(RTN_Valid(mallocRtn));

        freeRtn = RTN_FindByName(img, "free");
        ASSERTX(RTN_Valid(freeRtn));

        if (PIN_IsProbeMode())
        {
            appTP = PIN_GetAppTP();
            ReplaceRtns(appTP);
        }
    }
}


VOID Trace(TRACE trace, VOID * value)
{
    if (TRACE_Address(trace) == mainImgEntry)
    {
        TRACE_InsertCall(trace, IPOINT_BEFORE, AFUNPTR(ReplaceRtns), IARG_REG_VALUE, REG_TP, IARG_END);
    }
}

/* ===================================================================== */

int main(int argc, CHAR *argv[])
{
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    TraceFile.open(KnobOutputFile.Value().c_str());
    TraceFile << hex;
    TraceFile.setf(ios::showbase);

    IMG_AddInstrumentFunction(ImageLoad, 0);
    
    if (PIN_IsProbeMode()) {
        PIN_StartProgramProbed();
    } else {
        TRACE_AddInstrumentFunction(Trace, 0);
        PIN_StartProgram();
    }
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
