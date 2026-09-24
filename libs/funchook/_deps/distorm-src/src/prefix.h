#ifndef PREFIX_H
#define PREFIX_H

#include "config.h"
#include "decoder.h"

typedef enum {PET_NONE = 0, PET_REX, PET_VEX2BYTES, PET_VEX3BYTES} _PrefixExtType;

typedef enum {PFXIDX_NONE = -1, PFXIDX_REX, PFXIDX_LOREP, PFXIDX_SEG, PFXIDX_OP_SIZE, PFXIDX_ADRS, PFXIDX_MAX} _PrefixIndexer;

typedef struct {
	_iflags decodedPrefixes, usedPrefixes;

	unsigned int count;
	uint16_t unusedPrefixesMask;

	uint16_t pfxIndexer[PFXIDX_MAX];
	_PrefixExtType prefixExtType;

	int isOpSizeMandatory;

	unsigned int vexV;

	unsigned int vrex;
	const uint8_t* vexPos;
} _PrefixState;

#define MAX_PREFIXES (5)

extern int PrefixTables[256 * 2];

_INLINE_ int prefixes_is_valid(unsigned char ch, _DecodeType dt)
{

	return PrefixTables[ch + ((dt >> 1) << 8)];
}

_INLINE_ void prefixes_ignore(_PrefixState* ps, _PrefixIndexer pi)
{

	ps->unusedPrefixesMask |= ps->pfxIndexer[pi];
}

void prefixes_ignore_all(_PrefixState* ps);
uint16_t prefixes_set_unused_mask(_PrefixState* ps);
void prefixes_decode(_CodeInfo* ci, _PrefixState* ps);
void prefixes_use_segment(_iflags defaultSeg, _PrefixState* ps, _DecodeType dt, _DInst* di);

#endif
