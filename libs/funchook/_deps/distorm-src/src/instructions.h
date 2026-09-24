#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "config.h"
#include "prefix.h"

typedef enum OpType {

	OT_NONE = 0,

	OT_IMM8,

	OT_IMM16,

	OT_IMM_FULL,

	OT_IMM32,

	OT_SEIMM8,

	OT_REG8,

	OT_REG16,

	OT_REG_FULL,

	OT_REG32,

	OT_REG32_64,

	OT_ACC8,

	OT_ACC16,

	OT_ACC_FULL,

	OT_ACC_FULL_NOT64,

	OT_RELCB,

	OT_RELC_FULL,

	OT_IB_RB,

	OT_IB_R_FULL,

	OT_MOFFS8,
	OT_MOFFS_FULL,

	OT_REGI_ESI,

	OT_REGI_EDI,

	OT_REGI_EBXAL,

	OT_REGI_EAX,

	OT_REGDX,

	OT_REGECX,

	OT_FPU_SI,
	OT_FPU_SSI,
	OT_FPU_SIS,

	OT_XMM,

	OT_XMM_RM,

	OT_REGXMM0,

	OT_WREG32_64,

	OT_VXMM,

	OT_XMM_IMM,

	OT_YXMM,

	OT_YXMM_IMM,

	OT_YMM,

	OT_VYMM,

	OT_VYXMM,

	OT_CONST1,

	OT_REGCL,

	OT_CREG,

	OT_DREG,

	OT_SREG,

	OT_SEG,

	OT_IMM16_1,

	OT_IMM8_1,

	OT_IMM8_2,

	OT_PTR16_FULL,

	OT_FREG32_64_RM,

	OT_MM,

	OT_MM_RM,

	OT_MEM,
	OT_MEM32,

	OT_MEM32_64,
	OT_MEM64,

	OT_MEM64_128,
	OT_MEM128,

	OT_MEM16_FULL,

	OT_MEM16_3264,

	OT_MEM_OPT,

	OT_FPUM16,

	OT_FPUM32,

	OT_FPUM64,

	OT_FPUM80,

	OT_LMEM128_256,

	OT_RM8,

	OT_RM16,

	OT_RM32,

	OT_RFULL_M16,

	OT_RM_FULL,

	OT_WRM32_64,

	OT_R32_64_M8,

	OT_R32_64_M16,

	OT_RM32_64,

	OT_RM16_32,

	OT_R32_M8,

	OT_R32_M16,

	OT_REG32_64_M8,

	OT_REG32_64_M16,

	OT_MM32,

	OT_MM64,

	OT_XMM16,

	OT_XMM32,

	OT_XMM64,

	OT_XMM128,

	OT_WXMM32_64,

	OT_YMM256,

	OT_YXMM64_256,

	OT_YXMM128_256,

	OT_LXMM64_128
} _OpType;

#define INST_FLAGS_NONE (0)

#define INST_MODRM_REQUIRED (1)

#define INST_NOT_DIVIDED (1 << 1)

#define INST_16BITS (1 << 2)

#define INST_32BITS (1 << 3)

#define INST_PRE_LOCK (1 << 4)

#define INST_PRE_REPNZ (1 << 5)

#define INST_PRE_REP (1 << 6)

#define INST_PRE_CS (1 << 7)

#define INST_PRE_SS (1 << 8)

#define INST_PRE_DS (1 << 9)

#define INST_PRE_ES (1 << 10)

#define INST_PRE_FS (1 << 11)

#define INST_PRE_GS (1 << 12)

#define INST_PRE_OP_SIZE (1 << 13)

#define INST_PRE_ADDR_SIZE (1 << 14)

#define INST_NATIVE (1 << 15)

#define INST_USE_EXMNEMONIC (1 << 16)

#define INST_USE_OP3 (1 << 17)

#define INST_USE_OP4 (1 << 18)

#define INST_MNEMONIC_MODRM_BASED (1 << 19)

#define INST_MODRR_REQUIRED (1 << 20)

#define INST_3DNOW_FETCH (1 << 21)

#define INST_PSEUDO_OPCODE (1 << 22)

#define INST_INVALID_64BITS (1 << 23)

#define INST_64BITS (1 << 24)

#define INST_PRE_REX (1 << 25)

#define INST_USE_EXMNEMONIC2 (1 << 26)

#define INST_64BITS_FETCH (1 << 27)

#define INST_FORCE_REG0 (1 << 28)

#define INST_PRE_VEX (1 << 29)

#define INST_MODRM_INCLUDED (1 << 30)

#define INST_DST_WR (1 << 31)

#define INST_PRE_REPS (INST_PRE_REPNZ | INST_PRE_REP)
#define INST_PRE_LOKREP_MASK (INST_PRE_LOCK | INST_PRE_REPNZ | INST_PRE_REP)
#define INST_PRE_SEGOVRD_MASK32 (INST_PRE_CS | INST_PRE_SS | INST_PRE_DS | INST_PRE_ES)
#define INST_PRE_SEGOVRD_MASK64 (INST_PRE_FS | INST_PRE_GS)
#define INST_PRE_SEGOVRD_MASK (INST_PRE_SEGOVRD_MASK32 | INST_PRE_SEGOVRD_MASK64)

#define INST_VEX_L (1)

#define INST_VEX_W (1 << 1)

#define INST_MNEMONIC_VEXW_BASED (1 << 2)

#define INST_MNEMONIC_VEXL_BASED (1 << 3)

#define INST_FORCE_VEXL (1 << 4)

#define INST_MODRR_BASED (1 << 5)

#define INST_VEX_V_UNUSED (1 << 6)

#define META_INST_PRIVILEGED ((uint16_t)0x8000)

typedef enum {ONT_NONE = -1, ONT_1 = 0, ONT_2 = 1, ONT_3 = 2, ONT_4 = 3} _OperandNumberType;

#define D_COMPACT_CF 1
#define D_COMPACT_PF 4
#define D_COMPACT_AF 0x10
#define D_COMPACT_ZF 0x40
#define D_COMPACT_SF 0x80

#define D_COMPACT_IF 2
#define D_COMPACT_DF 8
#define D_COMPACT_OF 0x20

#define D_COMPACT_SAME_FLAGS (D_COMPACT_CF | D_COMPACT_PF | D_COMPACT_AF | D_COMPACT_ZF | D_COMPACT_SF)

typedef struct {
	uint8_t flagsIndex;
	uint8_t s, d;

	uint8_t modifiedFlagsMask;
	uint8_t testedFlagsMask;
	uint8_t undefinedFlagsMask;
	uint16_t meta;
} _InstSharedInfo;

typedef struct {
	uint16_t sharedIndex;
	uint16_t opcodeId;
} _InstInfo;

typedef struct {

	_InstInfo BASE;

	uint8_t flagsEx;
	uint8_t op3, op4;
	uint16_t opcodeId2, opcodeId3;
} _InstInfoEx;

typedef enum {
	INT_NOTEXISTS = 0,
	INT_INFO = 1,
	INT_INFOEX,
	INT_INFO_TREAT,
	INT_LIST_GROUP,
	INT_LIST_FULL,
	INT_LIST_DIVIDED,
	INT_LIST_PREFIXED
} _InstNodeType;

#define INT_INFOS (INT_LIST_GROUP)

typedef uint16_t _InstNode;

_InstInfo* inst_lookup(_CodeInfo* ci, _PrefixState* ps, int* isPrefixed);
_InstInfo* inst_lookup_3dnow(_CodeInfo* ci);

#endif
