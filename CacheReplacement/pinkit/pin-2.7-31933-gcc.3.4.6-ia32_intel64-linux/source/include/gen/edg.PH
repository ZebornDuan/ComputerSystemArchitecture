//Groups: @ingroup\s+(API_REF|KNOBS|IMG_BASIC_API|INS_BASIC_API|INS_INST_API|INS_BASIC_API_GEN_IA32|INS_BASIC_API_IA32|INS_BASIC_API_IPF|INS_MOD_API_GEN_IA32|SEC_BASIC_API|RTN_BASIC_API|REG_BASIC_API|REG_CPU_GENERIC|REG_CPU_IPF|REG_CPU_IA32|TRACE_BASIC_API|BBL_BASIC_API|SYM_BASIC_API|MISC_PRINT|MISC_PARSE|KNOB_API|KNOB_BASIC|KNOB_PRINT|LOCK|PIN_CONTROL|BUFFER_API|PROTO_API|PIN_PROCESS_API|PIN_THREAD_API|DEBUG_API|ERROR_FILE_BASIC|TYPE_BASE|INSTLIB|ALARM|ROGUE_EDG_BASIC_API|ROGUE_BBL_BASIC_API|ROGUE_INS_BASIC_API|ROGUE_TRACE_BASIC_API|ROGUE_CONTROL_BASIC_API|CODECACHE_API|CHILD_PROCESS_API|UTILS|IPF_UTILS_API|MISC|CONTEXT_API|PHYSICAL_CONTEXT_API|CHECKPOINTING|EXCEPTION_API|APPDEBUG_API|BUFFER_API|PROTO|INST_ARGS|INTERNAL_EXCEPTION_PRIVATE_UNDOCUMENTED|PIN_THREAD_PRIVATE|CHILD_PROCESS_INTERNAL|EDG_BASIC|BBL_BASIC|ROGUE_BASIC_API|CODECACHE_NYI_API|INS_BASIC_API_GEN_IPF|MESSAGE_TYPE|MESSAGE_BASIC|ERRFILE|MISC_BASIC|ITC_INST_API|CHECKPOINTING_PRIVATE|CONTEXT_API_UNDOC|EXCEPTION_API_UNDOC|SHADOW_PROFILING_UNDOC|UNDOCUMENTED_PIN_API|DEPRECATED_PIN_API|OPIN|TRACE_VERSIONS
/* PIN API */

/* THIS FILE IS AUTOMAGICALLY GENERATED - DO NOT CHANGE DIRECTLY*/


typedef enum 
{
    EDG_TYPE_INVALID,     ///< illegal type,
    EDG_TYPE_RETURN,      ///< return edge (not used)
    EDG_TYPE_CALL,        ///< call edge (callsite -> callee)
    EDG_TYPE_LINK,        ///< link edge (callsite->returnsite)
    EDG_TYPE_COMP,        ///< compensation edge (not used)
    EDG_TYPE_FALSE,       ///< generic fall through edge (predicated)
    EDG_TYPE_BRANCH,      ///< branch edge
    EDG_TYPE_SWITCH,      ///< switch edge
    EDG_TYPE_PREFETCH,    ///< prefetch edge
    EDG_TYPE_EXCEPTION,   ///< EH edge (callsite -> landing pad )
    EDG_TYPE_NORMAL,      ///< fall through edge (not predicated)
    EDG_TYPE_INDIRECT,    ///< indirect branch (hash lookup -> stub)
    EDG_TYPE_LAST
}EDG_TYPE;

                                                                  /* DO NOT EDIT */
typedef enum 
{
    EDG_STATE_INVALID,
    EDG_STATE_LAST
}EDG_STATE;

                                                                  /* DO NOT EDIT */
extern EDG BBL_SuccEdgFind(BBL src,  EDG_TYPE type);

                                                                  /* DO NOT EDIT */
extern BOOL EDG_InterProcedural(EDG e);

                                                                  /* DO NOT EDIT */
extern BOOL EDG_IsFallthrough(EDG edg);

                                                                  /* DO NOT EDIT */
extern EDG BBL_PredEdgFind(BBL dst,  EDG_TYPE type);

                                                                  /* DO NOT EDIT */
extern EDG BBL_SuccEdgFind(BBL src,  BBL dst);

                                                                  /* DO NOT EDIT */
extern EDG BBL_SuccEdgFindWithType(BBL src,  BBL dst, EDG_TYPE type);

                                                                  /* DO NOT EDIT */
extern EDG BBL_SuccEdgFindPcRelative(BBL bbl);

                                                                  /* DO NOT EDIT */
extern UINT32 BBL_NumPreds(BBL bbl);

                                                                  /* DO NOT EDIT */
extern UINT32 BBL_NumPredsInterproc(BBL bbl);

                                                                  /* DO NOT EDIT */
extern UINT32 BBL_NumPredsInterprocNonCall(BBL bbl);

                                                                  /* DO NOT EDIT */
extern UINT32 BBL_NumSuccs(BBL bbl);

                                                                  /* DO NOT EDIT */

