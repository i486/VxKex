IFDEF RAX
_TEXT SEGMENT

EXTERN KexPostSuccessfulInitializationApcRoutine : PROC
PUBLIC KexPostInitializationApcRoutine

;
; The NTSTATUS code of loader initialization is stored in EDI for all known
; 64-bit builds of Windows 7 NTDLL, including debug-checked builds.
;
; If loader initialization has failed, we must return from the APC procedure
; to allow the loader to display any applicable error boxes, etc.
;

KexPostInitializationApcRoutine PROC
	test		edi, edi
	jns			KexPostSuccessfulInitializationApcRoutine
	ret
KexPostInitializationApcRoutine ENDP

_TEXT ENDS

ENDIF
END