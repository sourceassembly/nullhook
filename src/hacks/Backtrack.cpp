#include "common.hpp"
#include "Backtrack.hpp"
#include "AntiCheatBypass.hpp"
#include <hacks/Aimbot.hpp>

namespace hacks::tf2::backtrack
{
static settings::Boolean enabled("backtrack.enabled", "false");
settings::Float latency("backtrack.latency", "0");
static settings::Int bt_slots("backtrack.slots", "0");

#if ENABLE_VISUALS
static settings::Boolean draw("backtrack.draw", "false");
settings::Boolean chams{ "backtrack.chams", "false" };
settings::Boolean chams_wireframe{ "backtrack.chams.wireframe", "false" };
settings::Int chams_ticks{ "backtrack.chams.ticks", "1" };
settings::Rgba chams_color{ "backtrack.chams.color", "ff00ff10" };
settings::Boolean chams_overlay{ "backtrack.chams.overlay", "true" };
settings::Rgba chams_color_overlay{ "backtrack.chams.color.overlay", "000000ff" };
settings::Float chams_envmap_tint_r{ "backtrack.chams.envmap.tint.r", "1" };
settings::Float chams_envmap_tint_g{ "backtrack.chams.envmap.tint.g", "0" };
settings::Float chams_envmap_tint_b{ "backtrack.chams.envmap.tint.b", "1" };
#endif

static bool isEnabled();

#define MAX_BACKTRACK_TICKS 80

// Check if backtrack is enabled
static bool isBacktrackEnabled = false;

// Is any backtrack tick set for a player currently?
static std::optional<BacktrackData> set_data;
bool hasData()
{
    return (bool) set_data;
}

std::optional<BacktrackData> getData()
{
    return set_data;
}

// Function to get usable ticks on given entity
std::optional<std::vector<BacktrackData>> getGoodTicks(CachedEntity *ent)
{
    if (CE_BAD(ent) || !ent->m_bAlivePlayer())
        return std::nullopt;
    if (ent->m_IDX <= 0 || ent->m_IDX > bt_data.size())
        return std::nullopt;
    if (bt_data[ent->m_IDX - 1].empty())
        return std::nullopt;
    std::optional<std::vector<BacktrackData>> valid_ticks = std::vector<BacktrackData>();
    for (auto &tick : bt_data[ent->m_IDX - 1])
    {
        if (!tick.in_range)
            continue;
        valid_ticks->push_back(tick);
    }

    return valid_ticks->empty() ? std::nullopt : valid_ticks;
}

static int lastincomingsequence = 0;
std::deque<CIncomingSequence> sequences;
static float latency_rampup = 0.0f;

// Store draw positions in CreateMove, also store the selected tick one too
static std::vector<Vector> draw_positions;
static std::optional<Vector> red_position;

std::vector<std::deque<BacktrackData>> bt_data;
// Update our sequences
void updateDatagram()
{
    CNetChan *ch = g_IEngine->GetNetChannelInfo();
    if (ch)
    {
        int m_nInSequenceNr = NetChan(ch)->m_nInSequenceNr();
        int instate         = NetChan(ch)->m_nInReliableState();
        if (m_nInSequenceNr > lastincomingsequence)
        {
            lastincomingsequence = m_nInSequenceNr;
            sequences.emplace_front(instate, m_nInSequenceNr, g_GlobalVars->realtime);
        }
        if (sequences.size() > 67)
            sequences.pop_back();
    }
}

// Latency to add for backtrack
float getLatency()
{
    CNetChan *ch = g_IEngine->GetNetChannelInfo();
    // Track what actual latency we have
    float real_latency = 0.0f;

    // If we have a netchannel (just in case) set real latency to it
    if (ch)
        real_latency = ch->GetLatency(FLOW_OUTGOING) * 1000.0f;

    // Clamp and apply rampup, also ensure we do not go out of the 1000.0f bounds
    float backtrack_latency = latency_rampup * std::clamp(*latency, 0.0f, 900.0f - real_latency);

    return backtrack_latency;
}

bool isTickInRange(int tickcount)
{
    int delta_tickcount = abs(tickcount - current_user_cmd->tick_count + TIME_TO_TICKS(getLatency() / 1000.0f));
    if (!hacks::tf2::antianticheat::enabled)
        return TICKS_TO_TIME(delta_tickcount) <= 0.2f - TICKS_TO_TIME(2);
    else
        return delta_tickcount <= 1;
}

// Is backtrack enabled?
bool isEnabled()
{
    if (!*enabled)
        return false;
    CachedEntity *wep = LOCAL_W;
    if (CE_BAD(wep))
    {
        if (hacks::tf2::antianticheat::enabled)
            return true;
        return false;
    }
    int slot = re::C_BaseCombatWeapon::GetSlot(RAW_ENT(wep));
    switch (*bt_slots)
    {
    // Not set
    case 0:
        return true;
    case 1:
        if (slot == 0)
            return true;
        break;
    case 2:
        if (slot == 1)
            return true;
        break;
    case 3:
        if (slot == 2)
            return true;
        break;
    case 4:
        if (slot == 0 || slot == 1)
            return true;
        break;
    case 5:
        if (slot == 0 || slot == 2)
            return true;
        break;
    case 6:
        if (slot == 1 || slot == 2)
            return true;
        break;
    }
    return false;
}

// Change Datagram data
void adjustPing(INetChannel *ch)
{
    if (!isBacktrackEnabled)
        return;
    for (auto &seq : sequences)
    {
        if (g_GlobalVars->realtime - seq.curtime >= getLatency() / 1000.0f)
        {
            NetChan(ch)->m_nInReliableState() = seq.inreliablestate;
            NetChan(ch)->m_nInSequenceNr()    = seq.sequencenr;
            break;
        }
    }
}

// Move target entity to tick
void MoveToTick(const BacktrackData &data)
{
    if (IDX_BAD(data.entidx) || data.entidx > g_IEngine->GetMaxClients())
        return;
    CachedEntity *target = ENTITY(data.entidx);

    if (CE_BAD(target))
        return;

    // Set entity data to match the target data
    re::C_BasePlayer::SetAbsOrigin(RAW_ENT(target), data.m_vecOrigin);
    re::C_BasePlayer::GetEyeAngles(RAW_ENT(target)) = data.m_vecAngles;

    // Need to reconstruct a bunch of data
    target->hitboxes.InvalidateCache();

    // Reuse the recorded hitboxes instead of recalculating them, but only flag
    // entries that actually contain a hitbox (recorded nulls stay invalid so
    // they get recomputed instead of serving zero data with a null bbox).
    target->hitboxes.m_CacheInternal.resize(data.hitboxes.size());
    for (size_t i = 0; i < data.hitboxes.size() && i < 64; ++i)
    {
        target->hitboxes.m_CacheInternal.at(i) = data.hitboxes.at(i);
        if (data.hitboxes.at(i).bbox)
            target->hitboxes.m_CacheValidationFlags |= 1ULL << i;
        else
            target->hitboxes.m_CacheValidationFlags &= ~(1ULL << i);
    }

    // Sync animation properly
    CE_FLOAT(target, netvar.m_flSimulationTime) = data.simtime;
    CE_FLOAT(target, netvar.m_flCycle)          = data.cycle;
    CE_FLOAT(target, netvar.m_flAnimTime)       = data.animtime;
    CE_INT(target, netvar.m_nSequence)          = data.sequence;

    // Thanks to the epic doghook developers (mainly F1ssion and MrSteyk)
    // I do not have to find all of these signatures and dig through ida
    struct BoneCache;

    typedef BoneCache *(*GetBoneCache_t)(uintptr_t);
    typedef void (*BoneCacheUpdateBones_t)(BoneCache *, matrix3x4_t * bones, unsigned, float time);
    static auto bone_handle_insn                = gSignatures.GetClientSignature(sigs::base_animating_bone_handle);
    static auto hitbox_bone_cache_handle_offset = bone_handle_insn ? *(unsigned *) (bone_handle_insn + 24) : 0u;
    static auto studio_get_bone_cache           = (GetBoneCache_t) gSignatures.GetClientSignature(sigs::studio_get_bone_cache);
    static auto bone_cache_update_bones         = (BoneCacheUpdateBones_t) gSignatures.GetClientSignature(sigs::studio_bone_cache_update_bones);

    if (!hitbox_bone_cache_handle_offset || hitbox_bone_cache_handle_offset > 0x10000 || !studio_get_bone_cache || !bone_cache_update_bones)
        return;
    auto hitbox_bone_cache_handle = CE_VAR(target, hitbox_bone_cache_handle_offset, uintptr_t);
    if (hitbox_bone_cache_handle)
    {
        BoneCache *bone_cache = studio_get_bone_cache(hitbox_bone_cache_handle);
        if (bone_cache && !data.bones.empty())
            bone_cache_update_bones(bone_cache, data.bones.data(), data.bones.size(), g_GlobalVars->curtime);
    }

    // Copy old bones to avoid really really slow setupbones logic
    target->hitboxes.bones       = data.bones;
    target->hitboxes.bones_setup = true;

    // We need to update their positions so the rays actually work, this requires some hacky stuff
    uintptr_t collisionprop = (uintptr_t) RAW_ENT(target) + netvar.m_Collision;

    typedef void (*UpdateParition_t)(uintptr_t prop);
    static auto sig_update                     = gSignatures.GetClientSignature(sigs::collision_update_partition);
    static UpdateParition_t UpdatePartition_fn = sig_update ? (UpdateParition_t) sig_update : nullptr;
    if (UpdatePartition_fn)
        UpdatePartition_fn(collisionprop);
    set_data = data;
}

// Restore ent to original state
void RestoreEntity(int entidx)
{
    if (entidx > bt_data.size() || bt_data[entidx - 1].empty())
        return;
    MoveToTick(bt_data[entidx - 1][0]);
    // Undo tick setting
    set_data = std::nullopt;
}

static void CreateMoveEarly()
{
    if (hacks::tf2::antianticheat::enabled && *latency > 200.0f)
        latency = 200.0f;
    draw_positions.clear();
    isBacktrackEnabled = isEnabled();
    if (!isBacktrackEnabled)
    {
        latency_rampup = 0.0f;
        bt_data.clear();
        return;
    }

    if (CE_GOOD(LOCAL_E))
        updateDatagram();
    else
        sequences.clear();

    latency_rampup += g_GlobalVars->interval_per_tick;
    latency_rampup = std::min(1.0f, latency_rampup);
    if ((int) bt_data.size() != g_IEngine->GetMaxClients())
        bt_data.resize(g_IEngine->GetMaxClients());

    for (auto const &ent: entity_cache::player_cache)
    {
        int i = ent->m_IDX;
        int index         = i - 1;

        auto &ent_data = bt_data[index];

        if (CE_BAD(ent) || !ent->m_bAlivePlayer() || HasCondition<TFCond_HalloweenGhostMode>(ent) || !ent->m_bEnemy())
        {
            ent_data.clear();
            continue;
        }
        const float simtime = CE_FLOAT(ent, netvar.m_flSimulationTime);
        if (ent_data.empty() || simtime > ent_data.front().simtime + 0.0001f)
        {
            BacktrackData data{};
            data.entidx      = i;
            data.m_vecAngles = ent->m_vecAngle();
            data.m_vecOrigin = ent->m_vecOrigin();
            data.tickcount   = current_user_cmd->tick_count;
            data.simtime     = simtime;
            data.animtime    = CE_FLOAT(ent, netvar.m_flAnimTime);
            data.cycle       = CE_FLOAT(ent, netvar.m_flCycle);
            data.sequence    = CE_INT(ent, netvar.m_nSequence);

            ent->hitboxes.GetHitbox(0);
            data.bones = ent->hitboxes.bones;

            for (int hb = head; hb <= foot_R; ++hb)
            {
                if (auto *box = ent->hitboxes.GetHitbox(hb))
                    data.hitboxes.at(hb) = *box;
            }

            ent_data.push_front(std::move(data));
            if (ent_data.size() > MAX_BACKTRACK_TICKS)
                ent_data.pop_back();
        }

        for (auto &tick : ent_data)
        {
            tick.in_range = isTickInRange(tick.tickcount);

            if (!tick.in_range)
                continue;

#if ENABLE_VISUALS
            if (draw)
                draw_positions.push_back(tick.hitboxes.at(0).center);
#endif
        }
    }
}

static void CreateMoveLate()
{
    if (!isBacktrackEnabled)
        return;

    red_position = std::nullopt;

    // Bad player
    if (CE_BAD(LOCAL_E) || HasCondition<TFCond_HalloweenGhostMode>(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;

    // No data set yet, try to get nearest to cursor. When the aimbot already aimed
    // this tick, its own tick selection (or live aim) wins: overwriting the tick
    // with some other entity's history would validate the shot against the wrong tick.
    if (!set_data && !g_pLocalPlayer->bUseSilentAngles && !hacks::shared::aimbot::isAiming())
    {
        float cursor_distance = FLT_MAX;
        for (auto const &ent_data : bt_data)
        {
            for (auto &tick_data : ent_data)
            {
                if (tick_data.in_range)
                {
                    float distance = GetFov(LOCAL_E->m_vecAngle(), g_pLocalPlayer->v_Eye, tick_data.hitboxes.at(0).center);
                    if (distance < cursor_distance)
                    {
                        cursor_distance = distance;
                        set_data        = tick_data;
                    }
                }
            }
        }
    }

    // Still no data set, be sad and return
    if (!set_data)
        return;
    if (!getGoodTicks(ENTITY(set_data->entidx)))
    {
        set_data = std::nullopt;
        return;
    }
    current_user_cmd->tick_count = set_data->tickcount;
    red_position                 = set_data->hitboxes.at(0).center;
    RestoreEntity(set_data->entidx);
}

void Shutdown()
{
    bt_data.clear();
    sequences.clear();
    lastincomingsequence = 0;
}

bool backtrackEnabled()
{
    return isBacktrackEnabled;
}

#if ENABLE_VISUALS
void Draw()
{
    if (!isBacktrackEnabled || !draw)
        return;

    if (!g_IEngine->IsInGame())
        return;

    Vector screen;
    for (auto &pos : draw_positions)
    {
        if (draw::WorldToScreen(pos, screen))
            draw::Rectangle(screen.x - 2, screen.y - 2, 4, 4, colors::green);
    }
    if (red_position && draw::WorldToScreen(*red_position, screen))
        draw::Rectangle(screen.x - 2, screen.y - 2, 4, 4, colors::red);
}
#endif

static InitRoutine init(
    []()
    {
        EC::Register(EC::CreateMove, CreateMoveEarly, "bt_update", EC::very_early);
        EC::Register(EC::CreateMove, CreateMoveLate, "bt_createmove", EC::very_late);
        EC::Register(EC::Shutdown, Shutdown, "bt_shutdown");
        EC::Register(EC::LevelInit, Shutdown, "bt_shutdown");
#if ENABLE_VISUALS
        EC::Register(EC::Draw, Draw, "bt_draw");
#endif
    });

} // namespace hacks::tf2::backtrack
