

.global ReadFpContext
.type ReadFpContext, @function

ReadFpContext:
	fxsave (%rdi)
	ret

.global WriteFpContext
.type WriteFpContext, @function

WriteFpContext:
	fxrstor (%rdi)
	ret


.global sched_yield

// void GetLock(long *mutex, long newVal)
.global GetLock
GetLock:
    push %rbp
    mov %rsp, %rbp

    xor %rax, %rax 
    
    // rdi - mutex
    // rsi - newVal

try_again:
    lock cmpxchg %rsi, (%rdi)
    je done
    call sched_yield
    jmp try_again
done:
    leave
    ret

// void ReleaseLock(long *mutex)

.global ReleaseLock
ReleaseLock:
    xor %rax, %rax
    lock xchg %rax, (%rdi) # put 0 in *mutex
    ret

// void InitLock(long *mutex)
.global InitLock
InitLock:
    xor %rax, %rax
    lock xchg %rax, (%rdi) # put 0 in *mutex
    ret
