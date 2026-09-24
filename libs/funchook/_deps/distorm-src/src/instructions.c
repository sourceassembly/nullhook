#include "instructions.h"

#include "insts.h"
#include "prefix.h"
#include "x86defs.h"
#include "../include/mnemonics.h"

#define INST_NODE_INDEX(n) ((n) & 0x1fff)
#define INST_NODE_TYPE(n) ((n) >> 13)

#define INST_INFO_FLAGS(ii) (FlagsTable[InstSharedInfoTable[(ii)->sharedIndex].flagsIndex])

static _InstInfo* inst_get_info(_InstNode in, int index)
{
	int instIndex = 0;

	in = InstructionsTree[INST_NODE_INDEX(in) + index];
	if (in == INT_NOTEXISTS) return NULL;

	instIndex = INST_NODE_INDEX(in);
	return INST_NODE_TYPE(in) == INT_INFO ? &InstInfos[instIndex] : (_InstInfo*)&InstInfosEx[instIndex];
}

static _InstInfo* inst_lookup_prefixed(_InstNode in, _PrefixState* ps)
{
	int checkOpSize = FALSE;
	int index = 0;
	_InstInfo* ii = NULL;

	switch (ps->decodedPrefixes & (INST_PRE_OP_SIZE | INST_PRE_REPS))
	{
		case 0:

			index = 0;
		break;
		case INST_PRE_OP_SIZE:

			index = 1;

			ps->isOpSizeMandatory = TRUE;
			ps->decodedPrefixes &= ~INST_PRE_OP_SIZE;
		break;
		case INST_PRE_REP:

			index = 2;
			ps->decodedPrefixes &= ~INST_PRE_REP;
		break;
		case INST_PRE_REPNZ:

			index = 3;
			ps->decodedPrefixes &= ~INST_PRE_REPNZ;
		break;
		default:

			if ((ps->decodedPrefixes & INST_PRE_REPS) == INST_PRE_REPS) return NULL;

			if (ps->decodedPrefixes & INST_PRE_REPNZ) {
				index = 3;
				ps->decodedPrefixes &= ~INST_PRE_REPNZ;
			}
			else if (ps->decodedPrefixes & INST_PRE_REP) {
				index = 2;
				ps->decodedPrefixes &= ~INST_PRE_REP;
			}

			checkOpSize = TRUE;
		break;
	}

	ii = inst_get_info(in, index);

	if (checkOpSize) {

		if ((ii == NULL) || (~INST_INFO_FLAGS(ii) & INST_PRE_OP_SIZE)) return NULL;
	}

	if (ii == NULL) ii = inst_get_info(in, 0);
	return ii;
}

static _InstInfo* inst_vex_mod_lookup(_CodeInfo* ci, _InstNode in, _InstInfo* ii, unsigned int index)
{

	ci->code += 1;
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;
	if (*ci->code < INST_DIVIDED_MODRM) {

		index += 4;

		return inst_get_info(in, index);
	}

	return ii;
}

static _InstInfo* inst_vex_lookup(_CodeInfo* ci, _PrefixState* ps)
{
	_InstNode in = 0;
	unsigned int pp = 0, start = 0;
	unsigned int index = 4;
	uint8_t vex = *ps->vexPos, vex2 = 0, v = 0;
	int instType = 0, instIndex = 0;

	_iflags illegal = (INST_PRE_OP_SIZE | INST_PRE_LOCK | INST_PRE_REP | INST_PRE_REPNZ | INST_PRE_REX);
	if ((ps->decodedPrefixes & illegal) != 0) return NULL;

	if (ps->prefixExtType == PET_VEX2BYTES) {
		ps->vexV = v = (~vex >> 3) & 0xf;
		pp = vex & 3;

		start = 1;
	}
	else {
		start = vex & 0x1f;
		vex2 = *(ps->vexPos + 1);
		ps->vexV = v = (~vex2 >> 3) & 0xf;
		pp = vex2 & 3;
	}

	switch (start)
	{
		case 1: in = Table_0F; break;
		case 2: in = Table_0F_38; break;
		case 3: in = Table_0F_3A; break;
		default: return NULL;
	}

	index += pp;

	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;

	in = InstructionsTree[INST_NODE_INDEX(in) + *ci->code];
	if (in == INT_NOTEXISTS) return NULL;

	instType = INST_NODE_TYPE(in);
	instIndex = INST_NODE_INDEX(in);

	if (instType == INT_LIST_PREFIXED) {
		_InstInfo* ii = inst_get_info(in, index);

		if ((ii != NULL) && (((_InstInfoEx*)ii)->flagsEx & INST_MODRR_BASED)) {
			ii = inst_vex_mod_lookup(ci, in, ii, index);
		}
		return ii;
	}

	if ((instType == INT_INFO) || (instType == INT_INFOEX) || (instType == INT_LIST_DIVIDED)) return NULL;

	ci->code += 1;
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;

	if (instType == INT_LIST_GROUP) {
		in = InstructionsTree[instIndex + ((*ci->code >> 3) & 7)];

	}
	else if (instType == INT_LIST_FULL) {
		in = InstructionsTree[instIndex + *ci->code];

	}

	if (INST_NODE_TYPE(in) == INT_LIST_PREFIXED) {
		_InstInfo* ii = inst_get_info(in, index);

		if ((ii != NULL) && (((_InstInfoEx*)ii)->flagsEx & INST_MODRR_BASED)) {
			ii = inst_vex_mod_lookup(ci, in, ii, index);
		}
		return ii;
	}

	return NULL;
}

_InstInfo* inst_lookup(_CodeInfo* ci, _PrefixState* ps, int* isPrefixed)
{
	unsigned int tmpIndex0, tmpIndex1, tmpIndex2;
	int instType;
	_InstNode in;
	_InstInfo* ii = NULL;
	int isWaitIncluded = FALSE;

	tmpIndex0 = *ci->code;

	if (prefixes_is_valid((unsigned char)tmpIndex0, ci->dt)) {
		*isPrefixed = TRUE;
		prefixes_decode(ci, ps);
		if (ci->codeLen < 1) return NULL;
		tmpIndex0 = *ci->code;

		if (ps->decodedPrefixes & INST_PRE_VEX) {
			ii = inst_vex_lookup(ci, ps);
			if (ii != NULL) {

				if ((((_InstInfoEx*)ii)->flagsEx & INST_FORCE_VEXL) && (~ps->vrex & PREFIX_EX_L)) return NULL;

				if ((((_InstInfoEx*)ii)->flagsEx & INST_VEX_V_UNUSED) && ps->vexV) return NULL;
			}
			return ii;
		}
	}

	ci->codeLen -= 1;

	if (tmpIndex0 == INST_WAIT_INDEX) {

		isWaitIncluded = TRUE;

		prefixes_ignore_all(ps);

		ci->code += 1;
		ci->codeLen -= 1;
		if (ci->codeLen < 0) return NULL;

		tmpIndex0 = *ci->code;
	}

	in = InstructionsTree[tmpIndex0];
	if ((uint32_t)in == INT_NOTEXISTS) return NULL;
	instType = INST_NODE_TYPE(in);

	if ((instType < INT_INFOS) && (!isWaitIncluded)) {

		if (instType == INT_INFO_TREAT) {
			if (tmpIndex0 == INST_NOP_INDEX) {

				if (ps->decodedPrefixes & INST_PRE_REP) {

					ps->usedPrefixes |= INST_PRE_REP;
					return &II_PAUSE;
				}

				if (ps->vrex & PREFIX_EX_W) ps->usedPrefixes |= INST_PRE_REX;
				if ((ci->dt != Decode64Bits) || (~ps->vrex & PREFIX_EX_B)) return &II_NOP;
			}
			else if (tmpIndex0 == INST_LEA_INDEX) {

				ps->decodedPrefixes &= ~INST_PRE_SEGOVRD_MASK;

				prefixes_ignore(ps, PFXIDX_SEG);
			}
			else if (tmpIndex0 == INST_ARPL_INDEX) {

					if (ci->dt == Decode64Bits) {
						return &II_MOVSXD;
					}
			}
		}

		return instType == INT_INFOEX ? (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)] : &InstInfos[INST_NODE_INDEX(in)];
	}

	ci->code += 1;
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;
	tmpIndex1 = *ci->code;

	if ((instType == INT_LIST_GROUP) && (!isWaitIncluded)) return inst_get_info(in, (tmpIndex1 >> 3) & 7);

	if (instType == INT_LIST_DIVIDED) {

		{
			_InstNode in2 = InstructionsTree[INST_NODE_INDEX(in) + ((tmpIndex1 >> 3) & 7)];

			instType = INST_NODE_TYPE(in2);

			if (instType == INT_INFO) ii = &InstInfos[INST_NODE_INDEX(in2)];
			else if (instType == INT_INFOEX) ii = (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in2)];
			if ((ii != NULL) && (INST_INFO_FLAGS(ii) & INST_NOT_DIVIDED)) return ii;

		}

		if (tmpIndex1 < INST_DIVIDED_MODRM) {

			tmpIndex1 = (tmpIndex1 >> 3) & 7;
		}
		else {

			tmpIndex1 -= INST_DIVIDED_MODRM - 8;
		}

		in = InstructionsTree[INST_NODE_INDEX(in) + tmpIndex1];
		if (in == INT_NOTEXISTS) return NULL;
		instType = INST_NODE_TYPE(in);

		if (instType < INT_INFOS) {

			ii = instType == INT_INFO ? &InstInfos[INST_NODE_INDEX(in)] : (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)];
			if ((~INST_INFO_FLAGS(ii) & INST_PRE_OP_SIZE) && (isWaitIncluded)) return NULL;
			return ii;
		}

		return inst_get_info(in, isWaitIncluded);
	}

	if (isWaitIncluded) return NULL;

	if (instType == INT_LIST_FULL) {
		in = InstructionsTree[INST_NODE_INDEX(in) + tmpIndex1];
		if (in == INT_NOTEXISTS) return NULL;
		instType = INST_NODE_TYPE(in);

		if ((tmpIndex0 == _3DNOW_ESCAPE_BYTE) && (tmpIndex1 == _3DNOW_ESCAPE_BYTE)) return &II_3DNOW;

		if (instType < INT_INFOS)
			return instType == INT_INFO ? &InstInfos[INST_NODE_INDEX(in)] : (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)];

		if (instType == INT_LIST_PREFIXED) return inst_lookup_prefixed(in, ps);
	}

	ci->code += 1;
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;
	tmpIndex2 = *ci->code;

	if (instType == INT_LIST_GROUP) {
		in = InstructionsTree[INST_NODE_INDEX(in) + ((tmpIndex2 >> 3) & 7)];
		if (in == INT_NOTEXISTS) return NULL;
		instType = INST_NODE_TYPE(in);

		if (instType < INT_INFOS)
			return instType == INT_INFO ? &InstInfos[INST_NODE_INDEX(in)] : (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)];

		ii = inst_lookup_prefixed(in, ps);

		if ((ii != NULL) && (ii->opcodeId == I_VMPTRLD) && (tmpIndex1 >= INST_DIVIDED_MODRM)) return &II_RDRAND;
		return ii;
	}

	if (instType == INT_LIST_DIVIDED) {
		_InstNode in2 = InstructionsTree[INST_NODE_INDEX(in) + ((tmpIndex2 >> 3) & 7)];

		instType = INST_NODE_TYPE(in2);

		if (instType == INT_INFO) ii = &InstInfos[INST_NODE_INDEX(in2)];
		else if (instType == INT_INFOEX) ii = (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in2)];

		if ((ii != NULL) && (INST_INFO_FLAGS(ii) & INST_NOT_DIVIDED)) return ii;

		if (tmpIndex2 >= INST_DIVIDED_MODRM) return inst_get_info(in, tmpIndex2 - INST_DIVIDED_MODRM + 8);

		return ii;
	}

	if (instType == INT_LIST_FULL) {

		in = InstructionsTree[INST_NODE_INDEX(in) + tmpIndex2];
		if (in == INT_NOTEXISTS) return NULL;
		instType = INST_NODE_TYPE(in);

		if (instType < INT_INFOS)
			return instType == INT_INFO ? &InstInfos[INST_NODE_INDEX(in)] : (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)];

		if (instType == INT_LIST_PREFIXED) return inst_lookup_prefixed(in, ps);
	}

	return NULL;
}

_InstInfo* inst_lookup_3dnow(_CodeInfo* ci)
{

	_InstNode in = Table_0F_0F;

	int index;

	if (ci->codeLen < 1) return NULL;

	index = *ci->code;

	ci->codeLen -= 1;
	ci->code += 1;
	return inst_get_info(in, index);
}
