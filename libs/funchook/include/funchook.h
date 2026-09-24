#ifndef FUNCHOOK_H
#define FUNCHOOK_H 1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FUNCHOOK_EXPORTS
#if defined(_WIN32)
#define FUNCHOOK_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define FUNCHOOK_EXPORT __attribute__((visibility("default")))
#endif
#endif
#ifndef FUNCHOOK_EXPORT
#define FUNCHOOK_EXPORT
#endif

typedef struct funchook funchook_t;

#define FUNCHOOK_ERROR_INTERNAL_ERROR         -1
#define FUNCHOOK_ERROR_SUCCESS                 0
#define FUNCHOOK_ERROR_OUT_OF_MEMORY           1
#define FUNCHOOK_ERROR_ALREADY_INSTALLED       2
#define FUNCHOOK_ERROR_DISASSEMBLY             3
#define FUNCHOOK_ERROR_IP_RELATIVE_OFFSET      4
#define FUNCHOOK_ERROR_CANNOT_FIX_IP_RELATIVE  5
#define FUNCHOOK_ERROR_FOUND_BACK_JUMP         6
#define FUNCHOOK_ERROR_TOO_SHORT_INSTRUCTIONS  7
#define FUNCHOOK_ERROR_MEMORY_ALLOCATION       8
#define FUNCHOOK_ERROR_MEMORY_FUNCTION         9
#define FUNCHOOK_ERROR_NOT_INSTALLED          10
#define FUNCHOOK_ERROR_NO_AVAILABLE_REGISTERS 11
#define FUNCHOOK_ERROR_NO_SPACE_NEAR_TARGET_ADDR 12

typedef struct funchook_arg_handle funchook_arg_handle_t;

typedef struct funchook_info {
    void *original_target_func;
    void *target_func;
    void *trampoline_func;
    void *hook_func;
    void *user_data;
    funchook_arg_handle_t *arg_handle;
} funchook_info_t;

typedef void (*funchook_hook_t)(funchook_info_t *fi);

typedef struct {
    void *hook_func;
    funchook_hook_t prehook;
    void *user_data;
    unsigned int flags;
} funchook_params_t;

FUNCHOOK_EXPORT funchook_t *funchook_create(void);

FUNCHOOK_EXPORT int funchook_prepare(funchook_t *funchook, void **target_func, void *hook_func);

FUNCHOOK_EXPORT int funchook_prepare_with_params(funchook_t *funchook,
    void **target_func, const funchook_params_t *params);

FUNCHOOK_EXPORT int funchook_install(funchook_t *funchook, int flags);

FUNCHOOK_EXPORT int funchook_uninstall(funchook_t *funchook, int flags);

FUNCHOOK_EXPORT int funchook_destroy(funchook_t *funchook);

FUNCHOOK_EXPORT const char *funchook_error_message(const funchook_t *funchook);

FUNCHOOK_EXPORT int funchook_set_debug_file(const char *name);

FUNCHOOK_EXPORT void *funchook_arg_get_int_reg_addr(const funchook_arg_handle_t *arg_handle, int pos);

FUNCHOOK_EXPORT void *funchook_arg_get_flt_reg_addr(const funchook_arg_handle_t *arg_handle, int pos);

FUNCHOOK_EXPORT void *funchook_arg_get_stack_addr(const funchook_arg_handle_t *arg_handle, int pos);

#ifdef __cplusplus
}
#endif

#endif
