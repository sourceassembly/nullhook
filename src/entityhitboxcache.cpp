/*
 * entityhitboxcache.cpp
 *
 *  Created on: Dec 23, 2016
 *      Author: nullifiedcat
 */

#include <settings/Int.hpp>
#include "common.hpp"
#include "MiscTemporary.hpp"
#include "SetupBonesReconst.hpp"

namespace hitbox_cache
{

void EntityHitboxCache::Init()
{
    model_t *model;
    studiohdr_t *shdr;
    mstudiohitboxset_t *set;
    m_bInit    = true;
    parent_ref = entity_cache::Get(hit_idx);
    if (CE_BAD(parent_ref))
        return;
    model = (model_t *) EntGetModel(RAW_ENT(parent_ref));
    if (!model)
        return;
    // NB: the hitbox set index can change while the model stays the same, and
    // studiohdr_t::pHitboxSet() does not range-check (assert-only), so validate
    // the index explicitly before trusting the returned pointer.
    const int hitbox_set = CE_INT(parent_ref, netvar.iHitboxSet);
    if (!m_bModelSet || model != m_pLastModel || hitbox_set != m_iLastHitboxSet)
    {
        shdr = g_IModelInfo->GetStudiomodel(model);
        if (!shdr)
            return;
        if (hitbox_set < 0 || hitbox_set >= shdr->numhitboxsets)
            return;
        set = shdr->pHitboxSet(hitbox_set);
        if (!set)
            return;
        m_pLastModel     = model;
        m_iLastHitboxSet = hitbox_set;
        m_nNumHitboxes   = 0;
        if (set)
        {
            m_nNumHitboxes = set->numhitboxes;
        }
        if (m_nNumHitboxes > CACHE_MAX_HITBOXES)
            m_nNumHitboxes = CACHE_MAX_HITBOXES;
        m_bModelSet = true;
    }
    m_bSuccess = true;
}

bool EntityHitboxCache::VisibilityCheck(int id)
{
    CachedHitbox *hitbox;

    if (!m_bInit)
        Init();
    if (id < 0 || id >= m_nNumHitboxes || id >= 64)
        return false;
    if (!m_bSuccess)
        return false;
    if ((m_VisCheckValidationFlags >> id) & 1)
        return (m_VisCheck >> id) & 1;
    // TODO corners
    hitbox = GetHitbox(id);
    if (!hitbox)
        return false;
    bool validation = (IsEntityVectorVisible(parent_ref, hitbox->center, true));
    // Bitmask works sort of like an index in our case. 1 would be the first bit, and we are shifting this by id to get our index
    uint_fast64_t mask = 1ULL << id;
    // No branch conditional set https://graphics.stanford.edu/~seander/bithacks.html#ConditionalSetOrClearBitsWithoutBranching
    m_VisCheck = (m_VisCheck & ~mask) | (-validation & mask);
    m_VisCheckValidationFlags |= 1ULL << id;
    return (m_VisCheck >> id) & 1;
}

static settings::Int setupbones_time{ "source.setupbones-time", "3" };

void EntityHitboxCache::UpdateBones()
{
    // Do not run for bad ents/non player ents
    if (!m_bInit)
        Init();
    auto bone_ptr = GetBones();
    if (!bone_ptr || bones.empty())
        return;

    // Thanks to the epic doghook developers (mainly F1ssion and MrSteyk)
    // I do not have to find all of these signatures and dig through ida

    struct BoneCache;
    typedef BoneCache *(*GetBoneCache_t)(uintptr_t);
    typedef void (*BoneCacheUpdateBones_t)(BoneCache *, matrix3x4_t * bones, unsigned, float time);
    static auto bone_handle_insn                = gSignatures.GetClientSignature(sigs::base_animating_bone_handle);
    static auto hitbox_bone_cache_handle_offset = bone_handle_insn ? *(unsigned *) (bone_handle_insn + 24) : 0u;
    static auto studio_get_bone_cache           = (GetBoneCache_t) gSignatures.GetClientSignature(sigs::studio_get_bone_cache);
    static auto bone_cache_update_bones         = (BoneCacheUpdateBones_t) gSignatures.GetClientSignature(sigs::studio_bone_cache_update_bones);

    // Sanity-check the signature-derived offset: a stale signature must not turn
    // into an arbitrary out-of-bounds entity read.
    if (!hitbox_bone_cache_handle_offset || hitbox_bone_cache_handle_offset > 0x10000 || !studio_get_bone_cache || !bone_cache_update_bones)
        return;
    auto hitbox_bone_cache_handle = CE_VAR(parent_ref, hitbox_bone_cache_handle_offset, uintptr_t);
    if (hitbox_bone_cache_handle)
    {
        BoneCache *bone_cache = studio_get_bone_cache(hitbox_bone_cache_handle);
        if (bone_cache && !bones.empty())
            bone_cache_update_bones(bone_cache, bones.data(), bones.size(), g_GlobalVars->curtime);
    }

    // Mark for update
    /*int *entity_flags = (int *) ((uintptr_t) RAW_ENT(parent_ref) + 400);
    // (EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS | EFL_DIRTY_SPATIAL_PARTITION)
    *entity_flags |= (1 << 14) | (1 << 15);*/
}

matrix3x4_t *EntityHitboxCache::GetBones(int numbones)
{
    static float bones_setup_time = 0.0f;
    switch (*setupbones_time)
    {
    case 0:
        bones_setup_time = 0.0f;
        break;
    case 1:
        bones_setup_time = g_GlobalVars->curtime;
        break;
    case 2:
        if (CE_GOOD(LOCAL_E))
            bones_setup_time = g_GlobalVars->interval_per_tick * CE_INT(LOCAL_E, netvar.nTickBase);
        break;
    case 3:
        if (CE_GOOD(parent_ref))
            bones_setup_time = CE_FLOAT(parent_ref, netvar.m_flSimulationTime);
        break;
    default:
        bones_setup_time = g_GlobalVars->curtime;
        break;
    }
    if (!bones_setup)
    {
        // If numbones is not set, get it from some terrible and unnamed variable
        if (numbones == -1)
        {
            numbones = MAXSTUDIOBONES;
            if (CE_GOOD(parent_ref))
            {
                auto *mdl = (const model_t *) EntGetModel(RAW_ENT(parent_ref));
                if (mdl)
                {
                    auto *hdr = g_IModelInfo->GetStudiomodel(mdl);
                    if (hdr && hdr->numbones > 0 && hdr->numbones <= 128)
                        numbones = hdr->numbones;
                }
            }
        }

        if (numbones <= 0 || numbones > 128)
            numbones = MAXSTUDIOBONES > 128 ? 128 : MAXSTUDIOBONES;
        if (bones.size() != (size_t) numbones)
            bones.resize(numbones);
        if (g_Settings.is_create_move)
        {
            PROF_SECTION(bone_setup);
            IClientEntity *raw = RAW_ENT(parent_ref);
            int overlay        = netvar.m_AnimOverlay;
            bool overlay_ok    = true;
            if (overlay > 0 && raw)
            {
                auto *mem  = *reinterpret_cast<void **>(uintptr_t(raw) + overlay);
                int n      = *reinterpret_cast<int *>(uintptr_t(raw) + overlay + 16);
                overlay_ok = mem && n >= 0 && n <= 15;
            }
            if (raw)
            {
                const model_t *mdl = EntGetModel(raw);
                studiohdr_t *shdr  = (mdl && g_IModelInfo) ? g_IModelInfo->GetStudiomodel(mdl) : nullptr;
                // Models with include-models go through the reconstruction: it only
                // needs the studiohdr, while engine SetupBones can fail when merged
                // wearables are unavailable (dormant/textmode entities).
                if (shdr && shdr->numincludemodels > 0)
                    bones_setup = setupbones_reconst::SetupBones(raw, bones.data(), 0x7FF00, bones_setup_time);
                else if (shdr)
                {
                    if (overlay_ok)
                        bones_setup = EntSetupBones(raw, bones.data(), numbones, 0x7FF00, bones_setup_time);
                    else
                        // No anim layers allocated yet (e.g. just spawned): the
                        // reconstruction tolerates missing layers, so use it instead
                        // of leaving zero matrices behind.
                        bones_setup = setupbones_reconst::SetupBones(raw, bones.data(), 0x7FF00, bones_setup_time);
                }
            }
        }
    }
    return bones.data();
}

CachedHitbox *EntityHitboxCache::GetHitbox(int id)
{
    // Validate before any bit shift: negative or huge ids are UB for >>/<<.
    if (id < 0 || id >= 64)
        return nullptr;
    if ((m_CacheValidationFlags >> id) & 1)
    {
        if (id >= (int) m_CacheInternal.size())
            return nullptr;
        return &m_CacheInternal[id];
    }
    mstudiobbox_t *box;

    if (!m_bInit)
        Init();
    if (id < 0 || id >= m_nNumHitboxes)
        return nullptr;
    if (!m_bSuccess)
        return nullptr;
    if (CE_BAD(parent_ref))
        return nullptr;
    auto model = (const model_t *) EntGetModel(RAW_ENT(parent_ref));
    if (!model)
        return nullptr;
    auto shdr = g_IModelInfo->GetStudiomodel(model);
    if (!shdr)
        return nullptr;
    const int hitbox_set = CE_INT(parent_ref, netvar.iHitboxSet);
    if (hitbox_set < 0 || hitbox_set >= shdr->numhitboxsets)
        return nullptr;
    auto set = shdr->pHitboxSet(hitbox_set);
    if (!set)
        return nullptr;
    if (id >= set->numhitboxes)
        return nullptr;
    if (m_nNumHitboxes > (int) m_CacheInternal.size())
        m_CacheInternal.resize(m_nNumHitboxes);
    box = set->pHitbox(id);
    if (!box)
        return nullptr;
    if (box->bone < 0 || box->bone >= MAXSTUDIOBONES)
        return nullptr;
    matrix3x4_t *bone_mats = GetBones(shdr->numbones);
    // Never serve hitboxes transformed by missing/failed bones (zero matrices
    // would collapse every hitbox to the world origin).
    if (!bones_setup || !bone_mats || box->bone >= (int) bones.size())
        return nullptr;
    VectorTransform(box->bbmin, bone_mats[box->bone], m_CacheInternal[id].min);
    VectorTransform(box->bbmax, bone_mats[box->bone], m_CacheInternal[id].max);
    m_CacheInternal[id].bbox   = box;
    m_CacheInternal[id].center = (m_CacheInternal[id].min + m_CacheInternal[id].max) / 2;
    m_CacheValidationFlags |= 1ULL << id;
    return &m_CacheInternal[id];
}

} // namespace hitbox_cache
