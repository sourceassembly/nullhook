#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "engine/ivmodelrender.h"

class IMaterial;

class CModelRender
{
public:
    void ForcedMaterialOverride(IMaterial *material, OverrideType_t type = OVERRIDE_NORMAL)
    {
        vfunc<void (*)(CModelRender *, IMaterial *, OverrideType_t)>(this, vtables::model_render::forced_material_override)(this, material, type);
    }
};
