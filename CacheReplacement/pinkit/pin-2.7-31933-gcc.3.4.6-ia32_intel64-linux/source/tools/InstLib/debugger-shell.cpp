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
 * This is the implementation file for the "debugger-shell" tool extensions.
 * See the header file "debugger-shell.H" for a description of what these
 * extensions are and how to use them in your tool.
 */

/*
 * This tool implements a number of extended debugger breakpoints, which use
 * Pin instrumentation to trigger the breakpoint condition.  This comment
 * provides an overview of the instrumentation strategy.
 *
 * Most of the extended debugger breakpoints insert instrumentation at
 * IPOINT_BEFORE, which tests the breakpoint condition.  We use if / then
 * instrumentation, where the "if" part tests the breakpoint condition and
 * the "then" part triggers the breakpoint.  The inserted analysis code follows
 * this pattern:
 *
 *      if TestCondition(....)
 *      {
 *          if (REG_SKIP_ONE == REG_INST_PTR)
 *              return;
 *          REG_SKIP_ONE = REG_INST_PTR;
 *          PIN_ApplicationBreakpoint(....);
 *      }
 *      [Original Instruction]
 *      REG_SKIP_ONE = 0
 *
 * The TestCondition() is a short in-lined "if" analysis call that returns
 * TRUE if the breakpoint condition is true, and the remainder of the code
 * before [Original Instruction] is in the "then" analysis call.  Usually,
 * all breakpoint types share the same "then" code, but use a different
 * TestCondition().
 *
 * The usage of REG_SKIP_ONE is a bit tricky.  When the debugger continues
 * from a breakpoint, it re-executes all the instrumentation on the [Original
 * Instruction], including the TestCondition() call.  Of course, we don't want
 * the breakpoint to immediately re-trigger, though, or the application would
 * never make forward progress.  We use a Pin virtual register (denoted by
 * REG_SKIP_ONE above) to solve this by skipping the next occurence of the
 * breakpoint when the application resumes.  Clearing REG_SKIP_ONE at
 * IPOINT_AFTER / IPOINT_TAKEN_BRANCH ensures that the breakpoint will
 * re-trigger if the application loops back to the same instruction.
 *
 * If more than one breakpoint is placed on the same instruction, each
 * breakpoint inserts its own instrumentation like this:
 *
 *      if TestCondition1(....)
 *      {
 *          if (REG_SKIP_ONE == REG_INST_PTR)
 *              return;
 *          REG_SKIP_ONE = REG_INST_PTR;
 *          PIN_ApplicationBreakpoint(....);
 *      }
 *      if TestCondition2(....)
 *      {
 *          if (REG_SKIP_ONE == REG_INST_PTR)
 *              return;
 *          REG_SKIP_ONE = REG_INST_PTR;
 *          PIN_ApplicationBreakpoint(....);
 *      }
 *      [Original Instruction]
 *      REG_SKIP_ONE = 0
 *
 * One breakpoint ("break if store to <addr> == <value>")  is checked at
 * IPOINT_AFTER or IPOINT_TAKEN_BRANCH.  This instrumentation looks like this:
 *
 *      REG_RECORD_EA = IARG_MEMORYWRITE_EA
 *      [Original Store Instruction]
 *      if (REG_RECORD_EA == <addr> && *REG_RECORD_EA == <value>)
 *      {
 *          PIN_ApplicationBreakpoint(....);
 *      }
 *
 * Here, we record the value of the effective address in a Pin virtual register
 * because IARG_MEMORYWRITE_EA cannot be computed at IPOINT_AFTER.  The
 * instrumentation at IPOINT_AFTER tests the breakpoint condition and triggers
 * the breakpoint.  If the breakpoint does trigger, the PC will point to the
 * next instruction after [Original Store Instruction].  Therefore, when the
 * debugger continues, execution immediately resumes at the next instruction
 * and there is no need to use the REG_SKIP_ONE technique.
 *
 * The tool also implements tracepoints.  A tracepoint is like a breakpoint,
 * except that instead of stopping when a condition is met, a trace record is
 * recorded.  The instrumentation for a tracepoint uses the same "if"
 * instrumentation to check the condition, but the "then" instrumentation is
 * different.  A typical example looks like this:
 *
 *      if TestCondition(....)
 *      {
 *          GetLock(....);
 *          TraceLog.push_back(....);
 *          ReleaseLock(....);
 *      }
 */

#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include "debugger-shell.H"


class DEBUGGER_SHELL : public IDEBUGGER_SHELL
{
private:
    BOOL _isEnabled;
    DEBUGGER_SHELL_ARGS _clientArgs;

    struct HELP
    {
        HELP(const char *cmd, const char *desc) : _command(cmd), _description(desc) {}

        std::string _command;           // Name of the extended debugger command.
        std::string _description;       // Help string for the command.
    };
    typedef std::vector<HELP> HELPS;
    HELPS _helpStrings;                 // Help messages for all the commands.
    std::string _formattedHelp;         // Formatted help message.

    typedef std::vector<std::string> WORDS;

    // These are the register numbers for the REG_SKIP_ONE and REG_RECORD_EA
    // virtual registers.  Each is one of the REG_INST_Gn registers.
    //
    REG _regSkipOne;
    REG _regRecordEa;

    // Possible trigger conditions for breakpoints or tracepoints.
    //
    enum TRIGGER
    {
        TRIGGER_AT,             // Trigger before PC.
        TRIGGER_STORE_TO,       // Trigger before store to address.
        TRIGGER_STORE_VALUE_TO, // Trigger after store of value to address.
        TRIGGER_JUMP_TO,        // Trigger before jump to PC.
        TRIGGER_REG_IS          // Trigger before PC if register has value.
    };

    // Possible event types.
    //
    enum ETYPE
    {
        ETYPE_BREAKPOINT,
        ETYPE_TRACEPOINT
    };

    struct EVENT
    {
        ETYPE _type;                // Breakpoint vs. tracepoint.
        TRIGGER _trigger;           // Trigger condition.
        std::string _listMsg;       // String printed when event is listed.
        std::string _triggerMsg;    // Message printed when breakpoint triggers or when tracepoint is printed.

        // Data specific to ETYPE_TRACEPOINT.
        //
        REG _reg;                   // If not REG_INVALID, trace this register.
        BOOL _isDeleted;            // TRUE: User has deleted, but may be referenced in TRACEREC.
        BOOL _isEnabled;            // TRUE if tracepoint is enabled.

        // Data specific to the trigger condition.
        //
        union
        {
            ADDRINT _ea;            // TRIGGER_STORE_TO: EA of store.
            ADDRINT _pc;            // TRIGGER_AT: PC of trigger location.
                                    // TRIGGER_JUMP_TO: PC of jump.

            struct
            {                       // TRIGGER_STORE_VALUE_TO:
                ADDRINT _ea;        //   EA of store.
                UINT64 _value;      //   value stored.
            } _storeValueTo;

            struct
            {                       // TRIGGER_REG_IS:
                ADDRINT _pc;        //   PC of breakpoint.
                REG _reg;           //   register ID.
                ADDRINT _value;     //   value of register.
            } _regIs;
        };
    };

    // All events, indexed by their ID.
    //
    typedef std::map<unsigned, EVENT> EVENTS;
    EVENTS _events;

    unsigned _nextEventId;

    // A trace record collected when executing a tracepoint.
    //
    struct TRACEREC
    {
        unsigned _id;       // Index of EVENT in '_events'.
        ADDRINT _pc;        // PC where tracepoint triggered.
        ADDRINT _regValue;  // If tracepoints traces a register, it's value.
    };

    // Log of all the tracepoint data.  Access is protected by the lock.
    //
    PIN_LOCK _traceLock;
    typedef std::vector<TRACEREC> TRACERECS;
    TRACERECS _traceLog;


public:
    // ----- constructor -----

    /*
     * This method completes construction of the object.  We do it here,
     * so we can return an error indication more easily.
     *
     * @return  TRUE on success.
     */
    BOOL Construct()
    {
        _regSkipOne = PIN_ClaimToolRegister();
        _regRecordEa = PIN_ClaimToolRegister();
        if (!REG_valid(_regSkipOne) || !REG_valid(_regRecordEa))
        {
            PrintError("Unable to allocate Pin virtual register");
            return FALSE;
        }

        InitLock(&_traceLock);
        ConstructHelpStrings();
        _formattedHelp = FormatHelp();
        _nextEventId = 1;
        _isEnabled = FALSE;
        return TRUE;
    }


    // ----- IDEBUGGER_SHELL -----

    BOOL Enable(const DEBUGGER_SHELL_ARGS &args)
    {
        if (_isEnabled)
        {
            PrintError("Do not call IDEBUGGER_SHELL::Enable() twice");
            return FALSE;
        }

        _clientArgs = args;
        PIN_AddDebugInterpreter(DebugInterpreter, this);
        TRACE_AddInstrumentFunction(InstrumentTrace, this);
        _isEnabled = TRUE;
        return TRUE;
    }

    unsigned GetHelpCount()
    {
        return _helpStrings.size();
    }

    BOOL GetHelpString(unsigned index, std::string *cmd, std::string *description)
    {
        if (index >= _helpStrings.size())
            return FALSE;

        HELP *hlp = &_helpStrings[index];
        *cmd = hlp->_command;
        *description = hlp->_description;
        return TRUE;
    }

    REG GetSkipOneRegister()
    {
        return _regSkipOne;
    }


private:
    /*
     * Pin call-back that implements an extended debugger command.
     *
     *  @param[in] tid          The debugger focus thread.
     *  @param[in,out] ctxt     Register state for the debugger's "focus" thread.
     *  @param[in] cmd          Text of the extended command.
     *  @param[out] result      Text that the debugger prints when the command finishes.
     *  @param[in] vme          Pointer to DEBUGGER_SHELL instance.
     *
     * @return  TRUE if we recognize this extended command.
     */
    static BOOL DebugInterpreter(THREADID tid, CONTEXT *ctxt, const string &cmd, string *result, VOID *vme)
    {
        DEBUGGER_SHELL *me = static_cast<DEBUGGER_SHELL *>(vme);

        /*
         * Breakpoint Commands:
         *
         *  break if store to <addr>
         *  break after store to <addr> == <value>
         *  break if jump to <pc>
         *  break at <pc> if <reg> == <value>
         *  list breakpoints
         *  delete breakpoint <id>
         *
         * Tracing Commands:
         *
         *  trace [<reg>] at <pc>
         *  trace [<reg>] if store to <addr>
         *  trace [<reg>] after store to <addr> == <value>
         *  trace enable [<id>]
         *  trace disable [<id>]
         *  trace clear
         *  trace print [to <file>]
         *  list tracepoints
         *  delete tracepoint <id>
         *
         * Example Trace Output:
         *
         *  0x1234: rax = 0x5678
         *  0x1234:
         *  0x1234: if store to 0x89abc: rax = 0x5678 
         *  0x1234: if store to 0x89abc
         *  0x1234: after store to 0x89abc = 0xdef00: rax = 0x5678 
         *  0x1234: after store to 0x89abc = 0xdef00
         */

        WORDS words;
        me->SplitWords(cmd, &words);
        size_t nWords = words.size();

        if (me->_clientArgs._enableHelp && nWords == 1 && words[0] == "help")
        {
            // help
            //
            *result = me->_formattedHelp;
            return TRUE;
        }
        else if (nWords == 2 && words[0] == "list" && words[1] == "breakpoints")
        {
            // list breakpoints
            //
            *result = me->ListBreakpoints();
            return TRUE;
        }
        else if (nWords == 2 && words[0] == "list" && words[1] == "tracepoints")
        {
            // list tracepoints
            //
            *result = me->ListTracepoints();
            return TRUE;
        }
        else if (nWords == 3 && words[0] == "delete" && words[1] == "breakpoint")
        {
            // delete breakpoint <id>
            //
            *result = me->DeleteEvent(ETYPE_BREAKPOINT, words[2]);
            return TRUE;
        }
        else if (nWords == 3 && words[0] == "delete" && words[1] == "tracepoint")
        {
            // delete tracepoint <id>
            //
            *result = me->DeleteEvent(ETYPE_TRACEPOINT, words[2]);
            return TRUE;
        }
        else if (nWords == 2 && words[0] == "trace" && words[1] == "enable")
        {
            // trace enable
            //
            *result = me->EnableDisableAllTraces(TRUE);
            return TRUE;
        }
        else if (nWords == 3 && words[0] == "trace" && words[1] == "enable")
        {
            // trace enable <id>
            //
            *result = me->EnableDisableTrace(words[2], TRUE);
            return TRUE;
        }
        else if (nWords == 2 && words[0] == "trace" && words[1] == "disable")
        {
            // trace disable
            //
            *result = me->EnableDisableAllTraces(FALSE);
            return TRUE;
        }
        else if (nWords == 3 && words[0] == "trace" && words[1] == "disable")
        {
            // trace disable <id>
            //
            *result = me->EnableDisableTrace(words[2], FALSE);
            return TRUE;
        }
        else if (nWords == 2 && words[0] == "trace" && words[1] == "clear")
        {
            // trace clear
            //
            *result = me->ClearTraceLog();
            return TRUE;
        }
        else if (nWords == 2 && words[0] == "trace" && words[1] == "print")
        {
            // trace print
            //
            *result = me->PrintTraceLog("");
            return TRUE;
        }
        else if (nWords == 4 && words[0] == "trace" && words[1] == "print" && words[2] == "to")
        {
            // trace print to <file>
            //
            *result = me->PrintTraceLog(words[3]);
            return TRUE;
        }
        else if (nWords == 3 && words[0] == "trace" && words[1] == "at")
        {
            // trace at <pc>
            //
            *result = me->ParseTriggerAtEvent(ETYPE_TRACEPOINT, words[2], "");
            return TRUE;
        }
        else if (nWords == 4 && words[0] == "trace" && words[2] == "at")
        {
            // trace <reg> at <pc>
            //
            *result = me->ParseTriggerAtEvent(ETYPE_TRACEPOINT, words[3], words[1]);
            return TRUE;
        }
        else if (nWords == 5 && words[0] == "break" && words[1] == "if" && words[2] == "store" &&
            words[3] == "to")
        {
            // break if store to <addr>
            //
            *result = me->ParseTriggerStoreToEvent(ETYPE_BREAKPOINT, words[4], "");
            return TRUE;
        }
        else if (nWords == 5 && words[0] == "trace" && words[1] == "if" && words[2] == "store" &&
            words[3] == "to")
        {
            // trace if store to <addr>
            //
            *result = me->ParseTriggerStoreToEvent(ETYPE_TRACEPOINT, words[4], "");
            return TRUE;
        }
        else if (nWords == 6 && words[0] == "trace" && words[2] == "if" && words[3] == "store" &&
            words[4] == "to")
        {
            // trace <reg> if store to <addr>
            //
            *result = me->ParseTriggerStoreToEvent(ETYPE_TRACEPOINT, words[5], words[1]);
            return TRUE;
        }
        else if (nWords == 7 && words[0] == "break" && words[1] == "after" && words[2] == "store" &&
            words[3] == "to" && words[5] == "==")
        {
            // break after store to <addr> == <value>
            //
            *result = me->ParseTriggerStoreValueToEvent(ETYPE_BREAKPOINT, words[4], words[6], "");
            return TRUE;
        }
        else if (nWords == 7 && words[0] == "trace" && words[1] == "after" && words[2] == "store" &&
            words[3] == "to" && words[5] == "==")
        {
            // trace after store to <addr> == <value>
            //
            *result = me->ParseTriggerStoreValueToEvent(ETYPE_TRACEPOINT, words[4], words[6], "");
            return TRUE;
        }
        else if (nWords == 8 && words[0] == "trace" && words[2] == "after" && words[3] == "store" &&
            words[4] == "to" && words[6] == "==")
        {
            // trace <reg> after store to <addr> == <value>
            //
            *result = me->ParseTriggerStoreValueToEvent(ETYPE_TRACEPOINT, words[5], words[7], words[1]);
            return TRUE;
        }
        else if (nWords == 5 && words[0] == "break" && words[1] == "if" && words[2] == "jump" &&
            words[3] == "to")
        {
            // break if jump to <pc>
            //
            *result = me->ParseTriggerJumpToEvent(ETYPE_BREAKPOINT, words[4], "");
            return TRUE;
        }
        else if (nWords == 7 && words[0] == "break" && words[1] == "at" && words[3] == "if" &&
            words[5] == "==")
        {
            // break at <pc> if <reg> == <value>
            //
            *result = me->ParseTriggerRegIsEvent(ETYPE_BREAKPOINT, words[2], words[4], words[6], "");
            return TRUE;
        }
        return FALSE;
    }


    /*
     * Split an input command into a series of whitespace-separated words.  Leading
     * and trailing whitespace is ignored.
     *
     *  @param[in] cmd      The input command.
     *  @param[in] words    An STL container that receives the parsed words.  Each
     *                       word is added with the push_back() method.
     */
    VOID SplitWords(const std::string &cmd, WORDS *words)
    {
        size_t pos = cmd.find_first_not_of(' ');
        while (pos != std::string::npos)
        {
            size_t end = cmd.find_first_of(' ', pos+1);
            if (end == std::string::npos)
            {
                words->push_back(cmd.substr(pos));
                pos = end;
            }
            else
            {
                words->push_back(cmd.substr(pos, end-pos));
                pos = cmd.find_first_not_of(' ' , end+1);
            }
        }
    }


    /*
     * Attempt to parse an unsigned integral number from a string.  The string's prefix
     * determines the radix: "0x" for hex, "0" for octal, otherwise decimal.
     *
     *  @param[in] val      The string to parse.
     *  @param[out] number  On success, receives the parsed number.
     *
     * @return  TRUE if a number is successfully parsed.
     */
    template<typename T> BOOL ParseNumber(const std::string &val, T *number)
    {
        std::istringstream is(val);

        T num;
        if (val.compare(0, 2, "0x") == 0)
            is >> std::hex >> num;
        else if (val.compare(0, 1, "0") == 0)
            is >> std::oct >> num;
        else
            is >> std::dec >> num;

        if (is.fail() || !is.eof())
            return FALSE;
        *number = num;
        return TRUE;
    }


    /*
     * Attempt to parse a "full" register name.
     *
     *  @param[in] name     String which possibly names a register.
     *
     * @return  If \a name is a register we recognize, returns that register ID.  Otherwise,
     *           returns REG_INVALID().
     */
    REG ParseRegName(const std::string &name)
    {
#if defined(TARGET_IA32E)
        if (name == "$rax")
            return REG_GAX;
        if (name == "$rbx")
            return REG_GBX;
        if (name == "$rcx")
            return REG_GCX;
        if (name == "$rdx")
            return REG_GDX;
        if (name == "$rsi")
            return REG_GSI;
        if (name == "$rdi")
            return REG_GDI;
        if (name == "$rbp")
            return REG_GBP;
        if (name == "$rsp")
            return REG_RSP;
        if (name == "$r8")
            return REG_R8;
        if (name == "$r9")
            return REG_R9;
        if (name == "$r10")
            return REG_R10;
        if (name == "$r11")
            return REG_R11;
        if (name == "$r12")
            return REG_R12;
        if (name == "$r13")
            return REG_R13;
        if (name == "$r14")
            return REG_R14;
        if (name == "$r15")
            return REG_R15;
#elif defined(TARGET_IA32)
        if (name == "$eax")
            return REG_GAX;
        if (name == "$ebx")
            return REG_GBX;
        if (name == "$ecx")
            return REG_GCX;
        if (name == "$edx")
            return REG_GDX;
        if (name == "$esi")
            return REG_GSI;
        if (name == "$edi")
            return REG_GDI;
        if (name == "$ebp")
            return REG_GBP;
        if (name == "$esp")
            return REG_ESP;
#endif
        return REG_INVALID();
    }


    /*
     * Get the name for a Pin register.
     *
     *  @param[in] reg  The register.
     *
     * @return  The name of \a reg.
     */
    std::string GetRegName(REG reg)
    {
        switch (reg)
        {
#if defined(TARGET_IA32E)
        case REG_GAX:
            return "$rax";
        case REG_GBX:
            return "$rbx";
        case REG_GCX:
            return "$rcx";
        case REG_GDX:
            return "$rdx";
        case REG_GSI:
            return "$rsi";
        case REG_GDI:
            return "$rdi";
        case REG_GBP:
            return "$rbp";
        case REG_RSP:
            return "$rsp";
        case REG_R8:
            return "$r8";
        case REG_R9:
            return "$r9";
        case REG_R10:
            return "$r10";
        case REG_R11:
            return "$r11";
        case REG_R12:
            return "$r12";
        case REG_R13:
            return "$r13";
        case REG_R14:
            return "$r14";
        case REG_R15:
            return "$r15";
#elif defined(TARGET_IA32)
        case REG_GAX:
            return "$eax";
        case REG_GBX:
            return "$ebx";
        case REG_GCX:
            return "$ecx";
        case REG_GDX:
            return "$edx";
        case REG_GSI:
            return "$esi";
        case REG_GDI:
            return "$edi";
        case REG_GBP:
            return "$ebp";
        case REG_ESP:
            return "$esp";
#endif
        default:
            ASSERTX(0);
            return "???";
        }
    }


    /*
     * Construct help strings for all the extended debugger commands.
     */
    void ConstructHelpStrings()
    {
        if (_clientArgs._enableHelp)
            _helpStrings.push_back(HELP("help", "Print this help message."));

        // Breakpoint commands.
        //
        _helpStrings.push_back(HELP("list breakpoints",
            "List all extended breakpoints."));
        _helpStrings.push_back(HELP("delete breakpoint <id>",
            "Delete extended breakpoint <id>."));
        _helpStrings.push_back(HELP("break if store to <addr>",
            "Break before any store to <addr>."));
        _helpStrings.push_back(HELP("break after store to <addr> == <value>",
            "Break after store if <value> stored to <addr>."));
        _helpStrings.push_back(HELP("break if jump to <pc>",
            "Break before any jump to <pc>."));
        _helpStrings.push_back(HELP("break at <pc> if <reg> == <value>",
            "Break before <pc> if <reg> contains <value>."));

        // Tracepoint commands.
        //
        _helpStrings.push_back(HELP("list tracepoints",
            "List all extended tracepoints."));
        _helpStrings.push_back(HELP("delete tracepoint <id>",
            "Delete extended tracepoint <id>."));
        _helpStrings.push_back(HELP("trace print [to <file>]",
            "Print contents of trace log to screen, or to <file>."));
        _helpStrings.push_back(HELP("trace clear",
            "Clear contents of trace log."));
        _helpStrings.push_back(HELP("trace disable [<id>]",
            "Disable all tracepoints, or only tracepoint <id>."));
        _helpStrings.push_back(HELP("trace enable [<id>].",
            "Enable all tracepoints, or only tracepoint <id>."));
        _helpStrings.push_back(HELP("trace [<reg>] at <pc>",
            "Record trace entry before executing instructon at <pc>.  If <reg> is "
            "specified, record that register's value too."));
        _helpStrings.push_back(HELP("trace [<reg>] if store to <addr>",
            "Record trace entry before executing any store to <addr>.  If <reg> is "
            "specified, record that register's value too."));
        _helpStrings.push_back(HELP("trace [<reg>] after store to <addr> == <value>",
            "Record trace entry after any store of <value> to <addr>.  If <reg> is "
            "specified, record that register's value too."));
    }


    /*
     * @return  A single formatted help message to use in response to the "help" debugger command.
     */
    std::string FormatHelp()
    {
        const size_t longCommandSize = 25;  // Description for long command is printed on separate line.
        const size_t maxWidth = 80;         // Maximum width of any line.

        // The description text starts 2 spaces to the right of the longest "short" command.
        //
        const size_t dashColumn = longCommandSize+2;

        std::string help;
        BOOL newLineBeforeNext = FALSE;
        for (HELPS::iterator it = _helpStrings.begin();  it != _helpStrings.end();  ++it)
        {
            std::string thisMessage = it->_command;

            BOOL isLongCommand = FALSE;
            if (it->_command.size() < longCommandSize)
            {
                // This is a "short" command.  The description starts on the same line as
                // the command, but may continue on subsequent lines.
                //
                size_t pad = dashColumn - it->_command.size();
                thisMessage.append(pad, ' ');
                thisMessage.append("- ");
                thisMessage.append(it->_description);
                if (thisMessage.size() > maxWidth)
                    thisMessage = SplitToMultipleLines(thisMessage, maxWidth, dashColumn+2);
            }
            else
            {
                // This is a "long" command.  The description starts on the next line.
                //
                thisMessage.append("\n");
                std::string desc(dashColumn+2, ' ');
                desc.append(it->_description);
                if (desc.size() > maxWidth)
                    desc = SplitToMultipleLines(desc, maxWidth, dashColumn+2);
                thisMessage.append(desc);
                isLongCommand = TRUE;
            }

            // It seems more readable if there is a blank line separating "long" commands
            // from their neighbors.
            //
            if (newLineBeforeNext || isLongCommand)
                help.append("\n");

            help.append(thisMessage);
            help.append("\n");
            newLineBeforeNext = isLongCommand;
        }
        return help;
    }


    /*
     * Split a line of text into multiple indented lines.
     *
     *  @param[in] str          The line of text to split.
     *  @param[in] maxWidth     None of the output lines will be wider than this limit.
     *  @param[in] indent       If the line is split, all line other than the first are indented
     *                           with this many spaces.
     *
     * @return  The formated text lines.
     */
    std::string SplitToMultipleLines(const std::string &str, size_t maxWidth, size_t indent)
    {
        BOOL isFirst = TRUE;
        std::string ret;

        std::string input = str;
        while (input.size() > maxWidth)
        {
            BOOL needHyphen = FALSE;
            size_t posBreakAfter = std::string::npos;

            // Point 'posBreakAfter' to the last character of the last word that fits
            // before 'maxWidth'.  We assume that words are separated by spaces.
            //
            size_t posSpace = input.rfind(' ', maxWidth-1);
            if (posSpace != std::string::npos)
                posBreakAfter = input.find_last_not_of(' ', posSpace);

            // If there's a really long word is itself longer than 'maxWidth', break
            // the word and put a hyphen in.
            //
            if (posBreakAfter == std::string::npos)
            {
                posBreakAfter = maxWidth-2;
                needHyphen = TRUE;
            }

            // Split the line, add indenting for all but the first one.
            //
            if (!isFirst)
                ret.append(indent, ' ');
            ret.append(input.substr(0, posBreakAfter+1));
            if (needHyphen)
                ret.append("-");
            ret.append("\n");

            // Strip off any spaces from the start of the next line.
            //
            size_t posNextWord = input.find_first_not_of(' ', posSpace);
            input.erase(0, posNextWord);

            // Lines after the first are indented, so this reduces the effective width
            // of the line.
            //
            if (isFirst)
            {
                isFirst = FALSE;
                maxWidth -= indent;
            }
        }

        if (input.size())
        {
            if (!isFirst)
                ret.append(indent, ' ');
            ret.append(input);
        }
        return ret;
    }


    /*
     * @return  A single string showing all the active breakpoints.
     */
    std::string ListBreakpoints()
    {
        std::string ret;

        for (EVENTS::iterator it = _events.begin();  it != _events.end();  ++it)
        {
            if (it->second._type == ETYPE_BREAKPOINT)
            {
                ret += it->second._listMsg;
                ret += "\n";
            }
        }
        return ret;
    }


    /*
     * @return  A single string showing all the active tracepoints.
     */
    std::string ListTracepoints()
    {
        std::string ret;

        for (EVENTS::iterator it = _events.begin();  it != _events.end();  ++it)
        {
            if (it->second._type == ETYPE_TRACEPOINT && !it->second._isDeleted)
            {
                ret += it->second._listMsg;
                if (!it->second._isEnabled)
                    ret += " (disabled)";
                ret += "\n";
            }
        }
        return ret;
    }


    /*
     * Delete an event.
     *
     *  @param[in] type     The type of event to delete.
     *  @param[in] idStr    The event ID.
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string DeleteEvent(ETYPE type, const std::string &idStr)
    {
        unsigned id;
        std::string ret = ValidateId(type, idStr, &id);
        if (!ret.empty())
            return ret;

        // The trace log may contain a pointer to the tracepoint, so don't really
        // delete it if the trace log is non-empty.
        //
        if (type == ETYPE_TRACEPOINT && !_traceLog.empty())
            _events[id]._isDeleted = TRUE;
        else
            _events.erase(id);
        CODECACHE_FlushCache();
        return "";
    }


    /*
     * Enable or disable all "trace" events.
     *
     *  @param[in] enable   TRUE if events should be enabled.
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string EnableDisableAllTraces(BOOL enable)
    {
        BOOL needFlush = FALSE;
        for (EVENTS::iterator it = _events.begin();  it != _events.end();  ++it)
        {
            if (it->second._type == ETYPE_TRACEPOINT && !it->second._isDeleted &&
                it->second._isEnabled != enable)
            {
                it->second._isEnabled = enable;
                needFlush = TRUE;
            }
        }
        if (needFlush)
            CODECACHE_FlushCache();
        return "";
    }


    /*
     * Enable or disable a single trace event.
     *
     *  @param[in] idStr    ID of the trace event to enable.
     *  @param[in] enable   TRUE if the event should be enabled.
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string EnableDisableTrace(const std::string &idStr, BOOL enable)
    {
        unsigned id;
        std::string ret = ValidateId(ETYPE_TRACEPOINT, idStr, &id);
        if (!ret.empty())
            return ret;

        if (_events[id]._isEnabled != enable)
        {
            _events[id]._isEnabled = enable;
            CODECACHE_FlushCache();
        }
        return "";
    }


    /*
     * Clear the trace log.
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string ClearTraceLog()
    {
        if (_traceLog.empty())
            return "";

        _traceLog.clear();

        // Now that the trace log is cleared, there's no danger that there are any
        // references to deleted "trace" events.  So, we can really delete them.
        //
        EVENTS::iterator it = _events.begin();
        while (it != _events.end())
        {
            EVENTS::iterator thisIt = it++;
            if (thisIt->second._type == ETYPE_TRACEPOINT && thisIt->second._isDeleted)
                _events.erase(thisIt);
        }
        return "";
    }


    /*
     * Print the contents of the trace log.
     *
     *  @param[in] file     If not empty, the file to print the log to.  Otherwise, the
     *                       content of the log is returned (and printed to the debugger prompt).
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string PrintTraceLog(const std::string &file)
    {
        std::ostringstream ss;
        std::ofstream fs;
        std::ostream *os;

        // We print the log either to a file, or to the "ss" buffer.
        //
        if (!file.empty())
        {
            fs.open(file.c_str());
            os = &fs;
        }
        else
        {
            os = &ss;
        }

        // We want to pad out the "pc" field with leading zeros.
        //
        os->fill('0');
        size_t width = 2*sizeof(ADDRINT);

        for (TRACERECS::iterator it = _traceLog.begin();  it != _traceLog.end();  ++it)
        {
            const EVENT &evnt = _events[it->_id];
            (*os) << "0x" << std::hex << std::setw(width) << it->_pc << std::setw(0);
            if (!evnt._triggerMsg.empty())
                (*os) << ": " << evnt._triggerMsg;
            if (REG_valid(evnt._reg))
                *os << ": " << GetRegName(evnt._reg) << " = 0x" << std::hex << it->_regValue;
            (*os) << "\n";
        }

        // If printing to the debugger prompt, this returns the output.  If not, the output
        // is flushed to the file when the 'fs' goes out of scope and the return statement
        // returns the empty string.
        //
        return ss.str();
    }


    /*
     * Parse an event ID and check that it is valid.
     *
     *  @param[in] type     The type of event that this ID should correspond to.
     *  @param[in] idStr    The candidate ID.
     *  @param[out] id      On success, receives the ID.
     *
     * @return  On success, the empty string.  On failure, an error message.
     */
    std::string ValidateId(ETYPE type, const std::string &idStr, unsigned *id)
    {
        if (!ParseNumber(idStr, id))
        {
            std::ostringstream os;
            os << "Invalid " << GetEventName(type) << " ID " << idStr << "\n";
            return os.str();
        }

        EVENTS::iterator it = _events.find(*id);
        if (it == _events.end() || it->second._type != type ||
            (type == ETYPE_TRACEPOINT && it->second._isDeleted))
        {
            std::ostringstream os;
            os << "Invalid " << GetEventName(type) << " ID " << idStr << "\n";
            return os.str();
        }

        return "";
    }


    /*
     * Get the name of an event type.
     *
     *  @param[in] type     The event type.
     *
     * @return  The name for \a type.
     */
    std::string GetEventName(ETYPE type)
    {
        switch (type)
        {
        case ETYPE_BREAKPOINT:
            return "breakpoint";
        case ETYPE_TRACEPOINT:
            return "tracepoint";
        default:
            ASSERTX(0);
            return "";
        }
    }


    /*
     * Parse an event with trigger type TRIGGER_AT.
     *
     *  @param[in] type     The type of event.
     *  @param[in] pcStr    The trigger's PC address.
     *  @param[in] regStr   If not empty, the register to trace.
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string ParseTriggerAtEvent(ETYPE type, const std::string &pcStr, const std::string &regStr)
    {
        ADDRINT pc;
        if (!ParseNumber(pcStr, &pc))
        {
            std::ostringstream os;            
            os << "Invalid address " << pcStr << "\n";
            return os.str();
        }

        REG reg = REG_INVALID();
        if (type == ETYPE_TRACEPOINT && !regStr.empty())
        {
            reg = ParseRegName(regStr);
            if (!REG_valid(reg))
            {
                std::ostringstream os;            
                os << "Invalid register " << regStr << "\n";
                return os.str();
            }
        }

        unsigned id = _nextEventId++;

        std::ostringstream os;
        os << "at 0x" << std::hex << pc;

        EVENT evnt;
        std::string ret;
        if (type == ETYPE_BREAKPOINT)
        {
            std::ostringstream osTrigger;
            osTrigger << "Triggered breakpoint #" << std::dec << id << ": break " << os.str();
            evnt._triggerMsg = osTrigger.str();

            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  break " << os.str();
            evnt._listMsg = osList.str();

            ret = "Breakpoint " + osList.str() + "\n";
        }
        else
        {
            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  trace";
            if (REG_valid(reg))
                osList << " " << GetRegName(reg);
            osList << " " << os.str();
            evnt._listMsg = osList.str();

            evnt._triggerMsg = "";
            evnt._reg = reg;
            evnt._isDeleted = FALSE;
            evnt._isEnabled = TRUE;

            ret = "Tracepoint " + osList.str() + "\n";
        }

        evnt._type = type;
        evnt._trigger = TRIGGER_AT;
        evnt._pc = pc;
        _events.insert(std::make_pair(id, evnt));
        CODECACHE_FlushCache();
        return ret;
    }


    /*
     * Parse an event with trigger type TRIGGER_STORE_TO.
     *
     *  @param[in] type     The type of event.
     *  @param[in] addrStr  The trigger's store address.
     *  @param[in] regStr   If not empty, the register to trace.
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string ParseTriggerStoreToEvent(ETYPE type, const std::string &addrStr, const std::string &regStr)
    {
        ADDRINT addr;
        if (!ParseNumber(addrStr, &addr))
        {
            std::ostringstream os;            
            os << "Invalid address " << addrStr << "\n";
            return os.str();
        }

        REG reg = REG_INVALID();
        if (type == ETYPE_TRACEPOINT && !regStr.empty())
        {
            reg = ParseRegName(regStr);
            if (!REG_valid(reg))
            {
                std::ostringstream os;            
                os << "Invalid register " << regStr << "\n";
                return os.str();
            }
        }

        unsigned id = _nextEventId++;

        std::ostringstream os;
        os << "if store to 0x" << std::hex << addr;

        EVENT evnt;
        std::string ret;
        if (type == ETYPE_BREAKPOINT)
        {
            std::ostringstream osTrigger;
            osTrigger << "Triggered breakpoint #" << std::dec << id << ": break " << os.str();
            evnt._triggerMsg = osTrigger.str();

            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  break " << os.str();
            evnt._listMsg = osList.str();

            ret = "Breakpoint " + osList.str() + "\n";
        }
        else
        {
            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  trace";
            if (REG_valid(reg))
                osList << " " << GetRegName(reg);
            osList << " " << os.str();
            evnt._listMsg = osList.str();

            evnt._triggerMsg = os.str();
            evnt._reg = reg;
            evnt._isDeleted = FALSE;
            evnt._isEnabled = TRUE;

            ret = "Tracepoint " + osList.str() + "\n";
        }

        evnt._type = type;
        evnt._trigger = TRIGGER_STORE_TO;
        evnt._ea = addr;
        _events.insert(std::make_pair(id, evnt));
        CODECACHE_FlushCache();
        return ret;
    }


    /*
     * Parse an event with trigger type TRIGGER_STORE_VALUE_TO.
     *
     *  @param[in] type     The type of event.
     *  @param[in] addrStr  The trigger's store address.
     *  @param[in] valueStr The trigger's store value.
     *  @param[in] regStr   If not empty, the register to trace.
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string ParseTriggerStoreValueToEvent(ETYPE type, const std::string &addrStr, const std::string &valueStr,
        const std::string &regStr)
    {
        ADDRINT addr;
        if (!ParseNumber(addrStr, &addr))
        {
            std::ostringstream os;            
            os << "Invalid address " << addrStr << "\n";
            return os.str();
        }

        UINT64 value = 0;
        if (!ParseNumber(valueStr, &value))
        {
            std::ostringstream os;            
            os << "Invalid value " << valueStr << "\n";
            return os.str();
        }

        REG reg = REG_INVALID();
        if (type == ETYPE_TRACEPOINT && !regStr.empty())
        {
            reg = ParseRegName(regStr);
            if (!REG_valid(reg))
            {
                std::ostringstream os;            
                os << "Invalid register " << regStr << "\n";
                return os.str();
            }
        }

        unsigned id = _nextEventId++;

        std::ostringstream os;
        os << "after store to 0x" << std::hex << addr << " == 0x" << std::hex << value;

        EVENT evnt;
        std::string ret;
        if (type == ETYPE_BREAKPOINT)
        {
            std::ostringstream osTrigger;
            osTrigger << "Triggered breakpoint #" << std::dec << id << ": break " << os.str();
            evnt._triggerMsg = osTrigger.str();

            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  break " << os.str();
            evnt._listMsg = osList.str();

            ret = "Breakpoint " + osList.str() + "\n";
        }
        else
        {
            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  trace";
            if (REG_valid(reg))
                osList << " " << GetRegName(reg);
            osList << " " << os.str();
            evnt._listMsg = osList.str();

            evnt._triggerMsg = os.str();
            evnt._reg = reg;
            evnt._isDeleted = FALSE;
            evnt._isEnabled = TRUE;

            ret = "Tracepoint " + osList.str() + "\n";
        }

        evnt._type = type;
        evnt._trigger = TRIGGER_STORE_VALUE_TO;
        evnt._storeValueTo._ea = addr;
        evnt._storeValueTo._value = value;
        _events.insert(std::make_pair(id, evnt));
        CODECACHE_FlushCache();
        return ret;
    }


    /*
     * Parse an event with trigger type TRIGGER_JUMP_TO.
     *
     *  @param[in] type     The type of event.
     *  @param[in] addrStr  The trigger's jump address.
     *  @param[in] regStr   If not empty, the register to trace.
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string ParseTriggerJumpToEvent(ETYPE type, const std::string &addrStr, const std::string &regStr)
    {
        ADDRINT addr;
        if (!ParseNumber(addrStr, &addr))
        {
            std::ostringstream os;            
            os << "Invalid address " << addrStr << "\n";
            return os.str();
        }

        REG reg = REG_INVALID();
        if (type == ETYPE_TRACEPOINT && !regStr.empty())
        {
            reg = ParseRegName(regStr);
            if (!REG_valid(reg))
            {
                std::ostringstream os;            
                os << "Invalid register " << regStr << "\n";
                return os.str();
            }
        }

        unsigned id = _nextEventId++;

        std::ostringstream os;
        os << "if jump to 0x" << std::hex << addr;

        EVENT evnt;
        std::string ret;
        if (type == ETYPE_BREAKPOINT)
        {
            std::ostringstream osTrigger;
            osTrigger << "Triggered breakpoint #" << std::dec << id << ": break " << os.str();
            evnt._triggerMsg = osTrigger.str();

            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  break " << os.str();
            evnt._listMsg = osList.str();

            ret = "Breakpoint " + osList.str() + "\n";
        }
        else
        {
            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  trace";
            if (REG_valid(reg))
                osList << " " << GetRegName(reg);
            osList << " " << os.str();
            evnt._listMsg = osList.str();

            evnt._triggerMsg = os.str();
            evnt._reg = reg;
            evnt._isDeleted = FALSE;
            evnt._isEnabled = TRUE;

            ret = "Tracepoint " + osList.str() + "\n";
        }

        evnt._type = type;
        evnt._trigger = TRIGGER_JUMP_TO;
        evnt._pc = addr;
        _events.insert(std::make_pair(id, evnt));
        CODECACHE_FlushCache();
        return ret;
    }


    /*
     * Parse an event with trigger type TRIGGER_REG_IS.
     *
     *  @param[in] type         The type of event.
     *  @param[in] pcStr        The trigger's PC address.
     *  @param[in] regCheckStr  The trigger's register name.
     *  @param[in] valueStr     The trigger's register value.
     *  @param[in] regTraceStr  If not empty, the register to trace.
     *
     * @return  A string to return to the debugger prompt.
     */
    std::string ParseTriggerRegIsEvent(ETYPE type, const std::string &pcStr, const std::string &regCheckStr,
        const std::string &valueStr, const std::string &regTraceStr)
    {
        ADDRINT pc;
        if (!ParseNumber(pcStr, &pc))
        {
            std::ostringstream os;            
            os << "Invalid address " << pcStr << "\n";
            return os.str();
        }

        REG regCheck = ParseRegName(regCheckStr);
        if (!REG_valid(regCheck))
        {
            std::ostringstream os;            
            os << "Invalid register " << regCheckStr << "\n";
            return os.str();
        }

        ADDRINT value;
        if (!ParseNumber(valueStr, &value))
        {
            std::ostringstream os;            
            os << "Invalid value " << valueStr << "\n";
            return os.str();
        }

        REG regTrace = REG_INVALID();
        if (type == ETYPE_TRACEPOINT && !regTraceStr.empty())
        {
            regTrace = ParseRegName(regTraceStr);
            if (!REG_valid(regTrace))
            {
                std::ostringstream os;            
                os << "Invalid register " << regTraceStr << "\n";
                return os.str();
            }
        }

        unsigned id = _nextEventId++;

        std::ostringstream os;
        os << " at 0x" << std::hex << pc << " if " << GetRegName(regCheck) << " == 0x" << std::hex << value;

        EVENT evnt;
        std::string ret;
        if (type == ETYPE_BREAKPOINT)
        {
            std::ostringstream osTrigger;
            osTrigger << "Triggered breakpoint #" << std::dec << id << ": break " << os.str();
            evnt._triggerMsg = osTrigger.str();

            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  break " << os.str();
            evnt._listMsg = osList.str();

            ret = "Breakpoint " + osList.str() + "\n";
        }
        else
        {
            std::ostringstream osList;
            osList << "#" << std::dec << id << ":  trace";
            if (REG_valid(regTrace))
                osList << " " << GetRegName(regTrace);
            osList << " " << os.str();
            evnt._listMsg = osList.str();

            evnt._triggerMsg = os.str();
            evnt._reg = regTrace;
            evnt._isDeleted = FALSE;
            evnt._isEnabled = TRUE;

            ret = "Tracepoint " + osList.str() + "\n";
        }

        evnt._type = type;
        evnt._trigger = TRIGGER_REG_IS;
        evnt._regIs._pc = pc;
        evnt._regIs._reg = regCheck;
        evnt._regIs._value = value;
        _events.insert(std::make_pair(id, evnt));
        CODECACHE_FlushCache();
        return ret;
    }


    /*
     * Print an error message.
     *
     *  @param[in] message  Text of the error message.
     */
    void PrintError(const std::string &message)
    {
        PIN_WriteErrorMessage(message.c_str(), 1000, PIN_ERR_NONFATAL, 0);
    }


    /* -------------- Instrumentation Functions -------------- */


    /*
     * Pin call-back to instrument a trace.
     *
     *  @param[in] trace    Trace to instrument.
     *  @param[in] vme      Pointer to DEBUGGER_SHELL instance.
     */
    static VOID InstrumentTrace(TRACE trace, void *vme)
    {
        DEBUGGER_SHELL *me = static_cast<DEBUGGER_SHELL *>(vme);

        for (BBL bbl = TRACE_BblHead(trace);  BBL_Valid(bbl);  bbl = BBL_Next(bbl))
        {
            for (INS ins = BBL_InsHead(bbl);  INS_Valid(ins);  ins = INS_Next(ins))
            {
                // Insert breakpoints before tracepoints because we don't want a tracepoint
                // to log anything until after execution resumes from the breakpoint.
                //
                BOOL insertSkipClear = FALSE;
                BOOL insertRecordEa = FALSE;
                me->InstrumentIns(ins, bbl, ETYPE_BREAKPOINT, &insertSkipClear, &insertRecordEa);
                me->InstrumentIns(ins, bbl, ETYPE_TRACEPOINT, &insertSkipClear, &insertRecordEa);

                // If there are any events with TRIGGER_STORE_VALUE_TO, record the store's effective address
                // at IPOINT_BEFORE.  We only need to do this once, even if there are many such events.
                //
                if (insertRecordEa)
                {
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ReturnAddrint,
                        IARG_CALL_ORDER, me->_clientArgs._callOrderBefore, IARG_FAST_ANALYSIS_CALL,
                        IARG_MEMORYWRITE_EA, IARG_RETURN_REGS, me->_regRecordEa, IARG_END);
                }

                // If there are any "before" breakpoints, we need to clear the REG_SKIP_ONE
                // virtual register.
                //
                if (insertSkipClear)
                    me->InsertSkipClear(ins);
            }
        }
    }


    /*
     * Instrument an instruction.
     *
     *  @param[in] ins                  Instruction to instrument.
     *  @param[in] bbl                  Basic block containing \a ins.
     *  @param[in] type                 Only insert instrumentation for events of this type.
     *  @param[out] insertSkipClear     If this instructions needs instrumentation to clear the
     *                                   REG_SKIP_ONE register, \a insertSkipClear is set TRUE.
     *  @param[out] insertRecordEa      If this instructions needs instrumentation to record a
     *                                   store's effective address, \a insertRecordEa is set TRUE.
     */
    VOID InstrumentIns(INS ins, BBL bbl, ETYPE type, BOOL *insertSkipClear, BOOL *insertRecordEa)
    {
        for (EVENTS::iterator it = _events.begin();  it != _events.end();  ++it)
        {
            if (it->second._type != type)
                continue;
            if (type == ETYPE_TRACEPOINT && (it->second._isDeleted || !it->second._isEnabled))
                continue;

            switch (it->second._trigger)
            {
            case TRIGGER_AT:
                if (INS_Address(ins) == it->second._pc)
                {
                    if (type == ETYPE_BREAKPOINT)
                    {
                        InsertBreakpoint(ins, bbl, FALSE, IPOINT_BEFORE, it->second);
                        *insertSkipClear = TRUE;
                    }
                    else
                    {
                        InsertTracepoint(ins, bbl, FALSE, IPOINT_BEFORE, it->first, it->second);
                    }
                }
                break;

            case TRIGGER_STORE_TO:
                if (INS_IsMemoryWrite(ins))
                {
                    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)CheckAddrint,
                        IARG_CALL_ORDER, _clientArgs._callOrderBefore,
                        IARG_FAST_ANALYSIS_CALL,
                        IARG_MEMORYWRITE_EA, IARG_ADDRINT, it->second._ea, IARG_END);
                    if (type == ETYPE_BREAKPOINT)
                    {
                        InsertBreakpoint(ins, bbl, TRUE, IPOINT_BEFORE, it->second);
                        *insertSkipClear = TRUE;
                    }
                    else
                    {
                        InsertTracepoint(ins, bbl, TRUE, IPOINT_BEFORE, it->first, it->second);
                    }
                }
                break;

            case TRIGGER_STORE_VALUE_TO:
                if (INS_IsMemoryWrite(ins))
                {
                    *insertRecordEa = TRUE;
                    InstrumentStoreValueTo(ins, bbl, it->first, it->second);
                }
                break;

            case TRIGGER_JUMP_TO:
                if (INS_IsBranchOrCall(ins))
                {
                    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)CheckAddrint,
                        IARG_CALL_ORDER, _clientArgs._callOrderBefore,
                        IARG_FAST_ANALYSIS_CALL,
                        IARG_BRANCH_TARGET_ADDR, IARG_ADDRINT, it->second._pc, IARG_END);
                    if (type == ETYPE_BREAKPOINT)
                    {
                        InsertBreakpoint(ins, bbl, TRUE, IPOINT_BEFORE, it->second);
                        *insertSkipClear = TRUE;
                    }
                    else
                    {
                        InsertTracepoint(ins, bbl, TRUE, IPOINT_BEFORE, it->first, it->second);
                    }
                }
                break;

            case TRIGGER_REG_IS:
                if (INS_Address(ins) == it->second._regIs._pc)
                {
                    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)CheckAddrint,
                        IARG_CALL_ORDER, _clientArgs._callOrderBefore,
                        IARG_FAST_ANALYSIS_CALL,
                        IARG_REG_VALUE, it->second._regIs._reg,
                        IARG_ADDRINT, it->second._regIs._value,
                        IARG_END);
                    if (type == ETYPE_BREAKPOINT)
                    {
                        InsertBreakpoint(ins, bbl, TRUE, IPOINT_BEFORE, it->second);
                        *insertSkipClear = TRUE;
                    }
                    else
                    {
                        InsertTracepoint(ins, bbl, TRUE, IPOINT_BEFORE, it->first, it->second);
                    }
                }
                break;
            }
        }
    }


    /*
     * Instrument a store instruction with a TRIGGER_STORE_VALUE_TO event.
     *
     *  @param[in] ins      The store instruction.
     *  @param[in] bbl      The basic block containing \a ins.
     *  @param[in] id       The event ID.
     *  @param[in] evnt     The event descrption.
     */
    VOID InstrumentStoreValueTo(INS ins, BBL bbl, unsigned id, const EVENT &evnt)
    {
        switch (INS_MemoryWriteSize(ins))
        {
        case 1:
            InstrumentStoreValueToForSize<UINT8>(ins, bbl, id, evnt, (AFUNPTR)CheckStore8);
            break;
        case 2:
            InstrumentStoreValueToForSize<UINT16>(ins, bbl, id, evnt, (AFUNPTR)CheckStore16);
            break;
        case 4:
            InstrumentStoreValueToForSize<UINT32>(ins, bbl, id, evnt, (AFUNPTR)CheckStore32);
            break;
        case 8:
            if (sizeof(ADDRINT) >= sizeof(UINT64))
                InstrumentStoreValueToForSize<UINT64>(ins, bbl, id, evnt, (AFUNPTR)CheckStoreAddrint);
            else
                InstrumentStoreValue64HiLo(ins, bbl, id, evnt);
            break;
        }
    }


    /*
     * Instrument a store instruction with a TRIGGER_STORE_VALUE_TO event.
     *
     *  @tparam UINTX           One of the UINT types, matching the size of the store.
     *                           There is an assumption that sizeof(UINTX) <= sizeof(ADDRINT).
     *  @param[in] ins          The store instruction.
     *  @param[in] bbl          The basic block containing \a ins.
     *  @param[in] id           The event ID.
     *  @param[in] evnt         The event descrption.
     *  @param[in] CheckStoreX  One of the CheckStore() analysis functions, matching the
     *                           size of the store.
     */
    template<typename UINTX> VOID InstrumentStoreValueToForSize(INS ins, BBL bbl,
        unsigned id, const EVENT &evnt, AFUNPTR CheckStoreX)
    {
        UINT64 value = evnt._storeValueTo._value;
        ETYPE type = evnt._type;

        if (static_cast<UINTX>(value) == value)
        {
            if (INS_HasFallThrough(ins))
            {
                INS_InsertIfCall(ins, IPOINT_AFTER, CheckStoreX,
                    IARG_CALL_ORDER, _clientArgs._callOrderAfter, IARG_FAST_ANALYSIS_CALL,
                    IARG_REG_VALUE, _regRecordEa,
                    IARG_ADDRINT, evnt._storeValueTo._ea,
                    IARG_ADDRINT, static_cast<ADDRINT>(value),
                    IARG_END);
                if (type == ETYPE_BREAKPOINT)
                    InsertBreakpoint(ins, bbl, TRUE, IPOINT_AFTER, evnt);
                else
                    InsertTracepoint(ins, bbl, TRUE, IPOINT_AFTER, id, evnt);
            }
            if (INS_IsBranchOrCall(ins))
            {
                INS_InsertIfCall(ins, IPOINT_TAKEN_BRANCH, CheckStoreX,
                    IARG_CALL_ORDER, _clientArgs._callOrderAfter, IARG_FAST_ANALYSIS_CALL,
                    IARG_REG_VALUE, _regRecordEa,
                    IARG_ADDRINT, evnt._storeValueTo._ea,
                    IARG_ADDRINT, static_cast<ADDRINT>(value),
                    IARG_END);
                if (type == ETYPE_BREAKPOINT)
                    InsertBreakpoint(ins, bbl, TRUE, IPOINT_TAKEN_BRANCH, evnt);
                else
                    InsertTracepoint(ins, bbl, TRUE, IPOINT_TAKEN_BRANCH, id, evnt);
            }
        }
    }


    /*
     * Instrument a 64-bit store instruction with a TRIGGER_STORE_VALUE_TO event.
     * The value is checked using high and low ADDRINT parts (where ADDRINT is assumed
     * to be 32-bits).
     *
     *  @param[in] ins          The store instruction.
     *  @param[in] bbl          The basic block containing \a ins.
     *  @param[in] id           The event ID.
     *  @param[in] evnt         The event descrption.
     */
    VOID InstrumentStoreValue64HiLo(INS ins, BBL bbl, unsigned id, const EVENT &evnt)
    {
        UINT64 value = evnt._storeValueTo._value;
        ETYPE type = evnt._type;
        ADDRINT hi = static_cast<ADDRINT>(value >> 32);
        ADDRINT lo = static_cast<ADDRINT>(value);

        if (INS_HasFallThrough(ins))
        {
            INS_InsertIfCall(ins, IPOINT_AFTER, (AFUNPTR)CheckStore64,
                IARG_CALL_ORDER, _clientArgs._callOrderAfter, IARG_FAST_ANALYSIS_CALL,
                IARG_REG_VALUE, _regRecordEa,
                IARG_ADDRINT, evnt._storeValueTo._ea,
                IARG_ADDRINT, hi, IARG_ADDRINT, lo,
                IARG_END);
            if (type == ETYPE_BREAKPOINT)
                InsertBreakpoint(ins, bbl, TRUE, IPOINT_AFTER, evnt);
            else
                InsertTracepoint(ins, bbl, TRUE, IPOINT_AFTER, id, evnt);
        }
        if (INS_IsBranchOrCall(ins))
        {
            INS_InsertIfCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)CheckStore64,
                IARG_CALL_ORDER, _clientArgs._callOrderAfter, IARG_FAST_ANALYSIS_CALL,
                IARG_REG_VALUE, _regRecordEa,
                IARG_ADDRINT, evnt._storeValueTo._ea,
                IARG_ADDRINT, hi, IARG_ADDRINT, lo,
                IARG_END);
            if (type == ETYPE_BREAKPOINT)
                InsertBreakpoint(ins, bbl, TRUE, IPOINT_TAKEN_BRANCH, evnt);
            else
                InsertTracepoint(ins, bbl, TRUE, IPOINT_TAKEN_BRANCH, id, evnt);
        }
    }


    /*
     * Add the instrumentation for a call to the breakpoint analysis routine.
     *
     *  @param[in] ins      The instruction being instrumented.
     *  @param[in] bbl      The basic block containing \a ins.
     *  @param[in] isThen   TRUE if this should be a "then" instrumentation call.
     *  @param[in] ipoint   Where to place the instrumentation.
     *  @param[in] evnt     The ETYPE_BREAKPOINT event description.
     */
    VOID InsertBreakpoint(INS ins, BBL bbl, BOOL isThen, IPOINT ipoint, const EVENT &evnt)
    {
        ASSERTX(evnt._type == ETYPE_BREAKPOINT);

        // Breakpoints alwas use "then" instrumentation currently.  If that ever changes,
        // we need to extend the IDEBUGGER_SHELL_INSTRUMENTOR interface to communicate "then"
        // vs. non-"then" instrumentation to the client.
        //
        ASSERTX(isThen);

        if (_clientArgs._overrideInstrumentation)
        {
            if (ipoint == IPOINT_BEFORE)
            {
                _clientArgs._overrideInstrumentation->InsertBreakpointBefore(ins, bbl,
                    _clientArgs._callOrderBefore, evnt._triggerMsg);
            }
            else
            {
                _clientArgs._overrideInstrumentation->InsertBreakpointAfter(ins, bbl,
                    ipoint, _clientArgs._callOrderAfter, evnt._triggerMsg);
            }
            return;
        }

        if (ipoint == IPOINT_BEFORE)
        {
            INS_InsertThenCall(ins, ipoint, (AFUNPTR)TriggerBreakpointBefore,
                IARG_CALL_ORDER, _clientArgs._callOrderBefore,
                IARG_CONTEXT, IARG_THREAD_ID,
                IARG_UINT32, static_cast<UINT32>(_regSkipOne),
                IARG_PTR, evnt._triggerMsg.c_str(),
                IARG_END);
        }
        else
        {
            INS_InsertThenCall(ins, ipoint, (AFUNPTR)TriggerBreakpointAfter,
                IARG_CALL_ORDER, _clientArgs._callOrderAfter,
                IARG_CONTEXT, IARG_INST_PTR, IARG_THREAD_ID,
                IARG_PTR, evnt._triggerMsg.c_str(),
                IARG_END);
        }
    }


    /*
     * Add the instrumentation for a call to the tracepoint analysis routine.
     *
     *  @param[in] ins      The instruction being instrumented.
     *  @param[in] bbl      The basic block containing \a ins.
     *  @param[in] isThen   TRUE if this should be a "then" instrumentation call.
     *  @param[in] ipoint   Where to place the instrumentation.
     *  @param[in] id       The event ID.
     *  @param[in] evnt     The ETYPE_TRACEPOINT event description.
     */
    VOID InsertTracepoint(INS ins, BBL bbl, BOOL isThen, IPOINT ipoint, unsigned id, const EVENT &evnt)
    {
        ASSERTX(evnt._type == ETYPE_TRACEPOINT);

        CALL_ORDER order;
        if (ipoint == IPOINT_BEFORE)
            order = _clientArgs._callOrderBefore;
        else
            order = _clientArgs._callOrderAfter;

        if (REG_valid(evnt._reg))
        {
            if (isThen)
            {
                INS_InsertThenCall(ins, ipoint, (AFUNPTR)RecordTracepointAndReg,
                    IARG_CALL_ORDER, order,
                    IARG_PTR, this,
                    IARG_UINT32, static_cast<UINT32>(id),
                    IARG_INST_PTR,
                    IARG_REG_VALUE, evnt._reg,
                    IARG_END);
            }
            else
            {
                INS_InsertCall(ins, ipoint, (AFUNPTR)RecordTracepointAndReg,
                    IARG_CALL_ORDER, order,
                    IARG_PTR, this,
                    IARG_UINT32, static_cast<UINT32>(id),
                    IARG_INST_PTR,
                    IARG_REG_VALUE, evnt._reg,
                    IARG_END);
            }
        }
        else
        {
            if (isThen)
            {
                INS_InsertThenCall(ins, ipoint, (AFUNPTR)RecordTracepoint,
                    IARG_CALL_ORDER, order,
                    IARG_PTR, this,
                    IARG_UINT32, static_cast<UINT32>(id),
                    IARG_INST_PTR,
                    IARG_END);
            }
            else
            {
                INS_InsertCall(ins, ipoint, (AFUNPTR)RecordTracepoint,
                    IARG_CALL_ORDER, order,
                    IARG_PTR, this,
                    IARG_UINT32, static_cast<UINT32>(id),
                    IARG_INST_PTR,
                    IARG_END);
            }
        }
    }


    /*
     * Insert instrumentation after an instruction to clear the "skip one" flag.
     *
     *  @param[in] ins  The instruction.
     */
    VOID InsertSkipClear(INS ins)
    {
        if (INS_HasFallThrough(ins))
        {
            INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)ReturnZero,
                IARG_CALL_ORDER, _clientArgs._callOrderAfter,
                IARG_FAST_ANALYSIS_CALL,
                IARG_RETURN_REGS, _regSkipOne, IARG_END);
        }
        if (INS_IsBranchOrCall(ins))
        {
            INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)ReturnZero,
                IARG_CALL_ORDER, _clientArgs._callOrderAfter,
                IARG_FAST_ANALYSIS_CALL,
                IARG_RETURN_REGS, _regSkipOne, IARG_END);
        }
    }


    /* -------------- Analysis Functions -------------- */


    /*
     * These are all analysis routines that check for a trigger condition.  They
     * should all be fast, and we expect Pin to inline them.
     */
    static ADDRINT PIN_FAST_ANALYSIS_CALL CheckAddrint(ADDRINT a, ADDRINT b)
    {
        return (a == b);
    }

    static ADDRINT PIN_FAST_ANALYSIS_CALL CheckStore8(ADDRINT ea, ADDRINT expect, ADDRINT value)
    {
        return (ea == expect) && (*reinterpret_cast<UINT8 *>(ea) == static_cast<UINT8>(value));
    }

    static ADDRINT PIN_FAST_ANALYSIS_CALL CheckStore16(ADDRINT ea, ADDRINT expect, ADDRINT value)
    {
        return (ea == expect) && (*reinterpret_cast<UINT16 *>(ea) == static_cast<UINT16>(value));
    }

    static ADDRINT PIN_FAST_ANALYSIS_CALL CheckStore32(ADDRINT ea, ADDRINT expect, ADDRINT value)
    {
        return (ea == expect) && (*reinterpret_cast<UINT32 *>(ea) == static_cast<UINT32>(value));
    }

    static ADDRINT PIN_FAST_ANALYSIS_CALL CheckStoreAddrint(ADDRINT ea, ADDRINT expect, ADDRINT value)
    {
        return (ea == expect) && (*reinterpret_cast<ADDRINT *>(ea) == value);
    }

    static ADDRINT PIN_FAST_ANALYSIS_CALL CheckStore64(ADDRINT ea, ADDRINT expect,
        ADDRINT valueHi, ADDRINT valueLo)
    {
        UINT64 value = (static_cast<UINT64>(valueHi) << 32) | valueLo;
        return (ea == expect) && (*reinterpret_cast<UINT64 *>(ea) == value);
    }


    /*
     * These are utility analysis routines that return values to be stored in a Pin virtual
     * register.  They are meant to be used with IARG_RETURN_REGS.
     */
    static ADDRINT PIN_FAST_ANALYSIS_CALL ReturnZero()
    {
        return 0;
    }

    static ADDRINT PIN_FAST_ANALYSIS_CALL ReturnAddrint(ADDRINT a)
    {
        return a;
    }


    /*
     * Trigger a breakpoint that occurs before an instruction.
     *
     *  @param[in] ctxt         Register state before the instruction.
     *  @param[in] tid          The calling thread.
     *  @param[in] regSkipOne   The REG_SKIP_ONE Pin virtual register.
     *  @param[in] message      Tells what breakpoint was triggered.
     */
    static VOID TriggerBreakpointBefore(CONTEXT *ctxt, THREADID tid, UINT32 regSkipOne, const char *message)
    {
        // When we resume from the breakpoint, this analysis routine is re-executed.
        // This logic prevents the breakpoint from being re-triggered when we resume.
        // The REG_SKIP_ONE virtual register is cleared in the instruction's "after"
        // analysis function.
        //
        ADDRINT skipPc = PIN_GetContextReg(ctxt, static_cast<REG>(regSkipOne));
        ADDRINT pc = PIN_GetContextReg(ctxt, REG_INST_PTR);
        if (skipPc == pc)
            return;

        PIN_SetContextReg(ctxt, static_cast<REG>(regSkipOne), pc);
        PIN_ApplicationBreakpoint(ctxt, tid, FALSE, message);
    }


    /*
     * Trigger a breakpoint that occurs after an instruction.
     *
     *  @param[in] ctxt     Register state after the instruction (PC points to next instruction).
     *  @param[in] pc       PC of instruction that triggered the breakpoint.
     *  @param[in] tid      The calling thread.
     *  @param[in] message  Tells what breakpoint was triggered.
     */
    static VOID TriggerBreakpointAfter(CONTEXT *ctxt, ADDRINT pc, THREADID tid, const char *message)
    {
        // Note, we don't need any special logic to prevent re-triggering this breakpoint
        // when we resume because 'ctxt' points at the next instruction.  When resuming, we
        // start executing at the next instruction, so avoid re-evaluating the breakpoint
        // condition.

        // Tell the user the PC of the instruction that triggered the breakpoint because
        // the PC in 'ctxt' points at the next instruction.  Otherwise, the triggering instruction
        // might not be obvious if it was a CALL or branch instruction.
        //
        std::ostringstream os;
        os << message << "\n";
        os << "Breakpoint triggered after instruction at 0x" << std::hex << pc;

        PIN_ApplicationBreakpoint(ctxt, tid, FALSE, os.str());
    }


    /*
     * Record a tracepoint with no register value.
     *
     *  @param[in] me   Points to our DEBUGGER_SHELL object.
     *  @param[in] id   Event ID for the tracepoint description.
     *  @param[in] pc   Trigger PC for tracepoint.
     */
    static VOID RecordTracepoint(DEBUGGER_SHELL *me, UINT32 id, ADDRINT pc)
    {
        TRACEREC rec;
        rec._id = static_cast<unsigned>(id);
        rec._pc = pc;

        GetLock(&me->_traceLock, 1);
        me->_traceLog.push_back(rec);
        ReleaseLock(&me->_traceLock);
    }


    /*
     * Record a tracepoint with a register value.
     *
     *  @param[in] me           Points to our DEBUGGER_SHELL object.
     *  @param[in] id           Event ID for the tracepoint description.
     *  @param[in] pc           Trigger PC for tracepoint.
     *  @param[in] regValue     Trigger PC for tracepoint.
     */
    static VOID RecordTracepointAndReg(DEBUGGER_SHELL *me, UINT32 id, ADDRINT pc, ADDRINT regValue)
    {
        TRACEREC rec;
        rec._id = static_cast<unsigned>(id);
        rec._pc = pc;
        rec._regValue = regValue;

        GetLock(&me->_traceLock, 1);
        me->_traceLog.push_back(rec);
        ReleaseLock(&me->_traceLock);
    }
};


IDEBUGGER_SHELL *CreateDebuggerShell()
{
    DEBUGGER_SHELL *shell = new DEBUGGER_SHELL();
    if (!shell->Construct())
    {
        delete shell;
        return 0;
    }
    return shell;
}
