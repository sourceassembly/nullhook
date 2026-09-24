#if defined(CPU_X86_64)

#define TRANSIT_CODE_TEMPLATE { \
    \
    0x4c,0x8d,0x1d,0xf9,0xff,0xff,0xff,   \
    0xff,0x25,0x00,0x00,0x00,0x00,        \
    \
    0x0f,0x1f,0x40,0x00,                  \
    0x0f,0x1f,0x40,0x00,                  \
  }
#define TRANSIT_HOOK_CALLER_ADDR 0xd
#define TRANSIT_CODE_SIZE 0x15

#elif defined(CPU_X86)

#define TRANSIT_CODE_TEMPLATE { \
    \
    0xe8,0x09,0x00,0x00,0x00,             \
    0x8d,0x40,0xfb,                       \
    0xff,0xa0,0x14,0x00,0x00,0x00,        \
    \
    0x8b,0x04,0x24,                       \
    0xc3,                                 \
    0x66,0x90,                            \
    \
    0x0f,0x1f,0x40,0x00,                  \
  }
#define TRANSIT_HOOK_CALLER_ADDR 0x14
#define TRANSIT_CODE_SIZE 0x18

#elif defined(CPU_ARM64)

#define TRANSIT_CODE_TEMPLATE { \
    \
    0x1000000a,                           \
    0x58000069,                           \
    0xd61f0120,                           \
    0xd503201f,                           \
    \
    0xd503201f,                           \
    0xd503201f,                           \
  }
#define TRANSIT_HOOK_CALLER_ADDR 0x4
#define TRANSIT_CODE_SIZE 0x6

#endif
