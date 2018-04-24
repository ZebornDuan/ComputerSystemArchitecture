##
## Tests to validate Pin's emulation of application signal handling
##


include ../makefile.gnu.config


#
# None of these tests run on Windows since signals are not supported there.
#

APPS_l        = simple nestmask syncasyncapp fpcheck maskcheck segv blockloop exithandler altstack errcheck fatal sigcont \
                sigenableapp sigprocmask_short sig64 fatalsync blockingsyscall futex asyncfpe fatal_block_sync \
                suspendmask efault shortmask
APPS_b        = simple nestmask syncasyncapp fpcheck maskcheck segv blockloop exithandler altstack errcheck fatal sigcont \
                sigenableapp sigprocmask_short sig64 fatalsync blockingsyscall futex asyncfpe fatal_block_sync \
                suspendmask efault shortmask
APPS_ipf_l    = faultcheck-ia64 natcheck segv_for_regbindings_cache_test
APPS_ia32_l   = xmmcheck_ia32 xmmfaultcheck_ia32 faultcheck-ia32 faultcheck_flags_ia32 movsbtest_ia32 \
                double hlt recursive setcontext sigaltstack sigsuspend sigusr2 dfcheck_ia32 fptags_ia32 segv_on_fetch \
                segv_for_regbindings_cache_test regbindings_cache_test_checker
APPS_ia32e_l  = xmmcheck_ia32e xmmfaultcheck_ia32e faultcheck-intel64 movsbtest_ia32e dfcheck_ia32e segv_on_fetch \
                segv_for_regbindings_cache_test regbindings_cache_test_checker
APPS_ia32e_b  = xmmcheck_ia32e xmmfaultcheck_ia32e faultcheck-intel64 movsbtest_ia32e dfcheck_ia32e segv_on_fetch \
                segv_for_regbindings_cache_test regbindings_cache_test_checker



TOOLS_l       = syncasynctool exittool interceptor logtool intercepttool sigflowcheck spilltool \
                fastsigchecktool sigenable interceptall faulttool action-pending
TOOLS_b       = syncasynctool exittool interceptor logtool intercepttool sigflowcheck spilltool \
                fastsigchecktool sigenable interceptall faulttool action-pending
TOOLS_ia32_l  = faultcheck_flags_tool1 faultcheck_flags_tool2 faultcheck_flags_tool3 interceptor_for_regbindings_cache_test \
                raise-exception-tool
TOOLS_ia32e_l = faultcheck_flags_tool1 faultcheck_flags_tool2 faultcheck_flags_tool3 interceptor_for_regbindings_cache_test \
                raise-exception-tool
TOOLS_ia32e_b = faultcheck_flags_tool1 faultcheck_flags_tool2 faultcheck_flags_tool3 interceptor_for_regbindings_cache_test
TOOLS_ipf     = interceptor_for_regbindings_cache_test



# please try to keep FreeBSD and Linux tests synchronized

TESTS_l       = nestmask altstack errcheck fatallog sigcont fastsigcheck sigenable interceptall sigprocmask_short sig64 \
                faulttool fatalsync blockingsyscall futex asyncfpe fatal_block_sync suspendmask efault shortmask \
                action-pending
ifneq ($(ENABLE_VS),1)
  TESTS_l += maskcheck
endif

# other than shortmask all FreeBSD tests are the same as Linux and one should try to maintain this 
# when adding a new signal test.
TESTS_b       = nestmask altstack errcheck sigcont fastsigcheck interceptall sig64 \
                fatalsync blockingsyscall futex efault suspendmask \
                fatallog sigenable sigprocmask_short faulttool fatal_block_sync maskcheck \
                action-pending

# See comment next to the test
#
TESTS_b_disabled = asyncfpe


TESTS_ipf_l   = simpleasync simplesync fpcheck interceptsegv interceptsegv_for_regbindings_cache log intercept sigflowcheck1 sigflowcheck2 syncasync \
                faultcheck-ia64 natcheck
TESTS_ia32_l  = simpleasync simplesync fpcheck interceptsegv interceptsegv_for_regbindings_cache xmmcheck_ia32 xmmfaultcheck_ia32 xmmfaultcheckspill_ia32 log \
                intercept sigflowcheck1 sigflowcheck2 syncasync faultcheck-ia32 faultcheck_flags_ia32_1 faultcheck_flags_ia32_2 movsbtest_ia32 \
                hlt sigaltstack dfcheck_ia32 fptags_ia32 segv_on_fetch raise-exception-ia32
###                
# sigsuspend was causing a hang, so it is disabled until the problem can be investigated
# also disabled double, recursive, setcontext and sigusr2 
# because even though they passed sanity they caused failures in the nightlies
#                double hlt recursive setcontext sigaltstack sigsuspend sigusr2
###
TESTS_ia32e_l = simpleasync simplesync fpcheck interceptsegv interceptsegv_for_regbindings_cache xmmcheck_ia32e xmmfaultcheck_ia32e log \
                intercept sigflowcheck1 sigflowcheck2 syncasync faultcheck-intel64 movsbtest_ia32e dfcheck_ia32e \
                segv_on_fetch raise-exception-intel64
TESTS_ia32e_b = fpcheck interceptsegv interceptsegv_for_regbindings_cache xmmcheck_ia32e xmmfaultcheck_ia32e \
                sigflowcheck1 syncasync dfcheck_ia32e faultcheck-intel64 movsbtest_ia32e \
                simpleasync simplesync intercept sigflowcheck2 log_bsd segv_on_fetch





apps  = $(APPS) $(APPS_$(TARGET)) $(APPS_$(TARGET_OS)) $(APPS_$(TARGET)_$(TARGET_OS))
tools = $(TOOLS) $(TOOLS_$(TARGET)) $(TOOLS_$(TARGET_OS)) $(TOOLS_$(TARGET)_$(TARGET_OS))
tests = $(TESTS) $(TESTS_$(TARGET)) $(TESTS_$(TARGET_OS)) $(TESTS_$(TARGET)_$(TARGET_OS)) dummy


all: $(apps:%=$(OBJDIR)%) $(tools:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
test: $(tests:=.test)
tests-sanity: test

$(apps:%=$(OBJDIR)%) $(tools:%=$(OBJDIR)%): $(OBJDIR)make-directory

$(OBJDIR)make-directory:
	mkdir -p $(OBJDIR)
	touch  $(OBJDIR)make-directory
$(OBJDIR):
	mkdir -p $(OBJDIR)



#
# Rules to build the applications
#
$(OBJDIR)nestmask: nestmask.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)segv: segv.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<
	
$(OBJDIR)regbindings_cache_test_checker: regbindings_cache_test_checker.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<
	
$(OBJDIR)segv_for_regbindings_cache_test: segv_for_regbindings_cache_test.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)syncasyncapp: syncasyncapp.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)fpcheck: fpcheck.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $< -lm

$(OBJDIR)xmmcheck_ia32: xmmcheck.c copywithxmm_ia32.s
	$(CC) $(APP_CXXFLAGS) -o $@ xmmcheck.c copywithxmm_ia32.s

$(OBJDIR)xmmcheck_ia32e: xmmcheck.c copywithxmm_ia32e.s
	$(CC) $(APP_CXXFLAGS) -o $@ xmmcheck.c copywithxmm_ia32e.s

$(OBJDIR)xmmfaultcheck_ia32: xmmfaultcheck.c copywithxmm_ia32.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32 -o $@ xmmfaultcheck.c copywithxmm_ia32.s

$(OBJDIR)xmmfaultcheck_ia32e: xmmfaultcheck.c copywithxmm_ia32e.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32E -o $@ xmmfaultcheck.c copywithxmm_ia32e.s

$(OBJDIR)blockloop: blockloop.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)maskcheck: maskcheck.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)exithandler: exithandler.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)simple: simple.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)altstack: altstack.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)faultcheck-ia32: faultcheck.c faultcheck-x86.c faultcheck-ia32-asm.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32 -o $@ faultcheck.c faultcheck-x86.c faultcheck-ia32-asm.s

$(OBJDIR)faultcheck-intel64: faultcheck.c faultcheck-x86.c faultcheck-intel64-asm.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32E -o $@ faultcheck.c faultcheck-x86.c faultcheck-intel64-asm.s

$(OBJDIR)faultcheck-ia64: faultcheck.c faultcheck-ia64.c faultcheck_ipf_asm.s
	$(CC) $(APP_CXXFLAGS) -o $@ faultcheck.c faultcheck-ia64.c faultcheck_ipf_asm.s

$(OBJDIR)faultcheck_flags_ia32: faultcheck_flags.c  faultcheck_flags_ia32_asm.s
	$(CC) $(APP_CXXFLAGS) -o $@ faultcheck_flags.c  faultcheck_flags_ia32_asm.s

$(OBJDIR)errcheck: errcheck.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)natcheck: natcheck.c natcheck_asm.s
	$(CC) $(APP_CXXFLAGS) -o $@ natcheck.c natcheck_asm.s

$(OBJDIR)movsbtest_ia32: movsbtest.c movsbtest_ia32.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32 -o $@ movsbtest.c movsbtest_ia32.s

$(OBJDIR)movsbtest_ia32e: movsbtest.c movsbtest_ia32e.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32E -o $@ movsbtest.c movsbtest_ia32e.s

$(OBJDIR)double: double.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $<

$(OBJDIR)hlt: hlt.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $<

$(OBJDIR)recursive: recursive.c
	$(CC) $(APP_CXXFLAGS) -g -O -o $@ $<

$(OBJDIR)setcontext: setcontext.c
	$(CC) $(APP_CXXFLAGS)  -g -O -o $@ $<

$(OBJDIR)sigaltstack: sigaltstack.c
	$(CC) $(APP_CXXFLAGS) -g -O $(APP_PTHREAD) -o $@ $<

$(OBJDIR)sigsuspend: sigsuspend.c
	$(CC) $(APP_CXXFLAGS) -g -O $(APP_PTHREAD) -o $@ $<

$(OBJDIR)sigusr2: sigusr2.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $<

$(OBJDIR)fatal: fatal.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $<

$(OBJDIR)sigcont: sigcont.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $<

$(OBJDIR)sigenableapp: sigenableapp.c sigenableapp2.c
	$(CC) $(APP_CXXFLAGS) -o $@ sigenableapp.c sigenableapp2.c

$(OBJDIR)sigprocmask_short: sigprocmask_short.cpp
	$(CXX) $(PIN_CXXFLAGS) -o $@ sigprocmask_short.cpp

$(OBJDIR)sig64: sig64.cpp
	$(CXX) $(APP_CXXFLAGS) -o $@ sig64.cpp

$(OBJDIR)dfcheck_ia32: dfcheck.c dfcheck_ia32.s
	$(CC) $(APP_CXXFLAGS) -o $@ dfcheck.c dfcheck_ia32.s

$(OBJDIR)dfcheck_ia32e: dfcheck.c dfcheck_ia32e.s
	$(CC) $(APP_CXXFLAGS) -o $@ dfcheck.c dfcheck_ia32e.s

$(OBJDIR)fatalsync: fatalsync.c
	$(CC) $(APP_CXXFLAGS) -o $@ fatalsync.c

$(OBJDIR)blockingsyscall: blockingsyscall.c
	$(CC) $(APP_CXXFLAGS) -o $@ blockingsyscall.c

$(OBJDIR)futex: futex.c
	$(CC) $(APP_CXXFLAGS) -o $@ futex.c $(APP_PTHREAD)

$(OBJDIR)fptags_ia32: fptags.c fptags_ia32.s
	$(CC) $(APP_CXXFLAGS) -o $@ fptags.c fptags_ia32.s

$(OBJDIR)asyncfpe: asyncfpe.c
	$(CC) $(APP_CXXFLAGS) -o $@ asyncfpe.c

$(OBJDIR)segv_on_fetch: segv_on_fetch.c
	$(CC) $(APP_CXXFLAGS) -o $@ segv_on_fetch.c

$(OBJDIR)fatal_block_sync: fatal_block_sync.c
	$(CC) $(APP_CXXFLAGS) -o $@ fatal_block_sync.c

$(OBJDIR)suspendmask: suspendmask.c
	$(CC) $(APP_CXXFLAGS) -o $@ suspendmask.c

$(OBJDIR)efault: efault.c
	$(CC) $(APP_CXXFLAGS) -o $@ efault.c

$(OBJDIR)shortmask: shortmask.c
	$(CC) $(APP_CXXFLAGS) -o $@ shortmask.c


#
# Rules to build the object files
#
$(OBJDIR)%.o: %.cpp $(OBJDIR)make-directory
	$(CXX) $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) -o $@ $<

#
# Rules to build the tools
#
$(OBJDIR)interceptor$(PINTOOL_SUFFIX): $(OBJDIR)interceptor.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)interceptor_for_regbindings_cache_test$(PINTOOL_SUFFIX): $(OBJDIR)interceptor_for_regbindings_cache_test.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)syncasynctool$(PINTOOL_SUFFIX): $(OBJDIR)syncasynctool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)logtool$(PINTOOL_SUFFIX): $(OBJDIR)logtool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)intercepttool$(PINTOOL_SUFFIX): $(OBJDIR)intercepttool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX): $(OBJDIR)sigflowcheck.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)exittool$(PINTOOL_SUFFIX): $(OBJDIR)exittool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)spilltool$(PINTOOL_SUFFIX): $(OBJDIR)spilltool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)fastsigchecktool$(PINTOOL_SUFFIX): $(OBJDIR)fastsigchecktool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)sigenable$(PINTOOL_SUFFIX): $(OBJDIR)sigenable.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)interceptall$(PINTOOL_SUFFIX): $(OBJDIR)interceptall.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)faulttool$(PINTOOL_SUFFIX): $(OBJDIR)faulttool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)faultcheck_flags_tool1$(PINTOOL_SUFFIX): $(OBJDIR)faultcheck_flags_tool1.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)faultcheck_flags_tool2$(PINTOOL_SUFFIX): $(OBJDIR)faultcheck_flags_tool2.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)faultcheck_flags_tool3$(PINTOOL_SUFFIX): $(OBJDIR)faultcheck_flags_tool3.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX): $(OBJDIR)raise-exception-tool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)action-pending$(PINTOOL_SUFFIX): $(OBJDIR)action-pending.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

#
# Rules to run the tests.
#
nestmask.test: $(OBJDIR)nestmask nestmask.tested nestmask.failed 
	$(PIN) -- ./$<
	rm $(@:.test=.failed)

interceptsegv.test: interceptsegv.tested interceptsegv.failed $(OBJDIR)interceptor$(PINTOOL_SUFFIX) $(OBJDIR)segv
	$(PIN) -t $(OBJDIR)interceptor$(PINTOOL_SUFFIX) -- ./$(OBJDIR)segv
	rm $(@:.test=.failed)

interceptsegv_for_regbindings_cache.test: interceptsegv_for_regbindings_cache.tested interceptsegv_for_regbindings_cache.failed $(OBJDIR)interceptor_for_regbindings_cache_test$(PINTOOL_SUFFIX) $(OBJDIR)segv_for_regbindings_cache_test $(OBJDIR)regbindings_cache_test_checker
	$(PIN) -xyzzy -mesgon log_ccd -mesgon log_rbc -logfile $(OBJDIR)interceptsegv_for_regbindings_cache.log -t $(OBJDIR)interceptor_for_regbindings_cache_test$(PINTOOL_SUFFIX) -- ./$(OBJDIR)segv_for_regbindings_cache_test
	$(OBJDIR)regbindings_cache_test_checker $(OBJDIR)interceptsegv_for_regbindings_cache.log
	rm $(OBJDIR)interceptsegv_for_regbindings_cache.log
	rm $(@:.test=.failed)

syncasync.test: syncasync.tested syncasync.failed $(OBJDIR)syncasynctool$(PINTOOL_SUFFIX) $(OBJDIR)syncasyncapp
	$(PIN) -t $(OBJDIR)syncasynctool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)syncasyncapp
	rm $(@:.test=.failed)

fpcheck.test: $(OBJDIR)fpcheck fpcheck.tested fpcheck.failed 
	$(PIN) -- ./$<
	rm $(@:.test=.failed)

xmmcheck_ia32.test: $(OBJDIR)xmmcheck_ia32 xmmcheck_ia32.tested xmmcheck_ia32.failed 
	$(PIN) -- ./$<
	rm $(@:.test=.failed)

xmmcheck_ia32e.test: $(OBJDIR)xmmcheck_ia32e xmmcheck_ia32e.tested xmmcheck_ia32e.failed 
	$(PIN) -- ./$<
	rm $(@:.test=.failed)

xmmfaultcheck_ia32.test: xmmfaultcheck_ia32.tested xmmfaultcheck_ia32.failed $(OBJDIR)xmmfaultcheck_ia32
	$(PIN) -- ./$(OBJDIR)$(@:.test=)
	rm $(@:.test=.failed)

xmmfaultcheckspill_ia32.test: xmmfaultcheckspill_ia32.tested xmmfaultcheckspill_ia32.failed $(OBJDIR)xmmfaultcheck_ia32 $(OBJDIR)spilltool$(PINTOOL_SUFFIX)
	$(PIN) -t $(OBJDIR)spilltool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)xmmfaultcheck_ia32
	rm $(@:.test=.failed)

xmmfaultcheck_ia32e.test: xmmfaultcheck_ia32e.tested xmmfaultcheck_ia32e.failed $(OBJDIR)xmmfaultcheck_ia32e
	$(PIN) -- ./$(OBJDIR)$(@:.test=)
	rm $(@:.test=.failed)

log.test: log.tested log.failed $(OBJDIR)logtool$(PINTOOL_SUFFIX) $(OBJDIR)nestmask
	$(PIN) -t $(OBJDIR)logtool$(PINTOOL_SUFFIX) -o log.out -- ./$(OBJDIR)nestmask
	$(PIN_CMP) log.out log.reference
	rm $(@:.test=.failed)

log_bsd.test: log_bsd.tested log_bsd.failed $(OBJDIR)logtool$(PINTOOL_SUFFIX) $(OBJDIR)nestmask
	$(PIN) -t $(OBJDIR)logtool$(PINTOOL_SUFFIX) -o log_bsd.out -- ./$(OBJDIR)nestmask
	$(PIN_CMP) log_bsd.out log_bsd.reference
	rm $(@:.test=.failed)

intercept.test: intercept.tested intercept.failed $(OBJDIR)intercepttool$(PINTOOL_SUFFIX) $(OBJDIR)blockloop
	rm -f intercept.out; touch intercept.out; \
        $(PIN) -pause_tool 1 -t $(OBJDIR)intercepttool$(PINTOOL_SUFFIX) -s 1 -c 2 -p -- ./$(OBJDIR)blockloop 2> intercept.tool.out | \
            tee intercept.out & \
        until grep 'Signals are blocked' intercept.out > /dev/null; \
            do sleep 5; \
            if ! kill -s 0 $$! > /dev/null 2>&1; then \
                echo "intercept.test: Aborted because Pin process terminated"; \
                exit 1; \
            fi; \
        done; \
        pid=`head -1 intercept.out | sed 's/.*to pid//'`; \
        kill -HUP $$pid; \
        until grep 'Intercepting signal' intercept.tool.out > /dev/null; \
            do sleep 1; \
            if ! kill -s 0 $$! > /dev/null 2>&1; then \
                echo "intercept.test: Aborted because Pin process terminated"; \
                exit 1; \
            fi; \
        done; \
        kill -HUP $$pid; \
        wait $$!
	$(PIN_CMP) intercept.tool.out intercept.tool.reference
	rm $(@:.test=.failed) intercept.tool.out intercept.out

maskcheck.test: maskcheck.tested maskcheck.failed $(OBJDIR)maskcheck
	./$(OBJDIR)$(@:.test=) > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

sigflowcheck1.test: sigflowcheck1.tested sigflowcheck1.failed $(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX) $(OBJDIR)nestmask
	$(PIN) -t $(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX) -- ./$(OBJDIR)nestmask > sigflowcheck1.out
	$(PIN_CMP) sigflowcheck1.out sigflowcheck1.reference
	rm $(@:.test=.failed)

sigflowcheck2.test: sigflowcheck2.tested sigflowcheck2.failed $(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX) $(OBJDIR)exithandler
	$(PIN) -t $(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX) -- ./$(OBJDIR)exithandler > sigflowcheck2.out
	$(PIN_CMP) sigflowcheck2.out sigflowcheck2.reference
	rm $(@:.test=.failed)

simpleasync.test: simpleasync.tested simpleasync.failed $(OBJDIR)simple $(OBJDIR)exittool$(PINTOOL_SUFFIX)
	$(PIN) -t $(OBJDIR)exittool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)simple > simpleasync.out
	$(PIN_CMP) simpleasync.out simpleasync.reference
	rm $(@:.test=.failed)

simplesync.test: simplesync.tested simplesync.failed $(OBJDIR)segv $(OBJDIR)exittool$(PINTOOL_SUFFIX)
	$(PIN) -t $(OBJDIR)exittool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)segv > simplesync.out
	$(PIN_CMP) simplesync.out simplesync.reference
	rm $(@:.test=.failed)

altstack.test: altstack.tested altstack.failed $(OBJDIR)altstack
	$(PIN) -- ./$(OBJDIR)altstack
	rm $(@:.test=.failed)

faultcheck-ia32.test: faultcheck-ia32.tested faultcheck-ia32.failed $(OBJDIR)faultcheck-ia32
	./$(OBJDIR)faultcheck-ia32 > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)faultcheck-ia32 > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

faultcheck-intel64.test: faultcheck-intel64.tested faultcheck-intel64.failed $(OBJDIR)faultcheck-intel64
	./$(OBJDIR)faultcheck-intel64 > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)faultcheck-intel64 > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

faultcheck-ia64.test: faultcheck-ia64.tested faultcheck-ia64.failed $(OBJDIR)faultcheck-ia64
	./$(OBJDIR)faultcheck-ia64 > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)faultcheck-ia64 > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

faultcheck_flags_ia32_1.test: faultcheck_flags_ia32_1.tested faultcheck_flags_ia32_1.failed $(OBJDIR)faultcheck_flags_ia32 $(OBJDIR)faultcheck_flags_tool1$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck_flags_ia32 > $(@:.test=.out)
	$(PIN) -t ./$(OBJDIR)faultcheck_flags_tool1$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck_flags_ia32 > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

faultcheck_flags_ia32_2.test: faultcheck_flags_ia32_2.tested faultcheck_flags_ia32_2.failed $(OBJDIR)faultcheck_flags_ia32 $(OBJDIR)faultcheck_flags_tool2$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck_flags_ia32 > $(@:.test=.out)
	$(PIN) -t ./$(OBJDIR)faultcheck_flags_tool2$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck_flags_ia32 > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

faultcheck_flags_ia32_3.test: faultcheck_flags_ia32_3.tested faultcheck_flags_ia32_3.failed faultcheck_flags_ia32 $(OBJDIR)faultcheck_flags_ia32 $(OBJDIR)faultcheck_flags_tool3$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck_flags_ia32 > $(@:.test=.out)
	$(PIN) -t ./$(OBJDIR)faultcheck_flags_tool3$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck_flags_ia32 > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

errcheck.test: errcheck.tested errcheck.failed $(OBJDIR)errcheck
	$(PIN) -- ./$(OBJDIR)errcheck
	rm $(@:.test=.failed)

natcheck.test: natcheck.tested natcheck.failed $(OBJDIR)natcheck
	$(PIN) -- ./$(OBJDIR)natcheck
	rm $(@:.test=.failed)

movsbtest_ia32.test: movsbtest_ia32.tested movsbtest_ia32.failed $(OBJDIR)movsbtest_ia32
	$(PIN) -- ./$(OBJDIR)movsbtest_ia32
	rm $(@:.test=.failed)

movsbtest_ia32e.test: movsbtest_ia32e.tested movsbtest_ia32e.failed $(OBJDIR)movsbtest_ia32e
	$(PIN) -- ./$(OBJDIR)movsbtest_ia32e
	rm $(@:.test=.failed)

##################################################################################################
# double, hlt, recursive, setcontext, sigaltstack, sigsuspend, and sigusr2 are only for Linux-ia32
##################################################################################################
double.test: double.tested double.failed $(OBJDIR)double
	./$(OBJDIR)$(@:.test=) > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

hlt.test: hlt.tested hlt.failed $(OBJDIR)hlt
	./$(OBJDIR)$(@:.test=) > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

recursive.test: recursive.tested recursive.failed $(OBJDIR)recursive
	./$(OBJDIR)$(@:.test=) > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

setcontext.test: setcontext.tested setcontext.failed $(OBJDIR)setcontext
	./$(OBJDIR)$(@:.test=) > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

sigaltstack.test: sigaltstack.tested sigaltstack.failed $(OBJDIR)sigaltstack
	$(PIN) -- ./$(OBJDIR)$(@:.test=)
	rm $(@:.test=.failed)

sigsuspend.test: sigsuspend.tested sigsuspend.failed $(OBJDIR)sigsuspend
	$(PIN) -- ./$(OBJDIR)$(@:.test=)
	rm $(@:.test=.failed)

sigusr2.test: sigusr2.tested sigusr2.failed $(OBJDIR)sigusr2
	./$(OBJDIR)$(@:.test=) > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

fatallog.test: fatallog.tested fatallog.failed $(OBJDIR)fatal $(OBJDIR)logtool$(PINTOOL_SUFFIX)
	-$(PIN) -t $(OBJDIR)logtool$(PINTOOL_SUFFIX) -o fatallog.out -- ./$(OBJDIR)fatal; exit 0 
	$(PIN_CMP) fatallog.out fatallog.reference
	rm $(@:.test=.failed)

sigcont.test: sigcont.tested sigcont.failed $(OBJDIR)sigcont
	$(PIN) -- ./$(OBJDIR)sigcont
	rm $(@:.test=.failed)

fastsigcheck.test: $(OBJDIR)fastsigchecktool$(PINTOOL_SUFFIX) $(OBJDIR)fpcheck fastsigcheck.tested fastsigcheck.failed
	$(PIN) -xyzzy -statistic -logfile fastsigcheck.log -t $(OBJDIR)fastsigchecktool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)fpcheck
	#
	# This tests if the number of "slow" signals is less than 5%
	#
	pctSlow=`fgrep 'pin::signal::total/slow' fastsigcheck.log | sed -e 's/.*norm: *\([0-9\.]*\).*/\1/'`; \
	    test `expr $$pctSlow \< 0.05` -eq "1"
	rm $(@:.test=.failed)

sigenable.test: $(OBJDIR)sigenable$(PINTOOL_SUFFIX) $(OBJDIR)sigenableapp sigenable.tested sigenable.failed
	$(PIN) -t $(OBJDIR)sigenable$(PINTOOL_SUFFIX) -- ./$(OBJDIR)sigenableapp > $(@:.test=.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.reference)
	rm $(@:.test=.failed)

interceptall.test: $(OBJDIR)interceptall$(PINTOOL_SUFFIX) interceptall.tested interceptall.failed
	$(PIN) -t $(OBJDIR)interceptall$(PINTOOL_SUFFIX) -- $(TESTAPP) makefile interceptall.makefile.copy
	rm $(@:.test=.failed)

sigprocmask_short.test: $(OBJDIR)sigprocmask_short sigprocmask_short.tested sigprocmask_short.failed
	$(PIN) -- ./$(OBJDIR)sigprocmask_short
	rm $(@:.test=.failed)

sig64.test: $(OBJDIR)sig64 sig64.tested sig64.failed
	$(PIN) -- ./$(OBJDIR)sig64
	rm $(@:.test=.failed)

faulttool.test: $(OBJDIR)faulttool$(PINTOOL_SUFFIX) $(OBJDIR)simple faulttool.tested faulttool.failed
	@echo "The 'faulttool' test is expected to seg fault and dump core"
	-$(PIN) -t $(OBJDIR)faulttool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)simple > $(@:.test=.out)
	grep "Tool (or Pin) caused signal" $(@:.test=.out) > /dev/null
	grep "Location may be in in-lined analysis code, try re-running with '-inline 0' to debug it" $(@:.test=.out) > /dev/null
	rm $(@:.test=.failed)

dfcheck_ia32.test: dfcheck_ia32.tested dfcheck_ia32.failed $(OBJDIR)dfcheck_ia32
	$(PIN) -- ./$(OBJDIR)dfcheck_ia32
	rm $(@:.test=.failed)

dfcheck_ia32e.test: dfcheck_ia32e.tested dfcheck_ia32e.failed $(OBJDIR)dfcheck_ia32e
	$(PIN) -- ./$(OBJDIR)dfcheck_ia32e
	rm $(@:.test=.failed)

fatalsync.test: fatalsync.tested fatalsync.failed $(OBJDIR)fatalsync
	@echo "The 'fatalsync' test is expected to seg fault and dump core"
	-$(PIN) -- ./$(OBJDIR)fatalsync > $(@:.test=.out)
	if grep "assertion failed" $(@:.test=.out); then exit 1; fi
	rm $(@:.test=.failed)

blockingsyscall.test: $(OBJDIR)blockingsyscall blockingsyscall.tested blockingsyscall.failed
	$(PIN) -- ./$(OBJDIR)blockingsyscall > $(@:.test=.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.reference)
	rm $(@:.test=.failed)

futex.test: $(OBJDIR)futex futex.tested futex.failed
	$(PIN) -- ./$(OBJDIR)futex
	rm $(@:.test=.failed)

fptags_ia32.test: $(OBJDIR)fptags_ia32 fptags_ia32.tested fptags_ia32.failed
	$(PIN) -- ./$(OBJDIR)fptags_ia32
	rm $(@:.test=.failed)

# currently have no way of distingushing between fpe signal generated by a real exception or
# sent by a user. this is a problem with the way FreeBSD kernel sets the si_code field in siginfo_t
# for now, we choose to support the synchronous mode and so the async test crashes.
#
asyncfpe.test: $(OBJDIR)asyncfpe asyncfpe.tested asyncfpe.failed
	$(PIN) -- ./$(OBJDIR)asyncfpe
	rm $(@:.test=.failed)

segv_on_fetch.test: $(OBJDIR)segv_on_fetch segv_on_fetch.tested segv_on_fetch.failed
	$(PIN) -- ./$(OBJDIR)segv_on_fetch
	rm $(@:.test=.failed)

fatal_block_sync.test: $(OBJDIR)fatal_block_sync $(OBJDIR)exittool$(PINTOOL_SUFFIX) fatal_block_sync.tested fatal_block_sync.failed
	@echo "The 'fatal_block_sync' test is expected to seg fault and dump core"
	-$(PIN) -t $(OBJDIR)exittool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)fatal_block_sync > $(@:.test=.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.reference)
	rm $(@:.test=.failed)

suspendmask.test: $(OBJDIR)suspendmask suspendmask.tested suspendmask.failed
	./$(OBJDIR)suspendmask > $(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)suspendmask > $(@:.test=.pin.out)
	cmp $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

efault.test: $(OBJDIR)efault efault.tested efault.failed
	$(PIN) -- ./$(OBJDIR)efault
	rm $(@:.test=.failed)

raise-exception-ia32.test: raise-exception-ia32.tested raise-exception-ia32.failed $(OBJDIR)faultcheck-ia32 $(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck-ia32 > $(@:.test=.out)
	$(PIN) -t $(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck-ia32 > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)

raise-exception-intel64.test: raise-exception-intel64.tested raise-exception-intel64.failed $(OBJDIR)faultcheck-intel64 $(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck-intel64 > $(@:.test=.out)
	$(PIN) -t $(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck-intel64 > $(@:.test=.pin.out)
	$(PIN_CMP) $(@:.test=.out) $(@:.test=.pin.out)
	rm $(@:.test=.failed)


# we do not use the shortmask test on FreeBSD because it tests legacy behavior for linux system calls
# and this behavior is not applicable to FreeBSD. We currently do not intend to support old FreeBSD 
# signal syscalls etc. 
#
shortmask.test: $(OBJDIR)shortmask shortmask.tested shortmask.failed
	$(PIN) -- ./$(OBJDIR)shortmask
	rm $(@:.test=.failed)

action-pending.test: $(OBJDIR)simple $(OBJDIR)action-pending$(PINTOOL_SUFFIX) action-pending.tested action-pending.failed
	$(PIN) -t $(OBJDIR)action-pending$(PINTOOL_SUFFIX) -- ./$(OBJDIR)simple
	rm $(@:.test=.failed)

dummy.test:


clean:
	rm -rf $(OBJDIR) $(tests:=.tested) $(tests:=.failed) $(tests:=.out) \
        $(tests:=.tool.out) $(tests:=.pin.out) $(tests:=.log) *.makefile.copy pin.log core*
