#if defined __APPLE__
#define funchook_hook_caller_asm _funchook_hook_caller_asm
#define funchook_hook_caller _funchook_hook_caller
#endif
	.arch armv8-a
	.text
	.globl	funchook_hook_caller_asm
#if defined __linux__
	.type	funchook_hook_caller_asm, %function
#endif
funchook_hook_caller_asm:
	.cfi_startproc

	stp x29, x30, [sp, -0xe0]!
	.cfi_def_cfa_offset 0xe0
	.cfi_offset 29, -0xe0
	.cfi_offset 30, -0xd8

	mov x29, sp

	stp x0, x1, [sp, 0x10]
	.cfi_offset 0, -0xd0
	.cfi_offset 1, -0xc8
	stp x2, x3, [sp, 0x20]
	.cfi_offset 2, -0xc0
	.cfi_offset 3, -0xb8
	stp x4, x5, [sp, 0x30]
	.cfi_offset 4, -0xb0
	.cfi_offset 5, -0xa8
	stp x6, x7, [sp, 0x40]
	.cfi_offset 6, -0xa0
	.cfi_offset 7, -0x98

	stp x8, x18, [sp, 0x50]
	.cfi_offset 8, -0x90
	.cfi_offset 18, -0x88

	stp q0, q1, [sp, 0x60]
	.cfi_offset q0, -0x80
	.cfi_offset q1, -0x70
	stp q2, q3, [sp, 0x80]
	.cfi_offset q2, -0x60
	.cfi_offset q3, -0x50
	stp q4, q5, [sp, 0xa0]
	.cfi_offset q4, -0x40
	.cfi_offset q5, -0x30
	stp q6, q7, [sp, 0xc0]
	.cfi_offset q6, -0x20
	.cfi_offset q7, -0x10

	mov x0, x10

	mov x1, x29

	bl  funchook_hook_caller
	mov x9, x0

	ldp x0, x1, [sp, 0x10]
	.cfi_restore 0
	.cfi_restore 1
	ldp x2, x3, [sp, 0x20]
	.cfi_restore 2
	.cfi_restore 3
	ldp x4, x5, [sp, 0x30]
	.cfi_restore 4
	.cfi_restore 5
	ldp x6, x7, [sp, 0x40]
	.cfi_restore 6
	.cfi_restore 7
	ldp x8, x18, [sp, 0x50]
	.cfi_restore 8
	.cfi_restore 18
	ldp q0, q1, [sp, 0x60]
	.cfi_restore q0
	.cfi_restore q1
	ldp q2, q3, [sp, 0x80]
	.cfi_restore q2
	.cfi_restore q3
	ldp q4, q5, [sp, 0xa0]
	.cfi_restore q4
	.cfi_restore q5
	ldp q6, q7, [sp, 0xc0]
	.cfi_restore q6
	.cfi_restore q7
	ldp x29, x30, [sp], 0xe0
	.cfi_restore 29
	.cfi_restore 30
	.cfi_def_cfa_offset 0

	br x9
	.cfi_endproc
