##
## PIN tools
##

##############################################################
#
# Here are some things you might want to configure
#
##############################################################

TARGET_COMPILER?=gnu
ifdef OS
    ifeq (${OS},Windows_NT)
        TARGET_COMPILER=ms
    endif
endif

##############################################################
#
# include *.config files
#
##############################################################

ifeq ($(TARGET_COMPILER),gnu)
    include ../makefile.gnu.config
    CXXFLAGS ?= -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
    ifeq ($(TARGET_OS),w)
        BIGBINARYFLAGS = -mno-cygwin
    else
	BIGBINARYFLAGS ?= 
    endif
endif

ifeq ($(TARGET_COMPILER),ms)
    include ../makefile.ms.config
    BIGBINARYFLAGS ?= /nologo
    DBG?=
endif


#bb_test and orig_address are not run on cygwin due to inapility to print from Fini function because on
#cygwin the runtime is shared between the app and the tool
ifneq ($(TARGET_OS),w)
    SPECIALRUN = bb_test 
    NORMALRUN = cache_simulator watch_fragmentation trace_insertions enter_exit link_unlink event_trace insertDelete deleteTrace  orig_address
else
    ifeq ($(TARGET_COMPILER),gnu)
        SPECIALRUN = 
        NORMALRUN = cache_simulator watch_fragmentation trace_insertions enter_exit link_unlink event_trace insertDelete deleteTrace 
    else
        SPECIALRUN = bb_test
        NORMALRUN = cache_simulator watch_fragmentation trace_insertions enter_exit link_unlink event_trace insertDelete deleteTrace  orig_address
    endif
endif

NORMALRUN += mem_usage cache_flusher cache_stats

TEST_NOTOOL = cache_block high_water

TOOL_ROOTS = $(SPECIALRUN) $(NORMALRUN)

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

SANITY_TOOLS = 

ifeq ($(TARGET_OS),w)
#Thread wait depends on pthreads, so it doesn't work on Windows
THREADTESTAPP =
BIGTESTAPP = ./$(OBJDIR)bigBinary
else 
THREADTESTAPP = ./$(OBJDIR)threadWait
BIGTESTAPP = ./$(OBJDIR)bigBinary
endif

#OS Dependent
ifeq ($(TARGET_OS),l)
SPECIALRUN += mtFlush
endif

CACHELIMIT = 262144
CACHEBLOCK = 131072
HIGHWATER = 75

ifeq ($(TARGET_OS),w)
CACHELIMIT_FOR_ACTION_PENDING = 10485760
else
CACHELIMIT_FOR_ACTION_PENDING = 2621440
endif


# Pin for the Intel(R) 64 architecture does not support variable size caches
#also  cache_doubler is not run on cygwin due to inapility to print from Fini function because on
#cygwin the runtime is shared between the app and the tool
ifneq ($(TARGET),ia32e)
    ifneq ($(TARGET_OS),w)
        TOOL_ROOTS += cache_doubler
    else
        ifneq ($(TARGET_COMPILER),gnu)
            TOOL_ROOTS += cache_doubler
        endif
    endif
endif

ifeq ($(TARGET),ia32e)
    TOOL_ROOTS += action_pending_cachefull
endif

THREAD_LIB=$(OBJDIR)threadlib.$(OBJEXT)
ifeq ($(TARGET_OS),w)
    OS_API=windows
else
    OS_API=unix
endif

all: tools 
tools: $(OBJDIR) $(TOOLS) 
test: $(OBJDIR) $(TOOL_ROOTS:%=%.test)  $(TEST_NOTOOL:%=%.test) 
tests-sanity: $(OBJDIR) $(SANITY_TOOLS:%=%.test)

$(OBJDIR):
	mkdir -p $(OBJDIR)


## special testing rules

action_pending_cachefull.test: $(OBJDIR)action_pending_cachefull$(PINTOOL_SUFFIX) action_pending_cachefull.tested action_pending_cachefull.failed $(OBJDIR)action_pending_cachefull_app
	$(PIN) -xyzzy -high_water_mark 25 -limit_code_cache $(CACHELIMIT_FOR_ACTION_PENDING) -t $<  -- $(OBJDIR)action_pending_cachefull_app
	rm action_pending_cachefull.failed
 
cache_flusher.test: $(OBJDIR)cache_flusher$(PINTOOL_SUFFIX) cache_flusher.tested cache_flusher.failed $(BIGTESTAPP)
	$(PIN) -limit_code_cache $(CACHELIMIT) -t $< -o $<.out -- $(BIGTESTAPP)
	grep -q SWOOSH $<.out
	grep -q eof $<.out
	rm cache_flusher.failed; rm $<.out

cache_doubler.test: $(OBJDIR)cache_doubler$(PINTOOL_SUFFIX) cache_doubler.tested cache_doubler.failed $(BIGTESTAPP)
	$(PIN) -limit_code_cache $(CACHELIMIT) -t $< -o $<.out -- $(BIGTESTAPP) 
	grep -q STRETCH $<.out
	grep -q eof $<.out
	rm cache_doubler.failed; rm $<.out

flushAndInsert.test: $(OBJDIR)flushAndInsert$(PINTOOL_SUFFIX) flushAndInsert.tested flushAndInsert.failed $(BIGTESTAPP)
	$(PIN) -limit_code_cache $(CACHELIMIT) -t $< -o $<.out -- $(BIGTESTAPP) 
	grep -q SWOOSH $<.out
	grep -q insertion $<.out
	rm flushAndInsert.failed; rm $<.out

cache_block.test: cache_block.tested cache_block.failed $(BIGTESTAPP)
	$(PIN) -xyzzy -cache_block_size $(CACHEBLOCK) -- $(BIGTESTAPP)
	rm cache_block.failed

high_water.test: high_water.tested high_water.failed $(BIGTESTAPP)
	$(PIN) -xyzzy -high_water_mark $(HIGHWATER) -- $(BIGTESTAPP)
	rm high_water.failed

bb_test.test: $(OBJDIR)bb_test$(PINTOOL_SUFFIX) bb_test.tested bb_test.failed $(BIGTESTAPP)
	$(PIN) -t $< -b 10 -o $<.out -- $(BIGTESTAPP)
	grep -q "Final trace count" $<.out
	grep -q -v "Final trace count: 0" $<.out
	grep -q -v "Final cache size: 0" $<.out
	rm bb_test.failed; rm $<.out

mtFlush.test: $(OBJDIR)mtFlush$(PINTOOL_SUFFIX) mtFlush.tested mtFlush.failed $(THREADTESTAPP)
	$(PIN)  -t $< -- $(THREADTESTAPP) 
	grep -q SWOOSH mtFlush.out
	rm mtFlush.failed; rm mtFlush.out

orig_address.test:$(OBJDIR)orig_address$(PINTOOL_SUFFIX) orig_address.tested orig_address.failed $(BIGTESTAPP)
	$(PIN) -t $< -o $<.out -- $(BIGTESTAPP) 
	grep -q "Success" $<.out
	grep -q -v "Failure" $<.out
	grep -q "eof" $<.out
	rm orig_address.failed; rm $<.out

$(OBJDIR)threadlib.$(OBJEXT): ../threadlib/threadlib_$(OS_API).c ../threadlib/threadlib.h
	$(CC) $(APP_CXXFLAGS) ${COPT} ${OUTOPT}$@ $< 

$(OBJDIR)threadWait: threadWait.c
	$(CC) $(APP_CXXFLAGS) $(DBG) -o $@ $< $(APP_PTHREAD)

$(OBJDIR)bigBinary: bigBinary.c
	$(CC) $(APP_CXXFLAGS) $(BIGBINARYFLAGS) ${OUTEXE}$@ $<

$(OBJDIR)action_pending_cachefull_app: action_pending_cachefull_app.c ../threadlib/threadlib.h $(THREAD_LIB)
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(THREAD_LIB) $(APP_PTHREAD)

## build rules

$(OBJDIR)%.o : %.cpp $(BIGTESTAPP) $(THREADTESTAPP)
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(PIN_LIBNAMES)

$(TOOLS): $(OBJDIR)%$(PINTOOL_SUFFIX) : $(OBJDIR)%.o $(OBJDIR)utils.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(OBJDIR)utils.o $(DBG)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed *.exp *.lib 

tidy:
	-rm -rf $(OBJDIR) *.tested *.failed

-include *.d
