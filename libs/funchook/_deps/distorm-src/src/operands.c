#include "config.h"
#include "operands.h"
#include "x86defs.h"
#include "insts.h"
#include "../include/mnemonics.h"

uint32_t _REGISTERTORCLASS[] =
{RM_AX, RM_CX, RM_DX, RM_BX, RM_SP, RM_BP, RM_SI, RM_DI, RM_R8, RM_R9, RM_R10, RM_R11, RM_R12, RM_R13, RM_R14, RM_R15,
 RM_AX, RM_CX, RM_DX, RM_BX, RM_SP, RM_BP, RM_SI, RM_DI, RM_R8, RM_R9, RM_R10, RM_R11, RM_R12, RM_R13, RM_R14, RM_R15,
 RM_AX, RM_CX, RM_DX, RM_BX, RM_SP, RM_BP, RM_SI, RM_DI, RM_R8, RM_R9, RM_R10, RM_R11, RM_R12, RM_R13, RM_R14, RM_R15,
 RM_AX, RM_CX, RM_DX, RM_BX, RM_AX, RM_CX, RM_DX, RM_BX, RM_R8, RM_R9, RM_R10, RM_R11, RM_R12, RM_R13, RM_R14, RM_R15,
 RM_SP, RM_BP, RM_SI, RM_DI,
 RM_SEG, RM_SEG, RM_SEG, RM_SEG, RM_SEG, RM_SEG,
 0,
 RM_FPU, RM_FPU, RM_FPU, RM_FPU, RM_FPU, RM_FPU, RM_FPU, RM_FPU,
 RM_MMX, RM_MMX, RM_MMX, RM_MMX, RM_MMX, RM_MMX, RM_MMX, RM_MMX,
 RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE,
 RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX,
 RM_CR, 0, RM_CR, RM_CR, RM_CR, 0, 0, 0, RM_CR,
 RM_DR, RM_DR, RM_DR, RM_DR, 0, 0, RM_DR, RM_DR
};

_INLINE_ unsigned int _FASTCALL_ operands_fix_8bit_rex_base(unsigned int reg)
{
	if ((reg >= 4) && (reg < 8)) return reg + REGS8_REX_BASE - 4;
	return reg + REGS8_BASE;
}

_INLINE_ void operands_set_ts(_Operand* op, _OperandType type, uint16_t size)
{
	op->type = type;
	op->size = size;
}

_INLINE_ void operands_set_tsi(_DInst* di, _Operand* op, _OperandType type, uint16_t size, unsigned int index)
{
	op->type = type;
	op->index = (uint8_t)index;
	op->size = size;
	di->usedRegistersMask |= _REGISTERTORCLASS[index];
}

_INLINE_ int read_stream_safe_uint8(_CodeInfo* ci, void* result)
{
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return FALSE;
	*(uint8_t*)result = *(uint8_t*)ci->code;
	ci->code += 1;
	return TRUE;
}

_INLINE_ int read_stream_safe_uint16(_CodeInfo* ci, void* result)
{
	ci->codeLen -= 2;
	if (ci->codeLen < 0) return FALSE;
	*(uint16_t*)result = RUSHORT(ci->code);
	ci->code += 2;
	return TRUE;
}

_INLINE_ int read_stream_safe_uint32(_CodeInfo* ci, void* result)
{
	ci->codeLen -= 4;
	if (ci->codeLen < 0) return FALSE;
	*(uint32_t*)result = RULONG(ci->code);
	ci->code += 4;
	return TRUE;
}

_INLINE_ int read_stream_safe_uint64(_CodeInfo* ci, void* result)
{
	ci->codeLen -= 8;
	if (ci->codeLen < 0) return FALSE;
	*(uint64_t*)result = RULLONG(ci->code);
	ci->code += 8;
	return TRUE;
}

_INLINE_ int read_stream_safe_sint8(_CodeInfo* ci, int64_t* result)
{
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return FALSE;
	*result = *(int8_t*)ci->code;
	ci->code += 1;
	return TRUE;
}

_INLINE_ int read_stream_safe_sint16(_CodeInfo* ci, int64_t* result)
{
	ci->codeLen -= 2;
	if (ci->codeLen < 0) return FALSE;
	*result = RSHORT(ci->code);
	ci->code += 2;
	return TRUE;
}

_INLINE_ int read_stream_safe_sint32(_CodeInfo* ci, int64_t* result)
{
	ci->codeLen -= 4;
	if (ci->codeLen < 0) return FALSE;
	*result = RLONG(ci->code);
	ci->code += 4;
	return TRUE;
}

static void operands_extract_sib(_DInst* di,
                                 _PrefixState* ps, _DecodeType effAdrSz,
                                 unsigned int sib, unsigned int mod, _Operand* op)
{
	unsigned char scale, index, base;
	unsigned int vrex = ps->vrex;
	uint8_t* pIndex = NULL;

	index = (sib >> 3) & 7;
	base = sib & 7;

	if (vrex & PREFIX_EX_X) {
		ps->usedPrefixes |= INST_PRE_REX;
		index += EX_GPR_BASE;
	}

	if (index == 4) {
		op->type = O_SMEM;
		pIndex = &op->index;
	} else {
		op->type = O_MEM;
		pIndex = &di->base;

	}

	if (base != 5) {
		if (vrex & PREFIX_EX_B) ps->usedPrefixes |= INST_PRE_REX;
		*pIndex = effAdrSz == Decode64Bits ? REGS64_BASE : REGS32_BASE;
		*pIndex += (uint8_t)(base + ((vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0));

		if (di->base != R_NONE) di->usedRegistersMask |= _REGISTERTORCLASS[di->base];
	} else if (mod != 0) {

		if (vrex & PREFIX_EX_B) ps->usedPrefixes |= INST_PRE_REX;
		if (effAdrSz == Decode64Bits) *pIndex = REGS64_BASE + 5 + ((vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0);
		else *pIndex = REGS32_BASE + 5 + ((vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0);

		if (di->base != R_NONE) di->usedRegistersMask |= _REGISTERTORCLASS[di->base];
	} else if (index == 4) {

		op->type = O_DISP;
		return;
	}

	if (index != 4) {
		scale = (sib >> 6) & 3;
		if (effAdrSz == Decode64Bits) op->index = (uint8_t)(REGS64_BASE + index);
		else op->index = (uint8_t)(REGS32_BASE + index);
		di->scale = scale != 0 ? (1 << scale) : 0;
	}
}

static int operands_extract_modrm(_CodeInfo* ci, _PrefixState* ps, _DInst* di,
                                  _DecodeType effAdrSz, unsigned int mod, unsigned int rm,
                                  _iflags instFlags, _Operand* op)
{
	unsigned char sib = 0, base = 0;

	ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
	if ((instFlags & INST_PRE_LOCK) && (ps->decodedPrefixes & INST_PRE_LOCK)) {
		ps->usedPrefixes |= INST_PRE_LOCK;
		di->flags |= FLAG_LOCK;
	}

	if (effAdrSz != Decode16Bits) {

		if ((rm == 5) && (mod == 0)) {

			di->dispSize = 32;
			if (!read_stream_safe_sint32(ci, (int64_t*)&di->disp)) return FALSE;

			op->type = O_DISP;

			if (ci->dt == Decode64Bits) {

				op->type = O_SMEM;
				op->index = R_RIP;
				di->flags |= FLAG_RIP_RELATIVE;
			}

			prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);
		}
		else {
			if (rm == 4) {

				if (!read_stream_safe_uint8(ci, &sib)) return FALSE;
				operands_extract_sib(di, ps, effAdrSz, sib, mod, op);
			}
			else {
				op->type = O_SMEM;
				if (ps->vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}

				if (effAdrSz == Decode64Bits) op->index = (uint8_t)(REGS64_BASE + rm);
				else op->index = (uint8_t)(REGS32_BASE + rm);
			}

			if (mod == 1) {
				di->dispSize = 8;
				if (!read_stream_safe_sint8(ci, (int64_t*)&di->disp)) return FALSE;
			}
			else if ((mod == 2) || ((sib & 7) == 5)) {
				di->dispSize = 32;
				if (!read_stream_safe_sint32(ci, (int64_t*)&di->disp)) return FALSE;
			}

			base = op->index;
			if (di->base != R_NONE) base = di->base;
			else if (di->scale >= 2) base = 0;

			if ((base == R_EBP) || (base == R_ESP)) prefixes_use_segment(INST_PRE_SS, ps, ci->dt, di);
			else prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);
		}
	}
	else {

		if ((mod == 0) && (rm == 6)) {

			op->type = O_DISP;
			di->dispSize = 16;
			if (!read_stream_safe_sint16(ci, (int64_t*)&di->disp)) return FALSE;
		}
		else {

			static uint8_t MODS[] = { R_BX, R_BX, R_BP, R_BP, R_SI, R_DI, R_BP, R_BX };
			static uint8_t MODS2[] = { R_SI, R_DI, R_SI, R_DI };
			if (rm < 4) {
				op->type = O_MEM;
				di->base = MODS[rm];
				di->usedRegistersMask |= _REGISTERTORCLASS[MODS[rm]];
				op->index = MODS2[rm];
			}
			else {
				op->type = O_SMEM;
				op->index = MODS[rm];
			}

			if (mod == 1) {
				di->dispSize = 8;
				if (!read_stream_safe_sint8(ci, (int64_t*)&di->disp)) return FALSE;
			}
			else if (mod == 2) {
				di->dispSize = 16;
				if (!read_stream_safe_sint16(ci, (int64_t*)&di->disp)) return FALSE;
			}
		}

		if ((rm == 2) || (rm == 3) || ((rm == 6) && (mod != 0))) {

			prefixes_use_segment(INST_PRE_SS, ps, ci->dt, di);
		}
		else {

			prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);
		}
	}

	return TRUE;
}

int operands_extract(_CodeInfo* ci, _DInst* di, _InstInfo* ii,
                     _iflags instFlags, _OpType type,
                     unsigned int modrm, _PrefixState* ps, _DecodeType effOpSz,
                     _DecodeType effAdrSz, _Operand* op)
{
	int ret = 0;
	unsigned int mod, reg, rm;
	unsigned int size = 0;

	if ((type >= OT_MEM) && (type <= OT_LMEM128_256)) {

		mod = (modrm >> 6) & 3;

		if (mod == 3) {
			if (type == OT_MEM_OPT) {

				return TRUE;
			}
			return FALSE;
		}

		switch (type)
		{
			case OT_MEM64_128:
				if (effOpSz == Decode64Bits) {
					ps->usedPrefixes |= INST_PRE_REX;
					size = 128;
				}
				else size = 64;
			break;
			case OT_MEM32: size = 32; break;
			case OT_MEM32_64:

				if (effOpSz == Decode64Bits) {
					ps->usedPrefixes |= INST_PRE_REX;
					size = 64;
				}
				else size = 32;
			break;
			case OT_MEM64: size = 64; break;
			case OT_MEM128: size = 128; break;
			case OT_MEM16_FULL:
				switch (effOpSz)
				{
					case Decode16Bits:
						ps->usedPrefixes |= INST_PRE_OP_SIZE;
						size = 16;
					break;
					case Decode32Bits:
						ps->usedPrefixes |= INST_PRE_OP_SIZE;
						size = 32;
					break;
					case Decode64Bits:

						if ((instFlags & (INST_64BITS | INST_PRE_REX)) == (INST_64BITS | INST_PRE_REX)) ps->usedPrefixes |= INST_PRE_REX;
						size = 64;
					break;
				}
			break;
			case OT_MEM16_3264:
				if (ci->dt == Decode64Bits) size = 64;
				else size = 32;
			break;
			case OT_FPUM16: size = 16; break;
			case OT_FPUM32: size = 32; break;
			case OT_FPUM64: size = 64; break;
			case OT_FPUM80: size = 80; break;
			case OT_LMEM128_256:
				if (ps->vrex & PREFIX_EX_L) size = 256;
				else size = 128;
			break;
			case OT_MEM_OPT:
			case OT_MEM: size = 0;   break;
			default: return FALSE;
		}
		rm = modrm & 7;
		ret = operands_extract_modrm(ci, ps, di, effAdrSz, mod, rm, instFlags, op);
		op->size = (uint16_t)size;
		if ((op->type == O_SMEM) || (op->type == O_MEM)) {
			di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
		}
		return ret;
	}

	if ((type >= OT_RM8) && (type <= OT_LXMM64_128)) {
		mod = (modrm >> 6) & 3;
		if (mod != 3) {
			switch (type)
			{
				case OT_RM_FULL:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;

					if (effOpSz == Decode32Bits) {
						size = 32;
						break;
					}
					else if (effOpSz == Decode64Bits) {

						if ((instFlags & INST_64BITS) == 0) ps->usedPrefixes |= INST_PRE_REX;
						size = 64;
						break;
					}

				case OT_RM16:

					if (type != OT_RM16) ps->usedPrefixes |= INST_PRE_OP_SIZE;
					size = 16;
				break;
				case OT_RM32_64:

					if (effOpSz == Decode64Bits) {
						size = 64;

						if ((instFlags & (INST_64BITS | INST_PRE_REX)) == (INST_64BITS | INST_PRE_REX)) {
							ps->usedPrefixes |= INST_PRE_REX;
						}
					}
					else size = 32;
				break;
				case OT_RM16_32:

					if (ps->decodedPrefixes & INST_PRE_OP_SIZE) {
						ps->usedPrefixes |= INST_PRE_OP_SIZE;

						size = 16;
					}
					else size = 32;
				break;
				case OT_WXMM32_64:
				case OT_WRM32_64:
					if (ps->vrex & PREFIX_EX_W) size = 64;
					else size = 32;
				break;
				case OT_YXMM64_256:
					if (ps->vrex & PREFIX_EX_L) size = 256;
					else size = 64;
				break;
				case OT_YXMM128_256:
					if (ps->vrex & PREFIX_EX_L) size = 256;
					else size = 128;
				break;
				case OT_LXMM64_128:
					if (ps->vrex & PREFIX_EX_L) size = 128;
					else size = 64;
				break;
				case OT_RFULL_M16:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					size = 16;
				break;

				case OT_RM8:
				case OT_R32_M8:
				case OT_R32_64_M8:
				case OT_REG32_64_M8:
					size = 8;
				break;

				case OT_XMM16:
				case OT_R32_M16:
				case OT_R32_64_M16:
				case OT_REG32_64_M16:
					size = 16;
				break;

				case OT_RM32:
				case OT_MM32:
				case OT_XMM32:
					size = 32;
				break;

				case OT_MM64:
				case OT_XMM64:
					size = 64;
				break;

				case OT_XMM128: size = 128; break;
				case OT_YMM256: size = 256; break;
				default: return FALSE;
			}

			rm = modrm & 7;
			ret = operands_extract_modrm(ci, ps, di, effAdrSz, mod, rm, instFlags, op);
			op->size = (uint16_t)size;
			if ((op->type == O_SMEM) || (op->type == O_MEM)) {
				di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
			}
			return ret;
		}
		else {

			rm = modrm & 7;
			size = 0;
			switch (type)
			{
			case OT_RFULL_M16:
			case OT_RM_FULL:
				switch (effOpSz)
				{
				case Decode16Bits:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					if (ps->vrex & PREFIX_EX_B) {
						ps->usedPrefixes |= INST_PRE_REX;
						rm += EX_GPR_BASE;
					}
					size = 16;
					rm += REGS16_BASE;
					break;
				case Decode32Bits:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					if (ps->vrex & PREFIX_EX_B) {
						ps->usedPrefixes |= INST_PRE_REX;
						rm += EX_GPR_BASE;
					}
					size = 32;
					rm += REGS32_BASE;
					break;
				case Decode64Bits:

					if (type == OT_RFULL_M16) ps->usedPrefixes |= INST_PRE_REX;

					if (instFlags & INST_PRE_REX) ps->usedPrefixes |= INST_PRE_REX;

					if ((instFlags & INST_64BITS) == 0) ps->usedPrefixes |= INST_PRE_REX;

					if (ps->vrex & PREFIX_EX_B) {
						ps->usedPrefixes |= INST_PRE_REX;
						rm += EX_GPR_BASE;
					}
					size = 64;
					rm += REGS64_BASE;
					break;
				}
				break;
			case OT_R32_64_M8:

			case OT_R32_64_M16:

			case OT_RM32_64:
				if (ps->vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}

				if ((ci->dt == Decode64Bits) && ((instFlags & (INST_64BITS | INST_PRE_REX)) == INST_64BITS)) {
					size = 64;
					rm += REGS64_BASE;
					break;
				}

				if (ps->vrex & PREFIX_EX_W) {
					ps->usedPrefixes |= INST_PRE_REX;
					size = 64;
					rm += REGS64_BASE;
				}
				else {
					size = 32;
					rm += REGS32_BASE;
				}
				break;
			case OT_RM16_32:
				if (ps->vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}

				if (ps->decodedPrefixes & INST_PRE_OP_SIZE) {
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					size = 16;
					rm += REGS16_BASE;
				}
				else {
					size = 32;
					rm += REGS32_BASE;
				}
				break;
			case OT_RM16:
				if (ps->vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}
				rm += REGS16_BASE;
				size = 16;
				break;
			case OT_RM8:
				if (ps->prefixExtType == PET_REX) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm = operands_fix_8bit_rex_base(rm + ((ps->vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0));
				}
				else rm += REGS8_BASE;
				size = 8;
				break;
			case OT_MM32:
			case OT_MM64:

				size = 64;
				rm += MMXREGS_BASE;
				break;

			case OT_XMM16:
			case OT_XMM32:
			case OT_XMM64:
			case OT_XMM128:
				if (ps->vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}
				size = 128;
				rm += SSEREGS_BASE;
				break;

			case OT_RM32:
			case OT_R32_M8:
			case OT_R32_M16:
				if (ps->vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}
				size = 32;
				rm += REGS32_BASE;
				break;

			case OT_YMM256:
				if (ps->vrex & PREFIX_EX_B) rm += EX_GPR_BASE;
				rm += AVXREGS_BASE;
				size = 256;
				break;
			case OT_YXMM64_256:
			case OT_YXMM128_256:
				if (ps->vrex & PREFIX_EX_B) rm += EX_GPR_BASE;
				if (ps->vrex & PREFIX_EX_L) {
					size = 256;
					rm += AVXREGS_BASE;
				}
				else {
					size = 128;
					rm += SSEREGS_BASE;
				}
				break;
			case OT_WXMM32_64:
			case OT_LXMM64_128:
				if (ps->vrex & PREFIX_EX_B) rm += EX_GPR_BASE;
				size = 128;
				rm += SSEREGS_BASE;
				break;

			case OT_WRM32_64:
			case OT_REG32_64_M8:
			case OT_REG32_64_M16:
				if (ps->vrex & PREFIX_EX_B) rm += EX_GPR_BASE;
				if (ps->vrex & PREFIX_EX_W) {
					size = 64;
					rm += REGS64_BASE;
				}
				else {
					size = 32;
					rm += REGS32_BASE;
				}
				break;

			default: return FALSE;
			}
			op->size = (uint16_t)size;
			op->index = (uint8_t)rm;
			op->type = O_REG;
			di->usedRegistersMask |= _REGISTERTORCLASS[rm];
			return TRUE;
		}
	}

	reg = (modrm >> 3) & 7;
	switch (type)
	{
		case OT_IMM8:
			operands_set_ts(op, O_IMM, 8);
			if (!read_stream_safe_uint8(ci, &di->imm.byte)) return FALSE;
		break;
		case OT_IMM_FULL:
			if (effOpSz == Decode16Bits) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;

		case OT_IMM16:
			operands_set_ts(op, O_IMM, 16);
			if (!read_stream_safe_uint16(ci, &di->imm.word)) return FALSE;
		break;

			} else if ((effOpSz == Decode64Bits) &&
				        ((instFlags & (INST_64BITS | INST_PRE_REX)) == (INST_64BITS | INST_PRE_REX))) {
				ps->usedPrefixes |= INST_PRE_REX;

				operands_set_ts(op, O_IMM, 64);
				if (!read_stream_safe_uint64(ci, &di->imm.qword)) return FALSE;
				break;
			} else ps->usedPrefixes |= INST_PRE_OP_SIZE;

		case OT_IMM32:
			op->type = O_IMM;
			if (ci->dt == Decode64Bits) {

				op->size = 32;

				di->flags |= FLAG_IMM_SIGNED;
				if (!read_stream_safe_sint32(ci, &di->imm.sqword)) return FALSE;
			} else {
				op->size = 32;
				if (!read_stream_safe_uint32(ci, &di->imm.dword)) return FALSE;
			}
		break;
		case OT_SEIMM8:

			op->type = O_IMM;
			if ((instFlags & INST_PRE_OP_SIZE) && (ps->decodedPrefixes & INST_PRE_OP_SIZE)) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				switch (ci->dt)
				{
					case Decode16Bits: op->size = 32; break;
					case Decode32Bits:
					case Decode64Bits:
						op->size = 16;
					break;
				}
			} else op->size = 8;
			di->flags |= FLAG_IMM_SIGNED;
			if (!read_stream_safe_sint8(ci, &di->imm.sqword)) return FALSE;
		break;
		case OT_IMM16_1:
			operands_set_ts(op, O_IMM1, 16);
			if (!read_stream_safe_uint16(ci, &di->imm.ex.i1)) return FALSE;
		break;
		case OT_IMM8_1:
			operands_set_ts(op, O_IMM1, 8);
			if (!read_stream_safe_uint8(ci, &di->imm.ex.i1)) return FALSE;
		break;
		case OT_IMM8_2:
			operands_set_ts(op, O_IMM2, 8);
			if (!read_stream_safe_uint8(ci, &di->imm.ex.i2)) return FALSE;
		break;
		case OT_REG8:
			operands_set_ts(op, O_REG, 8);
			if (ps->prefixExtType) {

				ps->usedPrefixes |= INST_PRE_REX;
				op->index = (uint8_t)operands_fix_8bit_rex_base(reg + ((ps->vrex & PREFIX_EX_R) ? EX_GPR_BASE : 0));
			} else op->index = (uint8_t)(REGS8_BASE + reg);
			di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
		break;
		case OT_REG16:
			operands_set_tsi(di, op, O_REG, 16, REGS16_BASE + reg);
		break;
		case OT_REG_FULL:
			switch (effOpSz)
			{
				case Decode16Bits:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					if (ps->vrex & PREFIX_EX_R) {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += EX_GPR_BASE;
					}
					operands_set_tsi(di, op, O_REG, 16, REGS16_BASE + reg);
				break;
				case Decode32Bits:
					if (ps->vrex & PREFIX_EX_R) {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += EX_GPR_BASE;
					} else ps->usedPrefixes |= INST_PRE_OP_SIZE;
					operands_set_tsi(di, op, O_REG, 32, REGS32_BASE + reg);
				break;
				case Decode64Bits:
					ps->usedPrefixes |= INST_PRE_REX;
					operands_set_tsi(di, op, O_REG, 64, REGS64_BASE + reg + ((ps->vrex & PREFIX_EX_R) ? EX_GPR_BASE : 0));
				break;
			}
		break;
		case OT_REG32:
			if (ps->vrex & PREFIX_EX_R) {
				ps->usedPrefixes |= INST_PRE_REX;
				reg += EX_GPR_BASE;
			}
			operands_set_tsi(di, op, O_REG, 32, REGS32_BASE + reg);
		break;
		case OT_REG32_64:
			if (ps->vrex & PREFIX_EX_R) {
				ps->usedPrefixes |= INST_PRE_REX;
				reg += EX_GPR_BASE;
			}

			if ((ci->dt == Decode64Bits) && ((instFlags & (INST_64BITS | INST_PRE_REX)) == INST_64BITS)) {
				operands_set_tsi(di, op, O_REG, 64, REGS64_BASE + reg);
				break;
			}

			if (ps->vrex & PREFIX_EX_W) {
				ps->usedPrefixes |= INST_PRE_REX;
				operands_set_tsi(di, op, O_REG, 64, REGS64_BASE + reg);
			} else operands_set_tsi(di, op, O_REG, 32, REGS32_BASE + reg);
		break;
		case OT_FREG32_64_RM:
			rm = modrm & 7;
			if (ps->vrex & PREFIX_EX_B) {
				ps->usedPrefixes |= INST_PRE_REX;
				rm += EX_GPR_BASE;
			}

			if (ci->dt == Decode64Bits) operands_set_tsi(di, op, O_REG, 64, REGS64_BASE + rm);
			else operands_set_tsi(di, op, O_REG, 32, REGS32_BASE + rm);
		break;
		case OT_MM:
			operands_set_tsi(di, op, O_REG, 64, MMXREGS_BASE + reg);
		break;
		case OT_MM_RM:
			rm = modrm & 7;
			operands_set_tsi(di, op, O_REG, 64, MMXREGS_BASE + rm);
		break;
		case OT_REGXMM0:
			operands_set_tsi(di, op, O_REG, 128, SSEREGS_BASE + 0);
			break;
		case OT_XMM:
			if (ps->vrex & PREFIX_EX_R) {
				ps->usedPrefixes |= INST_PRE_REX;
				reg += EX_GPR_BASE;
			}
			operands_set_tsi(di, op, O_REG, 128, SSEREGS_BASE + reg);
		break;
		case OT_XMM_RM:
			rm = modrm & 7;
			if (ps->vrex & PREFIX_EX_B) {
				ps->usedPrefixes |= INST_PRE_REX;
				rm += EX_GPR_BASE;
			}
			operands_set_tsi(di, op, O_REG, 128, SSEREGS_BASE + rm);
		break;
		case OT_CREG:

			if (ps->vrex & PREFIX_EX_R) {
				ps->usedPrefixes |= INST_PRE_REX;
				reg += EX_GPR_BASE;
			} else if ((ci->dt == Decode32Bits) && (ps->decodedPrefixes & INST_PRE_LOCK)) {

				reg += EX_GPR_BASE;
				ps->usedPrefixes |= INST_PRE_LOCK;
			}

			if ((reg >= CREGS_MAX) || (reg == 1) || ((reg >= 5) && (reg <= 7))) return FALSE;

			op->type = O_REG;
			if (ci->dt == Decode64Bits) op->size = 64;
			else op->size = 32;
			op->index = (uint8_t)(CREGS_BASE + reg);
			di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
		break;
		case OT_DREG:

			if ((reg == 4) || (reg == 5) || (ps->vrex & PREFIX_EX_R)) return FALSE;

			op->type = O_REG;
			if (ci->dt == Decode64Bits) op->size = 64;
			else op->size = 32;
			op->index = (uint8_t)(DREGS_BASE + reg);
			di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
		break;
		case OT_SREG:
			if ((&di->ops[0] == op) && (reg == 1)) return FALSE;

			if (reg <= SEG_REGS_MAX - 1) operands_set_tsi(di, op, O_REG, 16, SREGS_BASE + reg);
			else return FALSE;
		break;
		case OT_SEG:
			op->type = O_REG;

			op->size = 16;
			ps->usedPrefixes |= INST_PRE_OP_SIZE;

			switch (instFlags & INST_PRE_SEGOVRD_MASK)
			{
				case INST_PRE_ES: op->index = R_ES; break;
				case INST_PRE_CS: op->index = R_CS; break;
				case INST_PRE_SS: op->index = R_SS; break;
				case INST_PRE_DS: op->index = R_DS; break;
				case INST_PRE_FS: op->index = R_FS; break;
				case INST_PRE_GS: op->index = R_GS; break;
			}
			di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
		break;
		case OT_ACC8:
			operands_set_tsi(di, op, O_REG, 8, R_AL);
		break;
		case OT_ACC16:
			operands_set_tsi(di, op, O_REG, 16, R_AX);
		break;
		case OT_ACC_FULL_NOT64:

		case OT_ACC_FULL:
			if (effOpSz == Decode16Bits) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				operands_set_tsi(di, op, O_REG, 16, R_AX);
			} else if ((effOpSz == Decode32Bits) || (type == OT_ACC_FULL_NOT64)) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				operands_set_tsi(di, op, O_REG, 32, R_EAX);
			} else {

				if (!(instFlags & INST_64BITS)) {
					ps->usedPrefixes |= INST_PRE_REX;
				}
				operands_set_tsi(di, op, O_REG, 64, R_RAX);
			}
		break;
		case OT_PTR16_FULL:

			if (effOpSz == Decode16Bits) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				ci->codeLen -= sizeof(int16_t)*2;
				if (ci->codeLen < 0) return FALSE;

				operands_set_ts(op, O_PTR, 16);
				di->imm.ptr.off = RUSHORT(ci->code);
				di->imm.ptr.seg = RUSHORT((ci->code + sizeof(int16_t)));

				ci->code += sizeof(int16_t)*2;
			} else {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				ci->codeLen -= sizeof(int32_t) + sizeof(int16_t);
				if (ci->codeLen < 0) return FALSE;

				operands_set_ts(op, O_PTR, 32);
				di->imm.ptr.off = RULONG(ci->code);
				di->imm.ptr.seg = RUSHORT((ci->code + sizeof(int32_t)));

				ci->code += sizeof(int32_t) + sizeof(int16_t);
			}
		break;
		case OT_RELCB:
		case OT_RELC_FULL:

			if (type == OT_RELCB) {
				operands_set_ts(op, O_PC, 8);
				if (!read_stream_safe_sint8(ci, &di->imm.sqword)) return FALSE;
			} else {

				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				if (effOpSz == Decode16Bits) {
					operands_set_ts(op, O_PC, 16);
					if (!read_stream_safe_sint16(ci, &di->imm.sqword)) return FALSE;
				} else {
					operands_set_ts(op, O_PC, 32);
					if (!read_stream_safe_sint32(ci, &di->imm.sqword)) return FALSE;
				}
			}

			if ((ii->opcodeId >= I_JO) && (ii->opcodeId <= I_JG)) {
				if (ps->decodedPrefixes & INST_PRE_CS) {
					ps->usedPrefixes |= INST_PRE_CS;
					di->flags |= FLAG_HINT_NOT_TAKEN;
				} else if (ps->decodedPrefixes & INST_PRE_DS) {
					ps->usedPrefixes |= INST_PRE_DS;
					di->flags |= FLAG_HINT_TAKEN;
				}
			}
		break;
		case OT_MOFFS8:
			op->size = 8;

		case OT_MOFFS_FULL:
			op->type = O_DISP;
			if (op->size == 0) {

				switch (effOpSz)
				{
					case Decode16Bits: op->size = 16; break;
					case Decode32Bits: op->size = 32; break;
					case Decode64Bits: op->size = 64; break;
				}
			}

			prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);

			if (effAdrSz == Decode16Bits) {
				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

				di->dispSize = 16;
				if (!read_stream_safe_uint16(ci, &di->disp)) return FALSE;
			} else if (effAdrSz == Decode32Bits) {
				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

				di->dispSize = 32;
				if (!read_stream_safe_uint32(ci, &di->disp)) return FALSE;
			} else {
				di->dispSize = 64;
				if (!read_stream_safe_uint64(ci, &di->disp)) return FALSE;
			}
		break;
		case OT_CONST1:
			operands_set_ts(op, O_IMM, 8);
			di->imm.byte = 1;
		break;
		case OT_REGCL:
			operands_set_tsi(di, op, O_REG, 8, R_CL);
		break;

		case OT_FPU_SI:

			operands_set_tsi(di, op, O_REG, 32, FPUREGS_BASE + (*(ci->code-1) & 7));
		break;
		case OT_FPU_SSI:
			operands_set_tsi(di, op, O_REG, 32, R_ST0);
			operands_set_tsi(di, op + 1, O_REG, 32, FPUREGS_BASE + (*(ci->code-1) & 7));
			di->opsNo++;
		break;
		case OT_FPU_SIS:
			operands_set_tsi(di, op, O_REG, 32, FPUREGS_BASE + (*(ci->code-1) & 7));
			operands_set_tsi(di, op + 1, O_REG, 32, R_ST0);
			di->opsNo++;
		break;

		case OT_IB_RB:

			operands_set_ts(op, O_REG, 8);
			reg = *(ci->code-1) & 7;
			if (ps->vrex & PREFIX_EX_B) {
				ps->usedPrefixes |= INST_PRE_REX;
				op->index = (uint8_t)operands_fix_8bit_rex_base(reg + EX_GPR_BASE);
			} else if (ps->prefixExtType == PET_REX) {
				ps->usedPrefixes |= INST_PRE_REX;
				op->index = (uint8_t)operands_fix_8bit_rex_base(reg);
			} else op->index = (uint8_t)(REGS8_BASE + reg);

			di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
		break;
		case OT_IB_R_FULL:
			reg = *(ci->code-1) & 7;
			switch (effOpSz)
			{
				case Decode16Bits:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					if (ps->vrex & PREFIX_EX_B) {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += EX_GPR_BASE;
					}
					operands_set_tsi(di, op, O_REG, 16, REGS16_BASE + reg);
				break;
				case Decode32Bits:
					if (ps->vrex & PREFIX_EX_B) {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += EX_GPR_BASE;
					} else ps->usedPrefixes |= INST_PRE_OP_SIZE;
					operands_set_tsi(di, op, O_REG, 32, REGS32_BASE + reg);
				break;
				case Decode64Bits:

					if ((instFlags & INST_64BITS) && ((instFlags & INST_PRE_REX) == 0)) {
						if (ps->vrex & PREFIX_EX_B) {
							ps->usedPrefixes |= INST_PRE_REX;
							reg += EX_GPR_BASE;
						}
					} else {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += (ps->vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0;
					}
					operands_set_tsi(di, op, O_REG, 64, REGS64_BASE + reg);
				break;
			}
		break;

		case OT_REGI_ESI:
			ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

			op->type = O_SMEM;

			if (instFlags & INST_16BITS) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;

				if (effOpSz == Decode16Bits) op->size = 16;
				else if ((effOpSz == Decode64Bits) && (instFlags & INST_64BITS)) {
					ps->usedPrefixes |= INST_PRE_REX;
					op->size = 64;
				} else op->size = 32;
			} else op->size = 8;

			di->segment = R_NONE;
			prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);

			if (effAdrSz == Decode16Bits) op->index = R_SI;
			else if (effAdrSz == Decode32Bits) op->index = R_ESI;
			else op->index = R_RSI;

			di->usedRegistersMask |= _REGISTERTORCLASS[R_RSI];
		break;
		case OT_REGI_EDI:
			ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

			op->type = O_SMEM;

			if (instFlags & INST_16BITS) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;

				if (effOpSz == Decode16Bits) op->size = 16;
				else if ((effOpSz == Decode64Bits) && (instFlags & INST_64BITS)) {
					ps->usedPrefixes |= INST_PRE_REX;
					op->size = 64;
				} else op->size = 32;
			} else op->size = 8;

			if ((di->segment == R_NONE) && (ci->dt != Decode64Bits)) di->segment = R_ES | SEGMENT_DEFAULT;

			if (effAdrSz == Decode16Bits) op->index = R_DI;
			else if (effAdrSz == Decode32Bits) op->index = R_EDI;
			else op->index = R_RDI;

			di->usedRegistersMask |= _REGISTERTORCLASS[R_RDI];
		break;

		case OT_REGDX:

			operands_set_tsi(di, op, O_REG, 16, R_DX);
		break;

		case OT_REGECX:
			operands_set_tsi(di, op, O_REG, 32, R_ECX);
		break;
		case OT_REGI_EBXAL:

			ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

			prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);

			operands_set_tsi(di, op, O_MEM, 8, R_AL);

			if (effAdrSz == Decode16Bits) di->base = R_BX;
			else if (effAdrSz == Decode32Bits) di->base = R_EBX;
			else {
				ps->usedPrefixes |= INST_PRE_REX;
				di->base = R_RBX;
			}

			di->usedRegistersMask |= _REGISTERTORCLASS[di->base];
		break;
		case OT_REGI_EAX:

			if (effAdrSz == Decode64Bits) operands_set_tsi(di, op, O_SMEM, 64, R_RAX);
			else if (effAdrSz == Decode32Bits) {
				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
				operands_set_tsi(di, op, O_SMEM, 32, R_EAX);
			}
			else {
				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
				operands_set_tsi(di, op, O_SMEM, 16, R_AX);
			}
		break;
		case OT_VXMM:
			operands_set_tsi(di, op, O_REG, 128, SSEREGS_BASE + ps->vexV);
		break;
		case OT_XMM_IMM:
			ci->codeLen -= sizeof(int8_t);
			if (ci->codeLen < 0) return FALSE;

			if (ci->dt == Decode32Bits) reg = (*ci->code >> 4) & 0x7;
			else reg = (*ci->code >> 4) & 0xf;
			operands_set_tsi(di, op, O_REG, 128, SSEREGS_BASE + reg);

			ci->code += sizeof(int8_t);
		break;
		case OT_YXMM:
			if (ps->vrex & PREFIX_EX_R) reg += EX_GPR_BASE;
			if (ps->vrex & PREFIX_EX_L) operands_set_tsi(di, op, O_REG, 256, AVXREGS_BASE + reg);
			else operands_set_tsi(di, op, O_REG, 128, SSEREGS_BASE + reg);
		break;
		case OT_YXMM_IMM:
			ci->codeLen -= sizeof(int8_t);
			if (ci->codeLen < 0) return FALSE;

			if (ci->dt == Decode32Bits) reg = (*ci->code >> 4) & 0x7;
			else reg = (*ci->code >> 4) & 0xf;

			if (ps->vrex & PREFIX_EX_L) operands_set_tsi(di, op, O_REG, 256, AVXREGS_BASE + reg);
			else operands_set_tsi(di, op, O_REG, 128, SSEREGS_BASE + reg);

			ci->code += sizeof(int8_t);
		break;
		case OT_YMM:
			if (ps->vrex & PREFIX_EX_R) reg += EX_GPR_BASE;
			operands_set_tsi(di, op, O_REG, 256, AVXREGS_BASE + reg);
		break;
		case OT_VYMM:
			operands_set_tsi(di, op, O_REG, 256, AVXREGS_BASE + ps->vexV);
		break;
		case OT_VYXMM:
			if (ps->vrex & PREFIX_EX_L) operands_set_tsi(di, op, O_REG, 256, AVXREGS_BASE + ps->vexV);
			else operands_set_tsi(di, op, O_REG, 128, SSEREGS_BASE + ps->vexV);
		break;
		case OT_WREG32_64:
			if (ps->vrex & PREFIX_EX_R) reg += EX_GPR_BASE;
			if (ps->vrex & PREFIX_EX_W) operands_set_tsi(di, op, O_REG, 64, REGS64_BASE + reg);
			else operands_set_tsi(di, op, O_REG, 32, REGS32_BASE + reg);
		break;
		default: return FALSE;
	}
	return TRUE;
}
