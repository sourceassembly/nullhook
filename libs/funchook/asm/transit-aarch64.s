	.arch armv8-a
	.text
transit:

	adr x10, transit
	ldr x9, hook_caller_addr
	br x9

	.balign 8
hook_caller_addr:

	nop
	nop
