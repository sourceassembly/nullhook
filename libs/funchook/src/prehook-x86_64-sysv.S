#if defined __APPLE__
#define funchook_hook_caller_asm _funchook_hook_caller_asm
#define funchook_hook_caller _funchook_hook_caller
#endif
#if defined __linux__ && defined __PIC__
#define funchook_hook_caller funchook_hook_caller@PLT
#endif
	.text
	.globl	funchook_hook_caller_asm
#if defined __linux__
	.type	funchook_hook_caller_asm, @function
#endif
funchook_hook_caller_asm:
	.cfi_startproc
	.byte 0xf3, 0x0f, 0x1e, 0xfa
	pushq  %rbp
	.cfi_def_cfa_offset 0x10
	.cfi_offset rbp, -0x10
	movq   %rsp, %rbp
	.cfi_def_cfa_register rbp
	sub    $0xc0, %rsp

	mov    %rdi, -0xc0(%rbp)
	mov    %rsi, -0xb8(%rbp)
	mov    %rdx, -0xb0(%rbp)
	mov    %rcx, -0xa8(%rbp)
	mov    %r8, -0xa0(%rbp)
	mov    %r9, -0x98(%rbp)

	mov    %rax, -0x90(%rbp)

	mov    %r10, -0x88(%rbp)

	movdqu %xmm0, -0x80(%rbp)
	movdqu %xmm1, -0x70(%rbp)
	movdqu %xmm2, -0x60(%rbp)
	movdqu %xmm3, -0x50(%rbp)
	movdqu %xmm4, -0x40(%rbp)
	movdqu %xmm5, -0x30(%rbp)
	movdqu %xmm6, -0x20(%rbp)
	movdqu %xmm7, -0x10(%rbp)

	and    $0xfffffffffffffff0, %rsp

	mov    %r11, %rdi

	mov    %rbp, %rsi

	call   funchook_hook_caller
	mov    %rax, %r11

	mov    -0xc0(%rbp), %rdi
	mov    -0xb8(%rbp), %rsi
	mov    -0xb0(%rbp), %rdx
	mov    -0xa8(%rbp), %rcx
	mov    -0xa0(%rbp), %r8
	mov    -0x98(%rbp), %r9
	mov    -0x90(%rbp), %rax
	mov    -0x88(%rbp), %r10
	movdqu -0x80(%rbp), %xmm0
	movdqu -0x70(%rbp), %xmm1
	movdqu -0x60(%rbp), %xmm2
	movdqu -0x50(%rbp), %xmm3
	movdqu -0x40(%rbp), %xmm4
	movdqu -0x30(%rbp), %xmm5
	movdqu -0x20(%rbp), %xmm6
	movdqu -0x10(%rbp), %xmm7
	leave

	jmp    *%r11
	.cfi_endproc
#if !defined __APPLE__
	.size funchook_hook_caller_asm, .-funchook_hook_caller_asm
#endif
