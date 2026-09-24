#include <ntddk.h>
#include "../include/distorm.h"
#include "dummy.c"

#define MAX_INSTRUCTIONS (15)

void DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	UNICODE_STRING pFcnName;

	_DecodeResult res;

	_DecodedInst decodedInstructions[MAX_INSTRUCTIONS];

	unsigned int decodedInstructionsCount = 0, i, next;

	_DecodeType dt = Decode32Bits;

	_OffsetType offset = 0;
	char* errch = NULL;

	unsigned char *buf;
	int len = 100;

	DriverObject->DriverUnload = DriverUnload;

	DbgPrint("diStorm Loaded!\n");

	RtlInitUnicodeString(&pFcnName, L"KeBugCheck");
	buf = (char *)MmGetSystemRoutineAddress(&pFcnName);
	offset = (unsigned) (_OffsetType)buf;

	DbgPrint("Resolving KeBugCheck @ 0x%08x\n", buf);

	while (1) {
		res = distorm_decode64(offset, (const unsigned char*)buf, len, dt, decodedInstructions, MAX_INSTRUCTIONS, &decodedInstructionsCount);
		if (res == DECRES_INPUTERR) {
			DbgPrint(("NULL Buffer?!\n"));
			break;
		}

		for (i = 0; i < decodedInstructionsCount; i++) {

			DbgPrint("%08I64x (%02d) %s %s %s\n", decodedInstructions[i].offset, decodedInstructions[i].size,
				 (char*)decodedInstructions[i].instructionHex.p,
				 (char*)decodedInstructions[i].mnemonic.p,
				 (char*)decodedInstructions[i].operands.p);
		}

		if (res == DECRES_SUCCESS || decodedInstructionsCount == 0) {
			break;
		}

		next = (unsigned int)(decodedInstructions[decodedInstructionsCount-1].offset - offset);
		next += decodedInstructions[decodedInstructionsCount-1].size;

		buf += next;
		len -= next;
		offset += next;
	}

	DbgPrint(("Done!\n"));
	return STATUS_UNSUCCESSFUL;
}
