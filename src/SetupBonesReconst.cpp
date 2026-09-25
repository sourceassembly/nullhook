#include "common.hpp"
#include "MiscTemporary.hpp"
#include "bone_setup.h"
#include "animationlayer.h"
#include "sdk/client_entity.hpp"

static_assert(sizeof(C_AnimationLayer) == 0x2C, "TF2 linux64 C_AnimationLayer is 0x2C");

namespace setupbones_reconst
{
#define MAX_OVERLAYS 15

static settings::Boolean remove_taunts("remove.taunts", "false");

// Sequence indices are per-model, so a global id blocklist cannot identify taunts.
// Match the sequence label instead (TF2 taunts use the "taunt" prefix).
static bool IsTauntSequence(CStudioHdr *hdr, int sequence)
{
    if (!hdr || sequence < 0 || sequence >= hdr->GetNumSeq())
        return false;
    const char *label = hdr->pSeqdesc(sequence).pszLabel();
    if (!label || !label[0])
        return false;
    for (const char *p = label; p[0] && p[1] && p[2] && p[3] && p[4]; ++p)
    {
        if ((p[0] == 't' || p[0] == 'T') && (p[1] == 'a' || p[1] == 'A') && (p[2] == 'u' || p[2] == 'U') && (p[3] == 'n' || p[3] == 'N') && (p[4] == 't' || p[4] == 'T'))
            return true;
    }
    return false;
}

static Vector PlayerOrigin(IClientEntity *ent)
{
    const Vector abs = re::C_BaseEntity::GetAbsOrigin(ent);
    Vector net{};
    if (netvar.m_vecOrigin)
        net = NET_VECTOR(ent, netvar.m_vecOrigin);
    if (!nolerp && !abs.IsZero())
        return abs;
    if (!net.IsZero())
        return net;
    return abs;
}

static QAngle PlayerAngles(IClientEntity *ent)
{
    using fn_t = const QAngle &(*)(IClientEntity *);
    return vfunc<fn_t>(ent, vtables::entity::get_render_angles, 0)(ent);
}

static const float *PoseParameters(IClientEntity *ent)
{
    static float dummy[MAXSTUDIOPOSEPARAM];
    if (netvar.m_flPoseParameter)
        return &NET_FLOAT(ent, netvar.m_flPoseParameter);
    return dummy;
}

void GetSkeleton(IClientEntity *ent, CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], int boneMask, float time)
{
    if (!pStudioHdr)
        return;

    IBoneSetup boneSetup(pStudioHdr, boneMask, PoseParameters(ent));
    boneSetup.InitPose(pos, q);

    if (!pStudioHdr->SequencesAvailable())
        return;

    const int sequence = NET_INT(ent, netvar.m_nSequence);
    if (sequence >= 0 && sequence < pStudioHdr->GetNumSeq())
        boneSetup.AccumulatePose(pos, q, sequence, NET_FLOAT(ent, netvar.m_flCycle), 1.0, time, nullptr);

    int overlay_count = 0;
    C_AnimationLayer *layers = nullptr;
    if (netvar.m_AnimOverlay)
    {
        layers        = *reinterpret_cast<C_AnimationLayer **>(uintptr_t(ent) + netvar.m_AnimOverlay);
        overlay_count = *reinterpret_cast<int *>(uintptr_t(ent) + netvar.m_AnimOverlay + 16);
        if (!layers || overlay_count < 0 || overlay_count > MAX_OVERLAYS)
        {
            layers        = nullptr;
            overlay_count = 0;
        }
    }

    int layer[MAX_OVERLAYS];
    int i;
    for (i = 0; i < MAX_OVERLAYS; i++)
        layer[i] = MAX_OVERLAYS;
    for (i = 0; i < overlay_count; i++)
    {
        C_AnimationLayer &pLayer = layers[i];
        const int order          = pLayer.m_nOrder;
        if (order >= 0 && order < MAX_OVERLAYS && layer[order] == MAX_OVERLAYS)
            layer[order] = i;
    }
    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        if (layer[i] < 0 || layer[i] >= overlay_count)
            continue;
        C_AnimationLayer pLayer = layers[layer[i]];
        if (pLayer.m_flWeight <= 0)
            continue;
        const int layer_seq = pLayer.m_nSequence;
        if (layer_seq < 0 || layer_seq >= pStudioHdr->GetNumSeq())
            continue;
        if (!remove_taunts || !IsTauntSequence(pStudioHdr, layer_seq))
            boneSetup.AccumulatePose(pos, q, layer_seq, pLayer.m_flCycle, pLayer.m_flWeight, time, nullptr);
    }

    CIKContext auto_ik;
    auto_ik.Init(pStudioHdr, PlayerAngles(ent), PlayerOrigin(ent), time, 0, boneMask);
    boneSetup.CalcAutoplaySequences(pos, q, time, &auto_ik);

    if (netvar.m_flEncodedController)
        boneSetup.CalcBoneAdj(pos, q, &NET_FLOAT(ent, netvar.m_flEncodedController));
}

bool SetupBones(IClientEntity *ent, matrix3x4_t *pBoneToWorld, int boneMask, float time)
{
    if (!ent || !pBoneToWorld || !g_IModelInfo || !g_IMDLCache)
        return false;

    const model_t *model = EntGetModel(ent);
    if (!model)
        return false;
    studiohdr_t *raw = g_IModelInfo->GetStudiomodel(model);
    if (!raw)
        return false;

    CStudioHdr studioHdr(raw, g_IMDLCache);
    if (!studioHdr.IsValid())
        return false;

    Vector pos[MAXSTUDIOBONES];
    Quaternion q[MAXSTUDIOBONES];

    const Vector adjOrigin = PlayerOrigin(ent);
    const QAngle angles2   = PlayerAngles(ent);

    GetSkeleton(ent, &studioHdr, pos, q, boneMask, time);

    float scale = 1.0f;
    if (netvar.m_flModelScale)
        scale = NET_FLOAT(ent, netvar.m_flModelScale);
    if (scale <= 0.0f)
        scale = 1.0f;

    Studio_BuildMatrices(&studioHdr, angles2, adjOrigin, pos, q, -1, scale, pBoneToWorld, boneMask);
    return true;
}
} // namespace setupbones_reconst
