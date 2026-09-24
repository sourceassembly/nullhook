#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "materialsystem/imaterialsystem.h"
#include "sdk/material.hpp"
#include "sdk/texture.hpp"

class CMatRenderContext
{
public:
    int AddRef()
    {
        return vfunc<int (*)(CMatRenderContext *)>(this, vtables::mat_render_context::add_ref)(this);
    }
    int Release()
    {
        return vfunc<int (*)(CMatRenderContext *)>(this, vtables::mat_render_context::release)(this);
    }
    void BeginRender()
    {
        vfunc<void (*)(CMatRenderContext *)>(this, vtables::mat_render_context::begin_render)(this);
    }
    void EndRender()
    {
        vfunc<void (*)(CMatRenderContext *)>(this, vtables::mat_render_context::end_render)(this);
    }
    void SetRenderTarget(CTexture *texture)
    {
        vfunc<void (*)(CMatRenderContext *, CTexture *)>(this, vtables::mat_render_context::set_render_target)(this, texture);
    }
    void SetRenderTarget(ITexture *texture)
    {
        SetRenderTarget(LiveTexture(texture));
    }
    CTexture *GetRenderTarget()
    {
        return vfunc<CTexture *(*)(CMatRenderContext *)>(this, vtables::mat_render_context::get_render_target)(this);
    }
    void DepthRange(float zNear, float zFar)
    {
        vfunc<void (*)(CMatRenderContext *, float, float)>(this, vtables::mat_render_context::depth_range)(this, zNear, zFar);
    }
    void ClearBuffers(bool clearColor, bool clearDepth, bool clearStencil = false)
    {
        vfunc<void (*)(CMatRenderContext *, bool, bool, bool)>(this, vtables::mat_render_context::clear_buffers)(this, clearColor, clearDepth, clearStencil);
    }
    void Viewport(int x, int y, int width, int height)
    {
        vfunc<void (*)(CMatRenderContext *, int, int, int, int)>(this, vtables::mat_render_context::viewport)(this, x, y, width, height);
    }
    void ClearColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
    {
        vfunc<void (*)(CMatRenderContext *, unsigned char, unsigned char, unsigned char, unsigned char)>(this, vtables::mat_render_context::clear_color_4ub)(this, r, g, b, a);
    }
    void DrawScreenSpaceRectangle(CMaterial *material, int destx, int desty, int width, int height, float src_texture_x0, float src_texture_y0, float src_texture_x1, float src_texture_y1, int src_texture_width, int src_texture_height, void *client_renderable = nullptr, int x_dice = 1, int y_dice = 1)
    {
        using fn_t = void (*)(CMatRenderContext *, CMaterial *, int, int, int, int, float, float, float, float, int, int, void *, int, int);
        vfunc<fn_t>(this, vtables::mat_render_context::draw_screen_space_rectangle)(this, material, destx, desty, width, height, src_texture_x0, src_texture_y0, src_texture_x1, src_texture_y1, src_texture_width, src_texture_height, client_renderable, x_dice, y_dice);
    }
    void DrawScreenSpaceRectangle(IMaterial *material, int destx, int desty, int width, int height, float src_texture_x0, float src_texture_y0, float src_texture_x1, float src_texture_y1, int src_texture_width, int src_texture_height, void *client_renderable = nullptr, int x_dice = 1, int y_dice = 1)
    {
        DrawScreenSpaceRectangle(LiveMaterial(material), destx, desty, width, height, src_texture_x0, src_texture_y0, src_texture_x1, src_texture_y1, src_texture_width, src_texture_height, client_renderable, x_dice, y_dice);
    }
    void PushRenderTargetAndViewport()
    {
        vfunc<void (*)(CMatRenderContext *)>(this, vtables::mat_render_context::push_render_target_and_viewport)(this);
    }
    void PopRenderTargetAndViewport()
    {
        vfunc<void (*)(CMatRenderContext *)>(this, vtables::mat_render_context::pop_render_target_and_viewport)(this);
    }
    void SetStencilEnable(bool on)
    {
        vfunc<void (*)(CMatRenderContext *, bool)>(this, vtables::mat_render_context::set_stencil_enable)(this, on);
    }
    void SetStencilFailOperation(StencilOperation_t op)
    {
        vfunc<void (*)(CMatRenderContext *, StencilOperation_t)>(this, vtables::mat_render_context::set_stencil_fail_operation)(this, op);
    }
    void SetStencilZFailOperation(StencilOperation_t op)
    {
        vfunc<void (*)(CMatRenderContext *, StencilOperation_t)>(this, vtables::mat_render_context::set_stencil_zfail_operation)(this, op);
    }
    void SetStencilPassOperation(StencilOperation_t op)
    {
        vfunc<void (*)(CMatRenderContext *, StencilOperation_t)>(this, vtables::mat_render_context::set_stencil_pass_operation)(this, op);
    }
    void SetStencilCompareFunction(StencilComparisonFunction_t fn)
    {
        vfunc<void (*)(CMatRenderContext *, StencilComparisonFunction_t)>(this, vtables::mat_render_context::set_stencil_compare_function)(this, fn);
    }
    void SetStencilReferenceValue(int value)
    {
        vfunc<void (*)(CMatRenderContext *, int)>(this, vtables::mat_render_context::set_stencil_reference_value)(this, value);
    }
    void SetStencilTestMask(uint32 mask)
    {
        vfunc<void (*)(CMatRenderContext *, uint32)>(this, vtables::mat_render_context::set_stencil_test_mask)(this, mask);
    }
    void SetStencilWriteMask(uint32 mask)
    {
        vfunc<void (*)(CMatRenderContext *, uint32)>(this, vtables::mat_render_context::set_stencil_write_mask)(this, mask);
    }
    void ClearStencilBufferRectangle(int xmin, int ymin, int xmax, int ymax, int value)
    {
        vfunc<void (*)(CMatRenderContext *, int, int, int, int, int)>(this, vtables::mat_render_context::clear_stencil_buffer_rectangle)(this, xmin, ymin, xmax, ymax, value);
    }
    void OverrideAlphaWriteEnable(bool enable, bool alphaWriteEnable)
    {
        vfunc<void (*)(CMatRenderContext *, bool, bool)>(this, vtables::mat_render_context::override_alpha_write_enable)(this, enable, alphaWriteEnable);
    }
};

class MatRenderScope
{
    CMatRenderContext *ctx;

public:
    explicit MatRenderScope(CMatRenderContext *c) : ctx(c)
    {
        if (ctx)
            ctx->BeginRender();
    }
    ~MatRenderScope()
    {
        if (ctx)
        {
            ctx->EndRender();
            ctx->Release();
        }
    }
    CMatRenderContext *operator->() const
    {
        return ctx;
    }
    CMatRenderContext *get() const
    {
        return ctx;
    }
};
