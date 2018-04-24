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
/*
 * This is a test application for the extended debugger commands in the
 * "debugger-shell" instrumenation library.  Since those commands are
 * non-symbolic, the input commands must reference raw addresses in this
 * application not symbol names.  It would be difficult to keep the
 * addresses in the input commands in sync with the addresses in this
 * application, so the application itself prints out the debugger commands.
 * To run this test, we run the application twice.  The first run generates
 * the debugger command script, and the second run executes under the debugger.
 */

#include <iostream>
#include <fstream>
#include <cstring>

#if defined(_MSC_VER)
    typedef unsigned __int64 ADDR;
#elif defined(__GNUC__)
    #include <stdint.h>
    typedef uint64_t ADDR;
#endif

#if defined(TARGET_IA32)
#   define REGAX "$eax"
#elif defined(TARGET_IA32E)
#   define REGAX "$rax"
#endif

volatile unsigned Value = 0;
volatile unsigned Max = 10;


static void GenerateBreakpointScripts(const char *, const char *);
static void GenerateTracepointScripts(const char *, const char *);
static void RunTest();
extern "C" unsigned AssemblyReturn(unsigned);
extern "C" char Label_WriteAx;


int main(int argc, char **argv)
{
    if (argc != 1 && argc != 4)
    {
        std::cerr << "Must specify three arguments or none\n";
        return 1;
    }

    // When arguments are specified, just generate the
    // debugger scripts.
    //
    if (argc == 4 && strcmp(argv[1], "breakpoints") == 0)
    {
        GenerateBreakpointScripts(argv[2], argv[3]);
        return 0;
    }
    if (argc == 4 && strcmp(argv[1], "tracepoints") == 0)
    {
        GenerateTracepointScripts(argv[2], argv[3]);
        return 0;
    }

    // When run with no arguments, execute the test code.
    //
    RunTest();
    return 0;
}


static void RunTest()
{
    for (unsigned i = 0;  i < Max;  i++)
        Value = i;
    for (unsigned i = 0;  i < Max;  i++)
        Value = AssemblyReturn(i);
}


static void GenerateBreakpointScripts(const char *inFile, const char *compareFile)
{
    std::ofstream in(inFile);
    std::ofstream compare(compareFile);

    in << "monitor break if jump to 0x" << std::hex << reinterpret_cast<ADDR>(&RunTest) << "\n";
    in << "monitor break if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    in << "monitor list breakpoints\n";

    compare << "Breakpoint #1:\\s+break if jump to 0x" << std::hex << reinterpret_cast<ADDR>(&RunTest) << "\n";
    compare << "Breakpoint #2:\\s+break if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";

    in << "cont\n";         /* stop at RunTest */
    in << "cont\n";         /* stop at Value = 0 */
    in << "print i\n";
    in << "cont\n";         /* stop at Value = 1 */
    in << "print i\n";
    in << "monitor delete breakpoint 2\n";  /* delete "break if store to <Value>" */

    compare << "Triggered breakpoint #1:\n";
    compare << "Triggered breakpoint #2:\n";
    compare << ".*= 0\n";
    compare << "Triggered breakpoint #2:\n";
    compare << ".*= 1\n";

    in << "monitor break after store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << " == 5\n";
    in << "cont\n";         /* stop at Value = 5 */
    in << "print i\n";
    in << "monitor break at 0x" << std::hex << reinterpret_cast<ADDR>(&Label_WriteAx) << " if " REGAX " == 2\n";
    in << "cont\n";         /* stop in AssemblyReturn(2) */
    in << "print " REGAX "\n";
    in << "cont\n";         /* stop at Value = 5 */
    in << "cont\n";         /* program terminates */
    in << "quit\n";

    compare << "Triggered breakpoint #3:\n";
    compare << ".*= 5\n";
    compare << "Triggered breakpoint #4:\n";
    compare << ".*= 2\n";
    compare << "Triggered breakpoint #3:\n";
    compare << "Program exited normally\n";
}


static void GenerateTracepointScripts(const char *inFile, const char *compareFile)
{
    std::ofstream in(inFile);
    std::ofstream compare(compareFile);

    in << "monitor trace at 0x" << std::hex << reinterpret_cast<ADDR>(&main) << "\n";
    in << "break RunTest\n";
    in << "cont\n"; /* stop at RunTest */
    in << "monitor trace print\n";

    compare << "Tracepoint #1:\\s+trace at 0x" << std::hex << reinterpret_cast<ADDR>(&main) << "\n";
    compare << "Breakpoint 1,\\s*RunTest\n";
    compare << "0x0*" << std::hex << reinterpret_cast<ADDR>(&main) << "\n";

    in << "monitor trace clear\n";
    in << "monitor trace if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    in << "monitor break if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    in << "cont\n"; /* stop at Value = 0 (before trace occured) */
    in << "monitor trace print\n";

    compare << "Tracepoint #2:\\s*trace if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "Breakpoint #3:\\s*break if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "Triggered breakpoint #3:\n";
    /* no trace records printed */

    in << "monitor delete breakpoint 3\n";
    in << "monitor break after store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) <<
        " == 0x" << std::dec << Max-1 << "\n";
    in << "cont\n"; /* stop after Value = Max (end of first loop) */
    in << "monitor trace print\n";

    compare << "Breakpoint #4:\n";
    compare << "Triggered breakpoint #4:\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";
    compare << "0x[0-9,a-f]+:\\s*if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\n";

    in << "monitor trace disable\n";
    in << "monitor list tracepoints\n";
    in << "monitor trace clear\n";

    compare << "#1:\\s*trace at 0x" << std::hex << reinterpret_cast<ADDR>(&main) << "\\s*\\(disabled\\)\n";
    compare << "#2:\\s*trace if store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << "\\s*\\(disabled\\)\n";

    in << "monitor trace after store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << " == 2\n";
    in << "monitor trace after store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << " == 4\n";
    in << "cont\n"; /* stop after Value = Max (end of second loop) */
    in << "monitor trace print\n";

    compare << "Tracepoint #5:\\s*trace after store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value)
        << " == 0x2\n";
    compare << "Tracepoint #6:\\s*trace after store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value)
        << " == 0x4\n";
    compare << "Triggered breakpoint #4:\n";
    compare << "0x[0-9,a-f]+:\\s*after store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << " == 0x2\n";
    compare << "0x[0-9,a-f]+:\\s*after store to 0x" << std::hex << reinterpret_cast<ADDR>(&Value) << " == 0x4\n";

    in << "quit\n";
}
