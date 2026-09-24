#include "common.hpp"
#include "DetourHook.hpp"
#include "settings/Bool.hpp"

namespace hacks::tf2::medalchanger
{
static settings::Boolean changer{ "visual.medal-changer", "false" };
static settings::Int rank{ "visual.medal-changer.rank", "1" };

using rank_record_t = std::uintptr_t (*)(std::uintptr_t panel, bool target);
static DetourHook rank_record_detour{};

constexpr std::size_t rank_record_size      = 40;
constexpr std::uintptr_t match_group_offset = 584;
constexpr std::uintptr_t panel_table_offset = 632;
constexpr std::uintptr_t records_offset     = 32;
constexpr std::uintptr_t count_offset       = 48;
constexpr int casual_match_group_first      = 5;
constexpr int casual_match_group_last       = 7;

static thread_local std::array<std::byte, rank_record_size> overridden_record{};

static std::uintptr_t rank_record_hook(std::uintptr_t panel, bool target)
{
    const auto original = reinterpret_cast<rank_record_t>(rank_record_detour.GetOriginalFunc());
    if (!original)
        return 0;
    const std::uintptr_t original_record = original(panel, target);

    if (!changer || !panel)
        return original_record;

    const int match_group = *reinterpret_cast<const int *>(panel + match_group_offset);
    if (match_group < casual_match_group_first || match_group > casual_match_group_last)
        return original_record;

    const auto table = *reinterpret_cast<const std::uintptr_t *>(panel + panel_table_offset);
    if (!table)
        return original_record;

    const int count      = *reinterpret_cast<const int *>(table + count_offset);
    const auto records   = *reinterpret_cast<const std::uintptr_t *>(table + records_offset);
    if (!records || count <= 0)
        return original_record;

    const int level = std::clamp(int(*rank), 1, count);
    std::memcpy(overridden_record.data(), reinterpret_cast<const void *>(records + rank_record_size * std::size_t(level - 1)), rank_record_size);
    return reinterpret_cast<std::uintptr_t>(overridden_record.data());
}

static InitRoutine init(
    []()
    {
        const auto addr = gSignatures.GetClientSignature(sigs::casual_rank_record);
        if (addr)
            rank_record_detour.Init(addr, reinterpret_cast<void *>(&rank_record_hook));
        else
            logging::Info("medal changer: casual_rank_record signature not found");
    });
}
