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
#ifndef DEBUGGER_SHELL_H
#define DEBUGGER_SHELL_H

#include <string>
#include "pin.H"

class DEBUGGER_SHELL_ARGS;
class IDEBUGGER_SHELL_INSTRUMENTOR;


/*!
 * Pin tools that run with the "-appdebug" switch can use this class to implement
 * some common custom debugger commands.  Once enabled, these commands can be
 * typed interactively at the debugger prompt.  In GDB, type "monitor help" to
 * see a list of the available commands.
 *
 * Typical usage in the tool is as follows:
 *
 *  #include "debugger-shell.H"
 *
 *  int main(int argc, char **argv)
 *  {
 *      if (PIN_Init(argc,argv))
 *          return 1;
 *
 *      IDEBUGGER_SHELL *shell = CreateDebuggerShell();
 *      DEBUGGER_SHELL_ARGS args;
 *      if (!shell->Enable(args))
 *          return 1;
 *
 *      PIN_StartProgram();
 *  }
 *
 * The tool must also compile and link with "debugger-shell.cpp".
 */
class IDEBUGGER_SHELL
{
public:
    /*!
     * This method must be called once, typically from the tool's main() function.
     * It enables the Pin instrumentation which implements the debugger extensions.
     *
     *  @param[in] args     Arguments that customize the debugger shell.
     *
     * @return  TRUE on success.  If there is an error, a diagnostic is printed
     *           this method returns FALSE.
     */
    virtual BOOL Enable(const DEBUGGER_SHELL_ARGS &args) = 0;

    /*!
     * Tools that set DEBUGGER_SHELL_ARGS._enableHelp to FALSE, may want to use
     * this method and GetHelpString() to print their own help message.
     *
     * @return  The number of help commands.
     */
    virtual unsigned GetHelpCount() = 0;

    /*!
     * Get the help message describing a single debugger command.
     *
     *  @param[in] index            An index telling which command to return help for.
     *                               Must be in the range [0,n-1], where \e n is the
     *                               value returned by GetHelpCount().
     *  @param[out] cmd             Receives the name of the debugger command.
     *  @param[out] description     Receives a description of what the command does.
     *
     * @return  TRUE on success, FALSE if \a index is out of range.
     */
    virtual BOOL GetHelpString(unsigned index, std::string *cmd, std::string *description) = 0;

    /*!
     * Tools that override the default instrumentation with IDEBUGGER_SHELL_INSTRUMENTOR
     * may need a virtual register to hold the "skip one" flag.  This method provides the
     * register number to use for this.
     *
     * @return  The Pin virtual register to use for the "skip one" flag.  This is one
     *           of the REG_INST_Gn registers.
     */
    virtual REG GetSkipOneRegister() = 0;

    virtual ~IDEBUGGER_SHELL() {}   ///< Destroys the DEBUGGER_SHELL object.
};

/*!
 * Create a DEBUGGER_SHELL object.
 *
 * @return  A new DEBUGGER_SHELL object on success.  If there is an error, a diagnostic
 *           is printed and this method returns NULL.
 */
IDEBUGGER_SHELL *CreateDebuggerShell();


/*!
 * Arguments that customize the debugger shell.
 */
struct DEBUGGER_SHELL_ARGS
{
    DEBUGGER_SHELL_ARGS() :
        _callOrderBefore(CALL_ORDER_DEFAULT),
        _callOrderAfter(CALL_ORDER_DEFAULT),
        _enableHelp(TRUE),
        _overrideInstrumentation(0)
    {}

    /*!
     * Tells the relative position of instrumentation added by the debugger shell
     * for "before" and "after" instrumentation.
     */
    CALL_ORDER _callOrderBefore;
    CALL_ORDER _callOrderAfter;

    BOOL _enableHelp;   ///< Enables "help" command in debugger shell.

    /*!
     * Allows client to override some instrumentation.
     */
    IDEBUGGER_SHELL_INSTRUMENTOR *_overrideInstrumentation;
};


/*!
 * Some tools may need to define their own analysis routine that stops at a debugger
 * breakpoint.  Such tools can define their own class, which derives from
 * IDEBUGGER_SHELL_INSTRUMENTOR.  Pass a pointer to that object to
 * DEBUGGER_SHELL_ARGS::_overrideInstrumentation.
 *
 * Most tools do not need to override the default instrumenation, so most tools
 * need not use this interface.
 */
class IDEBUGGER_SHELL_INSTRUMENTOR
{
public:
    /*!
     * The debugger shell calls this method to insert a "then" instrumentation call
     * to an analysis routine that stops at a debugger breakpoint _before_ an instruction.
     * The default instrumenation looks like this.  Tools that implement this method should
     * insert similar instrumentation:
     *
     *  VOID InsertBreakpointBefore(INST ins, BBL bbl, CALL_ORDER order, const std::string &message)
     *  {
     *      INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)TriggerBreakpointBefore,
     *          IARG_CALL_ORDER, order,
     *          IARG_CONTEXT, IARG_THREAD_ID,
     *          IARG_UINT32, static_cast<UINT32>(RegSkipOne),
     *          IARG_PTR, message.c_str(),
     *          IARG_END);
     *  }
     *
     *  VOID TriggerBreakpointBefore(CONTEXT *ctxt, THREADID tid, UINT32 regSkipOne, const char *message)
     *  {
     *      ADDRINT skipPc = PIN_GetContextReg(ctxt, static_cast<REG>(regSkipOne));
     *      ADDRINT pc = PIN_GetContextReg(ctxt, REG_INST_PTR);
     *      if (skipPc == pc)
     *          return
     *
     *      PIN_SetContextReg(ctxt, static_cast<REG>(regSkipOne), pc);
     *      PIN_ApplicationBreakpoint(ctxt, tid, FALSE, message);
     *  }
     *
     * See the method IDEBUGGER_SHELL::GetSkipOneRegister() for the register number
     * to use for \e RegSkipOne.
     *
     *  @param[in] ins      Insert the instrumentation before this instruction.
     *  @param[in] bbl      The basic block containing \a ins.
     *  @param[in] order    The instrumentation call order to use for the instrumentation.
     *  @param[in] message  String telling why the breakpoint is triggered.  The
     *                       string is allocated in permanent storage, so the
     *                       client can pass it directly to an analysis routine.
     *                       If the debugger shell removes instrumenation, it will
     *                       also deallocate this string.
     */
    virtual VOID InsertBreakpointBefore(INS ins, BBL bbl, CALL_ORDER order, const std::string &message) = 0;

    /*!
     * The debugger shell calls this method to insert a "then" instrumentation call
     * to an analysis routine that stops at a debugger breakpoint _after_ an instruction.
     * The default instrumenation looks like this.  Tools that implement this method should
     * insert similar instrumentation:
     *
     *  VOID InsertBreakpointAfter(INST ins, BBL bbl, IPOINT ipoint, CALL_ORDER order,
     *      const std::string &message)
     *  {
     *      INS_InsertThenCall(ins, ipoint, (AFUNPTR)TriggerBreakpointAfter,
     *          IARG_CALL_ORDER, order,
     *          IARG_CONTEXT, IARG_INST_PTR, IARG_THREAD_ID,
     *          IARG_PTR, message.c_str(),
     *          IARG_END);
     *  }
     *
     *  VOID TriggerBreakpointAfter(CONTEXT *ctxt, ADDRINT pc, THREADID tid, const char *message)
     *  {
     *      std::ostringstream os;
     *      os << message << "\n";
     *      os << "Breakpoint triggered after instruction at 0x" << std::hex << pc;
     *
     *      PIN_ApplicationBreakpoint(ctxt, tid, FALSE, os.str());
     *  }
     *
     *  @param[in] ins      Insert the instrumentation after this instruction.
     *  @param[in] bbl      The basic block containing \a ins.
     *  @param[in] ipoint   Tells whether to instrument IPOINT_AFTER or IPOINT_TAKEN_BRANCH.
     *  @param[in] order    The instrumentation call order to use for the instrumentation.
     *  @param[in] message  String telling why the breakpoint is triggered.  The
     *                       string is allocated in permanent storage, so the
     *                       client can pass it directly to an analysis routine.
     *                       If the debugger shell removes instrumenation, it will
     *                       also deallocate this string.
     */
    virtual VOID InsertBreakpointAfter(INS ins, BBL bbl, IPOINT ipoint, CALL_ORDER order,
        const std::string &message) = 0;
};

#endif // file guard
