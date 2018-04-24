//Groups: @ingroup\s+(API_REF|KNOBS|IMG_BASIC_API|INS_BASIC_API|INS_INST_API|INS_BASIC_API_GEN_IA32|INS_BASIC_API_IA32|INS_BASIC_API_IPF|INS_MOD_API_GEN_IA32|SEC_BASIC_API|RTN_BASIC_API|REG_BASIC_API|REG_CPU_GENERIC|REG_CPU_IPF|REG_CPU_IA32|TRACE_BASIC_API|BBL_BASIC_API|SYM_BASIC_API|MISC_PRINT|MISC_PARSE|KNOB_API|KNOB_BASIC|KNOB_PRINT|LOCK|PIN_CONTROL|BUFFER_API|PROTO_API|PIN_PROCESS_API|PIN_THREAD_API|DEBUG_API|ERROR_FILE_BASIC|TYPE_BASE|INSTLIB|ALARM|ROGUE_EDG_BASIC_API|ROGUE_BBL_BASIC_API|ROGUE_INS_BASIC_API|ROGUE_TRACE_BASIC_API|ROGUE_CONTROL_BASIC_API|CODECACHE_API|CHILD_PROCESS_API|UTILS|IPF_UTILS_API|MISC|CONTEXT_API|PHYSICAL_CONTEXT_API|CHECKPOINTING|EXCEPTION_API|APPDEBUG_API|BUFFER_API|PROTO|INST_ARGS|INTERNAL_EXCEPTION_PRIVATE_UNDOCUMENTED|PIN_THREAD_PRIVATE|CHILD_PROCESS_INTERNAL|EDG_BASIC|BBL_BASIC|ROGUE_BASIC_API|CODECACHE_NYI_API|INS_BASIC_API_GEN_IPF|MESSAGE_TYPE|MESSAGE_BASIC|ERRFILE|MISC_BASIC|ITC_INST_API|CHECKPOINTING_PRIVATE|CONTEXT_API_UNDOC|EXCEPTION_API_UNDOC|SHADOW_PROFILING_UNDOC|UNDOCUMENTED_PIN_API|DEPRECATED_PIN_API|OPIN|TRACE_VERSIONS
/* PIN API */

/* THIS FILE IS AUTOMAGICALLY GENERATED - DO NOT CHANGE DIRECTLY*/


typedef BOOL (*DEBUG_INTERPRETER_CALLBACK)(THREADID threadIndex, CONTEXT *ctxt, const std::string &cmd,
    std::string *reply, VOID *v);

                                                                  /* DO NOT EDIT */
extern VOID PIN_AddDebugInterpreter(DEBUG_INTERPRETER_CALLBACK fun, VOID *val);

                                                                  /* DO NOT EDIT */
extern VOID PIN_RemoveDebugInterpreter(DEBUG_INTERPRETER_CALLBACK fun);

                                                                  /* DO NOT EDIT */
extern VOID PIN_ApplicationBreakpoint(const CONTEXT *ctxt, THREADID tid, BOOL waitIfNoDebugger,
    const std::string &msg);

                                                                  /* DO NOT EDIT */
extern DEBUG_STATUS PIN_GetDebugStatus();

                                                                  /* DO NOT EDIT */
extern BOOL PIN_GetDebugConnectionInfo(DEBUG_CONNECTION_INFO *info);

                                                                  /* DO NOT EDIT */
extern BOOL PIN_WaitForDebuggerToConnect(unsigned timeout);

                                                                  /* DO NOT EDIT */
extern BOOL PIN_AddEmulatedRegisterFunction(const std::string &emulatorName, unsigned numRegisters,
    const EMULATED_REGISTER *registerDescriptions, GET_EMULATED_REGISTER_CALLBACK getFun,
    SET_EMULATED_REGISTER_CALLBACK setFun, VOID *val);

                                                                  /* DO NOT EDIT */

