#ifndef DECODER_H
#define DECODER_H

#include "config.h"

typedef unsigned int _iflags;

_DecodeResult decode_internal(_CodeInfo* _ci, int supportOldIntr, _DInst result[], unsigned int maxResultCount, unsigned int* usedInstructionsCount);

#endif
