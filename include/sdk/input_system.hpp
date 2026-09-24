#pragma once

#include "core/vfunc.hpp"
#include "core/vtables.hpp"
#include "inputsystem/ButtonCode.h"

class CInputSystem
{
public:
    bool IsButtonDown(ButtonCode_t code)
    {
        return vfunc<bool (*)(CInputSystem *, ButtonCode_t)>(this, vtables::input_system::is_button_down)(this, code);
    }
    const char *ButtonCodeToString(ButtonCode_t code)
    {
        return vfunc<const char *(*)(CInputSystem *, ButtonCode_t)>(this, vtables::input_system::button_code_to_string)(this, code);
    }
};
