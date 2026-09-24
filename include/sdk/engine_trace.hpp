#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "cmodel.h"

class ITraceFilter;
class IHandleEntity;
class CGameTrace;
typedef CGameTrace trace_t;

static_assert(offsetof(Ray_t, m_IsRay) == 0x40, "Ray_t m_IsRay must match linux64 TF2 (no world-axis transform pointer)");
static_assert(sizeof(Ray_t) == 0x50, "Ray_t must be 0x50 on linux64 TF2");

class CEngineTrace
{
public:
    int GetPointContents(const Vector &abs_position, IHandleEntity **hit_entity = nullptr)
    {
        return vfunc<int (*)(CEngineTrace *, const Vector &, IHandleEntity **)>(this, vtables::engine_trace::get_point_contents)(this, abs_position, hit_entity);
    }
    void TraceRay(const Ray_t &ray, unsigned int mask, ITraceFilter *filter, trace_t *trace)
    {
        vfunc<void (*)(CEngineTrace *, const Ray_t &, unsigned int, ITraceFilter *, trace_t *)>(this, vtables::engine_trace::trace_ray)(this, ray, mask, filter, trace);
    }
};
