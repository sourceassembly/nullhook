/*
 * CTFPartyClient.hpp
 *
 *  Created on: Dec 7, 2017
 *      Author: nullifiedcat
 */

#pragma once
#include "reclasses.hpp"
#include <cstddef>
namespace re
{

class CTFPartyClient
{
public:
    static CTFPartyClient *GTFPartyClient();

    void SendPartyChat(const char *message);
    int LoadSavedCasualCriteria();
    static ITFGroupMatchCriteria *MutLocalGroupCriteria(CTFPartyClient *client);
    static bool BCanQueueForStandby(CTFPartyClient *this_);
    char RequestQueueForMatch(int type);
    void RequestQueueForStandby();
    void RequestLeaveStandby();
    bool BInQueueForStandby();
    bool BInQueueForMatchGroup(int type);
    bool BQueueRequestPending(int type);
    char RequestLeaveForMatch(int type);
    int BInvitePlayerToParty(CSteamID steamid);
    int BRequestJoinPlayer(CSteamID steamid);
    static bool BInQueue(CTFPartyClient *this_);
    int GetNumOnlineMembers();
    int GetNumMembers();
    int GetPendingInvites();
    int PromotePlayerToLeader(CSteamID steamid);
    std::vector<unsigned> GetPartySteamIDs();
    int KickPlayer(CSteamID steamid);
    bool GetCurrentPartyLeader(CSteamID &id);
};
class ITFMatchGroupDescription
{
public:
    struct Layout
    {
        std::size_t id                    = vtables::match_group::id;
        std::size_t force_client_settings = vtables::match_group::force_client_settings;
        int table_max                     = vtables::match_group::table_max;
    };
    static Layout &layout();

    int &m_iID()
    {
        return *reinterpret_cast<int *>(reinterpret_cast<char *>(this) + layout().id);
    }
    bool &m_bForceCompetitiveSettings()
    {
        return *reinterpret_cast<bool *>(reinterpret_cast<char *>(this) + layout().force_client_settings);
    }
};

ITFMatchGroupDescription *GetMatchGroupDescription(int &idx);
} // namespace re
