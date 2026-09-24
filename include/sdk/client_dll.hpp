#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"

class ClientClass;
class CViewSetup;
class bf_write;

class CHLClient
{
public:
    ClientClass *GetAllClasses()
    {
        return vfunc<ClientClass *(*)(CHLClient *)>(this, vtables::client_dll::get_all_classes)(this);
    }
    bool GetPlayerView(CViewSetup &playerView)
    {
        return vfunc<bool (*)(CHLClient *, CViewSetup &)>(this, vtables::client_dll::get_player_view)(this, playerView);
    }
    void InvalidateMdlCache()
    {
        vfunc<void (*)(CHLClient *)>(this, vtables::client_dll::invalidate_mdl_cache)(this);
    }
    void CreateMove(int sequence_number, float input_sample_frametime, bool active)
    {
        vfunc<void (*)(CHLClient *, int, float, bool)>(this, vtables::client_dll::create_move)(this, sequence_number, input_sample_frametime, active);
    }
    bool WriteUsercmdDeltaToBuffer(bf_write *buf, int from, int to, bool is_new_command)
    {
        return vfunc<bool (*)(CHLClient *, bf_write *, int, int, bool)>(this, vtables::client_dll::write_usercmd_delta)(this, buf, from, to, is_new_command);
    }
};
