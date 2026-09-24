#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "Color.h"
#include <cstdarg>
#include <cstdio>

class ConVar;
class ConCommand;
class ConCommandBase;

class CCvar
{
public:
    void RegisterConCommand(ConCommandBase *command)
    {
        vfunc<void (*)(CCvar *, ConCommandBase *)>(this, vtables::cvar::register_con_command)(this, command);
    }
    ConVar *FindVar(const char *name)
    {
        return vfunc<ConVar *(*)(CCvar *, const char *)>(this, vtables::cvar::find_var)(this, name);
    }
    ConCommand *FindCommand(const char *name)
    {
        return vfunc<ConCommand *(*)(CCvar *, const char *)>(this, vtables::cvar::find_command)(this, name);
    }
    void ConsoleColorPrintf(const Color &clr, const char *fmt, ...) const
    {
        char buf[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        using fn_t = void (*)(const CCvar *, const Color &, const char *, ...);
        vfunc<fn_t>(const_cast<CCvar *>(this), vtables::cvar::console_color_printf)(this, clr, "%s", buf);
    }
};
