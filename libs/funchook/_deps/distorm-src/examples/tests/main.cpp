#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "../../distorm.lib")

#include "../../include/distorm.h"

#define MAX_INSTRUCTIONS (1000)

int main(int argc, char **argv)
{
	_DecodeResult res;
	_DInst decodedInstructions[1000];
	_DecodedInst di;
	unsigned int decodedInstructionsCount = 0, i = 0;
	_OffsetType offset = 0;
	unsigned int dver = distorm_version();
	printf("diStorm version: %d.%d.%d\n", (dver >> 16), ((dver) >> 8) & 0xff, dver & 0xff);

	unsigned char rawData[] = {
		0x0f, 0x01, 0xcb
	};

	_CodeInfo ci = { 0 };
	ci.codeLen = sizeof(rawData);
	ci.code = rawData;
	ci.dt = Decode32Bits;
	ci.features = 0;
	distorm_decompose(&ci, decodedInstructions, 1000, &decodedInstructionsCount);

	for (int i = 0; i < decodedInstructionsCount; i++) {
		distorm_format(&ci, &decodedInstructions[i], &di);
		printf("%08I64x (%02d) %-24s %s%s%s\r\n", di.offset, di.size, (char*)di.instructionHex.p, (char*)di.mnemonic.p, di.operands.length != 0 ? " " : "", (char*)di.operands.p);
	}

	return 0;
}
