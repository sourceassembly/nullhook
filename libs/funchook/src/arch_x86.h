#ifndef FUNCHOOK_X86_H
#define FUNCHOOK_X86_H 1

#define MAX_INSN_LEN 16
#define MAX_INSN_CHECK_SIZE 256

#define REL2G_JUMP_SIZE 5

#if defined(__x86_64__) || defined(_M_AMD64)
#define TRAMPOLINE_PREFIX_SIZE 4
#else
#define TRAMPOLINE_PREFIX_SIZE 0
#endif

#define TRAMPOLINE_SIZE (TRAMPOLINE_PREFIX_SIZE + REL2G_JUMP_SIZE + (MAX_INSN_LEN - 1) + REL2G_JUMP_SIZE)
#define MAX_PATCH_CODE_SIZE (REL2G_JUMP_SIZE + MAX_INSN_LEN - 1)

typedef uint8_t insn_t;

typedef struct {
    const insn_t *dst_addr;
    intptr_t src_addr_offset;
    intptr_t pos_offset;
} ip_displacement_entry_t;

typedef struct {
    ip_displacement_entry_t disp[2];
} ip_displacement_t;

#endif
