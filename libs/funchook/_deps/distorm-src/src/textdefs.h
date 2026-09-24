#ifndef TEXTDEFS_H
#define TEXTDEFS_H

#include "config.h"
#include "wstring.h"

#ifndef DISTORM_LIGHT

#define PLUS_DISP_CHR '+'
#define MINUS_DISP_CHR '-'
#define OPEN_CHR '['
#define CLOSE_CHR ']'
#define SP_CHR ' '
#define SEG_OFF_CHR ':'

void str_hex(_WString* s, const uint8_t* buf, unsigned int len);

#ifdef SUPPORT_64BIT_OFFSET
#define str_int(s, x) str_int_impl((s), (x))
void str_int_impl(unsigned char** s, uint64_t x);
#else
#define str_int(s, x) str_int_impl((s), (uint8_t*)&(x))
void str_int_impl(unsigned char** s, uint8_t src[8]);
#endif

#endif

#endif
