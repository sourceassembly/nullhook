#include "Settings.hpp"
#include "init.hpp"
#include "HookTools.hpp"
#include "interfaces.hpp"
#include "navparser.hpp"
#include "playerresource.h"
#include "localplayer.hpp"
#include "sdk.hpp"
#include "entitycache.hpp"
#include "CaptureLogic.hpp"
#include "PlayerTools.hpp"
#include "Aimbot.hpp"
#include "navparser.hpp"
#include "MiscAimbot.hpp"
#include "Misc.hpp"
#include "helpers.hpp"

#include <fstream>
#include <unistd.h>
#include <climits>
#include <cstdio>

namespace hacks::tf2::NavBot
{
static settings::Boolean enabled("navbot.enabled", "false");
static settings::Boolean search_health("navbot.search-health", "true");
static settings::Boolean search_ammo("navbot.search-ammo", "true");
static settings::Boolean stay_near("navbot.stay-near", "true");
static settings::Boolean capture_objectives("navbot.capture-objectives", "true");
static settings::Boolean randomize_cpspot("navbot.randomize-cpspot", "true");
static settings::Boolean defend_while_patrolling("navbot.defend-while-patrolling", "false");
static settings::Boolean snipe_sentries("navbot.snipe-sentries", "true");
static settings::Boolean snipe_sentries_shortrange("navbot.snipe-sentries.shortrange", "false");
static settings::Boolean escape_danger("navbot.escape-danger", "true");
static settings::Boolean escape_danger_ctf_cap("navbot.escape-danger.ctf-cap", "false");
static settings::Boolean enable_slight_danger_when_capping("navbot.escape-danger.slight-danger.capping", "false");
static settings::Boolean autojump("navbot.autojump.enabled", "false");
static settings::Boolean primary_only("navbot.primary-only", "true");
static settings::Boolean melee_mode("navbot.melee-mode", "false");
static settings::Int force_slot("navbot.force-slot", "0");
static settings::Float jump_distance("navbot.autojump.trigger-distance", "300");
static settings::Int blacklist_delay("navbot.proximity-blacklist.delay", "500");
static settings::Boolean blacklist_dormat("navbot.proximity-blacklist.dormant", "false");
static settings::Int blacklist_delay_dormat("navbot.proximity-blacklist.delay-dormant", "1000");
static settings::Int blacklist_slightdanger_limit("navbot.proximity-blacklist.slight-danger.amount", "2");
static settings::Boolean engie_mode("navbot.engineer-mode", "true");
#if ENABLE_VISUALS
static settings::Boolean draw_danger("navbot.draw-danger", "false");
#endif

bool isEnabled()
{
    return *enabled;
}

bool isVisible = false;

// Allow for custom danger configs, mainly for debugging purposes
static settings::Boolean danger_config_custom("navbot.danger-config.enabled", "false");
static settings::Boolean danger_config_custom_prefer_far("navbot.danger-config.perfer_far", "true");
static settings::Float danger_config_custom_min_full_danger("navbot.danger-config.min_full_danger", "300");
static settings::Float danger_config_custom_min_slight_danger("navbot.danger-config.min_slight_danger", "500");
static settings::Float danger_config_custom_max_slight_danger("navbot.danger-config.max_slight_danger", "3000");

// Controls the bot parameters like distance from enemy
struct bot_class_config
{
    float min_full_danger;
    float min_slight_danger;
    float max;
    bool prefer_far;
};

constexpr bot_class_config CONFIG_SHORT_RANGE         = { 140.0f, 400.0f, 600.0f, false };
constexpr bot_class_config CONFIG_MID_RANGE           = { 200.0f, 500.0f, 3000.0f, true };
constexpr bot_class_config CONFIG_LONG_RANGE          = { 300.0f, 500.0f, 4000.0f, true };
constexpr bot_class_config CONFIG_ENGINEER            = { 200.0f, 500.0f, 3000.0f, false };
constexpr bot_class_config CONFIG_GUNSLINGER_ENGINEER = { 50.0f, 300.0f, 2000.0f, false };
bot_class_config selected_config                      = CONFIG_MID_RANGE;

enum SupplyFlags
{
    SupplyHealth = 1 << 0,
    SupplyAmmo   = 1 << 1,
    SupplyForced = 1 << 2,
    SupplyLowPrio = 1 << 3
};

struct SupplyData
{
    bool dispenser           = false;
    float respawn_time       = 0.0f;
    Vector origin            = {};
    SupplyData *original_ptr = nullptr;
};

constexpr int FILEWEAPONINFO_IMAXCLIP1 = 356;
constexpr int PLAYER_M_IAMMO = 4240;
constexpr int WEAPON_M_IPRIMARYAMMOTYPE = 3800;
constexpr int VT_GETMAXCLIP1 = 390;
constexpr int AMMO_INFINITE  = 999;
constexpr int TF_AMMO_METAL  = 3;

constexpr int TF_WEAPON_SHOTGUN_PRIMARY          = 12;
constexpr int TF_WEAPON_SHOTGUN_SOLDIER          = 13;
constexpr int TF_WEAPON_SHOTGUN_HWG              = 14;
constexpr int TF_WEAPON_SHOTGUN_PYRO             = 15;
constexpr int TF_WEAPON_SCATTERGUN               = 16;
constexpr int TF_WEAPON_SNIPERRIFLE              = 17;
constexpr int TF_WEAPON_MINIGUN                  = 18;
constexpr int TF_WEAPON_SMG                      = 19;
constexpr int TF_WEAPON_SYRINGEGUN_MEDIC         = 20;
constexpr int TF_WEAPON_ROCKETLAUNCHER           = 22;
constexpr int TF_WEAPON_GRENADELAUNCHER          = 23;
constexpr int TF_WEAPON_PIPEBOMBLAUNCHER         = 24;
constexpr int TF_WEAPON_FLAMETHROWER             = 25;
constexpr int TF_WEAPON_PISTOL                   = 41;
constexpr int TF_WEAPON_PISTOL_SCOUT             = 42;
constexpr int TF_WEAPON_REVOLVER                 = 43;
constexpr int TF_WEAPON_PDA                      = 45;
constexpr int TF_WEAPON_PDA_ENGINEER_BUILD       = 46;
constexpr int TF_WEAPON_PDA_ENGINEER_DESTROY     = 47;
constexpr int TF_WEAPON_PDA_SPY                  = 48;
constexpr int TF_WEAPON_BUILDER                  = 49;
constexpr int TF_WEAPON_MEDIGUN                  = 50;
constexpr int TF_WEAPON_INVIS                    = 57;
constexpr int TF_WEAPON_FLAREGUN                 = 58;
constexpr int TF_WEAPON_LUNCHBOX                 = 59;
constexpr int TF_WEAPON_JAR                      = 60;
constexpr int TF_WEAPON_JAR_MILK                 = 70;
constexpr int TF_WEAPON_COMPOUND_BOW             = 61;
constexpr int TF_WEAPON_BUFF_ITEM                = 62;
constexpr int TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT = 65;
constexpr int TF_WEAPON_LASER_POINTER            = 67;
constexpr int TF_WEAPON_SENTRY_REVENGE           = 69;
constexpr int TF_WEAPON_HANDGUN_SCOUT_PRIMARY    = 71;
constexpr int TF_WEAPON_CROSSBOW                 = 73;
constexpr int TF_WEAPON_STICKBOMB                = 74;
constexpr int TF_WEAPON_HANDGUN_SCOUT_SECONDARY  = 75;
constexpr int TF_WEAPON_SODA_POPPER              = 76;
constexpr int TF_WEAPON_SNIPERRIFLE_DECAP        = 77;
constexpr int TF_WEAPON_RAYGUN                   = 78;
constexpr int TF_WEAPON_PARTICLE_CANNON          = 79;
constexpr int TF_WEAPON_MECHANICAL_ARM           = 80;
constexpr int TF_WEAPON_DRG_POMSON               = 81;
constexpr int TF_WEAPON_FLAREGUN_REVENGE         = 84;
constexpr int TF_WEAPON_CLEAVER                  = 86;
constexpr int TF_WEAPON_PEP_BRAWLER_BLASTER      = 85;
constexpr int TF_WEAPON_STICKY_BALL_LAUNCHER     = 88;
constexpr int TF_WEAPON_SHOTGUN_BUILDING_RESCUE  = 90;
constexpr int TF_WEAPON_CANNON                   = 91;
constexpr int TF_WEAPON_THROWABLE                = 92;
constexpr int TF_WEAPON_PDA_SPY_BUILD            = 94;
constexpr int TF_WEAPON_SPELLBOOK                = 97;
constexpr int TF_WEAPON_SNIPERRIFLE_CLASSIC      = 99;
constexpr int TF_WEAPON_PARACHUTE                = 100;
constexpr int TF_WEAPON_GRAPPLINGHOOK            = 101;
constexpr int TF_WEAPON_PASSTIME_GUN             = 102;
constexpr int TF_WEAPON_CHARGED_SMG              = 103;
constexpr int TF_WEAPON_ROCKETPACK               = 105;
constexpr int TF_WEAPON_JAR_GAS                  = 107;
constexpr int TF_WEAPON_FLAME_BALL               = 109;

constexpr int DEF_FORCE_A_NATURE           = 45;
constexpr int DEF_RAZORBACK                = 57;
constexpr int DEF_BUFF_BANNER              = 129;
constexpr int DEF_SCOTTISH_RESISTANCE      = 130;
constexpr int DEF_CHARGIN_TARGE            = 131;
constexpr int DEF_WRANGLER                 = 140;
constexpr int DEF_BATTALIONS_BACKUP        = 226;
constexpr int DEF_DARWINS_DANGER_SHIELD    = 231;
constexpr int DEF_ROCKET_JUMPER            = 237;
constexpr int DEF_STICKY_JUMPER            = 265;
constexpr int DEF_CONCHEROR                = 354;
constexpr int DEF_ALI_BABAS_WEE_BOOTIES    = 405;
constexpr int DEF_SPLENDID_SCREEN          = 406;
constexpr int DEF_WIDOWMAKER               = 527;
constexpr int DEF_SHORT_CIRCUIT            = 528;
constexpr int DEF_BOOTLEGGER               = 608;
constexpr int DEF_COZY_CAMPER              = 642;
constexpr int DEF_FESTIVE_BUFF_BANNER      = 1001;
constexpr int DEF_FESTIVE_FORCE_A_NATURE   = 1078;
constexpr int DEF_FESTIVE_WRANGLER         = 1086;
constexpr int DEF_TIDE_TURNER              = 1099;
constexpr int DEF_FESTIVE_TARGE            = 1144;
constexpr int DEF_QUICKIEBOMB_LAUNCHER     = 1150;
constexpr int DEF_THERMAL_THRUSTER         = 1179;
constexpr int DEF_BACKCOUNTRY_BLASTER      = 15029;

static std::vector<SupplyData> cached_health_origins;
static std::vector<SupplyData> cached_ammo_origins;
static std::vector<SupplyData> temp_dispensers;
static std::vector<SupplyData> temp_main;

static bool supply_was_force            = false;
static bool has_remembered_dispenser    = false;
static Vector remembered_dispenser      = {};
static Timer remembered_dispenser_timer{};
static Timer sticky_supply_lock_timer{};
static Timer supply_cooldown_timer{};
static Timer supply_repath_timer{};

static void RelinkCachedSupplyPointers()
{
    for (auto &pack : cached_health_origins)
        pack.original_ptr = &pack;
    for (auto &pack : cached_ammo_origins)
        pack.original_ptr = &pack;
}

static void AddCachedSupplyOrigin(Vector origin, bool is_health)
{
    SupplyData data;
    data.origin = origin;
    if (is_health)
        cached_health_origins.push_back(data);
    else
        cached_ammo_origins.push_back(data);
}

static std::string resolveBspPath(const std::string &level_name)
{
    std::vector<std::string> candidates;
    const char *game_dir = g_IEngine->GetGameDirectory();
    if (game_dir)
    {
        candidates.emplace_back(std::string(game_dir) + "/maps/" + level_name + ".bsp");
        candidates.emplace_back(std::string(game_dir) + "/download/maps/" + level_name + ".bsp");
    }
    char cwd[PATH_MAX + 1];
    if (getcwd(cwd, sizeof(cwd)))
        candidates.emplace_back(std::string(cwd) + "/tf/maps/" + level_name + ".bsp");
    for (auto &candidate : candidates)
    {
        std::ifstream fs(candidate, std::ios::binary);
        if (fs.is_open())
            return candidate;
    }
    return "";
}

static void parseCachedSupplyOrigins()
{
    cached_health_origins.clear();
    cached_ammo_origins.clear();
    has_remembered_dispenser = false;

    std::string level_name = GetLevelName();
    if (level_name.empty())
        return;
    std::string bsp_path = resolveBspPath(level_name);
    if (bsp_path.empty())
        return;

    std::ifstream fs(bsp_path, std::ios::binary);
    if (!fs)
        return;

    struct BspLump
    {
        int fileofs;
        int filelen;
        int version;
        char fourCC[4];
    };
    struct BspHeader
    {
        int ident;
        int version;
        BspLump lumps[64];
    } header{};

    fs.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (!fs || header.ident != 0x50534256)
        return;

    const BspLump &ents = header.lumps[0];
    if (ents.filelen <= 0 || ents.filelen > 8 * 1024 * 1024)
        return;

    std::string lump(static_cast<size_t>(ents.filelen), '\0');
    fs.seekg(ents.fileofs);
    fs.read(lump.data(), ents.filelen);
    if (!fs)
        return;

    size_t i = 0;
    while (i < lump.size())
    {
        if (lump[i] != '{')
        {
            ++i;
            continue;
        }
        size_t end = lump.find('}', i);
        if (end == std::string::npos)
            break;
        std::string block = lump.substr(i + 1, end - i - 1);
        i                 = end + 1;

        std::string classname, origin;
        size_t p = 0;
        while (p < block.size())
        {
            size_t q1 = block.find('"', p);
            if (q1 == std::string::npos)
                break;
            size_t q2 = block.find('"', q1 + 1);
            if (q2 == std::string::npos)
                break;
            size_t q3 = block.find('"', q2 + 1);
            if (q3 == std::string::npos)
                break;
            size_t q4 = block.find('"', q3 + 1);
            if (q4 == std::string::npos)
                break;
            std::string key = block.substr(q1 + 1, q2 - q1 - 1);
            std::string val = block.substr(q3 + 1, q4 - q3 - 1);
            p               = q4 + 1;
            if (key == "classname")
                classname = val;
            else if (key == "origin")
                origin = val;
        }

        const bool is_health = classname == "item_healthkit_full" || classname == "item_healthkit_medium" || classname == "item_healthkit_small";
        const bool is_ammo   = classname == "item_ammopack_full" || classname == "item_ammopack_medium" || classname == "item_ammopack_small";
        if (!is_health && !is_ammo)
            continue;
        Vector vec{};
        if (sscanf(origin.c_str(), "%f %f %f", &vec.x, &vec.y, &vec.z) != 3)
            continue;
        AddCachedSupplyOrigin(vec, is_health);
    }
    RelinkCachedSupplyPointers();
}

static bool WeaponDoesNotUseAmmo(int weapon_id, int defidx, bool include_infinite = true)
{
    switch (defidx)
    {
    case DEF_BUFF_BANNER:
    case DEF_FESTIVE_BUFF_BANNER:
    case DEF_BATTALIONS_BACKUP:
    case DEF_CONCHEROR:
    case DEF_TIDE_TURNER:
    case DEF_CHARGIN_TARGE:
    case DEF_SPLENDID_SCREEN:
    case DEF_FESTIVE_TARGE:
    case DEF_BOOTLEGGER:
    case DEF_ALI_BABAS_WEE_BOOTIES:
    case DEF_WRANGLER:
    case DEF_FESTIVE_WRANGLER:
    case DEF_COZY_CAMPER:
    case DEF_DARWINS_DANGER_SHIELD:
    case DEF_RAZORBACK:
    case DEF_THERMAL_THRUSTER:
        return true;
    default:
        switch (weapon_id)
        {
        case TF_WEAPON_PARTICLE_CANNON:
        case TF_WEAPON_RAYGUN:
        case TF_WEAPON_DRG_POMSON:
        case TF_WEAPON_PASSTIME_GUN:
        case TF_WEAPON_SPELLBOOK:
            return include_infinite;
        case TF_WEAPON_FLAREGUN_REVENGE:
        case TF_WEAPON_MEDIGUN:
        case TF_WEAPON_BUFF_ITEM:
        case TF_WEAPON_LASER_POINTER:
        case TF_WEAPON_PDA:
        case TF_WEAPON_PDA_ENGINEER_BUILD:
        case TF_WEAPON_PDA_ENGINEER_DESTROY:
        case TF_WEAPON_PDA_SPY:
        case TF_WEAPON_PDA_SPY_BUILD:
        case TF_WEAPON_BUILDER:
        case TF_WEAPON_INVIS:
        case TF_WEAPON_LUNCHBOX:
        case TF_WEAPON_THROWABLE:
        case TF_WEAPON_CLEAVER:
        case TF_WEAPON_JAR:
        case TF_WEAPON_JAR_GAS:
        case TF_WEAPON_JAR_MILK:
        case TF_WEAPON_PARACHUTE:
        case TF_WEAPON_ROCKETPACK:
        case TF_WEAPON_GRAPPLINGHOOK:
            return true;
        default:
            return false;
        }
    }
}

static int GetWeaponMaxReserveAmmo(int weapon_id, int defidx)
{
    switch (defidx)
    {
    case DEF_WIDOWMAKER:
    case DEF_SHORT_CIRCUIT:
        return 200;
    case DEF_FORCE_A_NATURE:
    case DEF_FESTIVE_FORCE_A_NATURE:
    case DEF_BACKCOUNTRY_BLASTER:
        return 32;
    case DEF_QUICKIEBOMB_LAUNCHER:
        return 24;
    case DEF_SCOTTISH_RESISTANCE:
        return 36;
    case DEF_STICKY_JUMPER:
        return 72;
    case DEF_ROCKET_JUMPER:
        return 60;
    default:
        switch (weapon_id)
        {
        case TF_WEAPON_MINIGUN:
        case TF_WEAPON_PISTOL:
        case TF_WEAPON_FLAMETHROWER:
        case TF_WEAPON_MECHANICAL_ARM:
            return 200;
        case TF_WEAPON_SYRINGEGUN_MEDIC:
            return 150;
        case TF_WEAPON_SMG:
        case TF_WEAPON_CHARGED_SMG:
            return 75;
        case TF_WEAPON_FLAME_BALL:
            return 40;
        case TF_WEAPON_CROSSBOW:
            return 38;
        case TF_WEAPON_HANDGUN_SCOUT_SECONDARY:
        case TF_WEAPON_PISTOL_SCOUT:
        case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
            return 36;
        case TF_WEAPON_SCATTERGUN:
        case TF_WEAPON_PEP_BRAWLER_BLASTER:
        case TF_WEAPON_SODA_POPPER:
        case TF_WEAPON_SENTRY_REVENGE:
        case TF_WEAPON_SHOTGUN_HWG:
        case TF_WEAPON_SHOTGUN_PRIMARY:
        case TF_WEAPON_SHOTGUN_PYRO:
        case TF_WEAPON_SHOTGUN_SOLDIER:
            return 32;
        case TF_WEAPON_SNIPERRIFLE:
        case TF_WEAPON_SNIPERRIFLE_CLASSIC:
        case TF_WEAPON_SNIPERRIFLE_DECAP:
            return 25;
        case TF_WEAPON_STICKBOMB:
        case TF_WEAPON_STICKY_BALL_LAUNCHER:
        case TF_WEAPON_PIPEBOMBLAUNCHER:
        case TF_WEAPON_REVOLVER:
            return 24;
        case TF_WEAPON_ROCKETLAUNCHER:
        case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
            return 20;
        case TF_WEAPON_CANNON:
        case TF_WEAPON_SHOTGUN_BUILDING_RESCUE:
        case TF_WEAPON_GRENADELAUNCHER:
        case TF_WEAPON_FLAREGUN:
            return 16;
        case TF_WEAPON_COMPOUND_BOW:
            return 12;
        default:
            break;
        }
        break;
    }
    return 0;
}

static int GetWeaponMaxClip1(IClientEntity *weapon)
{
    if (!weapon)
        return 0;
    typedef int (*fn_t)(IClientEntity *);
    int max_clip = vfunc<fn_t>(weapon, VT_GETMAXCLIP1, 0)(weapon);
    if (max_clip != 0)
        return max_clip;
    if (!netvar.m_pWeaponInfo)
        return 0;
    auto *info = *reinterpret_cast<std::uint8_t **>(uintptr_t(weapon) + netvar.m_pWeaponInfo);
    if (!info)
        return 0;
    return *reinterpret_cast<int *>(info + FILEWEAPONINFO_IMAXCLIP1);
}

static int GetPrimaryAmmoType(IClientEntity *weapon)
{
    if (!weapon)
        return -1;
    if (netvar.m_iPrimaryAmmoType)
        return NET_INT(weapon, netvar.m_iPrimaryAmmoType);
    return *reinterpret_cast<int *>(uintptr_t(weapon) + WEAPON_M_IPRIMARYAMMOTYPE);
}

static int GetWeaponClip1(IClientEntity *weapon)
{
    if (!weapon)
        return 0;
    if (netvar.m_iClip1)
        return NET_INT(weapon, netvar.m_iClip1);
    return 0;
}

static int GetPlayerAmmoCount(IClientEntity *player, int ammo_type)
{
    if (!player || ammo_type < 0 || ammo_type > 31)
        return 0;
    const uintptr_t off = netvar.m_iAmmo ? uintptr_t(netvar.m_iAmmo) : uintptr_t(PLAYER_M_IAMMO);
    return reinterpret_cast<int *>(uintptr_t(player) + off)[ammo_type];
}

static bool IsEnergyAmmoWeapon(IClientEntity *weapon, int weapon_id)
{
    switch (weapon_id)
    {
    case TF_WEAPON_PARTICLE_CANNON:
    case TF_WEAPON_RAYGUN:
    case TF_WEAPON_DRG_POMSON:
    case TF_WEAPON_FLAREGUN_REVENGE:
    case TF_WEAPON_PASSTIME_GUN:
    case TF_WEAPON_SPELLBOOK:
        return true;
    default:
        break;
    }
    int eid = weapon ? EntIndex(weapon) : -1;
    if (!IDX_GOOD(eid))
        return false;
    CachedEntity *ent = ENTITY(eid);
    if (CE_INVALID(ent))
        return false;
    const int cid = ent->m_iClassID();
    return cid == CL_CLASS(CTFParticleCannon) || cid == CL_CLASS(CTFRaygun) || cid == CL_CLASS(CTFDRGPomson) || cid == CL_CLASS(CTFFlareGun_Revenge) || cid == CL_CLASS(CTFSpellBook);
}

static bool IsSniperRifleWeapon(IClientEntity *weapon, int weapon_id)
{
    if (weapon_id == TF_WEAPON_SNIPERRIFLE || weapon_id == TF_WEAPON_SNIPERRIFLE_CLASSIC || weapon_id == TF_WEAPON_SNIPERRIFLE_DECAP)
        return true;
    int eid = weapon ? EntIndex(weapon) : -1;
    if (!IDX_GOOD(eid))
        return false;
    CachedEntity *ent = ENTITY(eid);
    if (CE_INVALID(ent))
        return false;
    const int cid = ent->m_iClassID();
    return cid == CL_CLASS(CTFSniperRifle) || cid == CL_CLASS(CTFSniperRifleDecap) || cid == CL_CLASS(CTFSniperRifleClassic);
}

static void SortSuppliesByDistance(std::vector<SupplyData> &supplies, const Vector &local_origin)
{
    std::sort(supplies.begin(), supplies.end(), [&](const SupplyData &a, const SupplyData &b) { return a.origin.DistTo(local_origin) < b.origin.DistTo(local_origin); });
}

static Priority_list GetSupplyPriority(int flags)
{
    if (flags & SupplyHealth)
        return flags & SupplyLowPrio ? lowprio_health : health;
    return ammo;
}

static SupplyData BuildRememberedDispenser(const Vector &origin)
{
    SupplyData remembered{};
    remembered.dispenser = true;
    remembered.origin    = origin;
    return remembered;
}

static bool GetSuppliesData(bool &closest_taken, bool is_ammo)
{
    temp_main.clear();
    auto &cached = is_ammo ? cached_ammo_origins : cached_health_origins;

    bool learned_health = false;
    for (auto const &ent : entity_cache::valid_ents)
    {
        if (CE_BAD(ent))
            continue;
        auto type = ent->m_ItemType();
        if (is_ammo ? (type != ITEM_AMMO_SMALL && type != ITEM_AMMO_MEDIUM && type != ITEM_AMMO_LARGE)
                    : (type != ITEM_HEALTH_SMALL && type != ITEM_HEALTH_MEDIUM && type != ITEM_HEALTH_LARGE))
            continue;
        Vector origin = ent->m_vecOrigin();

        bool known = false;
        for (auto &pack : cached)
        {
            if (pack.origin.DistToSqr(origin) > 64.0f * 64.0f)
                continue;
            pack.respawn_time = 0.0f;
            known             = true;
            break;
        }
        if (known)
            continue;
        if (is_ammo)
        {
            SupplyData data;
            data.origin = origin;
            temp_main.push_back(data);
        }
        else
        {
            AddCachedSupplyOrigin(origin, true);
            learned_health = true;
        }
    }
    if (learned_health)
        RelinkCachedSupplyPointers();

    temp_main.reserve(temp_main.size() + cached.size());
    temp_main.insert(temp_main.end(), cached.begin(), cached.end());

    if (temp_main.empty())
        return false;
    SortSuppliesByDistance(temp_main, g_pLocalPlayer->v_Origin);
    closest_taken = temp_main.front().respawn_time != 0.0f;
    return true;
}

static bool GetDispensersData()
{
    temp_dispensers.clear();
    const int highest = g_IEntityList->GetHighestEntityIndex();
    for (int i = 1; i <= highest; ++i)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_INVALID(ent) || ent->m_iClassID() != CL_CLASS(CObjectDispenser) || ent->m_iTeam() != g_pLocalPlayer->team)
            continue;
        if (CE_BYTE(ent, netvar.m_bCarryDeploy) || CE_BYTE(ent, netvar.m_bHasSapper) || CE_BYTE(ent, netvar.m_bBuilding))
            continue;
        auto origin = ent->m_vecDormantOrigin();
        if (!origin)
            continue;

        auto *closest_area = navparser::NavEngine::findClosestNavSquare(*origin);
        if (!closest_area)
            continue;
        Vector nearest = closest_area->getNearestPoint(origin->AsVector2D());
        if (nearest.DistTo(*origin) > 300.0f || origin->z - nearest.z > navparser::PLAYER_JUMP_HEIGHT)
            continue;

        SupplyData data;
        data.dispenser = true;
        data.origin    = *origin;
        temp_dispensers.push_back(data);
    }
    if (temp_dispensers.empty())
        return false;
    SortSuppliesByDistance(temp_dispensers, g_pLocalPlayer->v_Origin);
    return true;
}

bool shouldSearchHealth(bool low_priority = false)
{
    if (!search_health)
        return false;
    if (navparser::NavEngine::current_priority > health)
        return false;

    float health_percent          = LOCAL_E->m_iHealth() / (float) g_pPlayerResource->GetMaxHealth(LOCAL_E);
    bool already_getting_health = navparser::NavEngine::current_priority == health || navparser::NavEngine::current_priority == lowprio_health;
    if (already_getting_health)
        return health_percent < (low_priority ? 0.92f : 0.9f);
    if (HasCondition<TFCond_Healing>(LOCAL_E))
        return false;
    return health_percent < 0.64f || (low_priority && (navparser::NavEngine::current_priority <= patrol || navparser::NavEngine::current_priority == lowprio_health) && health_percent <= 0.80f);
}

bool shouldSearchAmmo()
{
    if (!search_ammo)
        return false;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return false;
    if (navparser::NavEngine::current_priority > ammo)
        return false;

    bool already_getting_ammo = navparser::NavEngine::current_priority == ammo;
    int *weapon_list          = (int *) ((uint64_t) (RAW_ENT(LOCAL_E)) + netvar.hMyWeapons);
    if (!weapon_list)
        return false;

    for (int i = 0; i <= 4; ++i)
    {
        int handle = weapon_list[i];
        int eid    = HandleToIDX(handle);
        if (eid <= 0 || eid > HIGHEST_ENTITY)
            continue;
        IClientEntity *weapon = g_IEntityList->GetClientEntity(eid);
        if (!weapon || !re::C_BaseCombatWeapon::IsBaseCombatWeapon(weapon))
            continue;
        int slot = re::C_BaseCombatWeapon::GetSlot(weapon);
        if (slot != 0 && slot != 1)
            continue;
        int defidx    = NET_INT(weapon, netvar.iItemDefinitionIndex);
        int weapon_id = re::C_TFWeaponBase::GetWeaponID(weapon);
        if (WeaponDoesNotUseAmmo(weapon_id, defidx, true) || IsEnergyAmmoWeapon(weapon, weapon_id))
            continue;

        int ammo_type = GetPrimaryAmmoType(weapon);
        int reserve   = GetPlayerAmmoCount(RAW_ENT(LOCAL_E), ammo_type);
        if (ammo_type < 0 || reserve == AMMO_INFINITE)
            continue;

        int clip      = GetWeaponClip1(weapon);
        int max_clip  = GetWeaponMaxClip1(weapon);
        bool uses_clip = clip >= 0 && max_clip > 0;
        int max_reserve = GetWeaponMaxReserveAmmo(weapon_id, defidx);
        if (ammo_type == TF_AMMO_METAL && max_reserve <= 0)
            max_reserve = 200;

        const int shot_low                     = already_getting_ammo ? 10 : 5;
        const float clip_threshold             = already_getting_ammo ? 0.35f : 0.25f;
        const float reserve_critical_threshold = already_getting_ammo ? 0.35f : 0.25f;
        const float reserve_skip_threshold     = already_getting_ammo ? 0.75f : 0.6f;
        const float reserve_search_threshold   = already_getting_ammo ? 0.45f : (1.f / 3.f);

        if (IsSniperRifleWeapon(weapon, weapon_id))
        {
            if (uses_clip && clip > shot_low)
                continue;
            int pool = (uses_clip ? clip : 0) + (reserve > 0 ? reserve : 0);
            if (pool <= shot_low)
                return true;
            continue;
        }

        if (!uses_clip)
        {
            if (max_reserve <= 0)
            {
                if (reserve > 0 && reserve <= shot_low)
                    return true;
                continue;
            }
            if (reserve >= max_reserve * reserve_skip_threshold)
                continue;
            if (reserve <= max_reserve * reserve_search_threshold)
                return true;
            continue;
        }

        const bool reserve_pool = max_reserve > 0 && (reserve > 0 || clip <= 0);
        if (!reserve_pool)
        {
            if (clip <= max_clip * clip_threshold)
                return true;
            continue;
        }

        if (clip <= max_clip * clip_threshold && reserve <= max_reserve * reserve_critical_threshold)
            return true;
        if (reserve >= max_reserve * reserve_skip_threshold)
            continue;
        if (reserve <= max_reserve * reserve_search_threshold)
            return true;
    }
    return false;
}

static bool GetSupply(SupplyData *supply, int priority)
{
    float dist                     = supply->origin.DistTo(g_pLocalPlayer->v_Origin);
    const auto e_priority          = static_cast<Priority_list>(priority);
    if (!supply->dispenser)
    {
        if (dist < 75.0f)
        {
            CNavArea *local_area = navparser::NavEngine::findClosestNavSquare(g_pLocalPlayer->v_Origin);
            if (!local_area)
                return false;
            Vector path_point = local_area->getNearestPoint(supply->origin.AsVector2D());
            path_point.z      = supply->origin.z;
            if (supply->original_ptr && !supply->respawn_time && dist <= 20.0f)
                supply->original_ptr->respawn_time = g_GlobalVars->curtime + 10.0f;
            WalkTo(path_point);
            return true;
        }
    }
    else if (dist <= 150.0f)
    {
        if (navparser::NavEngine::current_priority != e_priority)
        {
            if (!navparser::NavEngine::navTo(supply->origin, e_priority))
                navparser::NavEngine::current_priority = e_priority;
        }
        return true;
    }
    return navparser::NavEngine::navTo(supply->origin, e_priority);
}

static void UpdateTakenState()
{
    float now = g_GlobalVars->curtime;
    for (auto &pack : cached_health_origins)
    {
        if (pack.respawn_time < now)
            pack.respawn_time = 0.0f;
    }
    for (auto &pack : cached_ammo_origins)
    {
        if (pack.respawn_time < now)
            pack.respawn_time = 0.0f;
    }
}

static bool RunSupply(int flags)
{
    temp_main.clear();
    temp_dispensers.clear();
    bool low_prio             = flags & SupplyLowPrio;
    const auto e_priority     = GetSupplyPriority(flags);
    bool should_force         = flags & SupplyForced;
    bool is_ammo              = e_priority == ammo;
    const int current_prio    = navparser::NavEngine::current_priority;
    const bool active_health  = current_prio == health || current_prio == lowprio_health;
    const bool active_supply  = is_ammo ? current_prio == ammo : active_health;
    const float health_percent = LOCAL_E->m_iHealth() / (float) g_pPlayerResource->GetMaxHealth(LOCAL_E);
    const bool needs_health   = health_percent < (low_prio ? 0.92f : 0.9f);
    const bool can_keep_lock  = is_ammo || needs_health;

    if (!should_force && !(is_ammo ? shouldSearchAmmo() : shouldSearchHealth(low_prio)))
    {
        if (!is_ammo && has_remembered_dispenser && needs_health && !remembered_dispenser_timer.check(2000))
        {
            auto remembered = BuildRememberedDispenser(remembered_dispenser);
            if (GetSupply(&remembered, e_priority))
                return true;
        }
        if (active_supply && can_keep_lock && !sticky_supply_lock_timer.check(1250))
            return true;
        if (active_supply && (!is_ammo || !supply_was_force))
            navparser::NavEngine::cancelPath();
        return false;
    }
    sticky_supply_lock_timer.update();

    if (!should_force && !supply_cooldown_timer.check(1000))
        return active_supply;
    if (active_supply && !supply_repath_timer.test_and_set(2000))
        return true;

    UpdateTakenState();
    supply_was_force            = false;
    bool closest_taken          = false;
    bool got_supplies           = GetSuppliesData(closest_taken, is_ammo);
    bool got_dispensers         = GetDispensersData();
    if (!is_ammo)
    {
        if (got_dispensers && !temp_dispensers.empty())
        {
            has_remembered_dispenser = true;
            remembered_dispenser     = temp_dispensers.front().origin;
            remembered_dispenser_timer.update();
        }
        else if (has_remembered_dispenser && needs_health && !remembered_dispenser_timer.check(2000))
        {
            auto remembered = BuildRememberedDispenser(remembered_dispenser);
            if (GetSupply(&remembered, e_priority))
                return true;
        }
        else if (has_remembered_dispenser && (remembered_dispenser_timer.check(2000) || !needs_health))
            has_remembered_dispenser = false;
    }
    if (!got_supplies && !got_dispensers)
    {
        if (active_supply && can_keep_lock && !sticky_supply_lock_timer.check(1250))
            return true;
        supply_cooldown_timer.update();
        return false;
    }

    const Vector local_origin = g_pLocalPlayer->v_Origin;
    bool has_close_dispenser  = false;
    if (got_dispensers)
    {
        has_close_dispenser = true;
        temp_main.reserve(temp_main.size() + temp_dispensers.size());
        temp_main.insert(temp_main.end(), temp_dispensers.begin(), temp_dispensers.end());
        SortSuppliesByDistance(temp_main, local_origin);
    }

    SupplyData *best = nullptr, *second = nullptr;
    if (closest_taken)
    {
        for (auto &data : temp_main)
        {
            if (data.respawn_time)
                continue;
            if (best)
            {
                second = &data;
                break;
            }
            best = &data;
        }
    }
    if (!best)
    {
        best = &temp_main.front();
        if (has_close_dispenser)
        {
            if (closest_taken)
                best = &temp_dispensers.front();
        }
        else if (temp_main.size() > 1)
            second = &temp_main.at(1);
    }

    if (second)
    {
        float first_cost  = navparser::NavEngine::getPathCost(local_origin, best->origin);
        float second_cost = navparser::NavEngine::getPathCost(local_origin, second->origin);
        if (second_cost < first_cost)
            best = second;
    }

    if (best && GetSupply(best, e_priority))
    {
        supply_was_force = should_force;
        sticky_supply_lock_timer.update();
        return true;
    }

    supply_cooldown_timer.update();
    return false;
}

bool getHealth(bool low_priority = false)
{
    return RunSupply(SupplyHealth | (low_priority ? SupplyLowPrio : 0));
}

bool getAmmo(bool force = false)
{
    return RunSupply(SupplyAmmo | (force ? SupplyForced : 0));
}

// Vector of sniper spot positions we can nav to
std::vector<Vector> sniper_spots;

// Used for time between refreshing sniperspots
static Timer refresh_sniperspots_timer{};
void refreshSniperSpots()
{
    if (!refresh_sniperspots_timer.test_and_set(60000))
        return;

    sniper_spots.clear();
    std::vector<Vector> exposed_spots;

    // Search all nav areas for valid sniper spots
    for (auto &area : navparser::NavEngine::getNavFile()->m_areas)
        for (auto &hiding_spot : area.m_hidingSpots)
        {
            if (hiding_spot.IsGoodSniperSpot() || hiding_spot.IsIdealSniperSpot() || hiding_spot.HasGoodCover())
            {
                sniper_spots.emplace_back(hiding_spot.m_pos);
                continue;
            }
            if (hiding_spot.IsExposed())
                exposed_spots.emplace_back(hiding_spot.m_pos);
        }

    if (sniper_spots.empty() && !exposed_spots.empty())
        sniper_spots = std::move(exposed_spots);
}

std::pair<CachedEntity *, float> getNearestPlayerDistance()
{
    float distance         = FLT_MAX;
    CachedEntity *best_ent = nullptr;
    for (auto const &ent: entity_cache::player_cache)
    {
        
        if (CE_VALID(ent) && ent->m_vecDormantOrigin() && g_pPlayerResource->isAlive(ent->m_IDX) && ent->m_bEnemy() && g_pLocalPlayer->v_Origin.DistTo(*ent->m_vecDormantOrigin()) < distance && player_tools::shouldTarget(ent) && !IsPlayerInvisible(ent))
        {
            distance = g_pLocalPlayer->v_Origin.DistTo(*ent->m_vecDormantOrigin());
            best_ent = ent;
        }
    }
    return { best_ent, distance };
}

static std::vector<Vector> building_spots;

inline bool HasGunslinger(CachedEntity *ent)
{
    return HasWeapon(ent, 142);
}

inline bool isEngieMode()
{
    return *engie_mode && g_pLocalPlayer->clazz == tf_engineer;
}

bool BlacklistedFromBuilding(CNavArea *area)
{
    // FIXME: Better way of doing this ?
    for (auto blacklisted_area : *navparser::NavEngine::getFreeBlacklist())
    {
        if (blacklisted_area.first == area && blacklisted_area.second.value == navparser::BlacklistReason_enum::BAD_BUILDING_SPOT)
            return true;
    }
    return false;
}

static Timer refresh_buildingspots_timer;
void refreshBuildingSpots(bool force = false)
{
    if (!isEngieMode())
        return;
    if (force || refresh_buildingspots_timer.test_and_set(HasGunslinger(LOCAL_E) ? 1000 : 5000))
    {
        building_spots.clear();
        std::optional<Vector> target;

        auto our_flag = flagcontroller::getFlag(g_pLocalPlayer->team);
        target        = our_flag.spawn_pos;

        if (!target)
        {
            auto nearest = getNearestPlayerDistance();
            if (CE_GOOD(nearest.first))
                target = *nearest.first->m_vecDormantOrigin();
            if (!target)
                target = LOCAL_E->m_vecOrigin();
        }
        if (target)
        {
            // Search all nav areas for valid spots
            for (auto &area : navparser::NavEngine::getNavFile()->m_areas)
            {
                // Blacklisted :(
                if (BlacklistedFromBuilding(&area))
                    continue;
                // BUG Ahead, these flag checks dont seem to work for me :/
                // Don't try to build in spawn lol
                if ((area.m_TFattributeFlags & TF_NAV_SPAWN_ROOM_RED) != 0 || (area.m_TFattributeFlags & TF_NAV_SPAWN_ROOM_BLUE) != 0 || (area.m_TFattributeFlags & TF_NAV_SPAWN_ROOM_EXIT) != 0)
                    continue;
                if ((area.m_TFattributeFlags & TF_NAV_SENTRY_SPOT) != 0)
                    building_spots.emplace_back(area.m_center);
                else
                {
                    for (auto &hiding_spot : area.m_hidingSpots)
                        if (hiding_spot.HasGoodCover())
                            building_spots.emplace_back(hiding_spot.m_pos);
                }
            }
            // Sort by distance to nearest, lower is better
            // TODO: This isnt really optimal, need a dif way to where it is a good distance from enemies but also bots dont build in the same spot
            std::sort(building_spots.begin(), building_spots.end(),
                      [target](Vector a, Vector b)
                      {
                          if (!HasGunslinger(LOCAL_E))
                          {
                              auto a_dist = a.DistTo(*target);
                              auto b_dist = b.DistTo(*target);

                              // Penalty for being in danger ranges
                              if (a_dist + 100.0f < selected_config.min_full_danger)
                                  a_dist += 4000.0f;
                              if (b_dist + 100.0f < selected_config.min_full_danger)
                                  b_dist += 4000.0f;

                              if (a_dist + 1000.0f < selected_config.min_slight_danger)
                                  a_dist += 1500.0f;
                              if (b_dist + 1000.0f < selected_config.min_slight_danger)
                                  b_dist += 1500.0f;

                              return a_dist < b_dist;
                          }
                          else
                              return a.DistTo(*target) < b.DistTo(*target);
                      });
        }
    }
}

static CachedEntity *mySentry    = nullptr;
static CachedEntity *myDispenser = nullptr;

void refreshLocalBuildings()
{
    if (isEngieMode())
    {
        mySentry    = nullptr;
        myDispenser = nullptr;
        if (CE_GOOD(LOCAL_E))
        {
            for (auto const &ent : entity_cache::valid_ents)
            {
                if (ent->m_bEnemy() || !ent->m_bAlivePlayer())
                    continue;
                auto cid = ent->m_iClassID();
                if (cid != CL_CLASS(CObjectSentrygun) && cid != CL_CLASS(CObjectDispenser))
                    continue;
                if (HandleToIDX(CE_INT(ent, netvar.m_hBuilder)) != LOCAL_E->m_IDX)
                    continue;
                if (CE_INT(ent, netvar.m_bPlacing))
                    continue;
                if (cid == CL_CLASS(CObjectSentrygun))
                    mySentry = ent;
                else if (cid == CL_CLASS(CObjectDispenser))
                    myDispenser = ent;
            }
        }
    }
}

static Vector current_building_spot;
static bool navToSentrySpot()
{
    static Timer wait_until_path_sentry;
    // Wait a bit before pathing again
    if (!wait_until_path_sentry.test_and_set(300))
        return false;
    // Try to nav to our existing sentry spot
    if (CE_GOOD(mySentry) && mySentry->m_bAlivePlayer() && mySentry->m_vecDormantOrigin())
    {
        // Don't overwrite current nav
        if (navparser::NavEngine::current_priority == engineer)
            return true;
        if (navparser::NavEngine::navTo(*mySentry->m_vecDormantOrigin(), engineer))
            return true;
    }
    else
        mySentry = nullptr;

    // No building spots
    if (building_spots.empty())
        return false;
    // Don't overwrite current nav
    if (navparser::NavEngine::current_priority == engineer)
        return false;
    // Max 10 attempts
    for (int attempts = 0; attempts < 10 && attempts < building_spots.size(); ++attempts)
    {
        // Get a semi-random building spot to still keep distance preferrance
        auto random_offset = RandomInt(0, std::min(3, (int) building_spots.size()));

        Vector random;

        // Wrap around
        if (attempts - random_offset < 0)
            random = building_spots[building_spots.size() + (attempts - random_offset)];
        else
            random = building_spots[attempts - random_offset];

        // Try to nav there
        if (navparser::NavEngine::navTo(random, engineer))
        {
            current_building_spot = random;
            return true;
        }
    }

    return false;
}

enum slots
{
    primary   = 1,
    secondary = 2,
    melee     = 3,
    pda1      = 4,
    pda2      = 5
};

#if ENABLE_VISUALS
std::vector<Vector> slight_danger_drawlist_normal;
std::vector<Vector> slight_danger_drawlist_dormant;
static std::mutex navbot_draw_mutex;
static std::vector<std::string> debug_lines_snapshot;
static std::vector<Vector> danger_normal_snapshot;
static std::vector<Vector> danger_dormant_snapshot;
static std::vector<Vector> blacklist_snapshot;
static bool nav_ready_snapshot = false;
#endif
static Timer blacklist_update_timer{};
static Timer dormant_update_timer{};
void updateEnemyBlacklist(int slot)
{
    static int last_slot_blacklist = primary;
    bool should_run_normal         = blacklist_update_timer.test_and_set(*blacklist_delay) || last_slot_blacklist != slot;
    bool should_run_dormant        = blacklist_dormat && (dormant_update_timer.test_and_set(*blacklist_delay_dormat) || last_slot_blacklist != slot);
    last_slot_blacklist            = slot;
    // Don't run since we do not care here
    if (!should_run_dormant && !should_run_normal)
        return;

    // Clear blacklist for normal entities
    if (should_run_normal)
        navparser::NavEngine::clearFreeBlacklist(navparser::ENEMY_NORMAL);
    // Clear blacklist for dormant entities
    if (should_run_dormant || !blacklist_dormat)
        navparser::NavEngine::clearFreeBlacklist(navparser::ENEMY_DORMANT);

    // #NoFear
    if (slot == melee)
        return;

    // Store the danger of the invidual nav areas
    boost::unordered_flat_map<CNavArea *, int> dormant_slight_danger;
    boost::unordered_flat_map<CNavArea *, int> normal_slight_danger;

    // This is used to cache Dangerous areas between ents
    boost::unordered_flat_map<CachedEntity *, std::vector<CNavArea *>> ent_marked_dormant_slight_danger;
    boost::unordered_flat_map<CachedEntity *, std::vector<CNavArea *>> ent_marked_normal_slight_danger;

    std::vector<std::pair<CachedEntity *, Vector>> checked_origins;
    for (auto const &ent: entity_cache::player_cache)
    {
        
        // Entity is generally invalid, ignore
        if (CE_INVALID(ent) || !g_pPlayerResource->isAlive(ent->m_IDX))
            continue;
        // On our team, do not care
        if (g_pPlayerResource->GetTeam(ent->m_IDX) == g_pLocalPlayer->team)
            continue;

        bool is_dormant = CE_BAD(ent);
        if (!should_run_dormant || !is_dormant)
            continue;

        // Avoid excessive calls by ignoring new checks if people are too close to eachother
        auto origin = ent->m_vecDormantOrigin();
        if (!origin)
            continue;
        bool should_check = true;

        // Find already dangerous marked areas by other entities
        auto to_loop = is_dormant ? &ent_marked_dormant_slight_danger : &ent_marked_normal_slight_danger;

        // Add new danger entries
        auto to_mark = is_dormant ? &dormant_slight_danger : &normal_slight_danger;

        for (auto &checked_origin : checked_origins)
        {
            // If this origin is closer than a quarter of the min HU (or less than 100 HU) to a cached one, don't go through
            // all nav areas again DistToSqr is much faster than DistTo which is why we use it here
            auto distance = selected_config.min_slight_danger;

            distance *= 0.25f;
            distance = std::max(100.0f, distance);

            // Square the distance
            distance *= distance;

            if ((*origin).DistToSqr(checked_origin.second) < distance)
            {
                should_check = false;

                bool is_absolute_danger = distance < selected_config.min_full_danger;
                if (!is_absolute_danger && (enable_slight_danger_when_capping || navparser::NavEngine::current_priority != capture))
                    for (auto &area : (*to_loop)[checked_origin.first])
                    {
                        (*to_mark)[area]++;
                        if ((*to_mark)[area] >= *blacklist_slightdanger_limit)
                            (*navparser::NavEngine::getFreeBlacklist())[area] = is_dormant ? navparser::ENEMY_DORMANT : navparser::ENEMY_NORMAL;
                    }

                break;
            }
        }
        if (!should_check)
            continue;

        // Now check which areas they are close to
        for (CNavArea &nav_area : navparser::NavEngine::getNavFile()->m_areas)
        {
            float distance             = nav_area.m_center.DistTo(*origin);
            float slight_danger_dist   = selected_config.min_slight_danger;
            float absolute_danger_dist = selected_config.min_full_danger;

            // Not dangerous, Still don't bump
            if (!player_tools::shouldTarget(ent))
            {
                slight_danger_dist   = navparser::PLAYER_WIDTH * 1.2f;
                absolute_danger_dist = navparser::PLAYER_WIDTH * 1.2f;
            }

            // Too close to count as slight danger
            bool is_absolute_danger = distance < absolute_danger_dist;
            if (distance < slight_danger_dist)
            {
                // Add as marked area
                (*to_loop)[ent].push_back(&nav_area);

                // Just slightly dangerous, only mark as such if it's clear
                if (!is_absolute_danger && (enable_slight_danger_when_capping || navparser::NavEngine::current_priority != capture))
                {
                    (*to_mark)[&nav_area]++;
                    if ((*to_mark)[&nav_area] < *blacklist_slightdanger_limit)
                        continue;
                }
                (*navparser::NavEngine::getFreeBlacklist())[&nav_area] = is_dormant ? navparser::ENEMY_DORMANT : navparser::ENEMY_NORMAL;
            }
        }
        checked_origins.emplace_back(ent, *origin);
    }
#if ENABLE_VISUALS
    if (should_run_dormant)
        slight_danger_drawlist_dormant.clear();
    if (should_run_normal)
        slight_danger_drawlist_normal.clear();

    // Store slight danger areas for drawing
    if (!normal_slight_danger.empty())
    {
        for (auto &area : normal_slight_danger)
            if (area.second < *blacklist_slightdanger_limit)
                slight_danger_drawlist_normal.push_back(area.first->m_center);
    }
    if (!dormant_slight_danger.empty())
    {
        for (auto &area : dormant_slight_danger)
            if (area.second < *blacklist_slightdanger_limit)
                slight_danger_drawlist_dormant.push_back(area.first->m_center);
    }
#endif
}

// Check if an area is valid for stay near. the Third parameter is to save some performance.
bool isAreaValidForStayNear(Vector ent_origin, CNavArea *area, bool fix_local_z = true, bool vischeck = true)
{
    if (fix_local_z)
        ent_origin.z += navparser::PLAYER_JUMP_HEIGHT;
    auto area_origin = area->m_center;
    area_origin.z += navparser::PLAYER_JUMP_HEIGHT;

    // Do all the distance checks
    float distance = ent_origin.DistToSqr(area_origin);

    // Too close
    if (distance < selected_config.min_full_danger * selected_config.min_full_danger)
        return false;
    // Blacklisted
    if (navparser::NavEngine::getFreeBlacklist()->find(area) != navparser::NavEngine::getFreeBlacklist()->end())
        return false;
    // Too far away
    if (distance > selected_config.max * selected_config.max)
        return false;
    // Attempt to vischeck
    if (vischeck && !IsVectorVisibleNavigation(ent_origin, area_origin))
        return false;
    return true;
}

// Actual logic, used to de-duplicate code
bool stayNearTarget(CachedEntity *ent)
{
    auto ent_origin = ent->m_vecDormantOrigin();
    // No origin recorded, don't bother
    if (!ent_origin)
        return false;

    // Add the vischeck height
    ent_origin->z += navparser::PLAYER_JUMP_HEIGHT;

    // Use std::pair to avoid using the distance functions more than once
    std::vector<std::pair<CNavArea *, float>> good_areas{};

    for (auto &area : navparser::NavEngine::getNavFile()->m_areas)
    {
        auto area_origin = area.m_center;

        if (!isAreaValidForStayNear(*ent_origin, &area, false, false))
            continue;

        float distance = (*ent_origin).DistToSqr(area_origin);
        // Good area found
        good_areas.push_back(std::pair<CNavArea *, float>(&area, distance));
    }
    // Sort based on distance
    if (selected_config.prefer_far)
        std::sort(good_areas.begin(), good_areas.end(), [](std::pair<CNavArea *, float> a, std::pair<CNavArea *, float> b) { return a.second > b.second; });
    else
        std::sort(good_areas.begin(), good_areas.end(), [](std::pair<CNavArea *, float> a, std::pair<CNavArea *, float> b) { return a.second < b.second; });

    for (auto &area : good_areas)
    {
        if (!isAreaValidForStayNear(*ent_origin, area.first, false, true))
            continue;
        if (navparser::NavEngine::navTo(area.first->m_center, staynear, true, !navparser::NavEngine::isPathing()))
            return true;
    }
    return false;
}

// A bunch of basic checks to ensure we don't try to target an invalid entity
bool isStayNearTargetValid(CachedEntity *ent)
{
    return CE_VALID(ent) && g_pPlayerResource->isAlive(ent->m_IDX) && ent->m_IDX != g_pLocalPlayer->entity_idx && g_pLocalPlayer->team != ent->m_iTeam() && player_tools::shouldTarget(ent) && !IsPlayerInvulnerable(ent);
}

// Recursive function to find hiding spot
std::optional<std::pair<CNavArea *, int>> findClosestHidingSpot(CNavArea *area, Vector vischeck_point, int recursion_count, int index = 0)
{
    static std::vector<CNavArea *> already_recursed;
    if (index == 0)
        already_recursed.clear();
    Vector area_origin = area->m_center;
    area_origin.z += navparser::PLAYER_JUMP_HEIGHT;

    // Increment recursion index
    index++;

    // If the area works, return it
    if (!IsVectorVisibleNavigation(area_origin, vischeck_point))
        return std::pair<CNavArea *, int>{ area, index - 1 };

    // Termination condition not hit yet
    else if (index != recursion_count)
    {
        // Store the nearest area
        std::optional<std::pair<CNavArea *, int>> best_area = std::nullopt;

        for (auto &connection : area->m_connections)
        {
            if (std::find(already_recursed.begin(), already_recursed.end(), connection.area) != already_recursed.end())
                continue;
            already_recursed.push_back(connection.area);
            auto area = findClosestHidingSpot(connection.area, vischeck_point, recursion_count, index);
            if (area && (!best_area || area->second < best_area->second))
                best_area = { area->first, area->second };
        }
        return best_area;
    }
    else
        return std::nullopt;
}

// Try to avoid enemy sightlines and reload in peace
bool runReload()
{
    PROF_SECTION(runReload)
    static Timer reloadrun_cooldown{};

    // Not reloading, do not run
    if (!(CE_GOOD(LOCAL_E) && !HasCondition<TFCond_HalloweenGhostMode>(LOCAL_E) && CE_GOOD(LOCAL_W) && re::C_BaseCombatWeapon::GetSlot(RAW_ENT(LOCAL_W)) + 1 != melee && !CanShoot()))
        return false;

    if (!stay_near)
        return false;

    // Re-calc only every once in a while
    if (!reloadrun_cooldown.test_and_set(1000))
        return navparser::NavEngine::current_priority == run_reload;

    // Too high priority, so don't try
    if (navparser::NavEngine::current_priority > run_reload)
        return false;

    // Get our area and start recursing the neighbours
    CNavArea *local_area = navparser::NavEngine::findClosestNavSquare(g_pLocalPlayer->v_Origin);
    if (!local_area)
        return false;

    // Get closest enemy to vicheck
    CachedEntity *closest_visible_enemy = nullptr;
    float best_distance                 = FLT_MAX;
    for (auto const &ent : entity_cache::valid_ents)
    {
        if (!ent->m_bAlivePlayer() || !ent->m_bEnemy())
            continue;
        if (ent->m_flDistance() > best_distance)
            continue;
        if (!ent->IsVisible())
            continue;
        if (!player_tools::shouldTarget(ent))
            continue;

        best_distance         = ent->m_flDistance();
        closest_visible_enemy = ent;
    }

    if (!closest_visible_enemy)
        return false;

    Vector vischeck_point = closest_visible_enemy->m_vecOrigin();
    vischeck_point.z += navparser::PLAYER_JUMP_HEIGHT;

    // Get the best non visible area
    auto best_area = findClosestHidingSpot(local_area, vischeck_point, 5);
    if (!best_area)
        return false;

    // If we can, path
    if (navparser::NavEngine::navTo((*best_area).first->m_center, run_reload, true, false, false))
        return true;
    else
        return false;
}

// Try to stay near enemies and stalk them (or in case of sniper, try to stay far from them
// and snipe them)
bool stayNear()
{
    PROF_SECTION(stayNear)
    static Timer staynear_cooldown{};
    static CachedEntity *previous_target = nullptr;

    // Stay near is expensive so we have to cache. We achieve this by only checking a pre-determined amount of players every
    // CreateMove
    constexpr int MAX_STAYNEAR_CHECKS_RANGE = 3;
    constexpr int MAX_STAYNEAR_CHECKS_CLOSE = 2;
    static int lowest_check_index           = 0;

    // Stay near is off
    if (!stay_near)
        return false;
    // Don't constantly path, it's slow.
    // Far range classes do not need to repath nearly as often as close range ones.
    if (!staynear_cooldown.test_and_set(selected_config.prefer_far ? 2000 : 500))
        return navparser::NavEngine::current_priority == staynear;

    // Too high priority, so don't try
    if (navparser::NavEngine::current_priority > staynear)
        return false;

    // Check and use our previous target if available
    if (isStayNearTargetValid(previous_target))
    {
        auto ent_origin = previous_target->m_vecDormantOrigin();
        if (ent_origin)
        {
            // Check if current target area is valid
            if (navparser::NavEngine::isPathing())
            {
                auto crumbs = navparser::NavEngine::getCrumbs();
                // We cannot just use the last crumb, as it is always nullptr
                if (crumbs->size() > 1)
                {
                    auto last_crumb = (*crumbs)[crumbs->size() - 2];
                    // Area is still valid, stay on it
                    if (isAreaValidForStayNear(*ent_origin, last_crumb.navarea))
                        return true;
                }
            }
            // Else Check our origin for validity (Only for ranged classes)
            else if (selected_config.prefer_far && isAreaValidForStayNear(*ent_origin, navparser::NavEngine::findClosestNavSquare(LOCAL_E->m_vecOrigin())))
                return true;
        }
        // Else we try to path again
        if (stayNearTarget(previous_target))
            return true;
        // Failed, invalidate previous target and try others
        previous_target = nullptr;
    }

    auto advance_count = selected_config.prefer_far ? MAX_STAYNEAR_CHECKS_RANGE : MAX_STAYNEAR_CHECKS_CLOSE;

    // Ensure it is in bounds and also wrap around
    if (lowest_check_index > g_IEngine->GetMaxClients())
        lowest_check_index = 0;

    int calls = 0;
    // Test all entities
    for (int i = lowest_check_index; i <= g_IEngine->GetMaxClients(); ++i)
    {
        CachedEntity* ent = ENTITY(i);
        if (calls >= advance_count)
            break;
        calls++;
        lowest_check_index++;
        
        if (!isStayNearTargetValid(ent))
        {
            calls--;
            continue;
        }
        // Succeeded pathing
        if (stayNearTarget(ent))
        {
            previous_target = ent;
            return true;
        }
    }
    // Stay near failed to find any good targets, add extra delay
    staynear_cooldown.last += std::chrono::seconds(3);
    return false;
}

// Try to attack people using melee if we are in a situation where this is viable
bool meleeAttack(int slot, std::pair<CachedEntity *, float> &nearest)
{
    // There is no point in engaging the melee AI if we are not using melee
    if (slot != melee || !nearest.first)
    {
        isVisible = false;
        if (navparser::NavEngine::current_priority == prio_melee)
            navparser::NavEngine::cancelPath();
        return false;
    }

    if (IsPlayerInvulnerable(nearest.first))
    {
        isVisible = false;
        if (navparser::NavEngine::current_priority == prio_melee)
            navparser::NavEngine::cancelPath();
        return false;
    }

    // Too high priority, so don't try
    if (navparser::NavEngine::current_priority > prio_melee)
        return false;

    auto raw_local = RAW_ENT(LOCAL_E);

    // We are charging, let the charge aimbot do it's job
    if (HasCondition<TFCond_Charging>(LOCAL_E))
    {
        navparser::NavEngine::cancelPath();
        return true;
    }

    static Timer melee_cooldown{};

    {
        Ray_t ray;
        trace_t trace;
        trace::filter_default.SetSelf(raw_local);

        auto hb = nearest.first->hitboxes.GetHitbox(spine_3);
        if (hb)
        {
            ray.Init(g_pLocalPlayer->v_Origin + Vector{ 0, 0, 20 }, hb->center, EntOBBMins(raw_local), EntOBBMaxs(raw_local));
            g_ITrace->TraceRay(ray, MASK_PLAYERSOLID, &trace::filter_default, &trace);
            isVisible = (IClientEntity *) trace.m_pEnt == RAW_ENT(nearest.first);
        }
        else
            isVisible = false;
    }

    // Charge aimbot things
    if (hacks::tf2::misc_aimbot::ShouldChargeAim() && re::C_BasePlayer::GetEquippedDemoShield(raw_local) && re::CTFPlayerShared::GetChargeMeter(re::CTFPlayerShared::GetPlayerShared(raw_local)) == 100.0f)
    {
        // Distance normally covered per second by charge
        float distance_per_second = 750.0f;
        // Apply modifiers to movespeed
        distance_per_second = ATTRIB_HOOK_FLOAT(distance_per_second, "mult_player_movespeed_shieldrequired", raw_local, 0x0, true);
        distance_per_second = ATTRIB_HOOK_FLOAT(distance_per_second, "mult_player_movespeed", raw_local, 0x0, true);
        // Max is still 750.0f
        distance_per_second = std::min(distance_per_second, 750.0f);
        // Time spent charging
        float seconds = 1.5f;
        // Apply modifiers that change charge length
        seconds = ATTRIB_HOOK_FLOAT(seconds, "mod_charge_time", RAW_ENT(LOCAL_E), 0x0, true);
        // Total distance covered by charge
        float total_distance = seconds * distance_per_second;
        if (nearest.second < total_distance && isVisible)
        {
            // Charge
            current_user_cmd->buttons |= IN_ATTACK2;
            AimAt(g_pLocalPlayer->v_Eye, nearest.first->m_vecOrigin(), current_user_cmd);
            navparser::NavEngine::cancelPath();
            return true;
        }
    }
    auto target_origin = nearest.first->m_vecDormantOrigin();
    if (!target_origin)
    {
        isVisible = false;
        if (navparser::NavEngine::current_priority == prio_melee)
            navparser::NavEngine::cancelPath();
        return false;
    }

    // If we are close enough, don't even bother with using the navparser to get there
    if (nearest.second < 400.0f && isVisible)
    {
        auto hb_head = nearest.first->hitboxes.GetHitbox(head);
        if (hb_head)
            AimAt(g_pLocalPlayer->v_Eye, hb_head->center, current_user_cmd);
        WalkTo(*target_origin);
        navparser::NavEngine::cancelPath();
        return true;
    }
    else
    {
        // Don't constantly path, it's slow.
        // The closer we are, the more we should try to path
        if (!melee_cooldown.test_and_set(nearest.second < 400.0f ? 200 : nearest.second < 1000 ? 500 : 2000) && navparser::NavEngine::isPathing())
            return navparser::NavEngine::current_priority == prio_melee;

        // Just walk at the enemy l0l
        if (navparser::NavEngine::navTo(*target_origin, prio_melee, true, !navparser::NavEngine::isPathing()))
            return true;
        return false;
    }
}

// Basically the same as isAreaValidForStayNear, but some restrictions lifted.
bool isAreaValidForSnipe(Vector ent_origin, Vector area_origin, bool fix_sentry_z = true, bool vischeck = true)
{
    if (fix_sentry_z)
        ent_origin.z += 40.0f;
    area_origin.z += navparser::PLAYER_JUMP_HEIGHT;

    float distance = ent_origin.DistToSqr(area_origin);
    // Too close to be valid
    if (distance <= (1100.0f + navparser::HALF_PLAYER_WIDTH) * (1100.0f + navparser::HALF_PLAYER_WIDTH))
        return false;
    // Fails vischeck, bad
    if (vischeck && !IsVectorVisibleNavigation(area_origin, ent_origin))
        return false;
    return true;
}

// Try to snipe the sentry
bool tryToSnipe(CachedEntity *ent)
{
    auto ent_origin = GetBuildingPosition(ent);
    // Add some z to dormant sentries as it only returns origin
    if (CE_BAD(ent))
        ent_origin.z += 40.0f;

    std::vector<std::pair<CNavArea *, float>> good_areas;
    for (auto &area : navparser::NavEngine::getNavFile()->m_areas)
    {
        if (!isAreaValidForSnipe(ent_origin, area.m_center, false, false))
            continue;
        good_areas.push_back(std::pair<CNavArea *, float>(&area, area.m_center.DistToSqr(ent_origin)));
    }

    // Sort based on distance
    if (selected_config.prefer_far)
        std::sort(good_areas.begin(), good_areas.end(), [](std::pair<CNavArea *, float> a, std::pair<CNavArea *, float> b) { return a.second > b.second; });
    else
        std::sort(good_areas.begin(), good_areas.end(), [](std::pair<CNavArea *, float> a, std::pair<CNavArea *, float> b) { return a.second < b.second; });

    int vischecks = 0;
    for (auto &area : good_areas)
    {
        if (vischecks >= 24)
            break;
        vischecks++;
        if (!isAreaValidForSnipe(ent_origin, area.first->m_center, false, true))
            continue;
        if (navparser::NavEngine::navTo(area.first->m_center, snipe_sentry))
            return true;
    }
    return false;
}

// Is our target valid?
bool isSnipeTargetValid(CachedEntity *ent)
{
    return CE_VALID(ent) && ent->m_bAlivePlayer() && ent->m_iTeam() != g_pLocalPlayer->team && ent->m_iClassID() == CL_CLASS(CObjectSentrygun);
}

// Try to Snipe sentries
bool snipeSentries()
{
    static Timer sentry_snipe_cooldown;
    static CachedEntity *previous_target = nullptr;

    if (!snipe_sentries)
        return false;

    // Sentries don't move often, so we can use a slightly longer timer
    if (!sentry_snipe_cooldown.test_and_set(2000))
        return navparser::NavEngine::current_priority == snipe_sentry || isSnipeTargetValid(previous_target);

    if (isSnipeTargetValid(previous_target))
    {
        auto crumbs = navparser::NavEngine::getCrumbs();
        // We cannot just use the last crumb, as it is always nullptr
        if (crumbs->size() > 1)
        {
            auto last_crumb = (*crumbs)[crumbs->size() - 2];
            // Area is still valid, stay on it
            if (isAreaValidForSnipe(GetBuildingPosition(previous_target), last_crumb.navarea->m_center))
                return true;
        }
        if (tryToSnipe(previous_target))
            return true;
    }

    // Make sure we don't try to do it on shortrange classes unless specified
    if (!snipe_sentries_shortrange && (g_pLocalPlayer->clazz == tf_scout || g_pLocalPlayer->clazz == tf_pyro))
        return false;

    for (auto const &ent : entity_cache::valid_ents)
    {
        // Invalid sentry
        if (!isSnipeTargetValid(ent))
            continue;
        // Succeeded in trying to snipe it
        if (tryToSnipe(ent))
        {
            previous_target = ent;
            return true;
        }
    }
    return false;
}

enum building
{
    dispenser = 0,
    sentry    = 2
};

static int build_attempts = 0;
static bool buildBuilding(int building)
{
    // Blacklist this spot and refresh the building spots
    if (build_attempts >= 15)
    {
        (*navparser::NavEngine::getFreeBlacklist())[navparser::NavEngine::findClosestNavSquare(g_pLocalPlayer->v_Origin)] = navparser::BlacklistReason_enum::BAD_BUILDING_SPOT;
        refreshBuildingSpots(true);
        current_building_spot.Invalidate();
        build_attempts = 0;
        return false;
    }
    // Make sure we have right amount of ammo
    int required = (HasGunslinger(LOCAL_E) || building == dispenser) ? 100 : 130;
    if (CE_INT(LOCAL_E, netvar.m_iAmmo + 12) < required)
        return getAmmo(true);

    // Try to build! we are close enough
    if (current_building_spot.IsValid() && current_building_spot.DistTo(g_pLocalPlayer->v_Origin) <= (building == dispenser ? 500.0f : 200.0f))
    {
        // TODO: Rotate our angle to a valid building spot ? also rotate building itself to face enemies ?
        current_user_cmd->viewangles.x = 20.0f;
        current_user_cmd->viewangles.y += 2.0f;

        // Gives us 4 1/2 seconds to build
        static Timer attempt_timer;
        if (attempt_timer.test_and_set(300))
            build_attempts++;

        if (hacks::shared::misc::getCarriedBuilding() == -1)
        {
            static Timer command_timer;
            if (command_timer.test_and_set(100))
                g_IEngine->ClientCmd_Unrestricted(strfmt("build %d", building).get());
        }
        else if (CE_INT(ENTITY(hacks::shared::misc::getCarriedBuilding()), netvar.m_bCanPlace))
            current_user_cmd->buttons |= IN_ATTACK;
        return true;
    }
    else
        return navToSentrySpot();

    return false;
}

static bool buildingNeedsToBeSmacked(CachedEntity *ent)
{
    if (CE_BAD(ent))
        return false;

    if (CE_INT(ent, netvar.iUpgradeLevel) != 3 || ent->m_iHealth() / ent->m_iMaxHealth() <= 0.80f)
        return true;
    if (ent->m_iClassID() == CL_CLASS(CObjectSentrygun))
    {
        int max_ammo = 0;
        switch (CE_INT(ent, netvar.iUpgradeLevel))
        {
        case 1:
            max_ammo = 150;
            break;
        case 2:
        case 3:
            max_ammo = 200;
            break;
        }

        return CE_INT(ent, netvar.m_iAmmoShells) / max_ammo <= 0.50f;
    }
    return false;
}

static bool smackBuilding(CachedEntity *ent)
{
    if (CE_BAD(ent))
        return false;
    if (!CE_INT(LOCAL_E, netvar.m_iAmmo + 12))
        return getAmmo(true);

    if (ent->m_flDistance() <= 100.0f && g_pLocalPlayer->weapon_mode == weapon_melee)
    {
        AimAt(g_pLocalPlayer->v_Eye, GetBuildingPosition(ent), current_user_cmd);
        current_user_cmd->buttons |= IN_ATTACK;
    }
    else if (navparser::NavEngine::current_priority != engineer)
        return navparser::NavEngine::navTo(*ent->m_vecDormantOrigin(), engineer);
    return true;
}

static bool runEngineerLogic()
{
    if (!isEngieMode())
        return false;

    // Already have a sentry
    if (CE_VALID(mySentry) && mySentry->m_bAlivePlayer())
    {
        if (HasGunslinger(LOCAL_E))
        {
            // Too far away, destroy it
            // BUG Ahead, building isnt destroyed lol
            if (mySentry->m_flDistance() >= 1800.0f)
            {
                // If we have a valid building
                if (mySentry->m_Type() == CL_CLASS(CObjectSentrygun))
                    g_IEngine->ClientCmd_Unrestricted("destroy 2");
            }
            // Return false so we run another task
            return false;
        }
        else
        {
            // Try to smack sentry first
            if (buildingNeedsToBeSmacked(mySentry))
                return smackBuilding(mySentry);
            else
            {
                // We put dispenser by sentry
                if (CE_BAD(myDispenser))
                    return buildBuilding(dispenser);
                else
                {
                    // We already have a dispenser, see if it needs to be smacked
                    if (buildingNeedsToBeSmacked(myDispenser))
                        return smackBuilding(myDispenser);
                }
            }
        }
    }
    else
        // Try to build a sentry
        return buildBuilding(sentry);
    return false;
}

enum capture_type
{
    no_capture,
    ctf,
    payload,
    controlpoints
};

static capture_type current_capturetype = no_capture;
// Overwrite to return true for payload carts as an example
static bool overwrite_capture = false;

std::optional<Vector> getCtfGoal(int our_team, int enemy_team)
{
    // Get Flag related information
    auto status   = flagcontroller::getStatus(enemy_team);
    auto position = flagcontroller::getPosition(enemy_team);
    auto carrier  = flagcontroller::getCarrier(enemy_team);

    // No flag :(
    if (!position)
        return std::nullopt;

    current_capturetype = ctf;

    if (status == TF_FLAGINFO_STOLEN && carrier != LOCAL_E && carrier && CE_VALID(carrier))
    {
        auto carrier_origin = carrier->m_vecDormantOrigin();
        if (carrier->player_info && carrier_origin && !player_tools::shouldTargetSteamId(carrier->player_info->friendsID))
            return carrier_origin;
    }

    // Flag is taken by us
    if (status == TF_FLAGINFO_STOLEN)
    {
        // CTF is the current capture type.
        if (carrier == LOCAL_E)
        {
            // Return our capture point location
            auto team_flag = flagcontroller::getFlag(our_team);
            return team_flag.spawn_pos;
        }
    }
    // Get the flag if not taken by us already
    else
    {
        return position;
    }
    return std::nullopt;
}

std::optional<Vector> getPayloadGoal(int our_team)
{
    auto position = plcontroller::getClosestPayload(g_pLocalPlayer->v_Origin, our_team);
    // No payloads found :(
    if (!position)
        return std::nullopt;
    current_capturetype = payload;

    // Adjust position so it's not floating high up, provided the local player is close.
    if (LOCAL_E->m_vecOrigin().DistTo(*position) <= 150.0f)
        (*position).z = LOCAL_E->m_vecOrigin().z;
    // If close enough, don't move (mostly due to lifts)
    if ((*position).DistTo(LOCAL_E->m_vecOrigin()) <= 50.0f)
    {
        overwrite_capture = true;
        return std::nullopt;
    }
    else
        return position;
}

std::optional<Vector> getControlPointGoal(int our_team)
{
    static Vector previous_position(0.0f);
    static Vector randomized_position(0.0f);

    auto position = cpcontroller::getClosestControlPoint(g_pLocalPlayer->v_Origin, our_team);
    // No points found :(
    if (!position)
        return std::nullopt;

    current_capturetype = controlpoints;

    if (position->DistTo(LOCAL_E->m_vecOrigin()) <= 50.0f && !*randomize_cpspot)
    {
        overwrite_capture = true;
        return std::nullopt;
    }

    if (randomize_cpspot)
    {
        if (previous_position != *position || !navparser::NavEngine::isPathing())
        {
            previous_position   = *position;
            randomized_position = *position;
            randomized_position.x += RandomFloat(0.0f, 100.0f);
            randomized_position.y += RandomFloat(0.0f, 100.0f);
        }
        return randomized_position;
    }
    return position;
}

// Try to capture objectives
bool captureObjectives()
{
    static Timer capture_timer;
    static Vector previous_target(0.0f);
    if (!capture_objectives || !g_pGameRules->PointsMayBeCaptured() || g_pGameRules->RoundHasBeenWon() || g_pGameRules->IsPlayingSpecialDeliveryMode() || !capture_timer.check(2000))
        return false;

    // Priority too high, don't try
    if (navparser::NavEngine::current_priority > capture)
        return false;

    // Where we want to go
    std::optional<Vector> target;

    int our_team   = g_pLocalPlayer->team;
    int enemy_team = our_team == TEAM_BLU ? TEAM_RED : TEAM_BLU;

    current_capturetype = no_capture;
    overwrite_capture   = false;

    // Run ctf logic
    target = getCtfGoal(our_team, enemy_team);
    // Not ctf, run payload
    if (current_capturetype == no_capture)
    {
        target = getPayloadGoal(our_team);
        // Not payload, run control points
        if (current_capturetype == no_capture)
        {
            target = getControlPointGoal(our_team);
        }
    }

    // Overwritten, for example because we are currently on the payload, cancel any sort of pathing and return true
    if (overwrite_capture)
    {
        navparser::NavEngine::cancelPath();
        return true;
    }
    // No target, bail and set on cooldown
    else if (!target)
    {
        capture_timer.update();
        return false;
    }
    // If priority is not capturing or we have a new target, try to path there
    else if (navparser::NavEngine::current_priority != capture || *target != previous_target)
    {
        if (navparser::NavEngine::navTo(*target, capture, true, !navparser::NavEngine::isPathing()))
        {
            previous_target = *target;
            return true;
        }
        else
            capture_timer.update();
    }
    return false;
}

// Roam around map
bool doRoam()
{
    static Timer roam_timer;
    // Don't path constantly
    if (!roam_timer.test_and_set(2000))
        return navparser::NavEngine::current_priority == patrol && navparser::NavEngine::isPathing();

    if (defend_while_patrolling)
    {
        int enemy_team = g_pLocalPlayer->team == TEAM_BLU ? TEAM_RED : TEAM_BLU;

        std::optional<Vector> target;
        target = getPayloadGoal(enemy_team);
        if (!target)
            target = getControlPointGoal(enemy_team);
        if (target)
        {
            if ((*target).DistTo(g_pLocalPlayer->v_Origin) <= 250.0f)
            {
                navparser::NavEngine::cancelPath();
                return true;
            }
            if (navparser::NavEngine::navTo(*target, patrol, true, navparser::NavEngine::current_priority != patrol))
                return true;
        }
    }

    // No sniper spots :shrug:
    if (sniper_spots.empty())
        return false;
    // Don't overwrite current roam
    if (navparser::NavEngine::current_priority == patrol)
        return navparser::NavEngine::isPathing();

    static std::vector<size_t> visited_areas;
    static Vector failed_spot{};
    static Timer failed_spot_timer{};
    static float ground_ceiling     = FLT_MAX;
    static std::string ceiling_map;
    const Vector &origin = g_pLocalPlayer->v_Origin;
    auto &areas          = navparser::NavEngine::getNavFile()->m_areas;
    if (areas.empty())
        return false;

    {
        std::string level = GetLevelName();
        if (level != ceiling_map)
        {
            std::vector<float> heights;
            heights.reserve(areas.size());
            for (auto &area : areas)
                heights.emplace_back(area.m_center.z);
            std::nth_element(heights.begin(), heights.begin() + heights.size() / 2, heights.end());
            ground_ceiling = heights[heights.size() / 2] + 96.0f;
            ceiling_map    = std::move(level);
            visited_areas.clear();
            failed_spot_timer.last -= std::chrono::seconds(60);
        }
    }

    auto build_candidates = [&](bool enforce_ceiling)
    {
        std::vector<size_t> out;
        out.reserve(areas.size());
        for (size_t i = 0; i < areas.size(); ++i)
        {
            const Vector &center = areas[i].m_center;
            if (std::find(visited_areas.begin(), visited_areas.end(), i) != visited_areas.end())
                continue;
            if (enforce_ceiling && center.z > ground_ceiling)
                continue;
            float dz = center.z - origin.z;
            if (dz > 64.0f || dz < -1024.0f)
                continue;
            if (center.DistToSqr(origin) < 200.0f * 200.0f)
                continue;
            if (!failed_spot_timer.check(30000) && center.DistTo(failed_spot) < 100.0f)
                continue;
            out.emplace_back(i);
        }
        return out;
    };

    auto candidates = build_candidates(true);
    if (candidates.empty())
    {
        visited_areas.clear();
        candidates = build_candidates(true);
        if (candidates.empty())
        {
            candidates = build_candidates(false);
            if (candidates.empty())
                return false;
        }
    }
    // Max 10 attempts
    for (int attempts = 0; attempts < 10 && !candidates.empty(); ++attempts)
    {
        auto random = select_randomly(candidates.begin(), candidates.end());
        size_t idx  = *random;
        candidates.erase(random);
        if (navparser::NavEngine::navTo(areas[idx].m_center, patrol))
        {
            visited_areas.emplace_back(idx);
            if (visited_areas.size() > 16)
                visited_areas.erase(visited_areas.begin());
            return true;
        }
        failed_spot = areas[idx].m_center;
        failed_spot_timer.update();
    }

    return false;
}

// Run away from dangerous areas
bool escapeDanger()
{
    if (!escape_danger)
        return false;
    // Don't escape while we have the intel
    if (!escape_danger_ctf_cap)
    {
        auto flag_carrier = flagcontroller::getCarrier(g_pLocalPlayer->team);
        if (flag_carrier == LOCAL_E)
            return false;
    }
    // Priority too high
    if (navparser::NavEngine::current_priority > danger)
        return false;

    auto *local_nav = navparser::NavEngine::findClosestNavSquare(g_pLocalPlayer->v_Origin);
    auto blacklist  = navparser::NavEngine::getFreeBlacklist();

    // In danger, try to run (besides if it's a building spot, don't run away from that)
    if (blacklist->find(local_nav) != blacklist->end())
    {
        if ((*blacklist)[local_nav].value == navparser::BlacklistReason_enum::BAD_BUILDING_SPOT)
            return false;

        static CNavArea *target_area = nullptr;
        // Already running and our target is still valid
        if (navparser::NavEngine::current_priority == danger && blacklist->find(target_area) == blacklist->end())
            return true;

        std::vector<CNavArea *> nav_areas_ptr;
        // Copy a ptr list (sadly cat_nav_init exists so this cannot be only done once)
        for (auto &nav_area : navparser::NavEngine::getNavFile()->m_areas)
            nav_areas_ptr.push_back(&nav_area);

        // Sort by distance
        std::sort(nav_areas_ptr.begin(), nav_areas_ptr.end(), [](CNavArea *a, CNavArea *b) { return a->m_center.DistToSqr(g_pLocalPlayer->v_Origin) < b->m_center.DistToSqr(g_pLocalPlayer->v_Origin); });

        int calls = 0;
        // Try to path away
        for (auto area : nav_areas_ptr)
        {
            if (blacklist->find(area) == blacklist->end())
            {
                // only try the 5 closest valid areas though, something is wrong if this fails
                calls++;
                if (calls > 5)
                    break;
                if (navparser::NavEngine::navTo(area->m_center, danger))
                {
                    target_area = area;
                    return true;
                }
            }
        }
    }
    // No longer in danger
    else if (navparser::NavEngine::current_priority == danger)
        navparser::NavEngine::cancelPath();
    return false;
}

static int slot = primary;
static int wanted_slot = primary;

static void autoJump(std::pair<CachedEntity *, float> &nearest)
{
    if (!autojump)
        return;
    static Timer last_jump{};
    if (!last_jump.test_and_set(200) || CE_BAD(nearest.first))
        return;

    if (nearest.second <= *jump_distance)
        current_user_cmd->buttons |= IN_JUMP | IN_DUCK;
}

static bool CheckMelee(CachedEntity *ent)
{
    if (!ent || !ent->m_bAlivePlayer() || IsPlayerInvulnerable(ent))
        return false;
    return true;
}

static slots getBestSlot(slots active_slot, std::pair<CachedEntity *, float> &nearest)
{
    if (melee_mode)
        return melee;
    if (force_slot)
        return (slots) *force_slot;
    switch (g_pLocalPlayer->clazz)
    {
    case tf_scout:
    {
        if (nearest.second > 450.0f && active_slot == secondary)
            return active_slot;
        if (nearest.second <= 300.0f && CheckMelee(nearest.first) && nearest.first->IsVisible())
            return melee;
        else if (nearest.second <= 550.0f)
            return primary;
        else
            return secondary;
    }
    case tf_heavy:
        return primary;
    case tf_medic:
        return secondary;
    case tf_spy:
    {
        if (nearest.second > 200 && active_slot == primary)
            return active_slot;
        else if (nearest.second >= 250)
            return primary;
        else
            return melee;
    }
    case tf_sniper:
    {
        // Have a Huntsman, Always use primary
        if (HasWeapon(LOCAL_E, 56) || HasWeapon(LOCAL_E, 1005) || HasWeapon(LOCAL_E, 1092))
            return primary;

        if (nearest.second <= 350.0f && CheckMelee(nearest.first) && nearest.first->IsVisible())
            return melee;
        else if (nearest.second <= 300 && nearest.first->m_iHealth() < 75)
            return secondary;
        else if (nearest.second <= 400 && nearest.first->m_iHealth() < 75)
            return active_slot;
        else
            return primary;
    }
    case tf_pyro:
    {
        if (nearest.second > 450 && active_slot == secondary)
            return active_slot;
        if (nearest.second <= 300.0f && CheckMelee(nearest.first) && nearest.first->IsVisible())
            return melee;
        else if (nearest.second <= 550)
            return primary;
        else
            return secondary;
    }
    case tf_soldier:
    {
        if (nearest.second <= 300.0f && CheckMelee(nearest.first) && nearest.first->IsVisible())
            return melee;
        else if (nearest.second <= 200)
            return secondary;
        else if (nearest.second <= 300)
            return active_slot;
        else
            return primary;
    }
    case tf_engineer:
    {
        if (((CE_GOOD(mySentry) && mySentry->m_flDistance() <= 300) || (CE_GOOD(myDispenser) && myDispenser->m_flDistance() <= 500)) || (current_building_spot.IsValid() && current_building_spot.DistTo(g_pLocalPlayer->v_Origin) <= 500.0f))
        {
            if (active_slot >= melee && navparser::NavEngine::current_priority != prio_melee)
                return active_slot;
            else
                return melee;
        }
        else if (nearest.second <= 500)
            return primary;
        else
            return secondary;
    }
    default:
    {
        if (nearest.second <= 300.0f && CheckMelee(nearest.first) && nearest.first->IsVisible())
            return melee;
        if (nearest.second <= 400)
            return secondary;
        else if (nearest.second <= 500)
            return active_slot;
        else
            return primary;
    }
    }
}

static void updateSlot(std::pair<CachedEntity *, float> &nearest)
{
    static Timer slot_timer{};
    if (CE_GOOD(LOCAL_E) && !HasCondition<TFCond_HalloweenGhostMode>(LOCAL_E) && CE_GOOD(LOCAL_W) && LOCAL_E->m_bAlivePlayer())
    {
        IClientEntity *weapon = RAW_ENT(LOCAL_W);
        if (re::C_BaseCombatWeapon::IsBaseCombatWeapon(weapon))
        {
            slot = re::C_BaseCombatWeapon::GetSlot(weapon) + 1;
            if (!force_slot && !primary_only && !melee_mode)
            {
                wanted_slot = slot;
                return;
            }
            if (!slot_timer.test_and_set(300))
                return;
            wanted_slot = getBestSlot(static_cast<slots>(slot), nearest);
            if (slot != wanted_slot)
                g_IEngine->ClientCmd_Unrestricted(format("slot", wanted_slot).c_str());
        }
    }
}

static const char *active_task     = "init";
static unsigned long long cm_calls = 0;

#if ENABLE_VISUALS
static void updateDrawSnapshot();
#endif

static void CreateMove()
{
    ++cm_calls;
#if ENABLE_VISUALS
    updateDrawSnapshot();
#endif
    if (!enabled)
    {
        if (navparser::NavEngine::current_priority >= patrol && navparser::NavEngine::current_priority <= danger && navparser::NavEngine::current_priority != followbot)
            navparser::NavEngine::cancelPath();
        isVisible   = false;
        active_task = "disabled";
        return;
    }
    if (!navparser::NavEngine::isReady())
    {
        active_task = "navengine not ready";
        return;
    }
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer() || HasCondition<TFCond_HalloweenGhostMode>(LOCAL_E))
    {
        isVisible   = false;
        active_task = "dead/invalid";
        return;
    }

    refreshSniperSpots();
    refreshLocalBuildings();
    refreshBuildingSpots();

    if (danger_config_custom)
    {
        selected_config = { *danger_config_custom_min_full_danger, *danger_config_custom_min_slight_danger, *danger_config_custom_max_slight_danger, *danger_config_custom_prefer_far };
    }
    else
    {
        // Update the distance config
        switch (g_pLocalPlayer->clazz)
        {
        case tf_scout:
        case tf_heavy:
            selected_config = CONFIG_SHORT_RANGE;
            break;
        case tf_engineer:
            selected_config = isEngieMode() ? HasGunslinger(LOCAL_E) ? CONFIG_GUNSLINGER_ENGINEER : CONFIG_ENGINEER : CONFIG_SHORT_RANGE;
            break;
        case tf_sniper:
            selected_config = g_pLocalPlayer->weapon()->m_iClassID() == CL_CLASS(CTFCompoundBow) ? CONFIG_MID_RANGE : CONFIG_LONG_RANGE;
            break;
        default:
            selected_config = CONFIG_MID_RANGE;
        }
    }

    auto nearest = getNearestPlayerDistance();

    updateSlot(nearest);
    autoJump(nearest);
    updateEnemyBlacklist(wanted_slot);

    if (meleeAttack(wanted_slot, nearest))
        active_task = "melee";
    else if (escapeDanger())
        active_task = "escape-danger";
    // Second priority should be getting health
    else if (getHealth())
        active_task = "health";
    // If we aren't getting health, get ammo
    else if (getAmmo())
        active_task = "ammo";
    // Try to run engineer logic
    else if (runEngineerLogic())
        active_task = "engineer";
    // Try to snipe sentries
    else if (snipeSentries())
        active_task = "snipe-sentries";
    else if (captureObjectives())
        active_task = "capture";
    // Try to hide if reloading
    else if (runReload())
        active_task = "reload";
    // Try to stalk enemies
    else if (stayNear())
        active_task = "stay-near";
    // Try to get health with a lower prioritiy
    else if (getHealth(true))
        active_task = "health-low";
    // We have nothing else to do, roam
    else if (doRoam())
        active_task = "roam";
    else
        active_task = "idle";
}

void LevelInit()
{
    // Make it run asap
    refresh_sniperspots_timer.last -= std::chrono::seconds(60);
    refresh_buildingspots_timer.last -= std::chrono::seconds(60);
    sniper_spots.clear();
    building_spots.clear();
    mySentry    = nullptr;
    myDispenser = nullptr;
    current_building_spot.Invalidate();
    parseCachedSupplyOrigins();
    supply_was_force         = false;
    has_remembered_dispenser = false;
    temp_main.clear();
    temp_dispensers.clear();
}

std::vector<std::string> getDebugInfoLines()
{
    std::vector<std::string> lines;
    lines.push_back(format("nb:", *enabled ? "1" : "0", " cm:", cm_calls, " task:", active_task, " prio:", navparser::getPriorityName(navparser::NavEngine::current_priority)));
    if (CE_GOOD(LOCAL_E))
    {
        auto nearest = getNearestPlayerDistance();
        lines.push_back(format("enemy ", nearest.first ? format("#", nearest.first->m_IDX) : std::string("none"), " d:", (int) nearest.second, " | hp:", LOCAL_E->m_iHealth(), "/", g_pPlayerResource->GetMaxHealth(LOCAL_E), " mtl:", CE_INT(LOCAL_E, netvar.m_iAmmo + 12), " s:", slot, " z:", g_pLocalPlayer->bZoomed ? "1" : "0"));
        lines.push_back(format("srch h:", shouldSearchHealth() ? "y" : "n", "/", shouldSearchHealth(true) ? "y" : "n", " a:", shouldSearchAmmo() ? "y" : "n", " packs h:", cached_health_origins.size(), " a:", cached_ammo_origins.size(), " | cfg ", (int) selected_config.min_full_danger, "/", (int) selected_config.min_slight_danger, "/", (int) selected_config.max, selected_config.prefer_far ? " far" : ""));
    }
    lines.push_back(format("spots sn:", sniper_spots.size(), " b:", building_spots.size(), " cap:", (int) current_capturetype, " ow:", overwrite_capture ? "1" : "0"));
    lines.push_back(format("engie m:", isEngieMode() ? "1" : "0", " s:", CE_GOOD(mySentry) ? "y" : "n", " d:", CE_GOOD(myDispenser) ? "y" : "n", " at:", build_attempts, " (", (int) current_building_spot.x, ",", (int) current_building_spot.y, ",", (int) current_building_spot.z, ")"));
    return lines;
}

#if ENABLE_VISUALS
static void updateDrawSnapshot()
{
    std::lock_guard<std::mutex> lock(navbot_draw_mutex);
    nav_ready_snapshot      = navparser::NavEngine::isReady();
    debug_lines_snapshot    = getDebugInfoLines();
    danger_normal_snapshot  = slight_danger_drawlist_normal;
    danger_dormant_snapshot = slight_danger_drawlist_dormant;
    blacklist_snapshot.clear();
    if (nav_ready_snapshot)
        for (auto &area : *navparser::NavEngine::getFreeBlacklist())
            blacklist_snapshot.push_back(area.first->m_center);
}
#endif

void drawDebugInfo()
{
#if ENABLE_VISUALS
    AddSideString("--- NavBot ---", colors::gui);
    std::lock_guard<std::mutex> lock(navbot_draw_mutex);
    for (auto &line : debug_lines_snapshot)
        AddSideString(line);
#endif
}
#if ENABLE_VISUALS
void Draw()
{
    std::lock_guard<std::mutex> lock(navbot_draw_mutex);
    if (!draw_danger || !nav_ready_snapshot)
        return;
    for (auto &area : danger_normal_snapshot)
    {
        Vector out;
        if (draw::WorldToScreen(area, out))
            draw::Rectangle(out.x - 2.0f, out.y - 2.0f, 4.0f, 4.0f, colors::orange);
    }
    for (auto &area : danger_dormant_snapshot)
    {
        Vector out;
        if (draw::WorldToScreen(area, out))
            draw::Rectangle(out.x - 2.0f, out.y - 2.0f, 4.0f, 4.0f, colors::orange);
    }
    for (auto &center : blacklist_snapshot)
    {
        Vector out;
        if (draw::WorldToScreen(center, out))
            draw::Rectangle(out.x - 2.0f, out.y - 2.0f, 4.0f, 4.0f, colors::red);
    }
}
#endif

static InitRoutine init(
    []()
    {
        EC::Register(EC::CreateMove, CreateMove, "navbot_cm");
        EC::Register(EC::CreateMoveWarp, CreateMove, "navbot_cm");
        EC::Register(EC::LevelInit, LevelInit, "navbot_levelinit");
#if ENABLE_VISUALS
        EC::Register(EC::Draw, Draw, "navbot_draw");
#endif
        LevelInit();
    });

} // namespace hacks::tf2::NavBot
