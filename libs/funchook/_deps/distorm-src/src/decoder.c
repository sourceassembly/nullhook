#include "decoder.h"
#include "instructions.h"
#include "insts.h"
#include "prefix.h"
#include "x86defs.h"
#include "operands.h"
#include "insts.h"
#include "../include/mnemonics.h"

static _DecodeType decode_get_effective_addr_size(_DecodeType dt, _iflags decodedPrefixes)
{

	if (decodedPrefixes & INST_PRE_ADDR_SIZE) {
		if (dt == Decode32Bits) return Decode16Bits;
		return Decode32Bits;
	}
	return dt;
}

static _DecodeType decode_get_effective_op_size(_DecodeType dt, _iflags decodedPrefixes, unsigned int rex, _iflags instFlags)
{

	if (decodedPrefixes & INST_PRE_OP_SIZE) {
		if (dt == Decode16Bits) return Decode32Bits;
		return Decode16Bits;
	}

	if (dt == Decode64Bits) {

		if (((instFlags & (INST_64BITS | INST_PRE_REX)) == INST_64BITS) ||

			((decodedPrefixes & INST_PRE_REX) && (rex & PREFIX_EX_W))) return Decode64Bits;
		return Decode32Bits;
	}

	return dt;
}

#define CONVERT_FLAGS_TO_EFLAGS(dst, src, field) dst->field = ((src->field & D_COMPACT_SAME_FLAGS) | \
	((src->field & D_COMPACT_IF) << (9 - 1)) | \
	((src->field & D_COMPACT_DF) << (10 - 3)) | \
	((src->field & D_COMPACT_OF) << (11 - 5)));

static _DecodeResult decode_inst(_CodeInfo* ci, _PrefixState* ps, const uint8_t* startCode, _DInst* di)
{

	_InstInfo* ii;
	_InstSharedInfo* isi;

	_DecodeType effOpSz, effAdrSz;
	_iflags instFlags;

	unsigned int modrm = 0;
	int isPrefixed = 0;

	ii = inst_lookup(ci, ps, &isPrefixed);
	if (ii == NULL) goto _Undecodable;

	isi = &InstSharedInfoTable[ii->sharedIndex];
	instFlags = FlagsTable[isi->flagsIndex];

	if (isPrefixed) {

		if ((ps->decodedPrefixes & INST_PRE_OP_SIZE) &&
			(ps->prefixExtType == PET_REX) &&
			(ps->vrex & PREFIX_EX_W) &&
			(!ps->isOpSizeMandatory)) {
			ps->decodedPrefixes &= ~INST_PRE_OP_SIZE;
			prefixes_ignore(ps, PFXIDX_OP_SIZE);
		}

		effAdrSz = decode_get_effective_addr_size(ci->dt, ps->decodedPrefixes);
		effOpSz = decode_get_effective_op_size(ci->dt, ps->decodedPrefixes, ps->vrex, instFlags);
	}
	else
	{
		effAdrSz = ci->dt;
		effOpSz = decode_get_effective_op_size(ci->dt, 0, 0, instFlags);
	}

	memset(di, 0, sizeof(_DInst));

	if (instFlags & INST_MODRM_REQUIRED) {

		if (!(instFlags & INST_MODRM_INCLUDED)) {
			ci->code++;
			if (--ci->codeLen < 0) goto _Undecodable;
		}
		modrm = *ci->code;
	}

	ci->code++;

	di->addr = ci->codeOffset & ci->addrMask;
	di->opcode = ii->opcodeId;
	di->flags = isi->meta & META_INST_PRIVILEGED;

	di->base = R_NONE;
	di->segment = R_NONE;

	FLAG_SET_ADDRSIZE(di, effAdrSz);

	if (isi->d != OT_NONE) {
		unsigned int opsNo = 1;
		_Operand* op = &di->ops[0];
		if (instFlags & (INST_MODRR_REQUIRED | INST_FORCE_REG0)) {

			if ((modrm < INST_DIVIDED_MODRM) && (instFlags & INST_MODRR_REQUIRED)) goto _Undecodable;

			if ((instFlags & INST_FORCE_REG0) && (((modrm >> 3) & 7) != 0)) goto _Undecodable;
		}
		if (!operands_extract(ci, di, ii, instFlags, (_OpType)isi->d, modrm, ps, effOpSz, effAdrSz, op++)) goto _Undecodable;

		if (isi->s != OT_NONE) {
			if (!operands_extract(ci, di, ii, instFlags, (_OpType)isi->s, modrm, ps, effOpSz, effAdrSz, op++)) goto _Undecodable;
			opsNo++;

			if (instFlags & INST_USE_OP3) {
				if (!operands_extract(ci, di, ii, instFlags, (_OpType)((_InstInfoEx*)ii)->op3, modrm, ps, effOpSz, effAdrSz, op++)) goto _Undecodable;
				opsNo++;

				if (instFlags & INST_USE_OP4) {
					if (!operands_extract(ci, di, ii, instFlags, (_OpType)((_InstInfoEx*)ii)->op4, modrm, ps, effOpSz, effAdrSz, op++)) goto _Undecodable;
					opsNo++;
				}
			}
		}

		di->flags |= (instFlags & INST_DST_WR) >> (31 - 6);

		di->opsNo += (uint8_t)opsNo;
	}

	if (instFlags & (INST_3DNOW_FETCH |
		INST_PSEUDO_OPCODE |
		INST_NATIVE |
		INST_PRE_REPNZ |
		INST_PRE_REP |
		INST_PRE_ADDR_SIZE |
		INST_INVALID_64BITS |
		INST_64BITS_FETCH)) {

		if (ps && instFlags & INST_NATIVE) ps->usedPrefixes |= (ps->decodedPrefixes & INST_PRE_OP_SIZE);

		if (ci->dt != Decode64Bits) {

			if (instFlags & INST_64BITS_FETCH) goto _Undecodable;
		}
		else {

			if (instFlags & INST_INVALID_64BITS) goto _Undecodable;
		}

		if (instFlags & INST_3DNOW_FETCH) {
			ii = inst_lookup_3dnow(ci);
			if (ii == NULL) goto _Undecodable;
			isi = &InstSharedInfoTable[ii->sharedIndex];
			instFlags = FlagsTable[isi->flagsIndex];
			di->opcode = ii->opcodeId;
		}

		if (instFlags & INST_PSEUDO_OPCODE) {

			unsigned int cmpType;

			if (--ci->codeLen < 0) goto _Undecodable;
			cmpType = *ci->code;
			ci->code++;

			if (instFlags & INST_PRE_VEX) {

				if (cmpType >= INST_VCMP_MAX_RANGE) goto _Undecodable;

				di->opcode = ii->opcodeId + VCmpMnemonicOffsets[cmpType];
			}
			else {

				if (cmpType >= INST_CMP_MAX_RANGE) goto _Undecodable;
				di->opcode = ii->opcodeId + CmpMnemonicOffsets[cmpType];
			}

			goto _SkipOpcoding;
		}

		if (isPrefixed && (instFlags & (INST_PRE_REPNZ | INST_PRE_REP))) {
			if ((instFlags & INST_PRE_REPNZ) && (ps->decodedPrefixes & INST_PRE_REPNZ)) {
				ps->usedPrefixes |= INST_PRE_REPNZ;
				di->flags |= FLAG_REPNZ;
			}
			else if ((instFlags & INST_PRE_REP) && (ps->decodedPrefixes & INST_PRE_REP)) {
				ps->usedPrefixes |= INST_PRE_REP;
				di->flags |= FLAG_REP;
			}
		}

		if (instFlags & INST_PRE_ADDR_SIZE) {

			if (instFlags & INST_USE_EXMNEMONIC) {
				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
				if (effAdrSz == Decode16Bits) di->opcode = ii->opcodeId;
				else if (effAdrSz == Decode32Bits) di->opcode = ((_InstInfoEx*)ii)->opcodeId2;

				else   di->opcode = ((_InstInfoEx*)ii)->opcodeId3;
			}

			else if (instFlags & INST_NATIVE) {
				di->opcode = ii->opcodeId;

				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
			}

			goto _SkipOpcoding;
		}
	}

	if (effOpSz == Decode32Bits) {

		FLAG_SET_OPSIZE(di, Decode32Bits);

		if (instFlags & INST_USE_EXMNEMONIC) {

			if (instFlags & INST_MNEMONIC_MODRM_BASED) {
				if (modrm < INST_DIVIDED_MODRM) di->opcode = ((_InstInfoEx*)ii)->opcodeId2;
			}
			else di->opcode = ((_InstInfoEx*)ii)->opcodeId2;
			ps->usedPrefixes |= INST_PRE_OP_SIZE;
		}
	}
	else if (effOpSz == Decode64Bits) {

		FLAG_SET_OPSIZE(di, Decode64Bits);

		if (instFlags & (INST_USE_EXMNEMONIC | INST_USE_EXMNEMONIC2)) {

			if ((modrm >= INST_DIVIDED_MODRM) && (instFlags & INST_MNEMONIC_MODRM_BASED)) goto _Undecodable;

			if ((instFlags & INST_USE_EXMNEMONIC2) && (ps->vrex & PREFIX_EX_W)) {
				ps->usedPrefixes |= INST_PRE_REX;
				di->opcode = ((_InstInfoEx*)ii)->opcodeId3;
			}
			else di->opcode = ((_InstInfoEx*)ii)->opcodeId2;
		}
	}
	else {

		FLAG_SET_OPSIZE(di, Decode16Bits);

		if ((instFlags & (INST_USE_EXMNEMONIC | INST_32BITS | INST_64BITS)) == INST_USE_EXMNEMONIC) ps->usedPrefixes |= INST_PRE_OP_SIZE;
	}

_SkipOpcoding:

	if (isPrefixed && (instFlags & INST_PRE_VEX) &&
		(((((_InstInfoEx*)ii)->flagsEx & INST_MNEMONIC_VEXW_BASED) && (ps->vrex & PREFIX_EX_W)) ||
			((((_InstInfoEx*)ii)->flagsEx & INST_MNEMONIC_VEXL_BASED) && (ps->vrex & PREFIX_EX_L)))) {
		di->opcode = ((_InstInfoEx*)ii)->opcodeId2;
	}

	di->size = (uint8_t)(ci->code - startCode);

	if (di->size > INST_MAXIMUM_SIZE) goto _Undecodable;

	if (isPrefixed) di->unusedPrefixesMask = prefixes_set_unused_mask(ps);

	di->meta = isi->meta;

	if (ci->features & DF_FILL_EFLAGS) {

		if (isi->testedFlagsMask) CONVERT_FLAGS_TO_EFLAGS(di, isi, testedFlagsMask);
		if (isi->modifiedFlagsMask) CONVERT_FLAGS_TO_EFLAGS(di, isi, modifiedFlagsMask);
		if (isi->undefinedFlagsMask) CONVERT_FLAGS_TO_EFLAGS(di, isi, undefinedFlagsMask);
	}

	return DECRES_SUCCESS;

_Undecodable:

	if (*startCode == INST_WAIT_INDEX) {
		int delta;
		memset(di, 0, sizeof(_DInst));
		di->addr = ci->codeOffset & ci->addrMask;
		di->imm.byte = INST_WAIT_INDEX;
		di->segment = R_NONE;
		di->base = R_NONE;
		di->size = 1;
		di->opcode = I_WAIT;
		META_SET_ISC(di, ISC_INTEGER);

		delta = (int)(ci->code - startCode);
		ci->codeLen += delta - 1;
		ci->code = startCode + 1;

		return DECRES_SUCCESS;
	}

	return DECRES_INPUTERR;
}

_DecodeResult decode_internal(_CodeInfo* _ci, int supportOldIntr, _DInst result[], unsigned int maxResultCount, unsigned int* usedInstructionsCount)
{
	_CodeInfo ci = *_ci;
	_PrefixState ps;

	const uint8_t* code;
	int codeLen;
	_OffsetType codeOffset;

	_DecodeResult ret = DECRES_SUCCESS;

	_DInst* pdi = (_DInst*)&result[0];
	_DInst* maxResultAddr;

	unsigned int features = ci.features;

	unsigned int diStructSize;

#ifndef DISTORM_LIGHT
	if (supportOldIntr) {
		diStructSize = sizeof(_DecodedInst);
		maxResultAddr = (_DInst*)((size_t)&result[0] + (maxResultCount * sizeof(_DecodedInst)));
	}
	else
#endif
	{
		diStructSize = sizeof(_DInst);
		maxResultAddr = &result[maxResultCount];
	}

	ci.addrMask = (_OffsetType)-1;

#ifdef DISTORM_LIGHT
	supportOldIntr;

	if (features & DF_MAXIMUM_ADDR32) ci.addrMask = 0xffffffff;
	else if (features & DF_MAXIMUM_ADDR16) ci.addrMask = 0xffff;
#endif

	ps.count = 1;

	while (ci.codeLen > 0) {
		code = ci.code;
		codeLen = ci.codeLen;
		codeOffset = ci.codeOffset;

		if (ps.count) memset(&ps, 0, sizeof(ps));

		if (pdi >= maxResultAddr) {
			ret = DECRES_MEMORYERR;
			break;
		}

		ret = decode_inst(&ci, &ps, code, pdi);

		ci.codeOffset += pdi->size;

		if (ret == DECRES_SUCCESS) {

			if (features & (DF_SINGLE_BYTE_STEP | DF_RETURN_FC_ONLY | DF_STOP_ON_PRIVILEGED | DF_STOP_ON_FLOW_CONTROL)) {

				if (features & DF_SINGLE_BYTE_STEP) {
					ci.code = code + 1;
					ci.codeLen = codeLen - 1;
					ci.codeOffset = codeOffset + 1;
				}

				if ((features & DF_RETURN_FC_ONLY) && (META_GET_FC(pdi->meta) == FC_NONE)) {
					continue;
				}

				if ((features & DF_STOP_ON_PRIVILEGED) && (FLAG_GET_PRIVILEGED(pdi->flags))) {
					pdi = (_DInst*)((char*)pdi + diStructSize);
					break;
				}

				if (features & DF_STOP_ON_FLOW_CONTROL) {
					unsigned int mfc = META_GET_FC(pdi->meta);
					if (mfc && (((features & DF_STOP_ON_CALL) && (mfc == FC_CALL)) ||
						((features & DF_STOP_ON_RET) && (mfc == FC_RET)) ||
						((features & DF_STOP_ON_SYS) && (mfc == FC_SYS)) ||
						((features & DF_STOP_ON_UNC_BRANCH) && (mfc == FC_UNC_BRANCH)) ||
						((features & DF_STOP_ON_CND_BRANCH) && (mfc == FC_CND_BRANCH)) ||
						((features & DF_STOP_ON_INT) && (mfc == FC_INT)) ||
						((features & DF_STOP_ON_CMOV) && (mfc == FC_CMOV)) ||
						((features & DF_STOP_ON_HLT) && (mfc == FC_HLT)))) {
						pdi = (_DInst*)((char*)pdi + diStructSize);
						break;
					}
				}
			}

			pdi = (_DInst*)((char*)pdi + diStructSize);
		}
		else {

			if ((!(features & DF_RETURN_FC_ONLY))) {
				memset(pdi, 0, sizeof(_DInst));
				pdi->flags = FLAG_NOT_DECODABLE;
				pdi->imm.byte = *code;
				pdi->size = 1;
				pdi->addr = codeOffset & ci.addrMask;
				pdi = (_DInst*)((char*)pdi + diStructSize);

				if (features & DF_STOP_ON_UNDECODEABLE) {
					ret = DECRES_SUCCESS;
					break;
				}
			}

			ci.code = code + 1;
			ci.codeLen = codeLen - 1;
			ci.codeOffset = codeOffset + 1;

			ret = DECRES_SUCCESS;
		}
	}

	*usedInstructionsCount = (unsigned int)(((size_t)pdi - (size_t)result) / (size_t)diStructSize);
	_ci->nextOffset = ci.codeOffset;

	return ret;
}
