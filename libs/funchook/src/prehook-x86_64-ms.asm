EXTRN	funchook_hook_caller:PROC

_text SEGMENT
funchook_hook_caller_asm PROC FRAME
	push rbp
.pushreg rbp
	mov  rbp, rsp
.setframe rbp, 0
	sub  rsp, 0a0h
.allocstack 0a0h
.endprolog

	mov  [rbp - 080h], rcx
	mov  [rbp - 078h], rdx
	mov  [rbp - 070h], r8
	mov  [rbp - 068h], r9

	movdqu [rbp - 060h], xmm0
	movdqu [rbp - 050h], xmm1
	movdqu [rbp - 040h], xmm2
	movdqu [rbp - 030h], xmm3

	movdqu [rbp - 020h], xmm4
	movdqu [rbp - 010h], xmm5

	and  rsp, 0fffffffffffffff0h

	mov  rcx, r11

	mov  rdx, rbp

	call funchook_hook_caller

	mov  rcx, [rbp - 080h]
	mov  rdx, [rbp - 078h]
	mov  r8, [rbp - 070h]
	mov  r9, [rbp - 068h]
	movdqu xmm0, [rbp - 060h]
	movdqu xmm1, [rbp - 050h]
	movdqu xmm2, [rbp - 040h]
	movdqu xmm3, [rbp - 030h]
	movdqu xmm4, [rbp - 020h]
	movdqu xmm5, [rbp - 010h]
	leave

	jmp  rax
funchook_hook_caller_asm ENDP
_text ENDS
END
