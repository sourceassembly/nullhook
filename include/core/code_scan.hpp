#ifndef CORE_MEMORY_CODE_SCAN_HPP
#define CORE_MEMORY_CODE_SCAN_HPP

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <vector>
#include <sys/mman.h>

#include "core/maps.hpp"

namespace cathook::core::memory
{

struct mapping
{
  const std::uint8_t* begin;
  const std::uint8_t* end;
};

inline std::vector<mapping> module_ranges(std::string_view module_name, int protection)
{
  std::vector<mapping> out;
  for_each([&](const entry& e) {
    if ((e.protection & protection) == protection &&
        module_name_matches(e.path, module_name, false)) {
      out.push_back({reinterpret_cast<const std::uint8_t*>(e.start),
                     reinterpret_cast<const std::uint8_t*>(e.end)});
    }
    return true;
  });
  return out;
}

inline bool contains(const std::vector<mapping>& ranges, std::uintptr_t address, std::size_t size)
{
  for (const auto& m : ranges) {
    const auto lo = reinterpret_cast<std::uintptr_t>(m.begin);
    if (address >= lo && address + size <= reinterpret_cast<std::uintptr_t>(m.end)) {
      return true;
    }
  }
  return false;
}

struct mem_insn
{
  std::uint8_t opcode = 0;
  std::uint8_t opcode2 = 0;
  std::uint8_t prefix = 0;
  bool rex_w = false;
  bool rex_r = false;
  bool rex_b = false;
  int mod = -1;
  int reg = -1;
  int base = -1;
  int rm = -1;
  std::int32_t disp = 0;
  std::size_t size = 0;
  std::uintptr_t rip_target = 0;
};

inline bool decode_mem_insn(const std::uint8_t* p, const std::uint8_t* end, mem_insn& out)
{
  const std::uint8_t* q = p;
  out = {};
  while (q < end) {
    const std::uint8_t b = *q;
    if (b == 0x66 || b == 0xF2 || b == 0xF3) {
      out.prefix = b;
      ++q;
      continue;
    }
    if (b >= 0x40 && b <= 0x4F) {
      out.rex_w = (b & 8) != 0;
      out.rex_r = (b & 4) != 0;
      out.rex_b = (b & 1) != 0;
      ++q;
      continue;
    }
    break;
  }
  if (q >= end) {
    return false;
  }
  out.opcode = *q++;
  bool has_modrm = false;
  switch (out.opcode) {
    case 0x63:
    case 0x69:
    case 0x81:
    case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8D:
    case 0xC6: case 0xC7:
      has_modrm = true;
      break;
    case 0x0F:
      if (q >= end) {
        return false;
      }
      out.opcode2 = *q++;
      has_modrm = out.opcode2 == 0x10 || out.opcode2 == 0x11 ||
                  out.opcode2 == 0x58 ||
                  out.opcode2 == 0xB6 || out.opcode2 == 0xB7 ||
                  out.opcode2 == 0x6E || out.opcode2 == 0x7E ||
                  out.opcode2 == 0xD6 ||
                  out.opcode2 == 0x7F ||
                  (out.opcode2 >= 0x90 && out.opcode2 <= 0x9F);
      break;
    default:
      return false;
  }
  if (!has_modrm || q >= end) {
    return false;
  }
  const std::uint8_t modrm = *q++;
  const int mod = modrm >> 6;
  out.mod = mod;
  out.reg = ((modrm >> 3) & 7) + (out.rex_r ? 8 : 0);
  int base = modrm & 7;
  out.disp = 0;
  if (mod == 3) {
    out.base = -2;
    out.rm = base + (out.rex_b ? 8 : 0);
    out.size = static_cast<std::size_t>(q - p);
    return true;
  }
  bool no_base = false;
  if (base == 4) {
    if (q >= end) {
      return false;
    }
    const std::uint8_t sib = *q++;
    base = sib & 7;
    if (mod == 0 && base == 5) {
      no_base = true;
    }
  }
  if (mod == 0 && base == 5) {
    no_base = true;
  }
  if (no_base) {
    if (end - q < 4) {
      return false;
    }
    std::memcpy(&out.disp, q, 4);
    q += 4;
    out.base = out.rex_b && mod == 0 && (modrm & 7) == 5 ? 13 : -1;
    if (out.base == -1) {
      out.rip_target = static_cast<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(q) + out.disp);
    }
  } else if (mod == 1) {
    if (q >= end) {
      return false;
    }
    out.disp = static_cast<std::int8_t>(*q++);
    out.base = base + (out.rex_b ? 8 : 0);
  } else if (mod == 2) {
    if (end - q < 4) {
      return false;
    }
    std::memcpy(&out.disp, q, 4);
    q += 4;
    out.base = base + (out.rex_b ? 8 : 0);
  } else {
    out.base = base + (out.rex_b ? 8 : 0);
  }
  out.size = static_cast<std::size_t>(q - p);
  return true;
}

inline bool is_rip_relative(const mem_insn& insn)
{
  return insn.base == -1;
}

inline std::size_t insn_length(const std::uint8_t* p, const std::uint8_t* end)
{
  const std::uint8_t* q = p;
  bool rex_w = false;
  bool opsize16 = false;
  while (q < end) {
    const std::uint8_t b = *q;
    if (b == 0x66) {
      opsize16 = true;
    } else if (b >= 0x48 && b <= 0x4F) {
      rex_w = true;
    } else if (!(b == 0x67 || b == 0xF0 || b == 0xF2 || b == 0xF3 || b == 0x2E ||
                 b == 0x36 || b == 0x3E || b == 0x26 || b == 0x64 || b == 0x65 ||
                 (b >= 0x40 && b <= 0x47))) {
      break;
    }
    ++q;
  }
  if (q >= end) {
    return 1;
  }
  const std::uint8_t op = *q++;
  bool modrm = false;
  std::size_t imm = 0;
  if (op == 0xC4 || op == 0xC5 || op == 0x62) {
    const int skip = op == 0xC5 ? 1 : (op == 0xC4 ? 2 : 3);
    if (q + skip > end) {
      return static_cast<std::size_t>(end - p);
    }
    int vex_map = 1;
    if (op == 0xC4) {
      vex_map = *q & 0x1F;
    } else if (op == 0x62) {
      vex_map = q[1] & 0x1F;
    }
    q += skip;
    if (q >= end) {
      return static_cast<std::size_t>(q - p);
    }
    const std::uint8_t vop = *q++;
    modrm = !(vex_map == 1 && vop == 0x77);
    if (vex_map == 3 || (vex_map == 1 && (vop == 0xC2 || (vop >= 0x70 && vop <= 0x73) ||
                                        vop == 0xC4 || vop == 0xC5 || vop == 0xC6))) {
      imm = 1;
    }
  } else if (op == 0x0F) {
    if (q >= end) {
      return static_cast<std::size_t>(q - p);
    }
    const std::uint8_t op2 = *q++;
    if (op2 == 0x38 || op2 == 0x3A) {
      if (q >= end) {
        return static_cast<std::size_t>(q - p);
      }
      ++q;
      modrm = true;
      imm = op2 == 0x3A ? 1 : 0;
    } else if (op2 >= 0x80 && op2 <= 0x8F) {
      imm = 4;
    } else if ((op2 >= 0x70 && op2 <= 0x73) || op2 == 0xC2) {
      modrm = true;
      imm = 1;
    } else {
      modrm = (op2 >= 0x10 && op2 <= 0x2F) || (op2 >= 0x40 && op2 <= 0x4F) ||
              (op2 >= 0x50 && op2 <= 0x6F) ||
              ((op2 >= 0x74 && op2 <= 0x7F) && op2 != 0x77) ||
              (op2 >= 0x90 && op2 <= 0x9F) ||
              (op2 >= 0xA3 && op2 <= 0xA5) || op2 == 0xAB || op2 == 0xAD ||
              op2 == 0xAE || (op2 >= 0xB0 && op2 <= 0xBF) ||
              (op2 >= 0xC0 && op2 <= 0xC1) || (op2 >= 0xC4 && op2 <= 0xC7) ||
              (op2 >= 0xD0 && op2 <= 0xFE) || op2 == 0x01;
      if (op2 == 0xA4 || op2 == 0xAC || op2 == 0xBA) {
        imm = 1;
      }
    }
  } else if (op >= 0x70 && op <= 0x7F) {
    imm = 1;
  } else if (op <= 0x3F) {
    const int lo = op & 7;
    if (lo <= 3) {
      modrm = true;
    } else if (lo == 4) {
      imm = 1;
    } else if (lo == 5) {
      imm = opsize16 ? 2 : 4;
    }
  } else {
    modrm = (op >= 0x80 && op <= 0x8F) ||
            op == 0x63 || op == 0x69 || op == 0x6B || op == 0xC0 || op == 0xC1 ||
            op == 0xC6 || op == 0xC7 || (op >= 0xD0 && op <= 0xD3) ||
            op == 0xF6 || op == 0xF7 || op == 0xFE || op == 0xFF;
    switch (op) {
      case 0x6A: case 0x6B: case 0x80: case 0x82: case 0x83: case 0xC0:
      case 0xC1: case 0xC6: case 0xE0: case 0xE1: case 0xE2: case 0xE3:
      case 0xE4: case 0xE5: case 0xE6: case 0xE7: case 0xEB: case 0xCD:
      case 0xA8:
        imm = 1;
        break;
      case 0x68: case 0x69: case 0x81: case 0xC7: case 0xE8: case 0xE9:
        imm = opsize16 ? 2 : 4;
        break;
      case 0xA0: case 0xA1: case 0xA2: case 0xA3:
        imm = 8;
        break;
      case 0xC2: case 0xCA:
        imm = 2;
        break;
      case 0xC8:
        imm = 3;
        break;
      default:
        break;
    }
    if (op >= 0xB0 && op <= 0xB7) {
      imm = 1;
    } else if (op >= 0xB8 && op <= 0xBF) {
      imm = rex_w ? 8 : (opsize16 ? 2 : 4);
    }
  }
  std::size_t group_imm = 0;
  if (modrm) {
    if (q >= end) {
      return static_cast<std::size_t>(q - p);
    }
    const std::uint8_t m = *q++;
    const int mod = m >> 6;
    int base = m & 7;
    if (mod != 3 && base == 4) {
      if (q >= end) {
        return static_cast<std::size_t>(q - p);
      }
      base = *q++ & 7;
    }
    if (mod == 0 && base == 5) {
      q += 4;
    } else if (mod == 1) {
      q += 1;
    } else if (mod == 2) {
      q += 4;
    }
    if (op == 0xF6 && ((m >> 3) & 7) == 0) {
      group_imm = 1;
    } else if (op == 0xF7 && ((m >> 3) & 7) == 0) {
      group_imm = opsize16 ? 2 : 4;
    }
  }
  q += imm + group_imm;
  return q > end ? static_cast<std::size_t>(end - p) : static_cast<std::size_t>(q - p);
}

struct lea_scan
{
  std::size_t range = 0;
  const std::uint8_t* position = nullptr;
};

inline const std::uint8_t* find_lea_to_string(const std::vector<mapping>& code,
                                              const std::vector<mapping>& data,
                                              std::string_view key, lea_scan& scan,
                                              const std::uint8_t** code_end, int required_reg = -1)
{
  for (; scan.range < code.size(); ++scan.range) {
    const auto& c = code[scan.range];
    const std::uint8_t* p = scan.position != nullptr ? scan.position : c.begin;
    for (; p + 16 < c.end; ++p) {
      mem_insn insn{};
      if (!decode_mem_insn(p, c.end, insn) || insn.opcode != 0x8D || !is_rip_relative(insn)) {
        continue;
      }
      if (required_reg >= 0 && (insn.reg != required_reg || !insn.rex_w)) {
        continue;
      }
      const std::uintptr_t target = insn.rip_target;
      if (!contains(data, target, key.size() + 1)) {
        continue;
      }
      if (std::memcmp(reinterpret_cast<const void*>(target), key.data(), key.size() + 1) != 0) {
        continue;
      }
      scan.position = p + 1;
      *code_end = c.end;
      return p;
    }
    scan.position = nullptr;
  }
  return nullptr;
}

inline bool is_scan_terminator(const std::uint8_t* p, const std::uint8_t* end)
{
  const std::uint8_t* q = p;
  while (q < end && (*q == 0x66 || *q == 0x67 || *q == 0xF0 || *q == 0xF2 ||
                     *q == 0xF3 || (*q >= 0x40 && *q <= 0x4F))) {
    ++q;
  }
  if (q >= end) {
    return true;
  }
  const std::uint8_t b = *q;
  if (b == 0xE8 || b == 0xE9 || b == 0xEB || b == 0xC3 || b == 0xC2 ||
      (b & 0xF0) == 0x70) {
    return true;
  }
  if (b == 0x0F && q + 1 < end && q[1] >= 0x80 && q[1] <= 0x8F) {
    return true;
  }
  return false;
}

inline bool is_flow_exit(const std::uint8_t* p, const std::uint8_t* end)
{
  const std::uint8_t* q = p;
  while (q < end && (*q == 0x66 || *q == 0x67 || *q == 0xF0 || *q == 0xF2 ||
                     *q == 0xF3 || (*q >= 0x40 && *q <= 0x4F))) {
    ++q;
  }
  if (q >= end) {
    return true;
  }
  const std::uint8_t b = *q;
  if (b == 0xC3 || b == 0xC2 || b == 0xE9 || b == 0xEB) {
    return true;
  }
  if (b == 0xFF && q + 1 < end && ((q[1] >> 3) & 7) == 4) {
    return true;
  }
  return false;
}

inline int member_access_disp(const std::uint8_t* p, const std::uint8_t* end,
                              std::uint8_t opcode, std::int32_t min_disp,
                              std::int32_t max_disp, int mod_mask = 0x6)
{
  for (const std::uint8_t* q = p; q < end;) {
    if (is_flow_exit(q, end)) {
      break;
    }
    mem_insn insn{};
    if (decode_mem_insn(q, end, insn) && insn.opcode == opcode &&
        insn.base >= 0 && insn.base != 4 && insn.base != 5 &&
        (mod_mask & (1 << insn.mod)) != 0 &&
        insn.disp >= min_disp && insn.disp < max_disp) {
      return insn.disp;
    }
    const std::size_t len = insn_length(q, end);
    q += len != 0 ? len : 1;
  }
  return 0;
}

inline std::size_t mov_imm(const std::uint8_t* p, const std::uint8_t* end,
                           std::uint64_t& imm)
{
  const std::uint8_t* q = p;
  bool rex_w = false;
  while (q < end) {
    const std::uint8_t b = *q;
    if (b == 0x66 || b == 0xF2 || b == 0xF3) {
      ++q;
      continue;
    }
    if (b >= 0x40 && b <= 0x4F) {
      rex_w = rex_w || (b & 8) != 0;
      ++q;
      continue;
    }
    break;
  }
  if (q >= end) {
    return 0;
  }
  const std::uint8_t op = *q++;
  if (op >= 0xB8 && op <= 0xBF) {
    const std::size_t n = rex_w ? 8 : 4;
    if (q + n > end) {
      return 0;
    }
    std::memcpy(&imm, q, n);
    return static_cast<std::size_t>(q - p) + n;
  }
  if (op == 0xC7 && q + 5 <= end && (*q >> 6) == 3 && ((*q >> 3) & 7) == 0) {
    ++q;
    std::uint32_t v = 0;
    std::memcpy(&v, q, 4);
    imm = v;
    return static_cast<std::size_t>(q - p) + 4;
  }
  return 0;
}

inline const std::uint8_t* first_call(const std::uint8_t* p, const std::uint8_t* limit,
                                      const std::uint8_t* end)
{
  while (p < limit && p < end) {
    const std::uint8_t* q = p;
    while (q < end && (*q == 0x66 || *q == 0x67 || *q == 0xF0 || *q == 0xF2 ||
                       *q == 0xF3 || (*q >= 0x40 && *q <= 0x4F))) {
      ++q;
    }
    if (q >= end) {
      return nullptr;
    }
    if (*q == 0xE8) {
      return q;
    }
    if (is_scan_terminator(p, end)) {
      return nullptr;
    }
    p += insn_length(p, end);
  }
  return nullptr;
}

inline int member_store_after_alloc(const std::uint8_t* fn, const std::uint8_t* end,
                                    std::uint32_t alloc_size)
{
  for (const std::uint8_t* p = fn; p < end;) {
    if (is_flow_exit(p, end)) {
      break;
    }
    std::uint64_t imm = 0;
    const std::size_t n = mov_imm(p, end, imm);
    if (n != 0 && imm == alloc_size) {
      const std::uint8_t* const call_limit =
        p + n + 0x80 < end ? p + n + 0x80 : end;
      if (const std::uint8_t* const call = first_call(p + n, call_limit, end)) {
        const std::uint8_t* const limit = call + 0x200 < end ? call + 0x200 : end;
        const int disp = member_access_disp(call + 5, limit, 0x89, 0x40, 0x1000);
        if (disp > 0) {
          return disp;
        }
      }
    }
    const std::size_t len = insn_length(p, end);
    p += len != 0 ? len : 1;
  }
  return 0;
}

inline int member_store_of_arg(const std::uint8_t* fn, const std::uint8_t* end,
                               int arg_reg)
{
  if (arg_reg < 0 || arg_reg >= 16) {
    return 0;
  }
  std::uint32_t tracked = 1u << arg_reg;
  for (const std::uint8_t* q = fn; q < end;) {
    if (is_flow_exit(q, end)) {
      break;
    }
    mem_insn insn{};
    if (decode_mem_insn(q, end, insn)) {
      const bool reg_src =
        insn.reg >= 0 && insn.reg < 16 && ((tracked >> insn.reg) & 1) != 0;
      const bool rm_src =
        insn.rm >= 0 && insn.rm < 16 && ((tracked >> insn.rm) & 1) != 0;
      if (insn.opcode == 0x89) {
        if (insn.mod == 3) {
          if (reg_src) {
            tracked |= 1u << insn.rm;
          }
        } else if (reg_src && insn.base >= 0 && insn.base != 4 &&
                   insn.base != 5 && insn.disp > 0) {
          return insn.disp;
        }
      } else if (insn.mod == 3 && rm_src &&
                 (insn.opcode == 0x8B || insn.opcode == 0x63 ||
                  (insn.opcode == 0x0F &&
                   (insn.opcode2 == 0xB6 || insn.opcode2 == 0xB7)))) {
        tracked |= 1u << insn.reg;
      }
    }
    const std::size_t len = insn_length(q, end);
    q += len != 0 ? len : 1;
  }
  return 0;
}

inline int first_store_disp(const std::uint8_t* p, const std::uint8_t* limit,
                            const std::uint8_t* end)
{
  while (p < limit && p < end) {
    if (is_scan_terminator(p, end)) {
      return 0;
    }
    mem_insn insn{};
    if (decode_mem_insn(p, end, insn) && insn.base >= 0 && insn.reg == 0) {
      if (insn.opcode == 0x88 || insn.opcode == 0x89 || insn.opcode == 0xC6 ||
          insn.opcode == 0xC7 ||
          (insn.opcode == 0x0F &&
           ((insn.opcode2 >= 0x90 && insn.opcode2 <= 0x9F) ||
            insn.opcode2 == 0xD6 || insn.opcode2 == 0x7F))) {
        return insn.disp;
      }
    }
    p += insn_length(p, end);
  }
  return 0;
}

inline int keyed_store_offset(std::string_view module_name, std::string_view key)
{
  const auto code = module_ranges(module_name, PROT_EXEC | PROT_READ);
  const auto data = module_ranges(module_name, PROT_READ);
  lea_scan scan{};
  const std::uint8_t* code_end = nullptr;
  while (const std::uint8_t* lea = find_lea_to_string(code, data, key, scan, &code_end, 6)) {
    const std::uint8_t* call = first_call(lea + 7, lea + 0x60, code_end);
    if (call == nullptr) {
      continue;
    }
    const int offset = first_store_disp(call + 5, call + 0x50, code_end);
    if (offset > 0) {
      return offset;
    }
  }
  return 0;
}

inline int keyed_movq_store_offset(std::string_view module_name, std::string_view key)
{
  const auto code = module_ranges(module_name, PROT_EXEC | PROT_READ);
  const auto data = module_ranges(module_name, PROT_READ);
  lea_scan scan{};
  const std::uint8_t* code_end = nullptr;
  while (const std::uint8_t* lea = find_lea_to_string(code, data, key, scan, &code_end, 6)) {
    const std::uint8_t* limit = lea + 0x800 < code_end ? lea + 0x800 : code_end;
    for (const std::uint8_t* p = lea + 7; p < limit; ) {
      if (*p == 0xC3 || *p == 0xC2) {
        break;
      }
      mem_insn insn{};
      if (decode_mem_insn(p, code_end, insn) && insn.base >= 0 &&
          insn.opcode == 0x0F && (insn.opcode2 == 0xD6 || insn.opcode2 == 0x7F) &&
          insn.reg == 0 && insn.disp > 0) {
        return insn.disp;
      }
      p += insn_length(p, code_end);
    }
  }
  return 0;
}

}
#endif
