//Groups: @ingroup\s+(API_REF|KNOBS|IMG_BASIC_API|INS_BASIC_API|INS_INST_API|INS_BASIC_API_GEN_IA32|INS_BASIC_API_IA32|INS_BASIC_API_IPF|INS_MOD_API_GEN_IA32|SEC_BASIC_API|RTN_BASIC_API|REG_BASIC_API|REG_CPU_GENERIC|REG_CPU_IPF|REG_CPU_IA32|TRACE_BASIC_API|BBL_BASIC_API|SYM_BASIC_API|MISC_PRINT|MISC_PARSE|KNOB_API|KNOB_BASIC|KNOB_PRINT|LOCK|PIN_CONTROL|BUFFER_API|PROTO_API|PIN_PROCESS_API|PIN_THREAD_API|DEBUG_API|ERROR_FILE_BASIC|TYPE_BASE|INSTLIB|ALARM|ROGUE_EDG_BASIC_API|ROGUE_BBL_BASIC_API|ROGUE_INS_BASIC_API|ROGUE_TRACE_BASIC_API|ROGUE_CONTROL_BASIC_API|CODECACHE_API|CHILD_PROCESS_API|UTILS|IPF_UTILS_API|MISC|CONTEXT_API|PHYSICAL_CONTEXT_API|CHECKPOINTING|EXCEPTION_API|APPDEBUG_API|BUFFER_API|PROTO|INST_ARGS|INTERNAL_EXCEPTION_PRIVATE_UNDOCUMENTED|PIN_THREAD_PRIVATE|CHILD_PROCESS_INTERNAL|EDG_BASIC|BBL_BASIC|ROGUE_BASIC_API|CODECACHE_NYI_API|INS_BASIC_API_GEN_IPF|MESSAGE_TYPE|MESSAGE_BASIC|ERRFILE|MISC_BASIC|ITC_INST_API|CHECKPOINTING_PRIVATE|CONTEXT_API_UNDOC|EXCEPTION_API_UNDOC|SHADOW_PROFILING_UNDOC|UNDOCUMENTED_PIN_API|DEPRECATED_PIN_API|OPIN|TRACE_VERSIONS
/* PIN API */

/* THIS FILE IS AUTOMAGICALLY GENERATED - DO NOT CHANGE DIRECTLY*/


typedef VOID (*SIGNAL_BEFORE_CALLBACK)(THREADID threadIndex, INT32 sig, const CONTEXT *ctxtFrom,
    const CONTEXT *ctxtTo, VOID *v);

                                                                  /* DO NOT EDIT */
typedef VOID (*SIGNAL_AFTER_CALLBACK)(THREADID threadIndex, const CONTEXT *ctxtFrom,
    const CONTEXT *ctxtTo, VOID *v);

                                                                  /* DO NOT EDIT */
typedef VOID (*THREAD_BEGIN_CALLBACK)(THREADID threadIndex, VOID * sp, int flags, VOID *v);

                                                                  /* DO NOT EDIT */
typedef VOID (*THREAD_END_CALLBACK)(THREADID threadIndex, INT32 code, VOID *v);

                                                                  /* DO NOT EDIT */
extern BOOL PIN_AddSignalBeforeFunction(SIGNAL_BEFORE_CALLBACK fun, VOID *val);

                                                                  /* DO NOT EDIT */
extern BOOL PIN_AddSignalAfterFunction(SIGNAL_AFTER_CALLBACK fun, VOID *val);

                                                                  /* DO NOT EDIT */
extern VOID PIN_AddThreadBeginFunction(THREAD_BEGIN_CALLBACK fun, VOID *val);

                                                                  /* DO NOT EDIT */
extern VOID PIN_AddThreadEndFunction(THREAD_END_CALLBACK fun, VOID *val);

                                                                  /* DO NOT EDIT */
extern VOID PIN_StartProbedProgram();

                                                                  /* DO NOT EDIT */

