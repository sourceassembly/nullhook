#ifndef INSTS_H
#define INSTS_H

#include "instructions.h"

extern _iflags FlagsTable[];

extern _InstSharedInfo InstSharedInfoTable[];
extern _InstInfo InstInfos[];
extern _InstInfoEx InstInfosEx[];
extern _InstNode InstructionsTree[];

extern _InstNode Table_0F_0F;

extern _InstNode Table_0F, Table_0F_38, Table_0F_3A;

extern _InstInfo II_MOVSXD;

extern _InstInfo II_NOP;
extern _InstInfo II_PAUSE;

extern _InstInfo II_RDRAND;

extern _InstInfo II_3DNOW;

extern uint16_t CmpMnemonicOffsets[8];
extern uint16_t VCmpMnemonicOffsets[32];

#endif
