#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "materialsystem/itexture.h"

class CTexture
{
public:
    int GetActualWidth() const
    {
        return vfunc<int (*)(const CTexture *)>(this, vtables::texture::get_actual_width)(this);
    }
    int GetActualHeight() const
    {
        return vfunc<int (*)(const CTexture *)>(this, vtables::texture::get_actual_height)(this);
    }
    void IncrementReferenceCount()
    {
        vfunc<void (*)(CTexture *)>(this, vtables::texture::increment_ref)(this);
    }
    void DecrementReferenceCount()
    {
        vfunc<void (*)(CTexture *)>(this, vtables::texture::decrement_ref)(this);
    }
    bool IsError() const
    {
        return vfunc<bool (*)(const CTexture *)>(this, vtables::texture::is_error)(this);
    }
    ITexture *AsITexture()
    {
        return reinterpret_cast<ITexture *>(this);
    }
};

inline CTexture *LiveTexture(ITexture *texture)
{
    return reinterpret_cast<CTexture *>(texture);
}
