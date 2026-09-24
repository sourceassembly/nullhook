#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include <vgui/VGUI.h>

class CPanel
{
public:
    const char *GetName(vgui::VPANEL panel)
    {
        return vfunc<const char *(*)(CPanel *, vgui::VPANEL)>(this, vtables::vgui_panel::get_name)(this, panel);
    }
    void SetTopmostPopup(vgui::VPANEL panel, bool state)
    {
        vfunc<void (*)(CPanel *, vgui::VPANEL, bool)>(this, vtables::vgui_panel::set_topmost_popup)(this, panel, state);
    }
};
