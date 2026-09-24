#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include <vgui/ISurface.h>

class CMatSystemSurface
{
public:
    void DrawSetColor(int r, int g, int b, int a)
    {
        vfunc<void (*)(CMatSystemSurface *, int, int, int, int)>(this, vtables::surface::draw_set_color)(this, r, g, b, a);
    }
    void DrawFilledRect(int x0, int y0, int x1, int y1)
    {
        vfunc<void (*)(CMatSystemSurface *, int, int, int, int)>(this, vtables::surface::draw_filled_rect)(this, x0, y0, x1, y1);
    }
    void DrawOutlinedRect(int x0, int y0, int x1, int y1)
    {
        vfunc<void (*)(CMatSystemSurface *, int, int, int, int)>(this, vtables::surface::draw_outlined_rect)(this, x0, y0, x1, y1);
    }
    void DrawLine(int x0, int y0, int x1, int y1)
    {
        vfunc<void (*)(CMatSystemSurface *, int, int, int, int)>(this, vtables::surface::draw_line)(this, x0, y0, x1, y1);
    }
    void DrawSetTextFont(vgui::HFont font)
    {
        vfunc<void (*)(CMatSystemSurface *, vgui::HFont)>(this, vtables::surface::draw_set_text_font)(this, font);
    }
    void DrawSetTextColor(int r, int g, int b, int a)
    {
        vfunc<void (*)(CMatSystemSurface *, int, int, int, int)>(this, vtables::surface::draw_set_text_color)(this, r, g, b, a);
    }
    void DrawSetTextPos(int x, int y)
    {
        vfunc<void (*)(CMatSystemSurface *, int, int)>(this, vtables::surface::draw_set_text_pos)(this, x, y);
    }
    void DrawPrintText(const wchar_t *text, int textLen, vgui::FontDrawType_t drawType = vgui::FONT_DRAW_DEFAULT)
    {
        vfunc<void (*)(CMatSystemSurface *, const wchar_t *, int, vgui::FontDrawType_t)>(this, vtables::surface::draw_print_text)(this, text, textLen, drawType);
    }
    void DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload)
    {
        using fn_t = void (*)(CMatSystemSurface *, int, const unsigned char *, int, int, int, bool);
        vfunc<fn_t>(this, vtables::surface::draw_set_texture_rgba)(this, id, rgba, wide, tall, hardwareFilter, forceReload);
    }
    void DrawSetTexture(int id)
    {
        vfunc<void (*)(CMatSystemSurface *, int)>(this, vtables::surface::draw_set_texture)(this, id);
    }
    bool IsTextureIDValid(int id)
    {
        return vfunc<bool (*)(CMatSystemSurface *, int)>(this, vtables::surface::is_texture_id_valid)(this, id);
    }
    bool DeleteTextureByID(int id)
    {
        return vfunc<bool (*)(CMatSystemSurface *, int)>(this, vtables::surface::delete_texture_by_id)(this, id);
    }
    int CreateNewTextureID(bool procedural = false)
    {
        return vfunc<int (*)(CMatSystemSurface *, bool)>(this, vtables::surface::create_new_texture_id)(this, procedural);
    }
    void GetScreenSize(int &wide, int &tall)
    {
        vfunc<void (*)(CMatSystemSurface *, int *, int *)>(this, vtables::surface::get_screen_size)(this, &wide, &tall);
    }
    void SetCursorAlwaysVisible(bool visible)
    {
        vfunc<void (*)(CMatSystemSurface *, bool)>(this, vtables::surface::set_cursor_always_visible)(this, visible);
    }
    void UnlockCursor()
    {
        vfunc<void (*)(CMatSystemSurface *)>(this, vtables::surface::unlock_cursor)(this);
    }
    void LockCursor()
    {
        vfunc<void (*)(CMatSystemSurface *)>(this, vtables::surface::lock_cursor)(this);
    }
    vgui::HFont CreateFont()
    {
        return vfunc<vgui::HFont (*)(CMatSystemSurface *)>(this, vtables::surface::create_font)(this);
    }
    bool SetFontGlyphSet(vgui::HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin = 0, int nRangeMax = 0)
    {
        using fn_t = bool (*)(CMatSystemSurface *, vgui::HFont, const char *, int, int, int, int, int, int, int);
        return vfunc<fn_t>(this, vtables::surface::set_font_glyph_set)(this, font, windowsFontName, tall, weight, blur, scanlines, flags, nRangeMin, nRangeMax);
    }
    bool AddCustomFontFile(const char *fontName, const char *fontFileName)
    {
        return vfunc<bool (*)(CMatSystemSurface *, const char *, const char *)>(this, vtables::surface::add_custom_font_file)(this, fontName, fontFileName);
    }
    void GetTextSize(vgui::HFont font, const wchar_t *text, int &wide, int &tall)
    {
        vfunc<void (*)(CMatSystemSurface *, vgui::HFont, const wchar_t *, int *, int *)>(this, vtables::surface::get_text_size)(this, font, text, &wide, &tall);
    }
    void PlaySound(const char *fileName)
    {
        vfunc<void (*)(CMatSystemSurface *, const char *)>(this, vtables::surface::play_sound)(this, fileName);
    }
    void DrawOutlinedCircle(int x, int y, int radius, int segments)
    {
        vfunc<void (*)(CMatSystemSurface *, int, int, int, int)>(this, vtables::surface::draw_outlined_circle)(this, x, y, radius, segments);
    }
    void DrawTexturedPolygon(int n, vgui::Vertex_t *verts, bool clipVertices = true)
    {
        vfunc<void (*)(CMatSystemSurface *, int, vgui::Vertex_t *, bool)>(this, vtables::surface::draw_textured_polygon)(this, n, verts, clipVertices);
    }
    void DrawSetTextureRGBAEx(int id, const unsigned char *rgba, int wide, int tall, ImageFormat imageFormat)
    {
        using fn_t = void (*)(CMatSystemSurface *, int, const unsigned char *, int, int, ImageFormat);
        vfunc<fn_t>(this, vtables::surface::draw_set_texture_rgba_ex)(this, id, rgba, wide, tall, imageFormat);
    }
};
