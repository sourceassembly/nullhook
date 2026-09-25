/*
 * nographics.cpp
 *
 *  Created on: Aug 1, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"

#if !ENABLE_TEXTMODE
static settings::Boolean null_graphics("hack.nullgraphics", "false");
#else
static settings::Boolean null_graphics("hack.nullgraphics", "true");
#endif
typedef ITexture *(*FindTexture_t)(void *, const char *, const char *, bool, int);
typedef IMaterial *(*FindMaterialEx_t)(void *, const char *, const char *, int, bool, const char *);
typedef IMaterial *(*FindMaterial_t)(void *, const char *, const char *, bool, const char *);
FindTexture_t FindTexture_Original;
FindMaterialEx_t FindMaterialEx_Original;
FindMaterial_t FindMaterial_Original;

ITexture *FindTexture_null_hook(void *this_, char const *pTextureName, const char *pTextureGroupName, bool complain, int nAdditionalCreationFlags)
{
    static ITexture *st = FindTexture_Original(this_, pTextureName, pTextureGroupName, complain, nAdditionalCreationFlags);
    return st;
}

IMaterial *FindMaterialEx_null_hook(void *this_, char const *pMaterialName, const char *pTextureGroupName, int nContext, bool complain, const char *pComplainPrefix)
{
    static IMaterial *st = FindMaterialEx_Original(this_, pMaterialName, pTextureGroupName, nContext, complain, pComplainPrefix);
    return st;
}

IMaterial *FindMaterial_null_hook(void *this_, char const *pMaterialName, const char *pTextureGroupName, bool complain, const char *pComplainPrefix)
{
    static IMaterial *st = FindMaterial_Original(this_, pMaterialName, pTextureGroupName, complain, pComplainPrefix);
    return st;
}

void ReloadTextures_null_hook(void *this_)
{
}
void ReloadMaterials_null_hook(void *this_, const char *pSubString)
{
}
void ReloadFilesInList_null_hook(void *this_, IFileList *pFilesToReload)
{
}
void NullHook()
{
    g_IMaterialSystem->SetInStubMode(true);
}
void RemoveNullHook()
{
    g_IMaterialSystem->SetInStubMode(false);
}
static CatCommand ApplyNullhook("debug_material_hook", "Debug", []() { NullHook(); });
static CatCommand RemoveNullhook("debug_material_hook_clear", "Debug", []() { RemoveNullHook(); });
static settings::Boolean debug_framerate("debug.framerate", "false");
static float framerate = 0.0f;
static Timer send_timer{};
static InitRoutine init_nographics(
    []()
    {
#if ENABLE_TEXTMODE
        NullHook();
#endif
        EC::Register(
            EC::Paint,
            []()
            {
                if (!*debug_framerate)
                    return;
                framerate = 0.9 * framerate + (1.0 - 0.9) * g_GlobalVars->absoluteframetime;
                if (send_timer.test_and_set(1000))
                    logging::Info("FPS: %f", 1.0f / framerate);
            },
            "material_cm");
    });
static bool blacklist_file(const char *filename)
{
    const static char *blacklist[] = { ".ani", ".wav", ".mp3", ".vvd", ".vtx", ".vtf", ".vfe", ".cache" /*, ".pcf"*/ };
    if (!filename || !std::strncmp(filename, "materials/console/", 18))
        return false;

    std::size_t len = std::strlen(filename);
    if (len <= 3)
        return false;

    auto ext_p = strrchr(filename, '.');
    if (!ext_p)
        return false;

    if (!std::strcmp(ext_p, ".vmt"))
    {
        /* Not loading it causes extreme console spam */
        if (std::strstr(filename, "corner"))
            return false;
        /* minor console spam */
        if (std::strstr(filename, "hud") || std::strstr(filename, "vgui"))
            return false;

        return true;
    }
    if (std::strstr(filename, "sound.cache") || std::strstr(filename, "tf2_sound") || std::strstr(filename, "game_sounds"))
        return false;
    if (!std::strncmp(filename, "sound/player/footsteps", 22))
        return false;
    if (!std::strcmp(ext_p, ".mdl"))
    {
        return false;
    }
    if (!std::strncmp(filename, "/decal", 6))
        return true;

    for (int i = 0; i < sizeof(blacklist) / sizeof(blacklist[0]); ++i)
        if (!std::strcmp(ext_p, blacklist[i]))
            return true;

    return false;
}

static void *(*FSorig_Open)(void *, const char *, const char *, const char *);
static void *FSHook_Open(void *this_, const char *pFileName, const char *pOptions, const char *pathID)
{
    // fprintf(stderr, "Open: %s\n", pFileName);
    if (blacklist_file(pFileName))
        return nullptr;

    return FSorig_Open(this_, pFileName, pOptions, pathID);
}

static bool (*FSorig_ReadFile)(void *, const char *, const char *, void *, int, int, void *);
static bool FSHook_ReadFile(void *this_, const char *pFileName, const char *pPath, void *buf, int nMaxBytes, int nStartingByte, void *pfnAlloc)
{
    // fprintf(stderr, "ReadFile: %s\n", pFileName);
    if (blacklist_file(pFileName))
        return false;

    return FSorig_ReadFile(this_, pFileName, pPath, buf, nMaxBytes, nStartingByte, pfnAlloc);
}

static void *(*FSorig_OpenEx)(void *, const char *, const char *, unsigned, const char *, char **);
static void *FSHook_OpenEx(void *this_, const char *pFileName, const char *pOptions, unsigned flags, const char *pathID, char **ppszResolvedFilename)
{
    // fprintf(stderr, "OpenEx: %s\n", pFileName);
    if (pFileName && blacklist_file(pFileName))
        return nullptr;

    return FSorig_OpenEx(this_, pFileName, pOptions, flags, pathID, ppszResolvedFilename);
}

static int (*FSorig_ReadFileEx)(void *, const char *, const char *, void **, bool, bool, int, int, void *);
static int FSHook_ReadFileEx(void *this_, const char *pFileName, const char *pPath, void **ppBuf, bool bNullTerminate, bool bOptimalAlloc, int nMaxBytes, int nStartingByte, void *pfnAlloc)
{
    // fprintf(stderr, "ReadFileEx: %s\n", pFileName);
    if (blacklist_file(pFileName))
        return 0;

    return FSorig_ReadFileEx(this_, pFileName, pPath, ppBuf, bNullTerminate, bOptimalAlloc, nMaxBytes, nStartingByte, pfnAlloc);
}

static void (*FSorig_AddFilesToFileCache)(void *, void *, const char **, int, const char *);
static void FSHook_AddFilesToFileCache(void *this_, void *cacheId, const char **ppFileNames, int nFileNames, const char *pPathID)
{
    if (!ppFileNames || nFileNames <= 0)
    {
        FSorig_AddFilesToFileCache(this_, cacheId, ppFileNames, nFileNames, pPathID);
        return;
    }
    std::vector<const char *> filtered;
    filtered.reserve(nFileNames);
    for (int i = 0; i < nFileNames; ++i)
    {
        if (ppFileNames[i] && blacklist_file(ppFileNames[i]))
            continue;
        filtered.push_back(ppFileNames[i]);
    }
    if (!filtered.empty())
        FSorig_AddFilesToFileCache(this_, cacheId, filtered.data(), (int) filtered.size(), pPathID);
}

static int (*FSorig_AsyncReadMultiple)(void *, const char **, int, void *);
static int FSHook_AsyncReadMultiple(void *this_, const char **pRequests, int nRequests, void *phControls)
{
    // NB: pRequests is really an array of FSAsyncFileRequest_t, not filenames,
    // so filtering here would read garbage pointers; failing the whole batch
    // over one entry would also break legitimate loads. Pass through untouched.
    return FSorig_AsyncReadMultiple(this_, pRequests, nRequests, phControls);
}

static const char *(*FSorig_FindNext)(void *, void *);
static const char *FSHook_FindNext(void *this_, void *handle)
{
    const char *p;
    do
        p = FSorig_FindNext(this_, handle);
    while (p && blacklist_file(p));

    return p;
}

static const char *(*FSorig_FindFirst)(void *, const char *, void **);
static const char *FSHook_FindFirst(void *this_, const char *pWildCard, void **pHandle)
{
    auto p = FSorig_FindFirst(this_, pWildCard, pHandle);
    while (p && blacklist_file(p))
        p = FSorig_FindNext(this_, *pHandle);

    return p;
}

static bool (*FSorig_Precache)(void *, const char *, const char *);
static bool FSHook_Precache(void *this_, const char *pFileName, const char *pPathID)
{
    // Only pretend success for files we deliberately skip; legitimate precaches
    // must really happen or the engine permanently falls back to error assets.
    if (pFileName && blacklist_file(pFileName))
        return true;
    return FSorig_Precache(this_, pFileName, pPathID);
}

static CatCommand debug_invalidate("invalidate_mdl_cache", "Invalidates MDL cache", []() { g_IBaseClient->InvalidateMdlCache(); });

static hooks::VMTHook fs_hook{}, fs_hook2{};
static bool hooked_fs = false;
// BytePatches applied by ReduceRamUsage (registered on first apply so UnHookFs
// can restore the original bytes when null-graphics is toggled off).
static std::vector<BytePatch *> ram_patches;
static void ReduceRamUsage()
{
    if (!hooked_fs)
    {
        /* TO DO: Improves load speeds but doesn't reduce memory usage a lot
         * It seems engine still allocates significant parts without them
         * being really used
         * Plan B: null subsystems (Particle, Material, Model partially, Sound and etc.)
         */
        hooked_fs = true;
        fs_hook.Set(reinterpret_cast<void *>(g_IFileSystem));
        fs_hook.HookMethod(FSHook_FindFirst, vtables::filesystem::find_first, &FSorig_FindFirst);
        fs_hook.HookMethod(FSHook_FindNext, vtables::filesystem::find_next, &FSorig_FindNext);
        fs_hook.HookMethod(FSHook_AsyncReadMultiple, vtables::filesystem::async_read_multiple, &FSorig_AsyncReadMultiple);
        fs_hook.HookMethod(FSHook_OpenEx, vtables::filesystem::open_ex, &FSorig_OpenEx);
        fs_hook.HookMethod(FSHook_ReadFileEx, vtables::filesystem::read_file_ex, &FSorig_ReadFileEx);
        fs_hook.HookMethod(FSHook_AddFilesToFileCache, vtables::filesystem::add_files_to_file_cache, &FSorig_AddFilesToFileCache);
        fs_hook.Apply();

        fs_hook2.Set(reinterpret_cast<void *>(g_IFileSystem), vtables::filesystem::ibasefilesystem_vptr_offset);
        fs_hook2.HookMethod(FSHook_Open, vtables::filesystem::open, &FSorig_Open);
        fs_hook2.HookMethod(FSHook_Precache, vtables::filesystem::precache, &FSorig_Precache);
        fs_hook2.HookMethod(FSHook_ReadFile, vtables::filesystem::read_file, &FSorig_ReadFile);
        fs_hook2.Apply();
        /* Might give performance benefit, but mostly fixes annoying console
         * spam related to mdl not being able to play sequence that it
         * cannot play on error.mdl
         */
    }

    if (g_IBaseClient)
    {
        static BytePatch playSequence{ gSignatures.GetClientSignature, sigs::play_sequence, 0x00, { 0xC3 } };
        playSequence.Patch();

        static BytePatch particleCreate{ gSignatures.GetClientSignature, sigs::particle_property_create, 0x00, { 0x31, 0xC0, 0xC3 } };
        static BytePatch particlePrecache{ gSignatures.GetClientSignature, sigs::particle_system_precache, 0x00, { 0x31, 0xC0, 0xC3 } };
        static BytePatch particleCreating{ gSignatures.GetClientSignature, sigs::particle_effect_create_event, 0x00, { 0x31, 0xC0, 0xC3 } };
        particleCreate.Patch();
        particlePrecache.Patch();
        particleCreating.Patch();
        if (ram_patches.empty())
            ram_patches = { &playSequence, &particleCreate, &particlePrecache, &particleCreating };
    }
}

static void UnHookFs()
{
    for (BytePatch *patch : ram_patches)
        patch->Shutdown();
    fs_hook.Release();
    fs_hook2.Release();
    hooked_fs = false;
    if (g_IBaseClient)
        g_IBaseClient->InvalidateMdlCache();
}

#if ENABLE_TEXTMODE
static InitRoutineEarly nullify_textmode(
    []()
    {
        std::vector<unsigned char> nop5 = { 0x90, 0x90, 0x90, 0x90, 0x90 };
        static BytePatch sdl_hidden(gSignatures.GetLauncherSignature, sigs::sdl_create_window_flags, 0x2, { 0x08 });
        static BytePatch sdl_skip_vk(gSignatures.GetLauncherSignature, sigs::sdl_create_window_flags, 0x9, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
        static BytePatch sdl_show_resize(gSignatures.GetLauncherSignature, sigs::sdl_show_window_resize, 0x4, nop5);
        static BytePatch sdl_show_create(gSignatures.GetLauncherSignature, sigs::sdl_show_window_after_create, 0x7, nop5);
        static BytePatch sdl_show_present(gSignatures.GetLauncherSignature, sigs::sdl_show_window_present, 0x4, nop5);

        ReduceRamUsage();
        static BytePatch patch5(gSignatures.GetEngineSignature(sigs::video_mode_setup_startup_graphic), { 0xC3 });
        static BytePatch patch6(gSignatures.GetMaterialSystemSignature, sigs::material_system_swap_buffers, 0x0, { 0x31, 0xC0, 0x40, 0xC3 });
        static BytePatch patch7(gSignatures.GetEngineSignature, sigs::v_render_view, 0x0, { 0xC3 });
        sdl_hidden.Patch();
        sdl_skip_vk.Patch();
        sdl_show_resize.Patch();
        sdl_show_create.Patch();
        sdl_show_present.Patch();
        patch5.Patch();
        patch6.Patch();
        patch7.Patch();
    });
#endif

static Timer signon_timer;
static InitRoutine nullifiy_textmode2(
    []()
    {
#if ENABLE_TEXTMODE
        ReduceRamUsage();
#endif
        null_graphics.installChangeCallback(
            [](settings::VariableBase<bool> &, bool after)
            {
                if (after)
                    ReduceRamUsage();
                else
                    UnHookFs();
            });
#if ENABLE_TEXTMODE
        ReduceRamUsage();
        static BytePatch patch2(gSignatures.GetClientSignature, sigs::view_render_render, 0x0, { 0x31, 0xC0, 0x40, 0xC3 });
        static BytePatch patch_ss(gSignatures.GetClientSignature, sigs::view_render_perform_screen_space_effects, 0x0, { 0xC3 });
        static BytePatch patch_ov(gSignatures.GetClientSignature, sigs::view_render_perform_screen_overlay, 0x0, { 0xC3 });
        static BytePatch patch_menu_anim(gSignatures.GetClientSignature, sigs::menu_model_anim_events, 0x0, { 0xC3 });
        static BytePatch patch_menu_update(gSignatures.GetClientSignature, sigs::menu_model_update, 0x0, { 0xC3 });
        static BytePatch patch_menu_apply(gSignatures.GetClientSignature, sigs::menu_model_apply_sequence, 0x0, { 0xC3 });
        static BytePatch patch_menu_set(gSignatures.GetClientSignature, sigs::menu_item_model_set, 0x0, { 0xC3 });
        static BytePatch patch_html_a(gSignatures.GetClientSignature, sigs::html_createbrowser_gate_a, 0x1c, { 0x48, 0x31, 0xFF });
        static BytePatch patch_html_b(gSignatures.GetClientSignature, sigs::html_createbrowser_gate_b, 0x1c, { 0x48, 0x31, 0xFF });
        static BytePatch patch_vox(gSignatures.GetEngineSignature, sigs::vox_shutdown_bad_vcall, 0x22, { 0x90, 0x90, 0x90 });
        static BytePatch patch_scene(gSignatures.GetServerSignature, sigs::server_scene_entity_should_transmit, 0x0, { 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });
        static BytePatch patch_base(gSignatures.GetServerSignature, sigs::server_base_entity_should_transmit, 0x0, { 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });
        patch2.Patch();
        patch_ss.Patch();
        patch_ov.Patch();
        patch_menu_anim.Patch();
        patch_menu_update.Patch();
        patch_menu_apply.Patch();
        patch_menu_set.Patch();
        patch_html_a.Patch();
        patch_html_b.Patch();
        patch_vox.Patch();
        patch_scene.Patch();
        patch_base.Patch();
        uintptr_t textmode_store = gSignatures.GetClientSignature(sigs::client_textmode_flag_store);
        if (textmode_store)
        {
            auto *flag = reinterpret_cast<bool *>(textmode_store + 14 + *reinterpret_cast<int *>(textmode_store + 9));
            BytePatch::mprotectAddr(uintptr_t(flag), 1, PROT_READ | PROT_WRITE | PROT_EXEC);
            *flag = true;
        }
#endif
    });
