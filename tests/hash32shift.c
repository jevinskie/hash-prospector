#include <stdint.h>

#ifndef __APPLE__
#define HF_ABI __attribute__((sysv_abi, visibility("default")))
#else
#define HF_ABI __attribute__((visibility("default")))
#endif

// exact bias: 44.000700486813841
HF_ABI uint32_t hash(uint32_t x) {
    x = ~x + (x << 15);
    x ^= x >> 12;
    x += x << 2;
    x ^= x >> 4;
    x *= 2057;
    x ^= x >> 16;
    return x;
}
