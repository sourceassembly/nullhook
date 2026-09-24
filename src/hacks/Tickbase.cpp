#include "common.hpp"
#include "Tickbase.hpp"
#include "Warp.hpp"
#include "DetourHook.hpp"
#include "WeaponData.hpp"
#include "MiscTemporary.hpp"
#include "Think.hpp"
#include "sdk/netmessage.hpp"

namespace hacks::tf2::tickbase
{
static settings::Boolean dt_enabled{ "dt.enabled", "false" };
static settings::Button dt_key{ "dt.key", "<null>" };
static settings::Int dt_key_mode{ "dt.key-mode", "1" };
static settings::Boolean dt_antiwarp{ "dt.anti-warp", "true" };

constexpr int kMaxNewCommands       = 15;
constexpr int kMaxBackupCommands    = 7;
constexpr int kMaxShift             = kMaxNewCommands + kMaxBackupCommands;
constexpr int kEngineBackupCommands = 2;

bool shifting     = false;
bool in_doubletap = false;

static bool in_recharge      = false;
static bool first_shift_tick = false;

static int shifted_ticks = 0;
static int shifted_goal  = 0;
static bool goal_reached = true;

static bool recharge_queued = false;
static int warp_use         = 0;
static bool warp_active     = false;
static bool dt_active       = false;
static int dt_wait          = 0;
static bool dt_key_valid    = true;

static DetourHook cl_move_detour;
using CL_Move_t = void (*)(float, bool);

constexpr int kSignonFull = 6;

int Ticks()
{
    return shifted_ticks;
}

int MaxTicks()
{
    int ticks = kMaxShift;
    static ConVar *sv_max = nullptr;
    if (!sv_max && g_ICvar)
        sv_max = g_ICvar->FindVar("sv_maxusrcmdprocessticks");
    if (sv_max && sv_max->GetInt() > 0)
        ticks = std::min(ticks, sv_max->GetInt());
    return std::clamp(ticks, 1, kMaxShift);
}

static int TickLimit()
{
    return std::max(2, MaxTicks());
}

void Reset()
{
    shifted_ticks = shifted_goal = 0;
    goal_reached                 = true;
    recharge_queued              = false;
    warp_use                     = 0;
    warp_active = dt_active = false;
    shifting = in_doubletap = in_recharge = first_shift_tick = false;
    dt_wait                                                  = 0;
}

void QueueRecharge()
{
    recharge_queued = true;
}

void RequestWarp(int use)
{
    if (use > 0)
        warp_use = use;
}

bool Active()
{
    return bool(hacks::tf2::warp::enabled) || bool(dt_enabled);
}

bool DoubletapEnabled()
{
    return bool(dt_enabled);
}

bool DoubletapHeld()
{
    return dt_key_valid;
}

bool DoubletapKeyBound()
{
    return bool(dt_key) && int(dt_key_mode) != 0;
}

static bool UpdateDTKey()
{
    static bool key_flip          = false;
    static bool pressed_last_tick = false;
    bool allow                    = true;
    if (dt_key && dt_key_mode)
    {
        bool down = dt_key.isKeyDown();
        switch ((int) *dt_key_mode)
        {
        case 1:
            if (!down)
                allow = false;
            break;
        case 2:
            if (down)
                allow = false;
            break;
        case 3:
            if (!pressed_last_tick && down)
                key_flip = !key_flip;
            if (!key_flip)
                allow = false;
            break;
        default:
            break;
        }
        pressed_last_tick = down;
    }
    return allow;
}

void PollDoubletapKey()
{
    dt_key_valid = UpdateDTKey();
}

static float FireDelay()
{
    if (CE_BAD(LOCAL_W))
        return 1.0f;
    auto *data = GetWeaponData(RAW_ENT(LOCAL_W));
    if (!data)
        return 1.0f;
    return re::C_TFWeaponBase::ApplyFireDelay(RAW_ENT(LOCAL_W), data->m_flTimeFireDelay);
}

static bool ValidDTWeapon()
{
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer() || CE_BAD(LOCAL_W))
        return false;
    if (g_pLocalPlayer->weapon_mode == weapon_pda || g_pLocalPlayer->weapon_mode == weapon_consumable || g_pLocalPlayer->weapon_mode == weapon_throwable || g_pLocalPlayer->weapon_mode == weapon_medigun)
        return false;
    int id = re::C_TFWeaponBase::GetWeaponID(RAW_ENT(LOCAL_W));
    if (id == 7)
        return false;
    int cls = LOCAL_W->m_iClassID();
    if (cls == CL_CLASS(CTFRocketPack) || cls == CL_CLASS(CTFCompoundBow) || cls == CL_CLASS(CTFGrapplingHook))
        return false;
    if (cls == CL_CLASS(CTFSniperRifle) || cls == CL_CLASS(CTFSniperRifleDecap) || cls == CL_CLASS(CTFSniperRifleClassic))
        return false;
    if (cls == CL_CLASS(CTFJar) || cls == CL_CLASS(CTFJarMilk) || cls == CL_CLASS(CTFJarGas))
        return false;
    if (cls == CL_CLASS(CTFMinigun))
    {
        int state = CE_INT(LOCAL_W, netvar.iWeaponState);
        if (state != AC_STATE_FIRING && state != AC_STATE_SPINNING)
            return false;
    }
    return true;
}

static int ShotsInPacket(int ticks)
{
    if (CE_BAD(LOCAL_W))
        return 1;
    int delay = 1;
    int cls   = LOCAL_W->m_iClassID();
    if (cls == CL_CLASS(CTFMinigun) || cls == CL_CLASS(CTFPipebombLauncher) || cls == CL_CLASS(CTFCannon))
        delay = 2;
    int fire = std::max(1, (int) std::ceil(FireDelay() / g_GlobalVars->interval_per_tick));
    ticks    = std::min(ticks, MaxTicks() + 1);
    if (ticks <= delay)
        return 1;
    return 1 + (ticks - delay) / fire;
}

static bool WorthShifting(int ticks)
{
    return ticks >= TickLimit() || ShotsInPacket(ticks) > 1;
}

static bool CanDoubletap()
{
    if (!dt_enabled || !dt_key_valid || dt_wait || warp_active || in_recharge)
        return false;
    if (!ValidDTWeapon() || !CanShoot())
        return false;
    return WorthShifting(std::min(shifted_ticks + 1, kMaxShift));
}

static bool Attacking()
{
    return current_user_cmd && (current_user_cmd->buttons & IN_ATTACK) && CanShoot();
}

bool WriteShiftMove()
{
    if (!shifting || !g_IBaseClientState || !g_IBaseClient)
        return false;

    auto *cs = g_IBaseClientState;
    auto *ch = cs->m_NetChannel();
    if (!ch)
        return false;

    int commands = 1 + cs->chokedcommands();
    if (commands <= kMaxNewCommands + kEngineBackupCommands)
        return false;

    CLC_Move msg;
    msg.m_pMessageHandler = nullptr;
    std::array<std::uint8_t, 4000> data;
    msg.m_DataOut.StartWriting(data.data(), data.size());

    msg.m_nNewCommands    = std::clamp(commands, 0, kMaxNewCommands);
    int extra             = commands - msg.m_nNewCommands;
    msg.m_nBackupCommands = std::clamp(extra, kEngineBackupCommands, kMaxBackupCommands);

    int total       = msg.m_nNewCommands + msg.m_nBackupCommands;
    int next_cmd_nr = cs->lastoutgoingcommand() + commands;
    int first_new   = next_cmd_nr - msg.m_nNewCommands + 1;

    for (int from = -1, to = next_cmd_nr - total + 1; to <= next_cmd_nr; ++to)
    {
        if (!g_IBaseClient->WriteUsercmdDeltaToBuffer(&msg.m_DataOut, from, to, to >= first_new))
            return false;
        from = to;
    }

    ch->m_nChokedPackets() = std::max(ch->m_nChokedPackets() - extra, 0);
    return ch->SendNetMsg(msg);
}

static void RunTick(float extra, bool final_tick, CL_Move_t orig)
{
    --shifted_ticks;
    if (dt_wait > 0)
        --dt_wait;
    if (!WorthShifting(std::min(shifted_ticks + 1, kMaxShift)))
        dt_wait = -1;

    goal_reached = final_tick && shifted_ticks == shifted_goal;

    if (!final_tick)
        hooked_methods::UpdatePred();
    orig(extra, final_tick);
}

static void MoveManage()
{
    in_recharge = false;
    if (goal_reached)
    {
        if (recharge_queued && !dt_active && !warp_use && shifted_ticks < MaxTicks())
        {
            in_recharge  = true;
            shifted_goal = shifted_ticks + 1;
        }
        else if (warp_use && shifted_ticks > 0 && !dt_active)
        {
            warp_active  = true;
            shifted_goal = std::max(shifted_ticks - warp_use, 0);
        }
    }
    recharge_queued = false;
    warp_use        = 0;

    if (!in_recharge)
        dt_wait = std::max(dt_wait, 0);

    if (!ValidDTWeapon())
        dt_wait = -1;
    else if (Attacking() || !CanShoot())
        dt_wait = TickLimit();
}

static void Move(float extra, bool final_tick)
{
    auto orig = (CL_Move_t) cl_move_detour.GetOriginalFunc();
    if (!orig)
        return;

    if (!Active() || !g_IEngine->IsInGame() || g_IEngine->IsPlayingTimeDemo() || !g_IBaseClientState || g_IBaseClientState->m_nSignonState() != kSignonFull)
    {
        if (shifted_ticks || shifted_goal)
            Reset();
        orig(extra, final_tick);
        return;
    }

    hacks::tf2::warp::PrepareShift();
    MoveManage();

    int max_shift = MaxTicks();
    while (shifted_ticks > max_shift)
        RunTick(extra, false, orig);

    shifted_ticks = std::max(shifted_ticks, 0) + 1;
    shifted_goal  = std::clamp(shifted_goal, 0, max_shift);

    if (shifted_ticks > shifted_goal)
    {
        shifting                  = shifted_ticks - 1 > shifted_goal;
        in_doubletap              = dt_active;
        hacks::tf2::warp::in_warp = warp_active;
        first_shift_tick          = true;

        while (shifted_ticks > shifted_goal)
        {
            bool last = shifted_ticks - 1 == shifted_goal || g_IBaseClientState->chokedcommands() >= kMaxShift - 1;
            RunTick(extra, last, orig);
            first_shift_tick = false;
        }

        shifting = in_doubletap = first_shift_tick = false;
        hacks::tf2::warp::in_warp                  = false;
        warp_active = dt_active = false;
    }
    else if (g_IBaseClientState->chokedcommands())
        RunTick(extra, final_tick, orig);
}

static void CL_Move_hook(float extra, bool final_tick)
{
    Move(extra, final_tick);
}

static void CreateMove()
{
    if (goal_reached && CanDoubletap() && (Attacking() || dt_active))
    {
        dt_active    = true;
        shifted_goal = std::max(shifted_ticks - TickLimit() + 1, 0);
    }

    if (in_doubletap && dt_antiwarp && CE_GOOD(LOCAL_E) && (CE_INT(LOCAL_E, netvar.iFlags) & FL_ONGROUND) && !first_shift_tick)
        FastStop();
}

static void LevelShutdown()
{
    Reset();
}

static InitRoutine init(
    []()
    {
        auto cl_move_addr = gSignatures.GetEngineSignature(sigs::cl_move);
        cl_move_detour.Init(cl_move_addr, (void *) CL_Move_hook);
        logging::Info("tickbase CL_Move=%p", (void *) cl_move_addr);

        EC::Register(EC::CreateMove, CreateMove, "tickbase_cm", EC::very_late);
        EC::Register(EC::CreateMoveWarp, CreateMove, "tickbase_cmw", EC::very_late);
        EC::Register(EC::LevelShutdown, LevelShutdown, "tickbase_levelshutdown");
        EC::Register(
            EC::Shutdown,
            []()
            {
                cl_move_detour.Shutdown();
            },
            "tickbase_shutdown");
    });
}
