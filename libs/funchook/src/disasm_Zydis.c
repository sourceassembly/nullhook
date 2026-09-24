#include "config.h"
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "funchook_internal.h"
#include "disasm.h"

#ifdef CPU_X86_64
#define MACHINE_MODE ZYDIS_MACHINE_MODE_LONG_64
#define STACK_WIDTH ZYDIS_STACK_WIDTH_64
#else
#define MACHINE_MODE ZYDIS_MACHINE_MODE_LONG_COMPAT_32
#define STACK_WIDTH ZYDIS_STACK_WIDTH_32
#endif

#define HEX(x) ((x) < 10 ? (x) + '0' : (x) - 10 + 'A')

int funchook_disasm_init(funchook_disasm_t *disasm, funchook_t *funchook, const uint8_t *code, size_t code_size, size_t address)
{
    if (ZydisGetVersion() != ZYDIS_VERSION) {
        funchook_set_error_message(funchook,
                                   "Invalid zydis version: expecte 0x%"PRIx64" but 0x%"PRIx64, ZYDIS_VERSION, ZydisGetVersion());
        return FUNCHOOK_ERROR_INTERNAL_ERROR;
    }

    disasm->funchook = funchook;
    ZydisDecoderInit(&disasm->decoder, MACHINE_MODE, STACK_WIDTH);
    ZydisFormatterInit(&disasm->formatter, ZYDIS_FORMATTER_STYLE_INTEL);
    disasm->insn.next_address = address;
    disasm->code = code;
    disasm->code_end = code + code_size;
    return 0;
}

void funchook_disasm_cleanup(funchook_disasm_t *disasm)
{

}

int funchook_disasm_next(funchook_disasm_t *disasm, const funchook_insn_t **next_insn)
{
    size_t code_size = disasm->code_end - disasm->code;
    ZyanStatus status = ZydisDecoderDecodeFull(&disasm->decoder, disasm->code, code_size,
                                               &disasm->insn.insn, disasm->insn.operands);

    if (ZYAN_SUCCESS(status)) {
        disasm->insn.next_address += disasm->insn.insn.length;
        disasm->code += disasm->insn.insn.length;
        *next_insn = &disasm->insn;
        return 0;
    }
#if 0
    if (status != ZYDIS_STATUS_NO_MORE_DATA && status != ZYDIS_STATUS_INVALID_MAP) {
        funchook_set_error_message(disasm->funchook, "Disassemble Error: 0x%08x", status);
        return FUNCHOOK_ERROR_DISASSEMBLY;
    }
#endif
    return FUNCHOOK_ERROR_END_OF_INSTRUCTION;
}

void funchook_disasm_log_instruction(funchook_disasm_t *disasm, const funchook_insn_t *insn)
{
    funchook_t *funchook = disasm->funchook;
    char buffer[256];
    size_t size = insn->insn.length;
    size_t addr = insn->next_address - size;
    const uint8_t *code = disasm->code - size;
    char hex[24 * 3];
    size_t i;

    ZydisFormatterFormatInstruction(&disasm->formatter, &insn->insn, insn->operands, insn->insn.operand_count,
                                    buffer, sizeof(buffer), addr, ZYAN_NULL);

    for (i = 0; i < size; i++) {
        hex[i * 3 + 0] = HEX(code[i] >> 4);
        hex[i * 3 + 1] = HEX(code[i] & 0x0F);
        hex[i * 3 + 2] = ' ';
    }
    hex[size * 3 - 1] = '\0';

    funchook_log(funchook, "    "ADDR_FMT" (%02d) %-24s %s\n",
                 (size_t)addr, insn->insn.length, hex, buffer);
}

void funchook_disasm_x86_rip_relative(funchook_disasm_t *disasm, const funchook_insn_t *insn, rip_relative_t *rel_disp, rip_relative_t *rel_imm)
{
    memset(rel_disp, 0, sizeof(rip_relative_t));
    memset(rel_imm, 0, sizeof(rip_relative_t));

    if (insn->insn.raw.imm[0].offset != 0) {
        if (insn->insn.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE) {

            rel_imm->addr = (uint8_t*)(size_t)(insn->next_address + insn->insn.raw.imm[0].value.s);
            rel_imm->raddr = (intptr_t)insn->insn.raw.imm[0].value.s;
            rel_imm->size = insn->insn.raw.imm[0].size;
            rel_imm->offset = insn->insn.raw.imm[0].offset;
        }
    }
    if (insn->insn.raw.disp.offset != 0) {
        int i;
        for (i = 0; i < insn->insn.operand_count; i++) {
            const ZydisDecodedOperand *op = &insn->operands[i];
            if (op->mem.disp.has_displacement && op->mem.base == ZYDIS_REGISTER_RIP) {

                rel_disp->addr = (uint8_t*)(size_t)(insn->next_address + insn->insn.raw.disp.value);
                rel_disp->raddr = (intptr_t)insn->insn.raw.disp.value;
                rel_disp->size = insn->insn.raw.disp.size;
                rel_disp->offset = insn->insn.raw.disp.offset;
                break;
            }
        }
    }
}
