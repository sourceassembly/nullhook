	.686P
	.XMM
	.model	flat

EXTRN	_funchook_hook_caller:PROC

_TEXT	SEGMENT
_funchook_hook_caller_asm PROC
	push ebp
	mov  ebp, esp

	push edx
	push ecx

	push ebp

	push eax

	call _funchook_hook_caller
	add  esp, 08h

	pop  ecx
	pop  edx

	leave

	jmp  eax
_funchook_hook_caller_asm ENDP
_TEXT	ENDS
END
