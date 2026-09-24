#ifndef WSTRING_H
#define WSTRING_H

#include "config.h"
#include "../include/mnemonics.h"

#ifndef DISTORM_LIGHT

_INLINE_ void strcat_WSR(unsigned char** str, const _WRegister* reg)
{

	memcpy((int8_t*)*str, (const int8_t*)reg->p, 8);
	*str += reg->length;
}

#define strfinalize_WS(s, end) do { *end = 0; s.length = (unsigned int)((size_t)end - (size_t)s.p); } while (0)
#define chrcat_WS(s, ch) do { *s = ch; s += 1; } while (0)
#define strcat_WS(s, buf, copylen, advancelen) do { memcpy((int8_t*)s, buf, copylen); s += advancelen; } while(0)

#endif

#endif
