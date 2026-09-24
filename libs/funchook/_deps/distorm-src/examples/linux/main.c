#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#define EX_OK           0
#define EX_USAGE       64
#define EX_DATAERR     65
#define EX_NOINPUT     66
#define EX_NOUSER      67
#define EX_NOHOST      68
#define EX_UNAVAILABLE 69
#define EX_SOFTWARE    70
#define EX_OSERR       71
#define EX_OSFILE      72
#define EX_CANTCREAT   73
#define EX_IOERR       74
#define EX_TEMPFAIL    75
#define EX_PROTOCOL    76
#define EX_NOPERM      77
#define EX_CONFIG      78

#include "../../include/distorm.h"

#define MAX_INSTRUCTIONS (1000)

int main(int argc, char **argv)
{

	unsigned int dver = 0;

	_DecodeResult res;

	_DecodedInst decodedInstructions[MAX_INSTRUCTIONS];

	unsigned int decodedInstructionsCount = 0, i, next;

	_DecodeType dt = Decode32Bits;

	_OffsetType offset = 0;
	char* errch = NULL;

	int param = 1;

	FILE* f;
	unsigned long filesize = 0, bytesread = 0;
	struct stat st;

	unsigned char *buf, *buf2;

	dver = distorm_version();
	printf("diStorm version: %u.%u.%u\n", (dver >> 16), ((dver) >> 8) & 0xff, dver & 0xff);

	if (argc < 2 || argc > 4) {
		printf("Usage: ./disasm [-b16] [-b64] filename [memory offset]\r\nRaw disassembler output.\r\nMemory offset is origin of binary file in memory (address in hex).\r\nDefault decoding mode is -b32.\r\nexample:   disasm -b16 demo.com 789a\r\n");
		return EX_USAGE;
	}

	if (strncmp(argv[param], "-b16", 4) == 0) {
		dt = Decode16Bits;
		param++;
	} else if (strncmp(argv[param], "-b64", 4) == 0) {
		dt = Decode64Bits;
		param++;
	} else if (*argv[param] == '-') {
		fputs("Decoding mode size isn't specified!\n", stderr);
		return EX_USAGE;
	} else if (argc == 4) {
		fputs("Too many parameters are set.\n", stderr);
		return EX_USAGE;
	}
	if (param >= argc) {
		fputs("Filename is missing.\n", stderr);
		return EX_USAGE;
	}
	if (param + 1 == argc-1) {
#ifdef SUPPORT_64BIT_OFFSET
		offset = strtoull(argv[param + 1], &errch, 16);
#else
		offset = strtoul(argv[param + 1], &errch, 16);
#endif
		if (*errch != '\0') {
			fprintf(stderr, "Offset `%s' couldn't be converted.\n", argv[param + 1]);
			return EX_USAGE;
		}
	}

	f = fopen(argv[param], "rb");
	if (f == NULL) {
		perror(argv[param]);
		return EX_NOINPUT;
	}

	if (fstat(fileno(f), &st) != 0) {
		perror("fstat");
		fclose(f);
		return EX_NOINPUT;
	}
	filesize = st.st_size;

	buf2 = buf = malloc(filesize);
	if (buf == NULL) {
		perror("File too large.");
		fclose(f);
		return EX_UNAVAILABLE;
	}
	bytesread = fread(buf, 1, filesize, f);
	if (bytesread != filesize) {
		perror("Can't read file into memory.");
		free(buf);
		fclose(f);
		return EX_IOERR;
	}

	fclose(f);

	printf("bits: %d\nfilename: %s\norigin: ", dt == Decode16Bits ? 16 : dt == Decode32Bits ? 32 : 64, argv[param]);
#ifdef SUPPORT_64BIT_OFFSET
	if (dt != Decode64Bits) printf("%08llx\n", offset);
	else printf("%016llx\n", offset);
#else
	printf("%08x\n", offset);
#endif

	while (1) {

		res = distorm_decode(offset, (const unsigned char*)buf, filesize, dt, decodedInstructions, MAX_INSTRUCTIONS, &decodedInstructionsCount);
		if (res == DECRES_INPUTERR) {

			fputs("Input error, halting!\n", stderr);
			free(buf2);
			return EX_SOFTWARE;
		}

		for (i = 0; i < decodedInstructionsCount; i++)
#ifdef SUPPORT_64BIT_OFFSET
			printf("%0*llx (%02d) %-24s %s%s%s\r\n", dt != Decode64Bits ? 8 : 16, decodedInstructions[i].offset, decodedInstructions[i].size, (char*)decodedInstructions[i].instructionHex.p, (char*)decodedInstructions[i].mnemonic.p, decodedInstructions[i].operands.length != 0 ? " " : "", (char*)decodedInstructions[i].operands.p);
#else
			printf("%08x (%02d) %-24s %s%s%s\r\n", decodedInstructions[i].offset, decodedInstructions[i].size, (char*)decodedInstructions[i].instructionHex.p, (char*)decodedInstructions[i].mnemonic.p, decodedInstructions[i].operands.length != 0 ? " " : "", (char*)decodedInstructions[i].operands.p);
#endif

		if (res == DECRES_SUCCESS) break;
		else if (decodedInstructionsCount == 0) break;

		next = (unsigned int)(decodedInstructions[decodedInstructionsCount-1].offset - offset);
		next += decodedInstructions[decodedInstructionsCount-1].size;

		buf += next;
		filesize -= next;
		offset += next;
	}

	free(buf2);

	return EX_OK;
}
