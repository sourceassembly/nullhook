#ifndef DISTORM_H
#define DISTORM_H

#if !(defined(DISTORM_STATIC) || defined(DISTORM_DYNAMIC))

	#define SUPPORT_64BIT_OFFSET
#endif

#ifdef __TINYC__
	#undef SUPPORT_64BIT_OFFSET
#endif

#ifndef _MSC_VER
#include <stdint.h>
#else

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#endif

#ifdef SUPPORT_64BIT_OFFSET
#define OFFSET_INTEGER uint64_t
#else

#define OFFSET_INTEGER uint32_t
#endif

#ifdef __cplusplus
 extern "C" {
#endif

#define META_GET_ISC(meta) (((meta) >> 8) & 0x1f)
#define META_SET_ISC(di, isc) (((di)->meta) |= ((isc) << 8))

#define META_GET_FC(meta) ((meta) & 0xf)

#define INSTRUCTION_GET_TARGET(di) ((_OffsetType)(((di)->addr + (di)->imm.addr + (di)->size)))

#define INSTRUCTION_GET_RIP_TARGET(di) ((_OffsetType)(((di)->addr + (di)->disp + (di)->size)))

#define FLAG_SET_OPSIZE(di, size) ((di->flags) |= (((size) & 3) << 8))
#define FLAG_SET_ADDRSIZE(di, size) ((di->flags) |= (((size) & 3) << 10))
#define FLAG_GET_OPSIZE(flags) (((flags) >> 8) & 3)
#define FLAG_GET_ADDRSIZE(flags) (((flags) >> 10) & 3)

#define FLAG_GET_PREFIX(flags) (((unsigned int)((int16_t)flags)) & 7)

#define FLAG_GET_PRIVILEGED(flags) (((flags) & FLAG_PRIVILEGED_INSTRUCTION) != 0)

#define SEGMENT_DEFAULT 0x80
#define SEGMENT_GET(segment) (((segment) == R_NONE) ? R_NONE : ((segment) & 0x7f))
#define SEGMENT_GET_UNSAFE(segment) ((segment) & 0x7f)
#define SEGMENT_IS_DEFAULT(segment) (((int8_t)segment) < -1)
#define SEGMENT_IS_DEFAULT_OR_NONE(segment) (((uint8_t)(segment)) > 0x80)

typedef enum { Decode16Bits = 0, Decode32Bits = 1, Decode64Bits = 2 } _DecodeType;

typedef OFFSET_INTEGER _OffsetType;

typedef struct {
	_OffsetType codeOffset, addrMask;
	_OffsetType nextOffset;
	const uint8_t* code;
	int codeLen;
	_DecodeType dt;
	unsigned int features;
} _CodeInfo;

typedef enum { O_NONE, O_REG, O_IMM, O_IMM1, O_IMM2, O_DISP, O_SMEM, O_MEM, O_PC, O_PTR } _OperandType;

typedef union {

	int8_t sbyte;
	uint8_t byte;
	int16_t sword;
	uint16_t word;
	int32_t sdword;
	uint32_t dword;
	int64_t sqword;
	uint64_t qword;

	_OffsetType addr;

	struct {
		uint16_t seg;

		uint32_t off;
	} ptr;

	struct {
		uint32_t i1;
		uint32_t i2;
	} ex;
} _Value;

typedef struct {

	uint8_t type;

	uint8_t index;

	uint16_t size;
} _Operand;

#define OPCODE_ID_NONE 0

#define FLAG_NOT_DECODABLE ((uint16_t)-1)

#define FLAG_LOCK (1 << 0)

#define FLAG_REPNZ (1 << 1)

#define FLAG_REP (1 << 2)

#define FLAG_HINT_TAKEN (1 << 3)

#define FLAG_HINT_NOT_TAKEN (1 << 4)

#define FLAG_IMM_SIGNED (1 << 5)

#define FLAG_DST_WR (1 << 6)

#define FLAG_RIP_RELATIVE (1 << 7)

#define FLAG_PRIVILEGED_INSTRUCTION (1 << 15)

#define R_NONE ((uint8_t)-1)

#define REGS64_BASE 0
#define REGS32_BASE 16
#define REGS16_BASE 32
#define REGS8_BASE 48
#define REGS8_REX_BASE 64
#define SREGS_BASE 68
#define FPUREGS_BASE 75
#define MMXREGS_BASE 83
#define SSEREGS_BASE 91
#define AVXREGS_BASE 107
#define CREGS_BASE 123
#define DREGS_BASE 132

#define OPERANDS_NO (4)

typedef struct {

	_Value imm;

	uint64_t disp;

	_OffsetType addr;

	uint16_t flags;

	uint16_t unusedPrefixesMask;

	uint32_t usedRegistersMask;

	uint16_t opcode;

	_Operand ops[OPERANDS_NO];

	uint8_t opsNo;

	uint8_t size;

	uint8_t segment;

	uint8_t base, scale;
	uint8_t dispSize;

	uint16_t meta;

	uint16_t modifiedFlagsMask, testedFlagsMask, undefinedFlagsMask;
} _DInst;

#ifndef DISTORM_LIGHT

#define MAX_TEXT_SIZE (48)
typedef struct {
	unsigned int length;
	unsigned char p[MAX_TEXT_SIZE];
} _WString;

typedef struct {
	_OffsetType offset;
	unsigned int size;
	_WString mnemonic;
	_WString operands;
	_WString instructionHex;
} _DecodedInst;

#endif

#define RM_AX 1
#define RM_CX 2
#define RM_DX 4
#define RM_BX 8
#define RM_SP 0x10
#define RM_BP 0x20
#define RM_SI 0x40
#define RM_DI 0x80
#define RM_FPU 0x100
#define RM_MMX 0x200
#define RM_SSE 0x400
#define RM_AVX 0x800
#define RM_CR 0x1000
#define RM_DR 0x2000
#define RM_R8 0x4000
#define RM_R9 0x8000
#define RM_R10 0x10000
#define RM_R11 0x20000
#define RM_R12 0x40000
#define RM_R13 0x80000
#define RM_R14 0x100000
#define RM_R15 0x200000
#define RM_SEG 0x400000

#define D_CF 1
#define D_PF 4
#define D_AF 0x10
#define D_ZF 0x40
#define D_SF 0x80
#define D_IF 0x200
#define D_DF 0x400
#define D_OF 0x800

#define ISC_INTEGER 1

#define ISC_FPU 2

#define ISC_P6 3

#define ISC_MMX 4

#define ISC_SSE 5

#define ISC_SSE2 6

#define ISC_SSE3 7

#define ISC_SSSE3 8

#define ISC_SSE4_1 9

#define ISC_SSE4_2 10

#define ISC_SSE4_A 11

#define ISC_3DNOW 12

#define ISC_3DNOWEXT 13

#define ISC_VMX 14

#define ISC_SVM 15

#define ISC_AVX 16

#define ISC_FMA 17

#define ISC_AES 18

#define ISC_CLMUL 19

#define DF_NONE 0

#define DF_MAXIMUM_ADDR16 1

#define DF_MAXIMUM_ADDR32 2

#define DF_RETURN_FC_ONLY 4

#define DF_STOP_ON_CALL 8

#define DF_STOP_ON_RET 0x10

#define DF_STOP_ON_SYS 0x20

#define DF_STOP_ON_UNC_BRANCH 0x40

#define DF_STOP_ON_CND_BRANCH 0x80

#define DF_STOP_ON_INT 0x100

#define DF_STOP_ON_CMOV 0x200

#define DF_STOP_ON_HLT 0x400

#define DF_STOP_ON_PRIVILEGED 0x800

#define DF_STOP_ON_UNDECODEABLE 0x1000

#define DF_SINGLE_BYTE_STEP 0x2000

#define DF_FILL_EFLAGS 0x4000

#define DF_USE_ADDR_MASK 0x8000

#define DF_STOP_ON_FLOW_CONTROL (DF_STOP_ON_CALL | DF_STOP_ON_RET | DF_STOP_ON_SYS | DF_STOP_ON_UNC_BRANCH | DF_STOP_ON_CND_BRANCH | DF_STOP_ON_INT | DF_STOP_ON_CMOV | DF_STOP_ON_HLT)

#define FC_NONE 0

#define FC_CALL 1

#define FC_RET 2

#define FC_SYS 3

#define FC_UNC_BRANCH 4

#define FC_CND_BRANCH 5

#define FC_INT 6

#define FC_CMOV 7

#define FC_HLT 8

typedef enum { DECRES_NONE, DECRES_SUCCESS, DECRES_MEMORYERR, DECRES_INPUTERR } _DecodeResult;

#if !(defined(DISTORM_STATIC) || defined(DISTORM_DYNAMIC))

#ifdef SUPPORT_64BIT_OFFSET

	_DecodeResult distorm_decompose64(_CodeInfo* ci, _DInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount);
	#define distorm_decompose distorm_decompose64

#ifndef DISTORM_LIGHT

	_DecodeResult distorm_decode64(_OffsetType codeOffset, const unsigned char* code, int codeLen, _DecodeType dt, _DecodedInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount);
	void distorm_format64(const _CodeInfo* ci, const _DInst* di, _DecodedInst* result);
	#define distorm_decode distorm_decode64
	#define distorm_format distorm_format64
#endif

#else

	_DecodeResult distorm_decompose32(_CodeInfo* ci, _DInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount);
	#define distorm_decompose distorm_decompose32

#ifndef DISTORM_LIGHT

	_DecodeResult distorm_decode32(_OffsetType codeOffset, const unsigned char* code, int codeLen, _DecodeType dt, _DecodedInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount);
	void distorm_format32(const _CodeInfo* ci, const _DInst* di, _DecodedInst* result);
	#define distorm_decode distorm_decode32
	#define distorm_format distorm_format32
#endif

#endif

unsigned int distorm_version(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
