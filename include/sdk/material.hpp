#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "materialsystem/imaterial.h"
#include "sdk/material_var.hpp"

class KeyValues;

class CMaterial
{
public:
    const char *GetName() const
    {
        return vfunc<const char *(*)(const CMaterial *)>(this, vtables::material::get_name)(this);
    }
    const char *GetTextureGroupName() const
    {
        return vfunc<const char *(*)(const CMaterial *)>(this, vtables::material::get_texture_group_name)(this);
    }
    CMaterialVar *FindVar(const char *varName, bool *found, bool complain = true)
    {
        return vfunc<CMaterialVar *(*)(CMaterial *, const char *, bool *, bool)>(this, vtables::material::find_var)(this, varName, found, complain);
    }
    void IncrementReferenceCount()
    {
        vfunc<void (*)(CMaterial *)>(this, vtables::material::increment_reference_count)(this);
    }
    void DecrementReferenceCount()
    {
        vfunc<void (*)(CMaterial *)>(this, vtables::material::decrement_reference_count)(this);
    }
    bool IsTranslucent()
    {
        return vfunc<bool (*)(CMaterial *)>(this, vtables::material::is_translucent)(this);
    }
    void AlphaModulate(float alpha)
    {
        vfunc<void (*)(CMaterial *, float)>(this, vtables::material::alpha_modulate)(this, alpha);
    }
    void ColorModulate(float r, float g, float b)
    {
        vfunc<void (*)(CMaterial *, float, float, float)>(this, vtables::material::color_modulate)(this, r, g, b);
    }
    void SetMaterialVarFlag(MaterialVarFlags_t flag, bool on)
    {
        vfunc<void (*)(CMaterial *, MaterialVarFlags_t, bool)>(this, vtables::material::set_material_var_flag)(this, flag, on);
    }
    bool GetMaterialVarFlag(MaterialVarFlags_t flag) const
    {
        return vfunc<bool (*)(const CMaterial *, MaterialVarFlags_t)>(this, vtables::material::get_material_var_flag)(this, flag);
    }
    void Refresh()
    {
        vfunc<void (*)(CMaterial *)>(this, vtables::material::refresh)(this);
    }
    bool IsErrorMaterial() const
    {
        return vfunc<bool (*)(const CMaterial *)>(this, vtables::material::is_error_material)(this);
    }
    float GetAlphaModulation()
    {
        return vfunc<float (*)(CMaterial *)>(this, vtables::material::get_alpha_modulation)(this);
    }
    void GetColorModulation(float *r, float *g, float *b)
    {
        vfunc<void (*)(CMaterial *, float *, float *, float *)>(this, vtables::material::get_color_modulation)(this, r, g, b);
    }
    void SetShaderAndParams(KeyValues *keyValues)
    {
        vfunc<void (*)(CMaterial *, KeyValues *)>(this, vtables::material::set_shader_and_params)(this, keyValues);
    }
    bool IsSpriteCard()
    {
        return vfunc<bool (*)(CMaterial *)>(this, vtables::material::is_sprite_card)(this);
    }
    bool IsPrecached() const
    {
        return vfunc<bool (*)(const CMaterial *)>(this, vtables::material::is_precached)(this);
    }
    IMaterial *AsIMaterial()
    {
        return reinterpret_cast<IMaterial *>(this);
    }
};

inline CMaterial *LiveMaterial(IMaterial *material)
{
    return reinterpret_cast<CMaterial *>(material);
}
