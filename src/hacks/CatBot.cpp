/*
 * CatBot.cpp
 *
 *  Created on: Dec 30, 2017
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include <unordered_set>
#include "CatBot.hpp"
#include "common.hpp"
#include "hack.hpp"
#include "PlayerTools.hpp"
#include "e8call.hpp"
#include "NavBot.hpp"
#include "navparser.hpp"
#include "SettingCommands.hpp"
#include "glob.h"
#include "DetourHook.hpp"
#include "discord.hpp"

namespace hacks::shared::catbot
{
static settings::Boolean auto_disguise{ "misc.autodisguise", "true" };

static settings::Int abandon_if_ipc_bots_gte{ "cat-bot.abandon-if.ipc-bots-gte", "0" };
static settings::Int abandon_if_humans_lte{ "cat-bot.abandon-if.humans-lte", "0" };
static settings::Int abandon_if_players_lte{ "cat-bot.abandon-if.players-lte", "0" };
static settings::Boolean abandon_if_no_navmesh{ "cat-bot.abandon-if.no-navmesh", "false" };
static settings::Boolean requeue_without_abandon{ "cat-bot.requeue-without-abandon", "false" };

static settings::Boolean micspam{ "cat-bot.micspam.enable", "false" };
static settings::Int micspam_on{ "cat-bot.micspam.interval-on", "3" };
static settings::Int micspam_off{ "cat-bot.micspam.interval-off", "60" };

static settings::Boolean auto_crouch{ "cat-bot.auto-crouch", "false" };
static settings::Boolean always_crouch{ "cat-bot.always-crouch", "false" };
static settings::Boolean random_votekicks{ "cat-bot.votekicks", "false" };
static settings::Boolean votekick_rage_only{ "cat-bot.votekicks.rage-only", "false" };
static settings::Boolean autoReport{ "cat-bot.autoreport", "true" };
static settings::Boolean autovote_map{ "cat-bot.autovote-map", "true" };
static settings::Boolean autoreport_bypass_cooldown{ "cat-bot.autoreport.bypass-cooldown", "false" };

static settings::Boolean autoqueue_report{ "cat-bot.autoqueue-report", "false" };

static settings::Boolean mvm_autoupgrade{ "mvm.autoupgrade", "false" };

settings::Boolean catbotmode{ "cat-bot.enable", "false" };
settings::Boolean anti_motd{ "cat-bot.anti-motd", "false" };

// These are used for randomly loading a config on respawn for the bots

// Master switch
static settings::Boolean enable_reload{ "cat-bot.autoload.enable", "false" };

// Misc Settings
static settings::Float reload_chance{ "cat-bot.autoload.chance", "100" };
static settings::Int reload_deaths{ "cat-bot.autoload.deaths", "0" };
static settings::Boolean load_same_config{ "cat-bot.autoload.load-same-config", "true" };

// Config to load
static settings::String conf1{ "cat-bot.autoload.conf1", "bot_*" };
static settings::String conf2{ "cat-bot.autoload.conf2", "" };
static settings::String conf3{ "cat-bot.autoload.conf3", "" };

// Should that config get loaded?
static settings::Boolean conf1_enable{ "cat-bot.autoload.conf1.enable", "false" };
static settings::Boolean conf2_enable{ "cat-bot.autoload.conf2.enable", "false" };
static settings::Boolean conf3_enable{ "cat-bot.autoload.conf3.enable", "false" };

struct catbot_user_state
{
    int treacherous_kills{ 0 };
};

static boost::unordered_flat_map<unsigned, catbot_user_state> human_detecting_map{};

int globerr(const char *path, int eerrno)
{
    logging::Info("%s: %s\n", path, strerror(eerrno));
    // let glob() keep going
    return 0;
}

bool hasEnding(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length())
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    else
        return false;
}

std::vector<std::string> config_list(std::string in)
{
    std::string complete_in = paths::getConfigPath() + "/" + in;
    if (!hasEnding(complete_in, ".conf"))
        complete_in = complete_in + ".conf";
    std::vector<std::string> config_vec;
    size_t i;
    int flags = 0;
    glob_t results;
    int ret;

    flags |= 0;
    ret = glob(complete_in.c_str(), flags, globerr, &results);
    if (ret != 0)
    {
        std::string ret_str;
        switch (ret)
        {
        case GLOB_ABORTED:
            ret_str = "filesystem problem";
            break;
        case GLOB_NOMATCH:
            ret_str = "no match of pattern";
            break;
        case GLOB_NOSPACE:
            ret_str = "out of memory";
            break;
        default:
            ret_str = "Unknown problem";
            break;
        }

        logging::Info("problem with %s (%s), stopping early\n", in.c_str(), ret_str.c_str());
        return config_vec;
    }

    for (i = 0; i < results.gl_pathc; ++i)
        // /configs/ is 9 extra chars i have to remove
        config_vec.push_back(std::string(results.gl_pathv[i]).substr(paths::getDataPath().length() + 9));

    globfree(&results);
    return config_vec;
}
static std::string blacklist;
static int deaths = 0;
void on_killed_by(int userid)
{
    if (enable_reload)
    {
        // Should we load yet?
        bool should_load = false;

        // Default to chance if no deaths are set
        if (!reload_deaths)
        {
            // RNG
            if (UniformRandomInt(0, 99) < *reload_chance)
                should_load = true;
        }
        // You died more than the specified amount of times
        else if (deaths++ >= *reload_deaths)
        {
            should_load = true;
            deaths      = 0;
        }
        if (should_load)
        {
            // Candidates for loading
            std::vector<std::string> temp_candidates;
            std::vector<std::string> load_candidates;
            if (conf1_enable)
            {
                temp_candidates = config_list(*conf1);
                for (auto &i : temp_candidates)
                    load_candidates.push_back(i);
            }
            if (conf2_enable)
            {
                temp_candidates = config_list(*conf2);
                for (auto &i : temp_candidates)
                    load_candidates.push_back(i);
            }
            if (conf3_enable)
            {
                temp_candidates = config_list(*conf3);
                for (auto &i : temp_candidates)
                    load_candidates.push_back(i);
            }
            // Remove blacklisted
            if (!load_same_config)
                for (auto it = load_candidates.begin(); it != load_candidates.end();)
                {
                    if (*it == blacklist)
                        load_candidates.erase(it);
                    else
                        it++;
                }
            if (!load_candidates.empty())
            {
                // Load the config
                std::string to_load  = load_candidates.at(UniformRandomInt(0, load_candidates.size() - 1));
                to_load              = to_load.substr(0, to_load.size() - 5);
                std::string load_cmd = "cat_load " + to_load;
                g_IEngine->ClientCmd_Unrestricted(load_cmd.c_str());
                if (!load_same_config)
                    blacklist = to_load;
            }
        }
    }
}

void do_random_votekick()
{
    std::vector<int> targets;
    player_info_s local_info;

    if (CE_BAD(LOCAL_E) || !GetPlayerInfo(LOCAL_E->m_IDX, &local_info))
        return;
    ForEachConnectedPlayer(
        [&](int i, unsigned id, const player_info_s &info)
        {
            if (!info.userID)
                return;
            if (g_pPlayerResource && g_pPlayerResource->GetTeam(i) != g_pLocalPlayer->team)
                return;
            if (id == local_info.friendsID)
                return;
            auto &pl = playerlist::AccessData(id);
            if (votekick_rage_only && pl.state != playerlist::k_EState::RAGE)
                return;
            if (pl.state != playerlist::k_EState::RAGE && pl.state != playerlist::k_EState::DEFAULT)
                return;
            targets.push_back(info.userID);
        });

    if (targets.empty())
        return;

    int target = targets[rand() % targets.size()];
    player_info_s info;
    if (!GetPlayerInfo(GetPlayerForUserID(target), &info))
        return;
    hack::ExecuteCommand("callvote kick \"" + std::to_string(target) + " cheating\"");
}

// Get Muh money
int GetMvmCredits()
{
    if (CE_GOOD(LOCAL_E))
        return NET_INT(RAW_ENT(LOCAL_E), netvar.m_nCurrency);
    return 0;
}

static CatCommand debug_money("debug_mvmmoney", "Print MVM Money", []() { logging::Info("%d", GetMvmCredits()); });
// Store information
struct Posinfo
{
    float x;
    float y;
    float z;
    std::string lvlname;
    Posinfo(float _x, float _y, float _z, std::string _lvlname)
    {
        x       = _x;
        y       = _y;
        z       = _z;
        lvlname = _lvlname;
    }
    Posinfo(){};
};
struct Upgradeinfo
{
    int id;
    int cost;
    int clazz;
    // Higher = better
    int priority;
    int priority_falloff;
    Upgradeinfo(){};
    Upgradeinfo(int _id, int _cost, int _clazz, int _priority, int _priority_falloff)
    {
        id               = _id;
        cost             = _cost;
        clazz            = _clazz;
        priority         = _priority;
        priority_falloff = _priority_falloff;
    }
};
static std::vector<Upgradeinfo> upgrade_list;

static bool inited_upgrades = false;
// Pick a upgrade
Upgradeinfo PickUpgrade()
{
    if (!inited_upgrades)
    {
        // Damage ( Important )
        upgrade_list.push_back({ 0, 400, tf_sniper, 4, 1 });
        // Projectile Penetration ( Good )
        upgrade_list.push_back({ 12, 400, tf_sniper, 5, 500 });
        // Explosive Headshot
        upgrade_list.push_back({ 40, 350, tf_sniper, 6, 2 });
        // +50% Ammo ( Basically least valuable upgrade for primary )
        upgrade_list.push_back({ 6, 250, tf_sniper, 3, 1 });
        // +20% Reload Speed ( Pretty important )
        upgrade_list.push_back({ 35, 250, tf_sniper, 6, 1 });
        // +25 Health on kill ( It's meh yet you need it to not die )
        upgrade_list.push_back({ 11, 200, tf_sniper, 3, 2 });
        // +25% Faster charge ( Good for "Wait for charge" catbots )
        upgrade_list.push_back({ 17, 200, tf_sniper, 4, 1 });
        inited_upgrades = true;
    }
    int highest_priority = INT_MIN;
    std::vector<Upgradeinfo *> potential_upgrades;
    for (auto &i : upgrade_list)
    {
        // Don't want wrong class
        if (i.clazz != g_pLocalPlayer->clazz)
            continue;
        // Can't buy something we can't afford lol
        if (i.cost > GetMvmCredits())
            continue;
        // Not a Priority right now
        if (i.priority < highest_priority)
            continue;
        // Clear out everything incase a higher priority is found
        if (i.priority > highest_priority)
            potential_upgrades.clear();
        highest_priority = i.priority;
        potential_upgrades.push_back(&i);
    }
    int vec_size = potential_upgrades.size();
    if (!vec_size)
        return { -1, -1, -1, -1, -1 };
    else
    {
        auto choosen_element = potential_upgrades[rand() % vec_size];
        // Less important after an upgrade
        choosen_element->priority -= choosen_element->priority_falloff;
        return *choosen_element;
    }
}
static std::vector<Posinfo> spot_list;
// Upgrade Navigation
/*void NavUpgrade()
{
    std::string lvlname = g_IEngine->GetLevelName();
    std::vector<Posinfo> potential_spots{};

    for (auto &i : spot_list)
    {
        if (lvlname.find(i.lvlname) != lvlname.npos)
            potential_spots.push_back({ i.x, i.y, i.z, lvlname });
    }
    Posinfo best_spot{};
    float best_score = FLT_MAX;
    for (auto &i : potential_spots)
    {
        Vector pos  = { i.x, i.y, 0.0f };
        float score = pos.DistTo(LOCAL_E->m_vecOrigin());
        if (score < best_score)
        {
            best_spot  = i;
            best_score = score;
        }
    }
    Posinfo to_path                        = best_spot;
    hacks::tf2::NavBot::task::current_task = hacks::tf2::NavBot::task::outofbounds;
    bool success                           = nav::navTo(Vector{ to_path.x, to_path.y, to_path.z }, 8, true, true);
    if (!success)
    {
        logging::Info("No valid spots found!");
        hacks::tf2::NavBot::task::current_task = hacks::tf2::NavBot::task::none;
        return;
    }
}

static bool run = false;
static Timer run_delay;
static Timer buy_upgrade;
static InitRoutine init_routine([]() {
    EC::Register(
        EC::Paint,
        []() {
            if (run && run_delay.test_and_set(200))
            {
                run                = false;
                auto upgrade_panel = g_CHUD->FindElement("CHudUpgradePanel");
                typedef int (*CancelUpgrade_t)(CHudElement *);
                static uintptr_t addr = 0;
                static bool resolved  = false;
                if (!resolved)
                {
                    resolved = true;
                    addr     = gSignatures.GetClientSignature(sigs::hud_upgrade_panel_cancel_upgrades);
                }
                if (upgrade_panel && addr)
                {
                    static CancelUpgrade_t CancelUpgrade_fn = CancelUpgrade_t(addr);
                    CancelUpgrade_fn(upgrade_panel);
                }
            }
        },
        "mvmupgrade_paint");
    EC::Register(
        EC::CreateMove,
        []() {
            if (!*mvm_autoupgrade)
                return;
            std::string lvlname = g_IEngine->GetLevelName();
            if (lvlname.find("mvm_") == lvlname.npos)
                return;
            if (hacks::tf2::NavBot::task::current_task == hacks::tf2::NavBot::task::outofbounds)
            {
                if (nav::ReadyForCommands)
                    hacks::tf2::NavBot::task::current_task = hacks::tf2::NavBot::task::none;
                else
                    return;
            }
            if (GetMvmCredits() <= 250)
                return;
            if (CE_BAD(LOCAL_E))
                return;
            if (!buy_upgrade.check(5000))
                return;
            std::vector<Posinfo> potential_spots{};

            for (auto &i : spot_list)
            {
                if (lvlname.find(i.lvlname) != lvlname.npos)
                    potential_spots.push_back({ i.x, i.y, i.z, lvlname });
            }
            Posinfo best_spot{};
            float best_score = FLT_MAX;
            for (auto &i : potential_spots)
            {
                Vector pos  = { i.x, i.y, 0.0f };
                float score = pos.DistTo(LOCAL_E->m_vecOrigin());
                if (score < best_score)
                {
                    best_spot  = i;
                    best_score = score;
                }
            }
            if (GetMvmCredits() >= 400 || Vector{ best_spot.x, best_spot.y, best_spot.z }.DistTo(LOCAL_E->m_vecOrigin()) <= 500.0f)
            {
                NavUpgrade();
                buy_upgrade.update();
            }
        },
        "mvm_upgrade_createmove");
    EC::Register(
        EC::LevelShutdown, []() { inited_upgrades = false; }, "mvmupgrades_levelshutdown");
    spot_list.push_back(Posinfo(851.0f, -2509.0f, 577.0f, "mvm_coaltown"));
    spot_list.push_back(Posinfo(935.0f, -2626.0f, 577.0f, "mvm_bigrock"));
    spot_list.push_back(Posinfo(-885, -2229, 545, "mvm_decoy"));
    spot_list.push_back(Posinfo(851, -2509, 577, "mvm_ghost_town"));
    spot_list.push_back(Posinfo(-625, 2273, -95, "mvm_mannhatten"));
    spot_list.push_back(Posinfo(517, -2599, 450, "mvm_mannworks"));
    spot_list.push_back(Posinfo(-1346, 625, -102, "mvm_rottenburg"));
});

void MvM_Autoupgrade(KeyValues *event)
{
    if (!isHackActive())
        return;
    if (!*mvm_autoupgrade)
        return;
    std::string name = std::string(event->GetName());
    if (!name.compare("MVM_Upgrade"))
    {
        KeyValues *new_key = new KeyValues("upgrade");
        KeyValues *key     = event->FindKey("upgrade", false);
        if (!key)
        {
            new_key->SetInt("itemslot", 0);
            new_key->SetInt("upgrade", 0);
            new_key->SetInt("count", 1);
            event->AddSubKey(new_key);
            key = event->FindKey("upgrade", false);
        }
        else
            delete new_key;
        if (key)
        {

            auto upgrade = PickUpgrade();
            if (upgrade.id == -1)
                return;
            key->SetInt("itemslot", 0);
            key->SetInt("upgrade", upgrade.id);
            key->SetInt("count", 1);
        }
        else
            logging::Info("Key process failed!");
    }
    if (!name.compare("MvM_UpgradesBegin"))
    {
        logging::Info("Sent Upgrades");
        run = true;
        run_delay.update();
    }
}
*/
void SendNetMsg(INetMessage &msg)
{
    /*
    if (!strcmp(msg.GetName(), "clc_CmdKeyValues"))
    {
        if ((KeyValues *) (((unsigned *) &msg)[4]))
            MvM_Autoupgrade((KeyValues *) (((unsigned *) &msg)[4]));
    }*/
}
/*
class CatBotEventListener : public IGameEventListener2
{
    void FireGameEvent(IGameEvent *event) override
    {

        int killer_id = GetPlayerForUserID(event->GetInt("attacker"));
        int victim_id = GetPlayerForUserID(event->GetInt("userid"));

        if (victim_id == g_IEngine->GetLocalPlayer())
        {
            on_killed_by(killer_id);
            return;
        }
    }
};

CatBotEventListener &listener()
{
    static CatBotEventListener object{};
    return object;
}*/

class CatBotEventListener2 : public IGameEventListener2
{
    void FireGameEvent(IGameEvent *) override
    {
        // vote for current map if catbot mode and autovote is on
        if (catbotmode && autovote_map)
            g_IEngine->ServerCmd("next_map_vote 0");
    }
};

CatBotEventListener2 &listener2()
{
    static CatBotEventListener2 object{};
    return object;
}

Timer timer_votekicks{};
static Timer timer_catbot_list{};
static Timer timer_abandon{};

static int count_ipc = 0;
static std::vector<unsigned> ipc_list{ 0 };

static bool waiting_for_quit_bool{ false };
static Timer waiting_for_quit_timer{};

static std::vector<unsigned> ipc_blacklist{};

static bool requeue_active{ false };

#if ENABLE_IPC
void update_ipc_data(ipc::user_data_s &data)
{
    data.ingame.bot_count = count_ipc;
}
#endif

Timer level_init_timer{};

Timer micspam_on_timer{};
Timer micspam_off_timer{};
static std::atomic_bool can_report = false;
static std::vector<unsigned> to_report;
static std::unordered_set<unsigned> already_reported;

static unsigned local_friendsid()
{
    if (g_ISteamUser)
        return g_ISteamUser->GetSteamID().GetAccountID();
    player_info_s info{};
    if (g_IEngine && GetPlayerInfo(g_IEngine->GetLocalPlayer(), &info))
        return info.friendsID;
    return 0;
}

static bool on_match_team(int idx)
{
    if (!g_pPlayerResource)
        return true;
    const int team = g_pPlayerResource->GetTeam(idx);
    return !team || team == TEAM_RED || team == TEAM_BLU;
}

static int collect_report_targets(std::vector<unsigned> &out)
{
    out.clear();
    const unsigned self = local_friendsid();
    int connected       = 0;
    ForEachConnectedPlayer(
        [&](int i, unsigned id, const player_info_s &)
        {
            if (!on_match_team(i))
                return;
            ++connected;
            if (id == self)
                return;
            if (!player_tools::shouldTargetSteamId(id))
                return;
            out.push_back(id);
        });
    return connected;
}

void reportall()
{
    can_report = false;
    const int connected = collect_report_targets(to_report);
    logging::Info("reportall: %zu targets, %d on teams", to_report.size(), connected);
    can_report = !to_report.empty();
}

static DetourHook report_recent_check_detour;
static char report_recent_check_hook(uint64_t steamid64, char warn)
{
    if (*autoreport_bypass_cooldown)
        return 1;
    using Fn  = char (*)(uint64_t, char);
    auto orig = Fn(report_recent_check_detour.GetOriginalFunc());
    return orig ? orig(steamid64, warn) : 1;
}

static std::string name_for_friendsid(unsigned friendsID)
{
    std::string name;
    ForEachConnectedPlayer(
        [&](int, unsigned id, const player_info_s &info)
        {
            if (name.empty() && id == friendsID && info.name[0])
                name = info.name;
        });
    return name;
}

static void send_report(unsigned friendsID)
{
    typedef uint64_t (*ReportPlayer_t)(uint64_t, int);
    static uintptr_t addr1                = gSignatures.GetClientSignature(sigs::report_player_account);
    static ReportPlayer_t ReportPlayer_fn = ReportPlayer_t(addr1);
    if (!addr1)
        return;
    CSteamID id(friendsID, EUniverse::k_EUniversePublic, EAccountType::k_EAccountTypeIndividual);
    ReportPlayer_fn(id.ConvertToUint64(), 1);
    discord::LogReport(friendsID, name_for_friendsid(friendsID));
}

CatCommand report("report_all", "Report all players", []() { reportall(); });
CatCommand report_uid("report_steamid", "Report with steamid",
                      [](const CCommand &args)
                      {
                          if (args.ArgC() < 2)
                              return;
                          unsigned steamid = 0;
                          try
                          {
                              steamid = std::stoi(args.Arg(1));
                          }
                          catch (const std::invalid_argument &)
                          {
                              logging::Info("Report machine broke");
                              return;
                          }
                          if (!steamid)
                          {
                              logging::Info("Report machine broke");
                              return;
                          }
                          send_report(steamid);
                      });

namespace autoqueue_report_state
{
enum class State
{
    Idle,
    Queueing,
    WaitingForClass,
    SettlingIn,
    Reporting,
    Abandoning,
    PostAbandonCooldown,
};

static State state = State::Idle;
static Timer state_timer{};
static std::string reported_match;
static int settle_tries   = 0;
static int last_connected = 0;
static int stable_ticks   = 0;

static std::string currentMatchId()
{
    if (!g_IEngine || !g_IEngine->IsInGame())
        return {};
    std::string id;
    if (CNetChan *ch = g_IEngine->GetNetChannelInfo())
    {
        if (const char *addr = ch->GetAddress())
            id = addr;
    }
    if (const char *map = g_IEngine->GetLevelName())
    {
        if (!id.empty())
            id.push_back('/');
        id += map;
    }
    return id;
}

static void startCasualQueue()
{
    re::CTFPartyClient *client = re::CTFPartyClient::GTFPartyClient();
    if (!client)
    {
        logging::Info("autoqueue-report: CTFPartyClient == null!");
        return;
    }
    if (auto *criteria = client->MutLocalGroupCriteria(client))
        re::ITFGroupMatchCriteria::SetMatchGroup(criteria, 7);
    client->LoadSavedCasualCriteria();
    client->RequestQueueForMatch(7);
}

static void reset()
{
    state          = State::Idle;
    settle_tries   = 0;
    last_connected = 0;
    stable_ticks   = 0;
    reported_match.clear();
}

void onLevelInit()
{
    settle_tries   = 0;
    last_connected = 0;
    stable_ticks   = 0;
    if (state == State::Reporting)
        return;
    to_report.clear();
    already_reported.clear();
    can_report = false;
    if (state == State::Abandoning || state == State::PostAbandonCooldown)
    {
        logging::Info("autoqueue-report: new map, starting report cycle");
        reported_match.clear();
        state_timer.update();
        state = State::WaitingForClass;
    }
}

void update()
{
    if (!*autoqueue_report)
    {
        if (state != State::Idle)
            reset();
        return;
    }

    re::CTFGCClientSystem *gc = re::CTFGCClientSystem::GTFGCClientSystem();
    re::CTFPartyClient *pc    = re::CTFPartyClient::GTFPartyClient();
    bool in_game               = g_IEngine->IsInGame();

    switch (state)
    {
    case State::Idle:
        if (in_game)
        {
            state = State::WaitingForClass;
            break;
        }
        if (tfmm::shouldHoldQueueForMapLoad())
            break;
        if (!pc || !gc)
            break;
        if (gc->BConnectedToMatchServer(false) || gc->BHaveLiveMatch())
            break;
        if (pc->BInQueueForMatchGroup(7) || pc->BInQueueForStandby())
            break;
        if (pc->GetPendingInvites())
            break;
        if (state_timer.test_and_set(5000))
        {
            logging::Info("autoqueue-report: queueing for Casual 12v12");
            startCasualQueue();
            state = State::Queueing;
        }
        break;

    case State::Queueing:
        if (in_game)
            state = State::WaitingForClass;
        break;

    case State::WaitingForClass:
        if (!in_game)
        {
            reset();
            break;
        }
        if (!g_pPlayerResource)
            break;
        logging::Info("autoqueue-report: connected, settling in before reporting");
        settle_tries   = 0;
        last_connected = 0;
        stable_ticks   = 0;
        can_report     = false;
        state_timer.update();
        state = State::SettlingIn;
        break;

    case State::SettlingIn:
        if (!in_game)
        {
            reset();
            break;
        }
        if (state_timer.test_and_set(4000))
        {
            const int connected = collect_report_targets(to_report);
            can_report          = false;
            ++settle_tries;
            if (connected == last_connected && connected > 0)
                ++stable_ticks;
            else
                stable_ticks = 0;
            last_connected = connected;
            logging::Info("autoqueue-report: settle %d connected=%d reportable=%zu stable=%d", settle_tries, connected, to_report.size(), stable_ticks);

            const bool fullish = connected >= 10 && stable_ticks >= 1 && !to_report.empty();
            const bool stable  = stable_ticks >= 2 && connected >= 4 && !to_report.empty();
            const bool timeout = settle_tries >= 8 && !to_report.empty();
            if (fullish || stable || timeout)
            {
                already_reported.clear();
                reported_match = currentMatchId();
                can_report     = true;
                logging::Info("autoqueue-report: reporting %zu players", to_report.size());
                state = State::Reporting;
                break;
            }
            if (settle_tries >= 8 && to_report.empty())
            {
                logging::Info("autoqueue-report: no players to report, abandoning match");
                reported_match = currentMatchId();
                tfmm::disconnectAndAbandon();
                state_timer.update();
                state = State::Abandoning;
            }
        }
        break;

    case State::Reporting:
        if (!in_game)
        {
            reset();
            break;
        }
        {
            const auto now = currentMatchId();
            if (!now.empty() && !reported_match.empty() && now != reported_match)
            {
                settle_tries   = 0;
                last_connected = 0;
                stable_ticks   = 0;
                state          = State::WaitingForClass;
                break;
            }
        }
        if (!can_report && to_report.empty())
        {
            std::vector<unsigned> extra;
            collect_report_targets(extra);
            for (unsigned id : extra)
            {
                if (already_reported.count(id))
                    continue;
                to_report.push_back(id);
            }
            if (!to_report.empty())
            {
                logging::Info("autoqueue-report: %zu more players showed up, keeping reports", to_report.size());
                can_report = true;
                break;
            }
            logging::Info("autoqueue-report: done reporting %zu players, abandoning match", already_reported.size());
            tfmm::disconnectAndAbandon();
            state_timer.update();
            state = State::Abandoning;
        }
        break;

    case State::Abandoning:
    {
        const auto now = currentMatchId();
        if (in_game && !now.empty() && !reported_match.empty() && now != reported_match)
        {
            logging::Info("autoqueue-report: new match after abandon, reporting this one");
            settle_tries = 0;
            reported_match.clear();
            state_timer.update();
            state = State::WaitingForClass;
            break;
        }
        if (!in_game && !(gc && gc->BConnectedToMatchServer(false)))
        {
            state_timer.update();
            state = State::PostAbandonCooldown;
        }
        else if (!now.empty() && now == reported_match && state_timer.test_and_set(15000))
        {
            tfmm::disconnectAndAbandon();
        }
        break;
    }

    case State::PostAbandonCooldown:
        if (state_timer.test_and_set(5000))
            state = State::Idle;
        break;
    }
}
}


Timer crouchcdr{};
void smart_crouch()
{
    if (g_Settings.bInvalid)
        return;
    if (!current_user_cmd)
        return;
    if (*always_crouch)
    {
        current_user_cmd->buttons |= IN_DUCK;
        if (crouchcdr.test_and_set(10000))
            current_user_cmd->buttons &= ~IN_DUCK;
        return;
    }
    bool foundtar      = false;
    static bool crouch = false;
    if (crouchcdr.test_and_set(2000))
    {
        for (auto const &ent: entity_cache::player_cache)
        {
            if (CE_BAD(ent) || ent->m_Type() != ENTITY_PLAYER || ent->m_iTeam() == LOCAL_E->m_iTeam() || !(ent->hitboxes.GetHitbox(0)) || !(ent->m_bAlivePlayer()) || !player_tools::shouldTarget(ent))
                continue;
            bool failedvis = false;
            for (int j = 0; j < 18; j++)
            {
                auto *box = ent->hitboxes.GetHitbox(j);
                if (box && IsVectorVisible(g_pLocalPlayer->v_Eye, box->center))
                    failedvis = true;
            }
            if (failedvis)
                continue;
            for (int j = 0; j < 18; j++)
            {
                if (!LOCAL_E->hitboxes.GetHitbox(j))
                    continue;
                // Check if they see my hitboxes
                if (!IsVectorVisible(ent->hitboxes.GetHitbox(0)->center, LOCAL_E->hitboxes.GetHitbox(j)->center) && !IsVectorVisible(ent->hitboxes.GetHitbox(0)->center, LOCAL_E->hitboxes.GetHitbox(j)->min) && !IsVectorVisible(ent->hitboxes.GetHitbox(0)->center, LOCAL_E->hitboxes.GetHitbox(j)->max))
                    continue;
                foundtar = true;
                crouch   = true;
            }
        }
        if (!foundtar && crouch)
            crouch = false;
    }
    if (crouch)
        current_user_cmd->buttons |= IN_DUCK;
}

CatCommand print_ammo("debug_print_ammo", "debug",
                      []()
                      {
                          if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer() || CE_BAD(LOCAL_W))
                              return;
                          logging::Info("Current slot: %d", re::C_BaseCombatWeapon::GetSlot(RAW_ENT(LOCAL_W)));
                          for (int i = 0; i < 10; ++i)
                              logging::Info("Ammo Table %d: %d", i, CE_INT(LOCAL_E, netvar.m_iAmmo + i * 4));
                      });
static Timer disguise{};
static Timer report_timer{};
static std::string health = "Health: 0/0";
static std::string ammo   = "Ammo: 0/0";
static int max_ammo;
static CachedEntity *local_w;
// TODO: add more stuffs
static void cm()
{
    if (!*catbotmode)
        return;

    if (CE_GOOD(LOCAL_E))
    {
        if (LOCAL_W != local_w)
        {
            local_w  = LOCAL_W;
            max_ammo = 0;
        }
        float max_hp  = g_pPlayerResource->GetMaxHealth(LOCAL_E);
        float curr_hp = CE_INT(LOCAL_E, netvar.iHealth);
        int ammo0     = CE_INT(LOCAL_E, netvar.m_iClip2);
        int ammo2     = CE_INT(LOCAL_E, netvar.m_iClip1);
        if (ammo0 + ammo2 > max_ammo)
            max_ammo = ammo0 + ammo2;
        health = format("Health: ", curr_hp, "/", max_hp);
        ammo   = format("Ammo: ", ammo0 + ammo2, "/", max_ammo);
    }
    if (g_Settings.bInvalid)
        return;

    if (*autoReport && !*autoqueue_report && report_timer.test_and_set(60000))
        reportall();

    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return;

    if (*auto_crouch)
        smart_crouch();

    //
    static const int classes[3]{ tf_spy, tf_sniper, tf_pyro };
    if (*auto_disguise && g_pPlayerResource->GetClass(LOCAL_E) == tf_spy && !IsPlayerDisguised(LOCAL_E) && disguise.test_and_set(3000))
    {
        int teamtodisguise = (LOCAL_E->m_iTeam() == TEAM_RED) ? TEAM_RED - 1 : TEAM_BLU - 1;
        int classtojoin    = classes[rand() % 3];
        g_IEngine->ClientCmd_Unrestricted(format("disguise ", classtojoin, " ", teamtodisguise).c_str());
    }
}

static Timer unstuck{};
static int unstucks;
static Timer report_timer2{};

static bool partyInQueue()
{
    re::CTFPartyClient *pc = re::CTFPartyClient::GTFPartyClient();
    return pc && (pc->BInQueueForMatchGroup(tfmm::getQueue()) || pc->BInQueueForStandby());
}

static bool partyQueueRequestPending()
{
    re::CTFPartyClient *pc = re::CTFPartyClient::GTFPartyClient();
    return pc && pc->BQueueRequestPending(tfmm::getQueue());
}

static void requeueStayingInMatch()
{
    if (!partyInQueue() && !partyQueueRequestPending())
        tfmm::startQueue();
    requeue_active = true;
}

static void abandon_or_requeue(const std::string &reason)
{
    if (*requeue_without_abandon)
    {
        if (!requeue_active)
            logging::Info("Requeueing without abandon, staying in match: %s", reason.c_str());
        requeueStayingInMatch();
    }
    else
    {
        logging::Info("Abandoning: %s", reason.c_str());
        requeue_active = false;
        tfmm::abandon();
    }
}

static bool any_requeue_condition(int count_total, int count_ipc)
{
    if (abandon_if_ipc_bots_gte && count_ipc >= int(abandon_if_ipc_bots_gte))
        return true;
    if (abandon_if_humans_lte && count_total - count_ipc <= int(abandon_if_humans_lte))
        return true;
    if (abandon_if_players_lte && count_total <= int(abandon_if_players_lte))
        return true;
    if (*abandon_if_no_navmesh && !tfmm::isLoadingMap() && !navparser::NavEngine::hasNavMesh())
        return true;
    return false;
}

void update()
{
    if (g_Settings.bInvalid)
        return;

    if (can_report)
    {
        if (report_timer2.test_and_set(400))
        {
            if (to_report.empty())
                can_report = false;
            else
            {
                auto rep = to_report.back();
                to_report.pop_back();
                if (already_reported.insert(rep).second)
                    send_report(rep);
            }
        }
    }
    if (!catbotmode)
        return;

    if (CE_BAD(LOCAL_E))
        return;

    if (LOCAL_E->m_bAlivePlayer())
    {
        unstuck.update();
        unstucks = 0;
    }
    if (unstuck.test_and_set(10000))
    {
        unstucks++;
        // Send menuclosed to tell the server that we want to respawn
        hack::command_stack().push("menuclosed");
        // If that didnt work, force pick a team and class
        if (unstucks > 3)
            hack::command_stack().push("autoteam; join_class sniper");
    }

    if (micspam)
    {
        if (micspam_on && micspam_on_timer.test_and_set(*micspam_on * 1000))
            g_IEngine->ClientCmd_Unrestricted("+voicerecord");
        if (micspam_off && micspam_off_timer.test_and_set(*micspam_off * 1000))
            g_IEngine->ClientCmd_Unrestricted("-voicerecord");
    }

    if (random_votekicks && timer_votekicks.test_and_set(5000))
        do_random_votekick();
    if (timer_abandon.test_and_set(1000) && level_init_timer.check(13000))
    {
        count_ipc = 0;
        ipc_list.clear();
        int count_total = 0;

        ForEachConnectedPlayer(
            [&](int i, unsigned id, const player_info_s &)
            {
                if (!on_match_team(i))
                    return;
                const auto state = playerlist::AccessData(id).state;
                if (state == playerlist::k_EState::CAT)
                    return;
                ++count_total;
                if (state == playerlist::k_EState::IPC || state == playerlist::k_EState::TEXTMODE)
                {
                    ipc_list.push_back(id);
                    ++count_ipc;
                }
            });

        if (abandon_if_ipc_bots_gte)
        {
            if (count_ipc >= int(abandon_if_ipc_bots_gte))
            {
                // Store local IPC Id and assign to the quit_id variable for later comparisions
                unsigned local_ipcid = ipc::peer->client_id;
                unsigned quit_id     = local_ipcid;

                // Iterate all the players marked as bot
                for (auto &id : ipc_list)
                {
                    // We already know we shouldn't quit, so just break out of the loop
                    if (quit_id < local_ipcid)
                        break;

                    // Reduce code size
                    auto &peer_mem = ipc::peer->memory;

                    // Iterate all ipc peers
                    for (unsigned i = 0; i < cat_ipc::max_peers; ++i)
                    {
                        // If that ipc peer is alive and in has the steamid of that player
                        if (!peer_mem->peer_data[i].free && peer_mem->peer_user_data[i].friendid == id)
                        {
                            // Check against blacklist
                            if (std::find(ipc_blacklist.begin(), ipc_blacklist.end(), i) != ipc_blacklist.end())
                                continue;

                            // Found someone with a lower ipc id
                            if (i < local_ipcid)
                            {
                                quit_id = i;
                                break;
                            }
                        }
                    }
                }
                // Only quit if you are the player with the lowest ipc id
                if (quit_id == local_ipcid)
                {
                    // Clear blacklist related stuff
                    waiting_for_quit_bool = false;
                    ipc_blacklist.clear();

                    abandon_or_requeue(format("there are ", count_ipc, " local players in game, and abandon_if_ipc_bots_gte is ", int(abandon_if_ipc_bots_gte), "."));
                    return;
                }
                else
                {
                    if (!waiting_for_quit_bool)
                    {
                        // Waiting for that ipc id to quit, we use this timer in order to blacklist
                        // ipc peers which refuse to quit for some reason
                        waiting_for_quit_bool = true;
                        waiting_for_quit_timer.update();
                    }
                    else
                    {
                        // IPC peer isn't leaving, blacklist for now
                        if (waiting_for_quit_timer.test_and_set(10000))
                        {
                            ipc_blacklist.push_back(quit_id);
                            waiting_for_quit_bool = false;
                        }
                    }
                }
            }
            else
            {
                // Reset Bool because no reason to quit
                waiting_for_quit_bool = false;
                ipc_blacklist.clear();
            }
        }
        if (abandon_if_humans_lte)
        {
            if (count_total - count_ipc <= int(abandon_if_humans_lte))
            {
                abandon_or_requeue(format("there are ", count_total - count_ipc, " non-bots in game, and abandon_if_humans_lte is ", int(abandon_if_humans_lte), "."));
                return;
            }
        }
        if (abandon_if_players_lte)
        {
            if (count_total <= int(abandon_if_players_lte))
            {
                abandon_or_requeue(format("there are ", count_total, " total players in game, and abandon_if_players_lte is ", int(abandon_if_players_lte), "."));
                return;
            }
        }
        if (*abandon_if_no_navmesh && !tfmm::isLoadingMap() && !navparser::NavEngine::hasNavMesh())
        {
            abandon_or_requeue("the current map has no navmesh.");
            return;
        }
        if (requeue_active && !any_requeue_condition(count_total, count_ipc))
        {
            if (partyInQueue())
            {
                logging::Info("Cancelling queue, match is acceptable now (humans %d, ipc %d, total %d).", count_total - count_ipc, count_ipc, count_total);
                tfmm::leaveQueue();
            }
            else if (!partyQueueRequestPending())
                requeue_active = false;
        }
    }
}

static std::unique_ptr<BytePatch> sdr_assert_reply_timeouts;
static std::unique_ptr<BytePatch> sdr_assert_expecting_acks;

void init()
{
    // g_IEventManager2->AddListener(&listener(), "player_death", false);
    sdr_assert_reply_timeouts = std::make_unique<BytePatch>(gSignatures.GetSteamClientSignature, sigs::sdr_assert_reply_timeouts, 0x13, std::vector<unsigned char>{ 0xB0, 0x01, 0x90, 0x90, 0x90 });
    sdr_assert_expecting_acks = std::make_unique<BytePatch>(gSignatures.GetSteamClientSignature, sigs::sdr_assert_expecting_acks, 0x39, std::vector<unsigned char>{ 0xB0, 0x01, 0x90, 0x90, 0x90 });
    sdr_assert_reply_timeouts->Patch();
    sdr_assert_expecting_acks->Patch();
    g_IEventManager2->AddListener(&listener2(), "vote_maps_changed", false);
}

void level_init()
{
    deaths = 0;
    requeue_active = false;
    level_init_timer.update();
    autoqueue_report_state::onLevelInit();
}

void shutdown()
{
    // g_IEventManager2->RemoveListener(&listener());
    if (sdr_assert_reply_timeouts)
        sdr_assert_reply_timeouts->Shutdown();
    if (sdr_assert_expecting_acks)
        sdr_assert_expecting_acks->Shutdown();
    g_IEventManager2->RemoveListener(&listener2());
}

#if ENABLE_VISUALS
static void draw()
{
    if (!catbotmode || !anti_motd)
        return;
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    AddCenterString(health, colors::green);
    AddCenterString(ammo, colors::yellow);
}
#endif

static InitRoutine runinit(
    []()
    {
        EC::Register(EC::CreateMove, cm, "cm_catbot", EC::average);
        EC::Register(EC::CreateMove, update, "cm2_catbot", EC::average);
        EC::Register(EC::CreateMove, autoqueue_report_state::update, "cm_autoqueue_report", EC::average);
        EC::Register(EC::LevelInit, level_init, "levelinit_catbot", EC::average);
        EC::Register(EC::Shutdown, shutdown, "shutdown_catbot", EC::average);
#if ENABLE_VISUALS
        EC::Register(EC::Draw, draw, "draw_catbot", EC::average);
#endif
        if (auto addr = gSignatures.GetClientSignature(sigs::report_player_recent_check))
            report_recent_check_detour.Init(addr, (void *) report_recent_check_hook);
        init();
    });
} // namespace hacks::shared::catbot
