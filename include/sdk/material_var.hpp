#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "materialsystem/imaterialvar.h"

class CMaterialVar
{
public:
    void SetIntValue(int value)
    {
        vfunc<void (*)(CMaterialVar *, int)>(this, vtables::material_var::set_int_value)(this, value);
    }
    void SetVecValue(float x, float y, float z)
    {
        vfunc<void (*)(CMaterialVar *, float, float, float)>(this, vtables::material_var::set_vec_value_xyz)(this, x, y, z);
    }
};

inline CMaterialVar *LiveMaterialVar(IMaterialVar *var)
{
    return reinterpret_cast<CMaterialVar *>(var);
}
