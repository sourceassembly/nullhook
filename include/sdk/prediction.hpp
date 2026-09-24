#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"

class IClientEntity;
class CUserCmd;
class CMoveData;
class IMoveHelper;
class Vector;
class QAngle;

class CPrediction
{
public:
    void Update(int startframe, bool validframe, int incoming_acknowledged, int outgoing_command)
    {
        vfunc<void (*)(CPrediction *, int, bool, int, int)>(this, vtables::prediction::update)(this, startframe, validframe, incoming_acknowledged, outgoing_command);
    }
    void SetupMove(IClientEntity *player, CUserCmd *cmd, IMoveHelper *helper, CMoveData *move)
    {
        vfunc<void (*)(CPrediction *, IClientEntity *, CUserCmd *, IMoveHelper *, CMoveData *)>(this, vtables::prediction::setup_move)(this, player, cmd, helper, move);
    }
    void FinishMove(IClientEntity *player, CUserCmd *cmd, CMoveData *move)
    {
        vfunc<void (*)(CPrediction *, IClientEntity *, CUserCmd *, CMoveData *)>(this, vtables::prediction::finish_move)(this, player, cmd, move);
    }
    bool InPrediction()
    {
        return vfunc<bool (*)(CPrediction *)>(this, vtables::prediction::in_prediction)(this);
    }
    bool IsFirstTimePredicted()
    {
        return vfunc<bool (*)(CPrediction *)>(this, vtables::prediction::is_first_time_predicted)(this);
    }
};
