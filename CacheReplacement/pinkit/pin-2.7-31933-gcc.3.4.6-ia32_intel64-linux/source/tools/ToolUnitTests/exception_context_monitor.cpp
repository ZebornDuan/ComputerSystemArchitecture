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
 *  A pin tool that intercepts exception and verifies the exception context.
 *  It works in pair with the win_exception_context application, that raises two 
 *  exceptions. The context of the second exception should have a predefined FP 
 *  state: all FPn and XMMn registers have value <n> in their first byte. 
 *  This is verified by the tool.
 */

#include <stdio.h>
#include <string>
#include <iostream>
#include <memory.h>
#include "pin.H"

using namespace std;


/*
 * Verify the FP state of the exception context
 */
KNOB<BOOL> KnobCheckFp(KNOB_MODE_WRITEONCE, "pintool", "checkfp", "0", "Check FP state");


/*
 * The memory layout written by FXSAVE and read by FXRSTOR.
 */

#if defined(TARGET_IA32)
struct FXSAVE
{
    UINT16 _fcw;
    UINT16 _fsw;
    UINT8  _ftw;
    UINT8  _pad1;
    UINT16 _fop;
    UINT32 _fpuip;
    UINT16 _cs;
    UINT16 _pad2;
    UINT32 _fpudp;
    UINT16 _ds;
    UINT16 _pad3;
    UINT32 _mxcsr;
    UINT32 _mxcsrmask;
    UINT8  _st[8 * 16];
    UINT8  _xmm[8 * 16];
    UINT8  _pad4[56 * 4];
};

#elif defined(TARGET_IA32E)

struct FXSAVE
{
    UINT16 _fcw;
    UINT16 _fsw;
    UINT8  _ftw;
    UINT8  _pad1;
    UINT16 _fop;
    UINT32 _fpuip;
    UINT16 _cs;
    UINT16 _pad2;
    UINT32 _fpudp;
    UINT16 _ds;
    UINT16 _pad3;
    UINT32 _mxcsr;
    UINT32 _mxcsrmask;
    UINT8  _st[8 * 16];
    UINT8  _xmm[16 * 16];
    UINT8  _pad4[24 * 4];
};

#endif

/*!
 * Exit with the specified error message
 */
static void Abort(const char * msg)
{
    cerr << msg << endl;
    exit(1);
}

/*!
 * Check to see that FP/XMM registers in the specified context have predefined
 * values assigned by the application: FPn and XMMn registers have value <n> in 
 * their first byte. 
 */
static bool CheckMyFpContext(const CONTEXT * pContext)
{
    FXSAVE fpState;

    PIN_GetContextFPState(pContext, &fpState);

    for (size_t i = 0; i < sizeof(fpState._st) ; ++i)
    {
        UINT8 regId = i/16;
        if ((i%16 == 0) && (fpState._st[i] != regId))
        {
            return false;
        }
        if ((i%16 != 0) && (fpState._st[i] != 0))
        {
            return false;
        }
    }

    for (size_t i = 0; i < sizeof(fpState._xmm); ++i)
    {
        UINT8 regId = i/16;
        if ((i%16 == 0) && (fpState._xmm[i] != regId))
        {
            return false;
        }
        if ((i%16 != 0) && (fpState._xmm[i] != 0))
        {
            return false;
        }
    }

    return true;
}

static void OnException(THREADID threadIndex, 
                  CONTEXT_CHANGE_REASON reason, 
                  const CONTEXT *ctxtFrom,
                  CONTEXT *ctxtTo,
                  INT32 info, 
                  VOID *v)
{
    if (reason == CONTEXT_CHANGE_REASON_EXCEPTION)
    {
        static bool first = true;

        if (first)
        {
            first = false;
        }
        else
        {
            if (KnobCheckFp && !CheckMyFpContext(ctxtFrom))
            {
                Abort("Tool: Mismatch in the FP context");
            }
        }
    }
}


int main(INT32 argc, CHAR **argv)
{
    if (PIN_Init(argc, argv))
    {
        Abort("Tool: Invalid arguments");
    }

    PIN_AddContextChangeFunction(OnException, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
