#ifndef X86DEFS_H
#define X86DEFS_H

#define SEG_REGS_MAX (6)
#define CREGS_MAX (9)
#define DREGS_MAX (8)

#define INST_MAXIMUM_SIZE (15)

#define INST_CMP_MAX_RANGE (8)

#define INST_VCMP_MAX_RANGE (32)

#define INST_WAIT_INDEX (0x9b)

#define INST_LEA_INDEX (0x8d)

#define INST_NOP_INDEX (0x90)

#define INST_ARPL_INDEX (0x63)

#define INST_DIVIDED_MODRM (0xc0)

#define _3DNOW_ESCAPE_BYTE (0x0f)

#define PREFIX_LOCK (0xf0)
#define PREFIX_REPNZ (0xf2)
#define PREFIX_REP (0xf3)
#define PREFIX_CS (0x2e)
#define PREFIX_SS (0x36)
#define PREFIX_DS (0x3e)
#define PREFIX_ES (0x26)
#define PREFIX_FS (0x64)
#define PREFIX_GS (0x65)
#define PREFIX_OP_SIZE (0x66)
#define PREFIX_ADDR_SIZE (0x67)
#define PREFIX_VEX2b (0xc5)
#define PREFIX_VEX3b (0xc4)

#define PREFIX_REX_LOW (0x40)
#define PREFIX_REX_HI (0x4f)

#define EX_GPR_BASE (8)

#define PREFIX_EX_B (1)

#define PREFIX_EX_X (2)

#define PREFIX_EX_R (4)

#define PREFIX_EX_W (8)

#define PREFIX_EX_L (0x10)

#endif
