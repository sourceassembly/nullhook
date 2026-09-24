#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"

struct model_t;
struct studiohdr_t;
struct vcollide_t;

class CModelInfoClient
{
public:
    const char *GetModelName(const model_t *model) const
    {
        return vfunc<const char *(*)(const CModelInfoClient *, const model_t *)>(this, vtables::model_info::get_model_name)(this, model);
    }
    vcollide_t *GetVCollide(const model_t *model)
    {
        return vfunc<vcollide_t *(*)(CModelInfoClient *, const model_t *)>(this, vtables::model_info::get_vcollide)(this, model);
    }
    studiohdr_t *GetStudiomodel(const model_t *model)
    {
        return vfunc<studiohdr_t *(*)(CModelInfoClient *, const model_t *)>(this, vtables::model_info::get_studiomodel)(this, model);
    }
};
