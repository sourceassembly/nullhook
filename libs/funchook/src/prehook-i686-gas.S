#if defined(__MINGW32__)
#define funchook_hook_caller_asm _funchook_hook_caller_asm
#define funchook_hook_caller _funchook_hook_caller
#endif

	.text
	.p2align 4
	.globl funchook_hook_caller_asm
#if defined(__linux__)
	.type funchook_hook_caller_asm, @function
#endif
#if defined(__MINGW32__)
	.def	funchook_hook_caller_asm;	.scl	2;	.type	32
#endif
funchook_hook_caller_asm:
	.cfi_startproc

	.byte 0xf3, 0x0f, 0x1e, 0xfb
	push %ebp
	.cfi_def_cfa_offset 8
	.cfi_offset ebp, -0x08
	mov  %esp, %ebp
	.cfi_def_cfa_register ebp

	push %edx
	.cfi_offset edx, -0x0C
	push %ecx
	.cfi_offset ecx, -0x10

	and  $0xfffffff0, %esp

	sub  $8, %esp

	push %ebp

	push %eax

	call funchook_hook_caller

	mov  -0x8(%ebp), %ecx
	mov  -0x4(%ebp), %edx

	leave
	.cfi_def_cfa esp, 4

	jmp  *%eax
	.cfi_endproc
#if defined(__linux__)
	.size funchook_hook_caller_asm, .-funchook_hook_caller_asm
#endif
#if defined(__MINGW32__)
	.endef
#endif
