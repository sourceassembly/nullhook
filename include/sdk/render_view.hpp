#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"

class CViewSetup;
class VMatrix;

class CRenderView
{
public:
    void SetBlend(float blend)
    {
        vfunc<void (*)(CRenderView *, float)>(this, vtables::render_view::set_blend)(this, blend);
    }
    float GetBlend()
    {
        return vfunc<float (*)(CRenderView *)>(this, vtables::render_view::get_blend)(this);
    }
    void SetColorModulation(float const *blend)
    {
        vfunc<void (*)(CRenderView *, float const *)>(this, vtables::render_view::set_color_modulation)(this, blend);
    }
    void GetColorModulation(float *blend)
    {
        vfunc<void (*)(CRenderView *, float *)>(this, vtables::render_view::get_color_modulation)(this, blend);
    }
    void GetMatricesForView(const CViewSetup &view, VMatrix *worldToView, VMatrix *viewToProjection, VMatrix *worldToProjection, VMatrix *worldToPixels)
    {
        using fn_t = void (*)(CRenderView *, const CViewSetup &, VMatrix *, VMatrix *, VMatrix *, VMatrix *);
        vfunc<fn_t>(this, vtables::render_view::get_matrices_for_view)(this, view, worldToView, viewToProjection, worldToProjection, worldToPixels);
    }
};
