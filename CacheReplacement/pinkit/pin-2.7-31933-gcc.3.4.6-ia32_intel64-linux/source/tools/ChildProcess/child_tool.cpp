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
#include "pin.H"
#include <iostream>
using namespace std;

/* ===================================================================== */
/* Command line Switches */
/* ===================================================================== */

//General configuration - Pin and Tool full path
KNOB<string> KnobPinFullPath(KNOB_MODE_WRITEONCE,         "pintool",
                             "pin_path", "", "pin full path");

KNOB<string> KnobToolsFullPath(KNOB_MODE_WRITEONCE,         "pintool",
                               "tools_path", "", "grand parent tool full path");


//Parent configuration - Application and PinTool name
KNOB<string> KnobParentApplicationName(KNOB_MODE_WRITEONCE,         "pintool",
                                       "parent_app_name", "win_parent_process", "parent application name");

KNOB<string> KnobParentToolName(KNOB_MODE_WRITEONCE,         "pintool",
                                "parent_tool_name", "parent_tool", "parent tool full path");


//Child configuration - Application and PinTool name
KNOB<string> KnobChildApplicationName(KNOB_MODE_WRITEONCE,         "pintool",
                                      "child_app_name", "win_child_process", "child application name");

KNOB<string> KnobChildToolName(KNOB_MODE_WRITEONCE,         "pintool",
                               "child_tool_name", "child_tool", "child tool full path");


/* ===================================================================== */
VOID Fini(INT32 code, VOID *v)
{
    cout << "In child_tool PinTool" << endl;
}

/* ===================================================================== */

int main(INT32 argc, CHAR **argv)
{
    PIN_Init(argc, argv);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
