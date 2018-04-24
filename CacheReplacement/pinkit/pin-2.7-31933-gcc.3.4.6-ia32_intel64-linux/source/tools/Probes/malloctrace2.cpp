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
  Similar to Probes/malloctrace.C, but puts replacement functions in the
  application namespace. Works in probed and jitted mode.

  Application functions can no longer be safely called from replacement
  routines via a function pointer in JIT mode.  They must be called
  using PIN_CallApplicationFunction().  Application functions cannot
  be called from a callback in JIT mode at all.

  Therefore, this test case has been restructured to separate Probe
  mode and JIT mode processing.  There is very little common code.
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>


using namespace std;


/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "malloctrace2.outfile", "specify trace file name");

/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This pin tool inserts a user-written version of malloc() and free() into the application.\n"
        "\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}


/* ===================================================================== */
/* Definitions for Probe mode */
/* ===================================================================== */

typedef typeof(malloc) * MallocType;
typedef typeof(free) * FreeType;
typedef typeof(dlopen) * DlopenType;
typedef typeof(dlsym) * DlsymType;

MallocType MallocWrapper = 0;
FreeType FreeWrapper = 0;
void * MallocTraceHandle = 0;

DlopenType AppDlopen = 0;
DlsymType AppDlsym = 0;

/* ===================================================================== */
/* Probe mode tool */
/* ===================================================================== */

VOID ImageLoadProbe(IMG img)
{
    if ( PIN_IsProbeMode() )
    {
        // Application functions can only be called using function pointers
        // in probe mode.  Application function cannot be safely called
        // from a callback in JIT mode.
        //
        if (strstr(IMG_Name(img).c_str(), "libdl.so"))
        {
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

        
            // inject mallocwrappers.so into application by executing application dlopen
            MallocTraceHandle = AppDlopen("mallocwrappers.so", RTLD_LAZY);
            ASSERTX(MallocTraceHandle);
            
            // Get function pointers for the wrappers
            MallocWrapper = MallocType(AppDlsym(MallocTraceHandle, "mallocWrapper"));
            FreeWrapper = FreeType(AppDlsym(MallocTraceHandle, "freeWrapper"));
            ASSERTX(MallocWrapper && FreeWrapper);
        }
    }

    if (strstr(IMG_Name(img).c_str(), "libc.so"))
    {
        // Replace malloc and free in application libc with wrappers in mallocwrappers.so
        RTN mallocRtn = RTN_FindByName(img, "malloc");
        ASSERTX(RTN_Valid(mallocRtn));
        if ( PIN_IsProbeMode() )
        {
            if ( ! RTN_IsSafeForProbedReplacement( mallocRtn ) )
            {
                cout << "Cannot replace malloc in " << IMG_Name(img) << endl;
                exit(1);
            }
        }
        
        AFUNPTR mallocimpl = RTN_ReplaceProbed(mallocRtn, AFUNPTR(MallocWrapper));
        
        // tell mallocwrappers.so how to call original code
        AFUNPTR *mallocptr = (AFUNPTR *)AppDlsym(MallocTraceHandle, "mallocFun");
        ASSERTX(mallocptr);
        *mallocptr = mallocimpl;
        
        RTN freeRtn = RTN_FindByName(img, "free");
        ASSERTX(RTN_Valid(freeRtn));
        if ( PIN_IsProbeMode() )
        {
            if ( ! RTN_IsSafeForProbedReplacement( freeRtn ) )
            {
                cout << "Cannot replace free in " << IMG_Name(img) << endl;
                exit(1);
            }
        }

        AFUNPTR freeimpl = RTN_ReplaceProbed(freeRtn, AFUNPTR(FreeWrapper));

        AFUNPTR *freeptr = (AFUNPTR *)AppDlsym(MallocTraceHandle, "freeFun");
        ASSERTX(freeptr);
        *freeptr = freeimpl;
    }
}

                    
/* ===================================================================== */
/* Replacement routines for JIT mode */
/* ===================================================================== */

void * MallocJitWrapper( CONTEXT * ctxt, AFUNPTR pf_malloc, size_t size)
{
    void * res;

    fprintf(stderr,"Calling malloc(%d)\n", (int)size);

    PIN_CallApplicationFunction( ctxt, PIN_ThreadId(),
                                 CALLINGSTD_DEFAULT, pf_malloc,
                                 PIN_PARG(void *), &res,
                                 PIN_PARG(size_t), size,
                                 PIN_PARG_END() );
    
    
    fprintf(stderr,"malloc(%d) = %p\n", (int)size, res);

    return res;
}

void FreeJitWrapper(CONTEXT * ctxt, AFUNPTR pf_free, void *p)
{
    fprintf(stderr,"Calling free(%p)\n",p);

    PIN_CallApplicationFunction( ctxt, PIN_ThreadId(),
                                 CALLINGSTD_DEFAULT, pf_free,
                                 PIN_PARG(void),
                                 PIN_PARG(void *), p,
                                 PIN_PARG_END() );
    
    fprintf(stderr,"free(%p)\n",p);
}

/* ===================================================================== */
// Called every time a new image is loaded
// Look for routines that we want to probe
VOID ImageLoad(IMG img, VOID *v)
{
    if ( PIN_IsProbeMode() )
    {
        ImageLoadProbe( img );
    }
    else
    {
        if (strstr(IMG_Name(img).c_str(), "libc.so"))
        {
            // Replace malloc and free in application libc with wrappers in mallocwrappers.so
            RTN mallocRtn = RTN_FindByName(img, "malloc");
            ASSERTX(RTN_Valid(mallocRtn));
            
            PROTO protoMalloc = PROTO_Allocate( PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                                "malloc", PIN_PARG(size_t), PIN_PARG_END() );
            
            RTN_ReplaceSignature(mallocRtn, AFUNPTR(MallocJitWrapper),
                                 IARG_PROTOTYPE, protoMalloc,
                                 IARG_CONTEXT,
                                 IARG_ORIG_FUNCPTR,
                                 IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                                 IARG_END);
            
            
            RTN freeRtn = RTN_FindByName(img, "free");
            ASSERTX(RTN_Valid(freeRtn));
            
            PROTO protoFree = PROTO_Allocate( PIN_PARG(void), CALLINGSTD_DEFAULT,
                                              "free", PIN_PARG(void *), PIN_PARG_END() );
            
            RTN_ReplaceSignature(freeRtn, AFUNPTR(FreeJitWrapper),
                                 IARG_PROTOTYPE, protoFree,
                                 IARG_CONTEXT,
                                 IARG_ORIG_FUNCPTR,
                                 IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                                 IARG_END);
        }
    }
}




/* ===================================================================== */
/* main */
/* ===================================================================== */

int main(int argc, CHAR *argv[])
{
    PIN_InitSymbols();
    
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    
    IMG_AddInstrumentFunction(ImageLoad, 0);
    
    
    if (PIN_IsProbeMode())
    {
        PIN_StartProgramProbed();
    }
    else
    {
        PIN_StartProgram();
    }
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
