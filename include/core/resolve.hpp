#ifndef CORE_MEMORY_RESOLVE_HPP
#define CORE_MEMORY_RESOLVE_HPP

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <sys/mman.h>

#include "core/maps.hpp"

namespace cathook::core::memory
{

inline std::int32_t read_disp32(const void* address, std::ptrdiff_t offset)
{
  std::int32_t displacement = 0;
  std::memcpy(&displacement, static_cast<const std::uint8_t*>(address) + offset, sizeof(displacement));
  return displacement;
}

inline void* resolve_rip_relative(const void* instruction, std::ptrdiff_t displacement_offset,
                                  std::ptrdiff_t instruction_size)
{
  const auto base = reinterpret_cast<std::uintptr_t>(instruction);
  const auto displacement = read_disp32(instruction, displacement_offset);
  return reinterpret_cast<void*>(base + instruction_size + static_cast<std::intptr_t>(displacement));
}

inline void* resolve_checked_rip_relative(const void* instruction, std::ptrdiff_t displacement_offset,
                                          std::ptrdiff_t instruction_size,
                                          std::initializer_list<std::uint8_t> opcode)
{
  if (instruction == nullptr || opcode.size() == 0 ||
      displacement_offset < 0 || instruction_size < 0 ||
      displacement_offset + static_cast<std::ptrdiff_t>(sizeof(std::int32_t)) > instruction_size) {
    return nullptr;
  }

  const int instruction_protection = protection_at(instruction);
  if (instruction_protection < 0 ||
      (instruction_protection & (PROT_READ | PROT_EXEC)) != (PROT_READ | PROT_EXEC)) {
    return nullptr;
  }

  const auto* bytes = static_cast<const std::uint8_t*>(instruction);
  std::size_t index = 0;
  for (const auto expected : opcode) {
    if (bytes[index++] != expected) {
      return nullptr;
    }
  }

  void* const target = resolve_rip_relative(instruction, displacement_offset, instruction_size);
  const int target_protection = protection_at(target);
  if (target_protection < 0 || (target_protection & PROT_READ) == 0) {
    return nullptr;
  }
  return target;
}

inline void* resolve_lea_rip(const void* instruction)
{
  return resolve_checked_rip_relative(instruction, 3, 7, {0x48, 0x8D, 0x05});
}

inline void* resolve_jmp_slot(const void* instruction)
{
  if (instruction == nullptr) {
    return nullptr;
  }
  auto* bytes = static_cast<const std::uint8_t*>(instruction);
  const int protection = protection_at(instruction);
  if (protection < 0 || (protection & (PROT_READ | PROT_EXEC)) != (PROT_READ | PROT_EXEC)) {
    return nullptr;
  }
  if (bytes[0] == 0xF3 && bytes[1] == 0x0F && bytes[2] == 0x1E &&
      (bytes[3] == 0xFA || bytes[3] == 0xFB)) {
    bytes += 4;
  }
  if (bytes[0] != 0xFF || bytes[1] != 0x25) {
    return nullptr;
  }
  return resolve_rip_relative(bytes, 2, 6);
}

}
#endif
