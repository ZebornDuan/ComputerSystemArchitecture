

.global ReadFpContext
.type ReadFpContext, @function

ReadFpContext:
	push %ebp
	mov %esp, %ebp
	mov 8(%ebp), %eax
	fxsave (%eax)
	leave
	ret

.global WriteFpContext
.type WriteFpContext, @function

WriteFpContext:
	push %ebp
	mov %esp, %ebp
	mov 8(%ebp), %eax
	fxrstor (%eax)
	leave
	ret

.global sched_yield

// void GetLock(long *mutex, long newVal)
.global GetLock
GetLock:
    push %ebp
    mov %esp, %ebp
    mov 8(%ebp), %esi # %esi <- mutex
    mov 0xc(%ebp), %edi # %edi <- new value
    xor %eax, %eax

try_again:
    lock cmpxchg %edi, (%esi)
    je done
    call sched_yield
    jmp try_again
done:
    leave
    ret
        
// void ReleaseLock(long *mutex)

.global ReleaseLock
ReleaseLock:
    push %ebp
    mov %esp, %ebp
    mov 8(%ebp), %edi
    xor %eax, %eax
    lock xchg %eax, (%edi)
    leave
    ret

// void InitLock(long *mutex)    
.global InitLock
InitLock:
    push %ebp
    mov %esp, %ebp
    mov 8(%ebp), %edi
    xor %eax, %eax
    lock xchg %eax, (%edi)
    leave
    ret
