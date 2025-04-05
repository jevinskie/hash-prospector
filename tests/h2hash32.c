/* H2 32-bit hash
 * https://github.com/h2database/h2database
 * src/test/org/h2/test/store/CalculateHashConstant.java
 */
#include <stdint.h>

#ifndef __APPLE__
#define HF_ABI __attribute__((sysv_abi, visibility("default")))
#else
#define HF_ABI __attribute__((visibility("default")))
#endif

// exact bias: 1.4249702882580686
HF_ABI uint32_t hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x45d9f3b;
    x ^= x >> 16;
    x *= 0x45d9f3b;
    x ^= x >> 16;
    return x;
}

HF_ABI uint32_t unhash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x119de1f3;
    x ^= x >> 16;
    x *= 0x119de1f3;
    x ^= x >> 16;
    return x;
}
