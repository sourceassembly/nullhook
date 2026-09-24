/*
 * CTFPartyClient.cpp
 *
 *  Created on: Dec 7, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "core/e8call.hpp"

static int party_movzx_rdi(const char *pattern, int fallback)
{
    uintptr_t addr = gSignatures.GetClientSignature(pattern);
    if (!addr)
        return fallback;
    auto *code = reinterpret_cast<uint8_t *>(addr);
    for (int i = 0; i < 32; ++i)
    {
        if (code[i] == 0x0F && code[i + 1] == 0xB6 && code[i + 2] == 0x47)
            return code[i + 3];
    }
    return fallback;
}

static int party_disp32(const char *pattern, const char *marker, int fallback)
{
    uintptr_t addr = gSignatures.GetClientSignature(pattern);
    if (!addr)
        return fallback;
    auto *code = reinterpret_cast<uint8_t *>(addr);
    for (int i = 0; i < 96; ++i)
    {
        if (code[i] == 0x48 && code[i + 1] == 0x81 && code[i + 2] == 0xC7)
            return *reinterpret_cast<int *>(code + i + 3);
    }
    (void) marker;
    return fallback;
}

static int party_queue_off()
{
    static int off = party_movzx_rdi(sigs::is_in_standby_queue, 0x68);
    return off;
}

static int party_pending_off()
{
    static int off = []() -> int {
        auto *code = reinterpret_cast<uint8_t *>(gSignatures.GetClientSignature(sigs::request_queue_for_match));
        if (code)
        {
            for (int i = 0; i < 0x88; ++i)
            {
                if (code[i] == 0x41 && code[i + 1] == 0x80 && code[i + 2] == 0xBC && code[i + 3] == 0x06)
                    return *reinterpret_cast<int *>(code + i + 4);
            }
        }
        return 0x31E;
    }();
    return off;
}

static int party_criteria_off()
{
    static int off = []() -> int {
        auto *p = reinterpret_cast<uint8_t *>(gSignatures.GetClientSignature(sigs::mut_local_group_criteria));
        if (p)
        {
            for (int i = 0; i < 32; ++i)
            {
                if (p[i] == 0x48 && p[i + 1] == 0x8D && p[i + 2] == 0x87)
                    return *reinterpret_cast<int *>(p + i + 3);
            }
        }
        return party_disp32(sigs::load_saved_casual_criteria, nullptr, 0x1B0);
    }();
    return off;
}

re::CTFPartyClient *re::CTFPartyClient::GTFPartyClient()
{
    typedef re::CTFPartyClient *(*GTFPartyClient_t)(void);
    static uintptr_t addr                     = SigAdd(gSignatures.GetClientSignature(sigs::get_party_client), sigs::get_party_client_offset);
    static GTFPartyClient_t GTFPartyClient_fn = GTFPartyClient_t(addr);
    return GTFPartyClient_fn ? GTFPartyClient_fn() : nullptr;
}

bool re::CTFPartyClient::BInQueue(re::CTFPartyClient *this_)
{
    return this_ && (*(uint8_t *) ((uint8_t *) this_ + party_queue_off()) || this_->BInQueueForMatchGroup(0));
}

int re::CTFPartyClient::GetNumOnlineMembers()
{
    typedef int (*GetNumOnlineMembers_t)(re::CTFPartyClient *);
    static auto fn = GetNumOnlineMembers_t(gSignatures.GetClientSignature(sigs::party_client_get_num_online_members));
    return fn ? fn(this) : 0;
}

int re::CTFPartyClient::GetNumMembers()
{
    typedef int (*GetNumMembers_t)(re::CTFPartyClient *);
    static auto fn = GetNumMembers_t(gSignatures.GetClientSignature(sigs::party_client_get_num_members));
    return fn ? fn(this) : 0;
}

int re::CTFPartyClient::GetPendingInvites()
{
    static int off = []() -> int
    {
        uintptr_t addr = gSignatures.GetClientSignature(sigs::party_incoming_invites_debug);
        if (!addr)
            return 0;
        auto *code = reinterpret_cast<uint8_t *>(addr);
        for (int i = 0; i < 96; ++i)
        {
            if (code[i] == 0x4C && code[i + 1] == 0x63 && code[i + 2] == 0xA0)
                return *reinterpret_cast<int *>(code + i + 3);
        }
        return 0;
    }();
    if (!this || off <= 0)
        return 0;
    return *reinterpret_cast<int *>(uintptr_t(this) + off);
}

void re::CTFPartyClient::SendPartyChat(const char *message)
{
    typedef void (*SendPartyChat_t)(re::CTFPartyClient *, const char *);
    static auto fn = SendPartyChat_t(gSignatures.GetClientSignature(sigs::party_client_send_party_chat));
    if (fn)
        fn(this, message);
}

bool re::CTFPartyClient::BCanQueueForStandby(re::CTFPartyClient *this_)
{
    return this_ && !this_->BInQueueForStandby();
}

re::ITFGroupMatchCriteria *re::CTFPartyClient::MutLocalGroupCriteria(re::CTFPartyClient *client)
{
    return client ? reinterpret_cast<re::ITFGroupMatchCriteria *>(uintptr_t(client) + party_criteria_off()) : nullptr;
}

int re::CTFPartyClient::LoadSavedCasualCriteria()
{
    typedef int (*LoadSavedCasualCriteria_t)(re::CTFPartyClient *);
    static auto fn = LoadSavedCasualCriteria_t(gSignatures.GetClientSignature(sigs::load_saved_casual_criteria));
    return fn ? fn(this) : 0;
}

void re::CTFPartyClient::RequestQueueForStandby()
{
    typedef void (*RequestStandby_t)(re::CTFPartyClient *);
    static auto fn = RequestStandby_t(gSignatures.GetClientSignature(sigs::request_queue_for_standby));
    if (fn)
        fn(this);
}

void re::CTFPartyClient::RequestLeaveStandby()
{
    typedef void (*fn_t)(re::CTFPartyClient *);
    static auto fn = fn_t(gSignatures.GetClientSignature(sigs::request_leave_standby));
    if (fn)
        fn(this);
}

char re::CTFPartyClient::RequestQueueForMatch(int type)
{
    typedef char (*RequestQueueForMatch_t)(re::CTFPartyClient *, int);
    static auto fn = RequestQueueForMatch_t(gSignatures.GetClientSignature(sigs::request_queue_for_match));
    return fn ? fn(this, type) : 0;
}

bool re::CTFPartyClient::BInQueueForMatchGroup(int type)
{
    typedef bool (*BInQueueForMatchGroup_t)(re::CTFPartyClient *, int);
    static auto fn = BInQueueForMatchGroup_t(gSignatures.GetClientSignature(sigs::is_in_queue_for_match_group));
    return fn ? fn(this, type) : false;
}

bool re::CTFPartyClient::BInQueueForStandby()
{
    return *((unsigned char *) this + party_queue_off());
}

bool re::CTFPartyClient::BQueueRequestPending(int type)
{
    if (type < 0 || type > 8)
        return false;
    return *((unsigned char *) this + party_pending_off() + type);
}

char re::CTFPartyClient::RequestLeaveForMatch(int type)
{
    typedef char (*RequestLeaveForMatch_t)(re::CTFPartyClient *, int);
    static auto fn = RequestLeaveForMatch_t(gSignatures.GetClientSignature(sigs::request_leave_for_match));
    return fn ? fn(this, type) : 0;
}

int re::CTFPartyClient::BInvitePlayerToParty(CSteamID steamid)
{
    typedef unsigned char (*Invite_t)(re::CTFPartyClient *, CSteamID, unsigned char);
    static auto fn = Invite_t(gSignatures.GetClientSignature(sigs::party_invite_player));
    if (fn)
        return fn(this, steamid, 0);
    if (!g_IEngine)
        return 0;
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "tf_party_invite_user %llu", static_cast<unsigned long long>(steamid.ConvertToUint64()));
    g_IEngine->ClientCmd_Unrestricted(cmd);
    return 1;
}
int re::CTFPartyClient::BRequestJoinPlayer(CSteamID steamid)
{
    typedef unsigned char (*Join_t)(re::CTFPartyClient *, CSteamID, unsigned char);
    static auto fn = Join_t(gSignatures.GetClientSignature(sigs::party_request_join_player));
    if (fn)
        return fn(this, steamid, 0);
    if (!g_IEngine)
        return 0;
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "tf_party_request_join_user %llu", static_cast<unsigned long long>(steamid.ConvertToUint64()));
    g_IEngine->ClientCmd_Unrestricted(cmd);
    return 1;
}

int re::CTFPartyClient::PromotePlayerToLeader(CSteamID steamid)
{
    typedef int (*PromotePlayerToLeader_t)(re::CTFPartyClient *, CSteamID);
    static auto fn = PromotePlayerToLeader_t(gSignatures.GetClientSignature(sigs::promote_to_leader));
    return fn ? fn(this, steamid) : 0;
}

std::vector<unsigned> re::CTFPartyClient::GetPartySteamIDs()
{
    typedef bool (*SteamIDOfSlot_t)(re::CTFPartyClient *, int, CSteamID *);
    static auto fn = SteamIDOfSlot_t(gSignatures.GetClientSignature(sigs::party_client_get_member_steamid));
    std::vector<unsigned> party_members;
    if (!fn)
        return party_members;
    for (int i = 0; i < GetNumMembers(); i++)
    {
        CSteamID out;
        fn(this, i, &out);
        if (out.GetAccountID())
            party_members.push_back(out.GetAccountID());
    }
    return party_members;
}

int re::CTFPartyClient::KickPlayer(CSteamID steamid)
{
    typedef int (*KickPlayer_t)(re::CTFPartyClient *, CSteamID);
    static auto fn = KickPlayer_t(gSignatures.GetClientSignature(sigs::party_client_kick_player));
    return fn ? fn(this, steamid) : 0;
}

bool re::CTFPartyClient::GetCurrentPartyLeader(CSteamID &id)
{
    typedef bool (*SteamIDOfSlot_t)(re::CTFPartyClient *, int, CSteamID *);
    static auto fn = SteamIDOfSlot_t(gSignatures.GetClientSignature(sigs::party_client_get_member_steamid));
    if (!fn)
        return false;
    const int not_leader_off = party_movzx_rdi(sigs::party_client_in_party_not_leader, 0x40);
    if (!*((uint8_t *) this + not_leader_off) && g_ISteamUser)
    {
        id = g_ISteamUser->GetSteamID();
        return id.GetAccountID() != 0;
    }
    CSteamID out;
    if (!fn(this, 0, &out) || !out.GetAccountID())
        return false;
    id = out;
    return true;
}

re::ITFMatchGroupDescription::Layout &re::ITFMatchGroupDescription::layout()
{
    static Layout live = [] {
        Layout out;
        auto *cmp = reinterpret_cast<uint8_t *>(gSignatures.GetClientSignature(sigs::get_match_group_description));
        if (cmp && cmp[2] == 0x07 && cmp[3] == 0x83 && cmp[4] == 0xF8)
            out.table_max = cmp[5];
        auto *force = reinterpret_cast<uint8_t *>(gSignatures.GetClientSignature(sigs::match_desc_force_client_settings));
        if (force && force[0] == 0x66 && force[1] == 0x45 && force[2] == 0x89 && force[3] == 0x54 && force[4] == 0x24)
            out.force_client_settings = force[5];
        logging::Info("ITFMatchGroupDescription layout id=%zx force=%zx max=%d", out.id, out.force_client_settings, out.table_max);
        return out;
    }();
    return live;
}

re::ITFMatchGroupDescription *re::GetMatchGroupDescription(int &idx)
{
    using Fn = re::ITFMatchGroupDescription *(*)(int *);
    static auto fn = Fn(gSignatures.GetClientSignature(sigs::get_match_group_description));
    if (!fn || idx < 0 || idx > ITFMatchGroupDescription::layout().table_max)
        return nullptr;
    return fn(&idx);
}
