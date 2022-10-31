IFDEF RAX

_TEXT SEGMENT
ALIGN 16
PUBLIC KexNtQuerySystemTime
KexNtQuerySystemTime PROC

	mov			r10, rcx
	mov			eax, 57h
	syscall
	ret

KexNtQuerySystemTime ENDP
_TEXT ENDS

ENDIF
END