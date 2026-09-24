#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "sdk/CNetChan.hpp"

class KeyValues;
class IAchievementMgr;
class QAngle;
struct player_info_s;

class CEngineClient
{
public:
    void GetScreenSize(int &width, int &height)
    {
        vfunc<void (*)(CEngineClient *, int *, int *)>(this, vtables::engine_client::get_screen_size)(this, &width, &height);
    }
    void ServerCmd(const char *cmd, bool reliable = true)
    {
        vfunc<void (*)(CEngineClient *, const char *, bool)>(this, vtables::engine_client::server_cmd)(this, cmd, reliable);
    }
    void ClientCmd(const char *cmd)
    {
        vfunc<void (*)(CEngineClient *, const char *)>(this, vtables::engine_client::client_cmd)(this, cmd);
    }
    bool GetPlayerInfo(int ent_num, player_info_s *pinfo)
    {
        return vfunc<bool (*)(CEngineClient *, int, player_info_s *)>(this, vtables::engine_client::get_player_info)(this, ent_num, pinfo);
    }
    int GetPlayerForUserID(int user_id)
    {
        return vfunc<int (*)(CEngineClient *, int)>(this, vtables::engine_client::get_player_for_user_id)(this, user_id);
    }
    int GetLocalPlayer()
    {
        return vfunc<int (*)(CEngineClient *)>(this, vtables::engine_client::get_local_player)(this);
    }
    void GetViewAngles(QAngle &angles)
    {
        vfunc<void (*)(CEngineClient *, QAngle &)>(this, vtables::engine_client::get_view_angles)(this, angles);
    }
    void SetViewAngles(QAngle &angles)
    {
        vfunc<void (*)(CEngineClient *, QAngle &)>(this, vtables::engine_client::set_view_angles)(this, angles);
    }
    int GetMaxClients()
    {
        return vfunc<int (*)(CEngineClient *)>(this, vtables::engine_client::get_max_clients)(this);
    }
    bool IsInGame()
    {
        return vfunc<bool (*)(CEngineClient *)>(this, vtables::engine_client::is_in_game)(this);
    }
    bool IsConnected()
    {
        return vfunc<bool (*)(CEngineClient *)>(this, vtables::engine_client::is_connected)(this);
    }
    bool IsDrawingLoadingImage()
    {
        return vfunc<bool (*)(CEngineClient *)>(this, vtables::engine_client::is_drawing_loading_image)(this);
    }
    const char *GetGameDirectory()
    {
        return vfunc<const char *(*)(CEngineClient *)>(this, vtables::engine_client::get_game_directory)(this);
    }
    const char *GetLevelName()
    {
        return vfunc<const char *(*)(CEngineClient *)>(this, vtables::engine_client::get_level_name)(this);
    }
    CNetChan *GetNetChannelInfo()
    {
        return vfunc<CNetChan *(*)(CEngineClient *)>(this, vtables::engine_client::get_net_channel_info)(this);
    }
    bool IsPlayingTimeDemo()
    {
        return vfunc<bool (*)(CEngineClient *)>(this, vtables::engine_client::is_playing_time_demo)(this);
    }
    bool IsTakingScreenshot()
    {
        return vfunc<bool (*)(CEngineClient *)>(this, vtables::engine_client::is_taking_screenshot)(this);
    }
    void ExecuteClientCmd(const char *cmd)
    {
        vfunc<void (*)(CEngineClient *, const char *)>(this, vtables::engine_client::execute_client_cmd)(this, cmd);
    }
    int GetAppID()
    {
        return vfunc<int (*)(CEngineClient *)>(this, vtables::engine_client::get_app_id)(this);
    }
    void ClientCmd_Unrestricted(const char *cmd)
    {
        vfunc<void (*)(CEngineClient *, const char *)>(this, vtables::engine_client::client_cmd_unrestricted)(this, cmd);
    }
    IAchievementMgr *GetAchievementMgr()
    {
        return vfunc<IAchievementMgr *(*)(CEngineClient *)>(this, vtables::engine_client::get_achievement_mgr)(this);
    }
    void ServerCmdKeyValues(KeyValues *kv)
    {
        vfunc<void (*)(CEngineClient *, KeyValues *)>(this, vtables::engine_client::server_cmd_key_values)(this, kv);
    }
};
