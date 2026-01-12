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
GENERATE_SYSCALL KexNtAllocateVirtualMemory,					0015h
GENERATE_SYSCALL KexNtQueryVirtualMemory,						0020h
GENERATE_SYSCALL KexNtFreeVirtualMemory,						001Bh
GENERATE_SYSCALL KexNtOpenKeyEx,								00F2h
GENERATE_SYSCALL KexNtQueryObject,								000Dh
GENERATE_SYSCALL KexNtOpenFile,									0030h
GENERATE_SYSCALL KexNtWriteFile,								0005h
GENERATE_SYSCALL KexNtRaiseHardError,							0130h
GENERATE_SYSCALL KexNtQueryInformationThread,					0022h
GENERATE_SYSCALL KexNtSetInformationThread,                     000Ah
GENERATE_SYSCALL KexNtNotifyChangeKey,							00EBh
GENERATE_SYSCALL KexNtNotifyChangeMultipleKeys,					00ECh
GENERATE_SYSCALL KexNtCreateSection,							0047h
GENERATE_SYSCALL KexNtQueryInformationProcess,					0016h
GENERATE_SYSCALL KexNtAssignProcessToJobObject,					0085h

_TEXT ENDS

ENDIF
END