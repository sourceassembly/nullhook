#ifndef CORE_MEMORY_MAPS_HPP
#define CORE_MEMORY_MAPS_HPP

#include <cstdint>
#include <cstdio>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
#include <sys/mman.h>

namespace cathook::core::memory
{

struct entry
{
  std::uintptr_t start;
  std::uintptr_t end;
  int protection;
  std::string_view path;
};

inline int protection_from_flags(const char* flags)
{
  return (flags[0] == 'r' ? PROT_READ : 0) |
         (flags[1] == 'w' ? PROT_WRITE : 0) |
         (flags[2] == 'x' ? PROT_EXEC : 0);
}

inline std::string_view file_name(std::string_view path)
{
  const auto offset = path.rfind('/');
  return offset == std::string_view::npos ? path : path.substr(offset + 1);
}

inline bool parse_line(const std::string& line, entry& out)
{
  unsigned long long start = 0;
  unsigned long long end = 0;
  char flags[5]{};
  if (std::sscanf(line.c_str(), "%llx-%llx %4s", &start, &end, flags) != 3 || end <= start) {
    return false;
  }
  out.start = start;
  out.end = end;
  out.protection = protection_from_flags(flags);
  const auto path_offset = line.find('/');
  out.path = path_offset == std::string::npos ? std::string_view{} : std::string_view{line}.substr(path_offset);
  return true;
}

template <typename Fn>
inline void for_each(Fn&& fn)
{
  std::ifstream maps{"/proc/self/maps"};
  std::string line;
  entry e{};
  while (std::getline(maps, line)) {
    if (parse_line(line, e) && !fn(e)) {
      return;
    }
  }
}

inline bool module_name_matches(std::string_view path, std::string_view module_name, bool prefix_match)
{
  if (path.empty() || module_name.empty()) {
    return false;
  }
  const auto name = file_name(path);
  return name == module_name || (prefix_match && name.starts_with(module_name));
}

inline std::string module_path(std::string_view module_name, bool prefix_match = false)
{
  std::string result;
  for_each([&](const entry& e) {
    if (module_name_matches(e.path, module_name, prefix_match)) {
      result = std::string{e.path};
      return false;
    }
    return true;
  });
  return result;
}

inline bool is_module_loaded(std::string_view module_name)
{
  return !module_path(module_name).empty();
}

inline void* module_base(std::string_view module_name, bool prefix_match = false)
{
  std::uintptr_t base = 0;
  for_each([&](const entry& e) {
    if (module_name_matches(e.path, module_name, prefix_match) && (base == 0 || e.start < base)) {
      base = e.start;
    }
    return true;
  });
  return reinterpret_cast<void*>(base);
}

inline int protection_at(const void* address)
{
  static std::vector<entry> entries;
  static std::deque<std::string> storage;
  static std::mutex mtx;
  const auto value = reinterpret_cast<std::uintptr_t>(address);
  std::lock_guard<std::mutex> lock(mtx);
  for (int attempt = 0; attempt < 2; ++attempt) {
    for (const auto& e : entries) {
      if (value >= e.start && value < e.end)
        return e.protection;
    }
    if (attempt)
      break;
    entries.clear();
    storage.clear();
    std::ifstream maps{"/proc/self/maps"};
    std::string line;
    while (std::getline(maps, line)) {
      entry e{};
      if (!parse_line(line, e))
        continue;
      if (!e.path.empty()) {
        storage.emplace_back(e.path);
        e.path = storage.back();
      }
      entries.push_back(e);
    }
  }
  return -1;
}

}
#endif
