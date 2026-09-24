
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <unistd.h>

#define SDL_MAIN_HANDLED
#ifndef DECLSPEC
#define DECLSPEC __attribute__((visibility("hidden")))
#endif
#include <SDL2/SDL.h>

#include "core/sharedobj.hpp"

static void tf2_sdl_note(const char *msg)
{
    if (!msg)
        return;
    (void) write(2, msg, strlen(msg));
}

static void *tf2_sdl_handle()
{
    static void *h;
    static bool warned;
    if (h)
        return h;
    if (sharedobj::libsdl().lmap)
        h = sharedobj::libsdl().lmap;
    if (!h)
    {
        std::string path;
        std::string name = "libSDL2-2.0.so.0";
        if (sharedobj::LocateSharedObject(name, path))
            h = dlopen(path.c_str(), RTLD_LAZY | RTLD_NOLOAD);
    }
    if (!h && !warned)
    {
        warned = true;
        tf2_sdl_note("tf2_sdl: game libSDL2 missing\n");
    }
    return h;
}

static void *tf2_sdl_sym(const char *name)
{
    void *h  = tf2_sdl_handle();
    void *fn = h ? dlsym(h, name) : nullptr;
    static bool warned;
    if (!fn && !warned)
    {
        warned = true;
        tf2_sdl_note("tf2_sdl: missing symbol\n");
    }
    return fn;
}

#define TF2_SDL_WRAP(ret, name, params, args, fail)                                                                                \
    extern "C" ret name params                                                                                                     \
    {                                                                                                                              \
        using Fn = ret(*) params;                                                                                                  \
        static Fn fn;                                                                                                              \
        if (!fn)                                                                                                                   \
            fn = reinterpret_cast<Fn>(tf2_sdl_sym(#name));                                                                         \
        if (!fn)                                                                                                                   \
            return fail;                                                                                                           \
        return fn args;                                                                                                            \
    }

#define TF2_SDL_WRAP_VOID(name, params, args)                                                                                      \
    extern "C" void name params                                                                                                    \
    {                                                                                                                              \
        using Fn = void(*) params;                                                                                                 \
        static Fn fn;                                                                                                              \
        if (!fn)                                                                                                                   \
            fn = reinterpret_cast<Fn>(tf2_sdl_sym(#name));                                                                         \
        if (!fn)                                                                                                                   \
            return;                                                                                                                \
        fn args;                                                                                                                   \
    }

TF2_SDL_WRAP(SDL_Cursor *, SDL_CreateSystemCursor, (SDL_SystemCursor id), (id), nullptr)
TF2_SDL_WRAP_VOID(SDL_free, (void *mem), (mem))
TF2_SDL_WRAP(char *, SDL_GetClipboardText, (void), (), nullptr)
TF2_SDL_WRAP(int, SDL_SetClipboardText, (const char *text), (text), -1)
TF2_SDL_WRAP(SDL_Keymod, SDL_GetModState, (void), (), (SDL_Keymod) 0)
TF2_SDL_WRAP_VOID(SDL_GetWindowSize, (SDL_Window * window, int *w, int *h), (window, w, h))
TF2_SDL_WRAP_VOID(SDL_GL_GetDrawableSize, (SDL_Window * window, int *w, int *h), (window, w, h))
TF2_SDL_WRAP(Uint64, SDL_GetPerformanceFrequency, (void), (), 1)
TF2_SDL_WRAP(Uint64, SDL_GetPerformanceCounter, (void), (), 0)
TF2_SDL_WRAP_VOID(SDL_WarpMouseInWindow, (SDL_Window * window, int x, int y), (window, x, y))
TF2_SDL_WRAP(Uint32, SDL_GetMouseState, (int *x, int *y), (x, y), 0)
TF2_SDL_WRAP(Uint32, SDL_GetWindowFlags, (SDL_Window * window), (window), 0)
TF2_SDL_WRAP(SDL_GLContext, SDL_GL_GetCurrentContext, (void), (), nullptr)
TF2_SDL_WRAP(void *, SDL_GL_GetProcAddress, (const char *proc), (proc), nullptr)
TF2_SDL_WRAP(SDL_GLContext, SDL_GL_CreateContext, (SDL_Window * window), (window), nullptr)
TF2_SDL_WRAP(int, SDL_GL_MakeCurrent, (SDL_Window * window, SDL_GLContext ctx), (window, ctx), -1)
TF2_SDL_WRAP(SDL_Keycode, SDL_GetKeyFromScancode, (SDL_Scancode scancode), (scancode), SDLK_UNKNOWN)
TF2_SDL_WRAP(SDL_Keycode, SDL_GetKeyFromName, (const char *name), (name), SDLK_UNKNOWN)
TF2_SDL_WRAP(SDL_Scancode, SDL_GetScancodeFromKey, (SDL_Keycode key), (key), SDL_SCANCODE_UNKNOWN)
TF2_SDL_WRAP(const char *, SDL_GetKeyName, (SDL_Keycode key), (key), "")
TF2_SDL_WRAP(const Uint8 *, SDL_GetKeyboardState, (int *numkeys), (numkeys), nullptr)
