#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "mathlib/vector.h"
#include <basehandle.h>

class IClientEntity;
class ClientClass;
class ICollideable;
struct model_t;
struct matrix3x4_t;

inline void *EntRenderable(IClientEntity *entity)
{
    return entity ? reinterpret_cast<void *>(uintptr_t(entity) + vtables::renderable::vptr_offset) : nullptr;
}

inline const Vector &EntGetRenderOrigin(IClientEntity *entity)
{
    void *renderable = EntRenderable(entity);
    return vfunc<const Vector &(*)(void *)>(renderable, vtables::renderable::get_render_origin)(renderable);
}

inline void *EntNetworkable(IClientEntity *entity)
{
    return entity ? reinterpret_cast<void *>(uintptr_t(entity) + vtables::networkable::vptr_offset) : nullptr;
}

inline ClientClass *EntClientClass(IClientEntity *entity)
{
    void *networkable = EntNetworkable(entity);
    if (!networkable)
        return nullptr;
    return vfunc<ClientClass *(*)(void *)>(networkable, vtables::networkable::get_client_class)(networkable);
}

inline bool EntIsDormant(IClientEntity *entity)
{
    void *networkable = EntNetworkable(entity);
    if (!networkable)
        return true;
    return vfunc<bool (*)(void *)>(networkable, vtables::networkable::is_dormant)(networkable);
}

inline int EntIndex(IClientEntity *entity)
{
    void *networkable = EntNetworkable(entity);
    if (!networkable)
        return -1;
    return vfunc<int (*)(void *)>(networkable, vtables::networkable::entindex)(networkable);
}

inline ICollideable *EntCollideable(IClientEntity *entity)
{
    if (!entity)
        return nullptr;
    return vfunc<ICollideable *(*)(IClientEntity *)>(entity, vtables::entity::get_collideable)(entity);
}

inline const Vector &EntOBBMins(IClientEntity *entity)
{
    static Vector empty;
    ICollideable *collideable = EntCollideable(entity);
    if (!collideable)
        return empty;
    return vfunc<const Vector &(*)(ICollideable *)>(collideable, vtables::collideable::obb_mins)(collideable);
}

inline const Vector &EntOBBMaxs(IClientEntity *entity)
{
    static Vector empty;
    ICollideable *collideable = EntCollideable(entity);
    if (!collideable)
        return empty;
    return vfunc<const Vector &(*)(ICollideable *)>(collideable, vtables::collideable::obb_maxs)(collideable);
}

inline const Vector &EntCollisionOrigin(IClientEntity *entity)
{
    static Vector empty;
    ICollideable *collideable = EntCollideable(entity);
    if (!collideable)
        return empty;
    return vfunc<const Vector &(*)(ICollideable *)>(collideable, vtables::collideable::get_collision_origin)(collideable);
}

inline const model_t *EntGetModel(IClientEntity *entity)
{
    void *renderable = EntRenderable(entity);
    if (!renderable)
        return nullptr;
    return vfunc<const model_t *(*)(void *)>(renderable, vtables::renderable::get_model)(renderable);
}

inline bool EntSetupBones(IClientEntity *entity, matrix3x4_t *out, int max_bones, int bone_mask, float time)
{
    void *renderable = EntRenderable(entity);
    if (!entity || !renderable || !out || max_bones <= 0 || max_bones > 128)
        return false;
    return vfunc<bool (*)(void *, matrix3x4_t *, int, int, float)>(renderable, vtables::renderable::setup_bones)(renderable, out, max_bones, bone_mask, time);
}

inline void EntGetRenderBounds(IClientEntity *entity, Vector &mins, Vector &maxs)
{
    void *renderable = EntRenderable(entity);
    if (!renderable)
    {
        mins.Init();
        maxs.Init();
        return;
    }
    vfunc<void (*)(void *, Vector &, Vector &)>(renderable, vtables::renderable::get_render_bounds)(renderable, mins, maxs);
}

inline IClientEntity *EntFromRenderable(void *renderable)
{
    return renderable ? reinterpret_cast<IClientEntity *>(uintptr_t(renderable) - vtables::renderable::vptr_offset) : nullptr;
}

inline const CBaseHandle &EntRefEHandle(IClientEntity *entity)
{
    static CBaseHandle empty;
    if (!entity)
        return empty;
    return vfunc<const CBaseHandle &(*)(IClientEntity *)>(entity, vtables::entity::get_ref_ehandle)(entity);
}

inline bool EntRenderableShouldDraw(IClientEntity *entity)
{
    void *renderable = EntRenderable(entity);
    return renderable && vfunc<bool (*)(void *)>(renderable, vtables::renderable::should_draw)(renderable);
}

inline int EntDrawModel(IClientEntity *entity, int flags)
{
    void *renderable = EntRenderable(entity);
    if (!renderable)
        return 0;
    return vfunc<int (*)(void *, int)>(renderable, vtables::renderable::draw_model)(renderable, flags);
}

inline int EntLookupAttachment(IClientEntity *entity, const char *name)
{
    void *renderable = EntRenderable(entity);
    if (!renderable || !name)
        return 0;
    return vfunc<int (*)(void *, const char *)>(renderable, vtables::renderable::lookup_attachment)(renderable, name);
}

inline bool EntGetAttachment(IClientEntity *entity, int number, Vector &origin, QAngle &angles)
{
    void *renderable = EntRenderable(entity);
    if (!renderable)
        return false;
    return vfunc<bool (*)(void *, int, Vector &, QAngle &)>(renderable, vtables::renderable::get_attachment)(renderable, number, origin, angles);
}
