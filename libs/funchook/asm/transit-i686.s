	.text
transit:

	call get_eip
	lea transit - . (%eax),%eax
	jmp *hook_caller_addr - transit (%eax)
get_eip:
	movl (%esp), %eax
	ret

	.balign 4
hook_caller_addr:

	.byte  0x0f,0x1f,0x40,0x00
