#ifndef CONFIG_H
#define CONFIG_H

#define __DISTORMV__ 0x030502

#include <string.h>

#include "../include/distorm.h"

#ifdef __GNUC__

#include <stdint.h>

#define _DLLEXPORT_
#define _FASTCALL_

#define _INLINE_ static

#ifdef __BIG_ENDIAN__
	#define BE_SYSTEM
#endif

#elif __WATCOMC__

#include <stdint.h>

#define _DLLEXPORT_
#define _FASTCALL_
#define _INLINE_ __inline

#elif __DMC__

#include <stdint.h>

#define _DLLEXPORT_
#define _FASTCALL_
#define _INLINE_ __inline

#elif __TINYC__

#include <stdint.h>

#define _DLLEXPORT_
#define _FASTCALL_
#define _INLINE_ static

#elif _MSC_VER

#define _DLLEXPORT_ __declspec(dllexport)
#define _FASTCALL_ __fastcall
#define _INLINE_ __inline

#if !defined(_M_IX86) && !defined(_M_X64)
	#define BE_SYSTEM
#endif

#endif

#ifndef DISTORM_DYNAMIC
#undef _DLLEXPORT_
#define _DLLEXPORT_
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifdef BE_SYSTEM

#ifndef __GNUC__
#define STATIC_INLINE static _INLINE_
#else
#define STATIC_INLINE static
#endif

STATIC_INLINE int16_t RSHORT(const uint8_t *s)
{
	return s[0] | (s[1] << 8);
}
STATIC_INLINE uint16_t RUSHORT(const uint8_t *s)
{
	return s[0] | (s[1] << 8);
}
STATIC_INLINE int32_t RLONG(const uint8_t *s)
{
	return s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
}
STATIC_INLINE uint32_t RULONG(const uint8_t *s)
{
	return s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
}
STATIC_INLINE int64_t RLLONG(const uint8_t *s)
{
	return s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24) | ((uint64_t)s[4] << 32) | ((uint64_t)s[5] << 40) | ((uint64_t)s[6] << 48) | ((uint64_t)s[7] << 56);
}
STATIC_INLINE uint64_t RULLONG(const uint8_t *s)
{
	return s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24) | ((uint64_t)s[4] << 32) | ((uint64_t)s[5] << 40) | ((uint64_t)s[6] << 48) | ((uint64_t)s[7] << 56);
}

#undef STATIC_INLINE

#else

#define RSHORT(x) *(int16_t *)x
#define RUSHORT(x) *(uint16_t *)x
#define RLONG(x) *(int32_t *)x
#define RULONG(x) *(uint32_t *)x
#define RLLONG(x) *(int64_t *)x
#define RULLONG(x) *(uint64_t *)x
#endif

#endif
