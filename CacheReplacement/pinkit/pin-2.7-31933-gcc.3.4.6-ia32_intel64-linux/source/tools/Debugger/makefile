##
## Tests to test the ability to debug the application while running with Pin.
##

TARGET_COMPILER?=gnu
ifdef OS
    ifeq (${OS},Windows_NT)
        TARGET_COMPILER=ms
    endif
endif

include ../makefile.$(TARGET_COMPILER).config
GDB = /usr/bin/gdb
GDB_MODERN = /usr/intel/pkgs/gdb/6.6/bin/gdb

# This is a time limit (in seconds) we use for some of the tests below.  It's intentionally high
# to avoid timeouts when the system load is very high, which can happen in our nightly tests.
#
TLIMIT=60


APPS_ia32_l     = simple simple-static exec fork bptest-ia32 action-pending-app xmm-ia32 thread simple-pindb \
                  checkpoint-app watchpoint-app callerapp fibonacci sleep-unix debugger-shell-app-ia32
APPS_ia32e_l    = simple simple-static exec fork bptest-intel64 action-pending-app xmm-intel64 thread simple-pindb \
                  checkpoint-app watchpoint-app callerapp fibonacci sleep-unix debugger-shell-app-intel64
APPS_ia32_w     = simple-pindb
APPS_ia32e_w    = simple-pindb

TOOLS_ia32_l    = breaktool int3-count action-pending-tool checkpoint watchpoint launch-gdb-tool stack-debugger emuregs32 \
                  use-debugger-shell
TOOLS_ia32e_l   = breaktool int3-count action-pending-tool checkpoint watchpoint launch-gdb-tool stack-debugger \
                  use-debugger-shell
TOOLS_ia32_w    = stack-debugger
TOOLS_ia32e_w   = stack-debugger

TESTS_ia32_l    = simple execfail fork breaktool breaktool-wait breaktool-nodebugger bptest-ia32 bp-icount \
                  action-pending xmm-ia32 thread simple-pindb-attach simple-pindb-launch launch-gdb stack-debugger \
                  emuregs32 pindb-async-stop debugger-shell-breakpoints debugger-shell-tracepoints
TESTS_ia32e_l   = simple execfail fork breaktool breaktool-wait breaktool-nodebugger bptest-intel64 bp-icount \
                  action-pending xmm-intel64 thread simple-pindb-attach simple-pindb-launch launch-gdb stack-debugger \
                  pindb-async-stop debugger-shell-breakpoints debugger-shell-tracepoints
TESTS_ia32_w    = simple-pindb-attach simple-pindb-launch
TESTS_ia32e_w   = simple-pindb-attach simple-pindb-launch

apps  = $(APPS) $(APPS_$(TARGET)) $(APPS_$(TARGET_OS)) $(APPS_$(TARGET)_$(TARGET_OS))
tools = $(TOOLS) $(TOOLS_$(TARGET)) $(TOOLS_$(TARGET_OS)) $(TOOLS_$(TARGET)_$(TARGET_OS))
tests = $(TESTS) $(TESTS_$(TARGET)) $(TESTS_$(TARGET_OS)) $(TESTS_$(TARGET)_$(TARGET_OS)) dummy


# There is a bug in some 2.4 kernels that prevents Pin's debugger feature from
# working in cross-mode (32-bit application running on a 64-bit host).
# Disable testing in this case.  See Mantis #1414 for more details.
#
osrel = $(shell uname -r)
ifeq ($(findstring 2.4,$(osrel))-$(HOST_ARCH)-$(TARGET),2.4-ia32e-ia32)
  tests = 
endif

# There is a bug in the GDB installed on Redhat 9 (and probably previous versions too) which prevents debugging of MT
# programs with Pin.  Use a modern GDB version instead.
#
redhat = $(shell test -f /etc/redhat-release && cat /etc/redhat-release)
ifeq ($(findstring release 9,$(redhat)),release 9)
  GDB = $(GDB_MODERN)
endif

# These are the Pin flags needed to enable application debugging.
#
PINFLAGS_DEBUG_l = -appdebug
PINFLAGS_DEBUG_w = -xyzzy -appdebug -late_injection 1
PINFLAGS_DEBUG = $(PINFLAGS_DEBUG_$(TARGET_OS))

PINFLAGS_DEBUG_RUNFREE_l = -appdebug_enable
PINFLAGS_DEBUG_RUNFREE_w = -xyzzy -appdebug_enable
PINFLAGS_DEBUG_RUNFREE = $(PINFLAGS_DEBUG_RUNFREE_$(TARGET_OS))


all: $(apps:%=$(OBJDIR)%$(EXEEXT)) $(tools:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
test: $(tests:=.test)
tests-sanity: test

$(apps:%=$(OBJDIR)%$(EXEEXT)) $(tools:%=$(OBJDIR)%$(PINTOOL_SUFFIX)): $(OBJDIR)make-directory

$(OBJDIR)make-directory:
	mkdir -p $(OBJDIR)
	touch  $(OBJDIR)make-directory
$(OBJDIR):
	mkdir -p $(OBJDIR)


#
# Rules to build the applications
#
$(OBJDIR)simple: simple.c
	$(CC) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $<

$(OBJDIR)simple-static: simple.c
	$(CC) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) -static $(OUTOPT)$@ $<

$(OBJDIR)exec: exec.cpp
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $<

$(OBJDIR)fork: fork.cpp
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $<

$(OBJDIR)bptest-ia32: bptest.cpp bptest-asm-ia32.s
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ bptest.cpp bptest-asm-ia32.s

$(OBJDIR)bptest-intel64: bptest.cpp bptest-asm-intel64.s
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ bptest.cpp bptest-asm-intel64.s

$(OBJDIR)action-pending-app: action-pending-app.cpp
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $< $(APP_PTHREAD)

$(OBJDIR)xmm-ia32: xmm.c xmm-asm-ia32.s
	$(CC) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ xmm.c xmm-asm-ia32.s

$(OBJDIR)xmm-intel64: xmm.c xmm-asm-intel64.s
	$(CC) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ xmm.c xmm-asm-intel64.s

$(OBJDIR)thread: thread.cpp
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $< $(APP_PTHREAD)

$(OBJDIR)thread-static: thread.cpp
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) -static $(OUTOPT)$@ $< $(APP_PTHREAD)

$(OBJDIR)simple-pindb$(EXEEXT): $(OBJDIR)simple-pindb.$(OBJEXT) $(OBJDIR)simple-pindb-asm-$(TARGET_OS_LONG)-$(TARGET_LONG).$(OBJEXT)
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTEXE)$@ $(OBJDIR)simple-pindb.$(OBJEXT) $(OBJDIR)simple-pindb-asm-$(TARGET_OS_LONG)-$(TARGET_LONG).$(OBJEXT)
$(OBJDIR)simple-pindb.$(OBJEXT): $(OBJDIR)make-directory simple-pindb.cpp
	$(CXX) $(DBG) $(COPT) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ simple-pindb.cpp
$(OBJDIR)simple-pindb-asm-windows-$(TARGET_LONG).$(OBJEXT): $(OBJDIR)make-directory simple-pindb-asm-windows-$(TARGET_LONG).asm
	$(MASM) /nologo /c /Fo$@ simple-pindb-asm-windows-$(TARGET_LONG).asm
$(OBJDIR)simple-pindb-asm-linux-$(TARGET_LONG).$(OBJEXT): $(OBJDIR)make-directory simple-pindb-asm-linux-$(TARGET_LONG).s
	$(CXX) $(DBG) $(COPT) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ simple-pindb-asm-linux-$(TARGET_LONG).s

$(OBJDIR)checkpoint-app: checkpoint-app.c
	$(CC) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $<

$(OBJDIR)watchpoint-app: watchpoint-app.c
	$(CC) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $<

$(OBJDIR)callerapp: callerapp.c
	$(CC) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $<

$(OBJDIR)fibonacci: ../ManualExamples/fibonacci.cpp
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $<

$(OBJDIR)sleep-unix: sleep-unix.c
	$(CC) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ $<

$(OBJDIR)debugger-shell-app-$(TARGET_LONG): debugger-shell-app.cpp debugger-shell-app-$(TARGET_LONG).s
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS) $(OUTOPT)$@ debugger-shell-app.cpp debugger-shell-app-$(TARGET_LONG).s


#
# Rules to build the object files
#
$(OBJDIR)%.$(OBJEXT): %.cpp $(OBJDIR)make-directory
	$(CXX) $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) $(OUTOPT)$@ $<
$(OBJDIR)stack-debugger.$(OBJEXT): ../ManualExamples/stack-debugger.cpp $(OBJDIR)make-directory
	$(CXX) $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) $(OUTOPT)$@ $<
$(OBJDIR)debugger-shell.$(OBJEXT): ../InstLib/debugger-shell.cpp $(OBJDIR)make-directory
	$(CXX) $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) $(OUTOPT)$@ $<

#
# Rules to build the tools
#
$(OBJDIR)breaktool$(PINTOOL_SUFFIX): $(OBJDIR)breaktool.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)

$(OBJDIR)int3-count$(PINTOOL_SUFFIX): $(OBJDIR)int3-count.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)

$(OBJDIR)action-pending-tool$(PINTOOL_SUFFIX): $(OBJDIR)action-pending-tool.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)

$(OBJDIR)checkpoint$(PINTOOL_SUFFIX): $(OBJDIR)checkpoint.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)

$(OBJDIR)watchpoint$(PINTOOL_SUFFIX): $(OBJDIR)watchpoint.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)

$(OBJDIR)launch-gdb-tool$(PINTOOL_SUFFIX): $(OBJDIR)launch-gdb-tool.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)

$(OBJDIR)stack-debugger$(PINTOOL_SUFFIX): $(OBJDIR)stack-debugger.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)

$(OBJDIR)emuregs32$(PINTOOL_SUFFIX): $(OBJDIR)emuregs32.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)

$(OBJDIR)use-debugger-shell$(PINTOOL_SUFFIX): $(OBJDIR)use-debugger-shell.$(OBJEXT) $(OBJDIR)debugger-shell.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $(OBJDIR)use-debugger-shell.$(OBJEXT) $(OBJDIR)debugger-shell.$(OBJEXT) $(PIN_LIBS)


#
# Rules to run the tests.
#

# This is the example tool from the manual run such that you can attach the debugger after it triggers a stack
# breakpoint.
#
stack-debugger-late.test: $(OBJDIR)fibonacci $(OBJDIR)stack-debugger$(PINTOOL_SUFFIX) $(OBJDIR)stack-debugger-late.tested $(OBJDIR)stack-debugger-late.failed
	rm -f $(OBJDIR)$(@:.test=.toolout)
	$(PIN) $(PINFLAGS_DEBUG_RUNFREE) -appdebug_silent -t $(OBJDIR)stack-debugger$(PINTOOL_SUFFIX) \
	    -stackbreak 4000 -o $(OBJDIR)$(@:.test=.toolout) -timeout $(TLIMIT) -- \
	    $(OBJDIR)fibonacci 1000 > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.toolout) > /dev/null 2>&1 || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.toolout) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)fibonacci > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Basic test of debugger features.
#
simple.test: $(OBJDIR)simple $(OBJDIR)simple.tested $(OBJDIR)simple.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -- $(OBJDIR)simple > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)simple > $(OBJDIR)$(@:.test=.gdbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Verify we can debug across a failed exec() call.
#
execfail.test: $(OBJDIR)exec $(OBJDIR)execfail.tested $(OBJDIR)execfail.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -follow_execv -- $(OBJDIR)exec ./does-not-exist > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)exec > $(OBJDIR)$(@:.test=.gdbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Verify we can debug across a parent call to fork().
#
fork.test: $(OBJDIR)fork $(OBJDIR)fork.tested $(OBJDIR)fork.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -- $(OBJDIR)fork > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)fork > $(OBJDIR)$(@:.test=.gdbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Basic test of PIN_ApplicationBreakpoint()
#
breaktool.test: $(OBJDIR)simple $(OBJDIR)breaktool$(PINTOOL_SUFFIX) $(OBJDIR)breaktool.tested $(OBJDIR)breaktool.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -t $(OBJDIR)breaktool$(PINTOOL_SUFFIX) -wait_for_debugger 0 -- $(OBJDIR)simple > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)simple > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Verify that PIN_ApplicationBreakpoint(.., TRUE, ..) will wait if there's no debugger.
#
breaktool-wait.test: $(OBJDIR)simple $(OBJDIR)breaktool$(PINTOOL_SUFFIX) $(OBJDIR)breaktool-wait.tested $(OBJDIR)breaktool-wait.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG_RUNFREE) -t $(OBJDIR)breaktool$(PINTOOL_SUFFIX) -wait_for_debugger 1 -o $(OBJDIR)$(@:.test=.out) -- $(OBJDIR)simple &
	count=0; \
	until test -s $(OBJDIR)$(@:.test=.out) -o $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	sleep 5
	port=`cat $(OBJDIR)$(@:.test=.out)`; echo "target remote :$$port" >  $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)simple > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Verify that PIN_ApplicationBreakpoint(.., FALSE, ..) does not wait if there's no debugger.
#
breaktool-nodebugger.test: $(OBJDIR)simple $(OBJDIR)breaktool$(PINTOOL_SUFFIX) $(OBJDIR)breaktool-nodebugger.tested $(OBJDIR)breaktool-nodebugger.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG_RUNFREE) -t $(OBJDIR)breaktool$(PINTOOL_SUFFIX) -wait_for_debugger 0 -- $(OBJDIR)simple
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test breakpoints in various circumstances.
#
bptest-ia32.test: $(OBJDIR)bptest-ia32 $(OBJDIR)bptest-ia32.tested $(OBJDIR)bptest-ia32.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -- $(OBJDIR)bptest-ia32 > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)bptest-ia32 > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

bptest-intel64.test: $(OBJDIR)bptest-intel64 $(OBJDIR)bptest-intel64.tested $(OBJDIR)bptest-intel64.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -- $(OBJDIR)bptest-intel64 > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)bptest-intel64 > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test that breakpoints do not cause the tool to see any extra instructions (e.g. INT3).
#
bp-icount.test: $(OBJDIR)simple-static $(OBJDIR)int3-count$(PINTOOL_SUFFIX) $(OBJDIR)bp-icount.tested $(OBJDIR)bp-icount.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) -t $(OBJDIR)int3-count$(PINTOOL_SUFFIX) -func main -o $(OBJDIR)bp-icount.reference -- $(OBJDIR)simple-static
	$(PIN) $(PINFLAGS_DEBUG) -t $(OBJDIR)int3-count$(PINTOOL_SUFFIX) -func main -o $(OBJDIR)bp-icount.count -- $(OBJDIR)simple-static > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)simple-static > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	$(PIN_CMP) $(OBJDIR)bp-icount.reference $(OBJDIR)bp-icount.count
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test the PIN_IsActionPending() API.
#
action-pending.test: $(OBJDIR)action-pending-app $(OBJDIR)action-pending-tool$(PINTOOL_SUFFIX) $(OBJDIR)action-pending.tested $(OBJDIR)action-pending.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -t $(OBJDIR)action-pending-tool$(PINTOOL_SUFFIX) -- $(OBJDIR)action-pending-app > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)action-pending-app > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test that we can print out XMM registers.  Older GDB's don't know how to print XMM registers well,
# so use a modern GDB for this test.
#
# We first test that GDB itself will run.  If not, we just skip the body of this test.  The modern
# GDB won't run on some old test systems.
#
xmm-ia32.test: $(OBJDIR)xmm-ia32 $(OBJDIR)xmm-ia32.tested $(OBJDIR)xmm-ia32.failed
	if $(GDB_MODERN) -batch -x quit.gdb -n; then \
	    rm -f $(OBJDIR)$(@:.test=.out); \
	    $(PIN) $(PINFLAGS_DEBUG) -- $(OBJDIR)xmm-ia32 > $(OBJDIR)$(@:.test=.out) & \
	    count=0; \
	    until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	        do sleep 1; count=`expr $$count + 1`; done; \
	    grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin); \
	    cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin); \
	    $(GDB_MODERN) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)xmm-ia32 > $(OBJDIR)$(@:.test=.gdbout) 2>&1; \
	    $(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout); \
	fi
	rm -f $(OBJDIR)$(@:.test=.failed)

xmm-intel64.test: $(OBJDIR)xmm-intel64 $(OBJDIR)xmm-intel64.tested $(OBJDIR)xmm-intel64.failed
	if $(GDB_MODERN) -batch -x quit.gdb -n; then \
	    rm -f $(OBJDIR)$(@:.test=.out); \
	    $(PIN) $(PINFLAGS_DEBUG) -- $(OBJDIR)xmm-intel64 > $(OBJDIR)$(@:.test=.out) & \
	    count=0; \
	    until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	        do sleep 1; count=`expr $$count + 1`; done; \
	    grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin); \
	    cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin); \
	    $(GDB_MODERN) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)xmm-intel64 > $(OBJDIR)$(@:.test=.gdbout) 2>&1; \
	    $(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout); \
	fi
	rm -f $(OBJDIR)$(@:.test=.failed)

# Simple test of a threaded program.
#
thread.test: $(OBJDIR)thread $(OBJDIR)thread.tested $(OBJDIR)thread.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -- $(OBJDIR)thread > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat thread.gdb >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)thread > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p thread.compare -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Simple test of a threaded program built statically (uses non-nptl thread package on Linux).
# NOTE: This test is disabled, so it does not run automatically.  Modern versions of GDB do not
#   support non-nptl threads well.
#
thread-static.test: $(OBJDIR)thread-static $(OBJDIR)thread-static.tested $(OBJDIR)thread-static.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -- $(OBJDIR)thread-static > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat thread.gdb >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)thread-static > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p thread.compare -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Simple test of the 'pindb' debugger.  We launch Pin separatly and pindb attaches.
#
simple-pindb-attach.test: $(OBJDIR)simple-pindb$(EXEEXT) $(OBJDIR)stack-debugger$(PINTOOL_SUFFIX) $(OBJDIR)simple-pindb-attach.tested $(OBJDIR)simple-pindb-attach.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(OBJDIR)simple-pindb$(EXEEXT) $(OBJDIR)$(@:.test=.pindbin) $(OBJDIR)$(@:.test=.compare)
	$(PIN) $(PINFLAGS_DEBUG) -t $(OBJDIR)stack-debugger$(PINTOOL_SUFFIX) -- $(OBJDIR)simple-pindb$(EXEEXT) > \
        $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) | sed 's/.*target remote //' > $(OBJDIR)$(@:.test=.ip)
	$(PINDB) --pin=$(PIN_EXE) --timeout=10000 --gdb-protocol=`cat $(OBJDIR)$(@:.test=.ip)` --cpu=$(TARGET_LONG) \
	    --os=$(TARGET_OS_LONG) --noprompt < $(OBJDIR)$(@:.test=.pindbin) > $(OBJDIR)$(@:.test=.pindbout)
	$(PYTHON) ../compare.py -p $(OBJDIR)$(@:.test=.compare) -c $(OBJDIR)$(@:.test=.pindbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Simple test of the 'pindb' debugger.  We use the pindb "run" command to launch and attach to pin.
#
simple-pindb-launch.test: $(OBJDIR)simple-pindb$(EXEEXT) $(OBJDIR)stack-debugger$(PINTOOL_SUFFIX) $(OBJDIR)simple-pindb-launch.tested $(OBJDIR)simple-pindb-launch.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(OBJDIR)simple-pindb$(EXEEXT) $(OBJDIR)$(@:.test=.pindbin.0) $(OBJDIR)$(@:.test=.compare)
	echo "set pinargs $(PIN_USERFLAGS) -t $(OBJDIR)stack-debugger$(PINTOOL_SUFFIX)" > $(OBJDIR)$(@:.test=.pindbin)
	echo "run $(OBJDIR)simple-pindb$(EXEEXT)" >> $(OBJDIR)$(@:.test=.pindbin)
	cat $(OBJDIR)$(@:.test=.pindbin.0) >> $(OBJDIR)$(@:.test=.pindbin)
	$(PINDB) --pin=$(PIN_EXE) --timeout=10000 --cpu=$(TARGET_LONG) --os=$(TARGET_OS_LONG) --noprompt \
	    < $(OBJDIR)$(@:.test=.pindbin) > $(OBJDIR)$(@:.test=.pindbout)
	$(PYTHON) ../compare.py -p $(OBJDIR)$(@:.test=.compare) -c $(OBJDIR)$(@:.test=.pindbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test the checkpoint tool.
#
checkpoint.test: $(OBJDIR)checkpoint-app $(OBJDIR)checkpoint$(PINTOOL_SUFFIX) $(OBJDIR)checkpoint.tested $(OBJDIR)checkpoint.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -t $(OBJDIR)checkpoint$(PINTOOL_SUFFIX) -- $(OBJDIR)checkpoint-app > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)checkpoint-app > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p checkpoint-gdb.compare -c $(OBJDIR)$(@:.test=.gdbout)
	$(PYTHON) ../compare.py -p checkpoint-app.compare -c $(OBJDIR)$(@:.test=.out)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test the watchpoint tool.
#
watchpoint.test: $(OBJDIR)watchpoint-app $(OBJDIR)watchpoint$(PINTOOL_SUFFIX) $(OBJDIR)watchpoint.tested $(OBJDIR)watchpoint.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(OBJDIR)watchpoint-app 1 > $(OBJDIR)$(@:.test=.gdb)
	$(PIN) $(PINFLAGS_DEBUG) -t $(OBJDIR)watchpoint$(PINTOOL_SUFFIX) -- $(OBJDIR)watchpoint-app > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(OBJDIR)$(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)watchpoint-app > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# This test starts Pin with debugger support enabled, but Pin does not stop at
# the first instruction waiting for a debugger to attach.  Instead, the
# application runs under Pin immediately.  Later, if the tool finds something
# interesting, it can ask the user (or a GUI shell) to start the debugger and
# attach at the interesting point.
#
launch-gdb.test: $(OBJDIR)callerapp $(OBJDIR)launch-gdb-tool$(PINTOOL_SUFFIX) $(OBJDIR)launch-gdb.tested $(OBJDIR)launch-gdb.failed
	rm -f $(OBJDIR)$(@:.test=.gdbin)
	$(PIN) $(PINFLAGS_DEBUG_RUNFREE) -t $(OBJDIR)launch-gdb-tool$(PINTOOL_SUFFIX) -timeout $(TLIMIT) \
	    -o $(OBJDIR)$(@:.test=.gdbin) -- $(OBJDIR)callerapp > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.gdbin) > /dev/null 2>&1 || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)callerapp > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# This is the example tool we describe in the manual, which demonstrates the major features
# of the application-level debugging API.  The tool tracks the application's stack usage
# and allows breakpoints to be set when the stack usage crosses a threshold.
#
stack-debugger.test: $(OBJDIR)fibonacci $(OBJDIR)stack-debugger$(PINTOOL_SUFFIX) $(OBJDIR)stack-debugger.tested $(OBJDIR)stack-debugger.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) $(PINFLAGS_DEBUG) -t $(OBJDIR)stack-debugger$(PINTOOL_SUFFIX) -- $(OBJDIR)fibonacci 1000 > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)fibonacci > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Minimal test that we can call PIN_AddEmulatedRegisterFunction().
#
emuregs32.test: $(OBJDIR)simple $(OBJDIR)emuregs32$(PINTOOL_SUFFIX) $(OBJDIR)emuregs32.tested $(OBJDIR)emuregs32.failed
	$(PIN) $(PINFLAGS_DEBUG_RUNFREE) -t $(OBJDIR)emuregs32$(PINTOOL_SUFFIX) -- $(OBJDIR)simple
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test that we can asynchronously stop the target in PinDB by sending CTRL-C (SIGINT).
#
pindb-async-stop.test: $(OBJDIR)sleep-unix $(OBJDIR)pindb-async-stop.tested $(OBJDIR)pindb-async-stop.failed
	echo "set pinargs $(PIN_USERFLAGS)" > $(OBJDIR)$(@:.test=.pindbin)
	echo "run $(OBJDIR)sleep-unix" >> $(OBJDIR)$(@:.test=.pindbin)
	cat $(@:.test=.pindb) >> $(OBJDIR)$(@:.test=.pindbin)
	$(PINDB) --pin=$(PIN_EXE) --timeout=10000 --cpu=$(TARGET_LONG) --os=$(TARGET_OS_LONG) --noprompt \
	    < $(OBJDIR)$(@:.test=.pindbin) > $(OBJDIR)$(@:.test=.pindbout) & \
	    pid=$$!; \
	    sleep 2; \
	    while kill -INT $$pid > /dev/null 2>&1; \
	    do \
	        sleep 2; \
	    done; \
	    wait
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test of breakpoint commands in the "debugger-shell" instrumentation library.
#
debugger-shell-breakpoints.test: $(OBJDIR)debugger-shell-app-$(TARGET_LONG) $(OBJDIR)use-debugger-shell$(PINTOOL_SUFFIX) $(OBJDIR)debugger-shell-breakpoints.tested $(OBJDIR)debugger-shell-breakpoints.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(OBJDIR)debugger-shell-app-$(TARGET_LONG) breakpoints $(OBJDIR)$(@:.test=.gdbin.0) $(OBJDIR)$(@:.test=.compare)
	$(PIN) $(PINFLAGS_DEBUG) -t $(OBJDIR)use-debugger-shell$(PINTOOL_SUFFIX) -- $(OBJDIR)debugger-shell-app-$(TARGET_LONG) > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(OBJDIR)$(@:.test=.gdbin.0) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)debugger-shell-app-$(TARGET_LONG) > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(OBJDIR)$(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test of tracepoint commands in the "debugger-shell" instrumentation library.
#
debugger-shell-tracepoints.test: $(OBJDIR)debugger-shell-app-$(TARGET_LONG) $(OBJDIR)use-debugger-shell$(PINTOOL_SUFFIX) $(OBJDIR)debugger-shell-tracepoints.tested $(OBJDIR)debugger-shell-tracepoints.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(OBJDIR)debugger-shell-app-$(TARGET_LONG) tracepoints $(OBJDIR)$(@:.test=.gdbin.0) $(OBJDIR)$(@:.test=.compare)
	$(PIN) $(PINFLAGS_DEBUG) -t $(OBJDIR)use-debugger-shell$(PINTOOL_SUFFIX) -- $(OBJDIR)debugger-shell-app-$(TARGET_LONG) > $(OBJDIR)$(@:.test=.out) &
	count=0; \
	until grep 'target remote' $(OBJDIR)$(@:.test=.out) > /dev/null || test $$count -gt $(TLIMIT); \
	    do sleep 1; count=`expr $$count + 1`; done
	grep 'target remote' $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	cat $(OBJDIR)$(@:.test=.gdbin.0) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(OBJDIR)debugger-shell-app-$(TARGET_LONG) > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	$(PYTHON) ../compare.py -p $(OBJDIR)$(@:.test=.compare) -c $(OBJDIR)$(@:.test=.gdbout)
	rm -f $(OBJDIR)$(@:.test=.failed)


dummy.test:


clean:
	rm -rf $(OBJDIR)
	rm -f pin.log
