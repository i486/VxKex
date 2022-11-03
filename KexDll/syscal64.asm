IFDEF RAX

_TEXT SEGMENT

GENERATE_SYSCALL MACRO SyscallName, SyscallNumber64
PUBLIC SyscallName
ALIGN 16
SyscallName PROC
	mov			r10, rcx
	mov			eax, SyscallNumber64
	syscall
	ret
SyscallName ENDP
ENDM

GENERATE_SYSCALL KexNtQuerySystemTime,							0057h
GENERATE_SYSCALL KexNtCreateUserProcess,						00AAh
GENERATE_SYSCALL KexNtProtectVirtualMemory,						004Dh

_TEXT ENDS

ENDIF
END