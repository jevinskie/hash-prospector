#include <stdint.h>

#ifndef __APPLE__
#define HF_ABI __attribute__((sysv_abi, visibility("default")))
#else
#define HF_ABI __attribute__((visibility("default")))
#endif

// exact bias: 0.26398543281818287
HF_ABI uint32_t hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
}
