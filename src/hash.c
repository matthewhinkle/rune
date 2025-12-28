/*
 * Hash module implementation - MurmurHash variants.
 *
 * Based on the public domain MurmurHash implementations by Austin Appleby.
 * https://github.com/aappleby/smhasher
 */

// ReSharper disable CppRedundantParentheses
#include "hash.h"

#include <string.h>

// =================================================================================================
// Internal: Rotation and mixing helpers
// =================================================================================================

static uint32_t rotl32(uint32_t x, int8_t r) {
    return (x << r) | (x >> (32 - r));
}

static uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

// =================================================================================================
// Internal: MurmurHash3 32-bit constants and mixing
// =================================================================================================

static constexpr uint32_t M3_32_C1 = 0xCC9E2D51;
static constexpr uint32_t M3_32_C2 = 0x1B873593;

static uint32_t m3_32_mix_k(uint32_t k) {
    k *= M3_32_C1;
    k = rotl32(k, 15);
    k *= M3_32_C2;
    return k;
}

static uint32_t m3_32_mix_h(uint32_t h, uint32_t k) {
    h ^= k;
    h = rotl32(h, 13);
    h = h * 5 + 0xE6546B64;
    return h;
}

static uint32_t m3_32_fmix(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85EBCA6B;
    h ^= h >> 13;
    h *= 0xC2B2AE35;
    h ^= h >> 16;
    return h;
}

// =================================================================================================
// Internal: MurmurHash2 64-bit constants
// =================================================================================================

static constexpr uint64_t M2_64_M = 0xC6A4A7935BD1E995ULL;
static constexpr int M2_64_R = 47;

// =================================================================================================
// Internal: MurmurHash3 128-bit constants and mixing
// =================================================================================================

static constexpr uint64_t M3_128_C1 = 0x87C37B91114253D5ULL;
static constexpr uint64_t M3_128_C2 = 0x4CF5AD432745937FULL;

static uint64_t m3_128_mix_k1(uint64_t k1) {
    k1 *= M3_128_C1;
    k1 = rotl64(k1, 31);
    k1 *= M3_128_C2;
    return k1;
}

static uint64_t m3_128_mix_k2(uint64_t k2) {
    k2 *= M3_128_C2;
    k2 = rotl64(k2, 33);
    k2 *= M3_128_C1;
    return k2;
}

static uint64_t m3_128_fmix(uint64_t k) {
    k ^= k >> 33;
    k *= 0xFF51AFD7ED558CCDULL;
    k ^= k >> 33;
    k *= 0xC4CEB9FE1A85EC53ULL;
    k ^= k >> 33;
    return k;
}

// =================================================================================================
// Internal: Safe unaligned reads
// =================================================================================================

static uint32_t read_u32(const uint8_t * p) {
    uint32_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static uint64_t read_u64(const uint8_t * p) {
    uint64_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

// =================================================================================================
// Public API: MurmurHash3 32-bit (x86)
// =================================================================================================

extern uint32_t murmur32(const void * data, size_t size, uint32_t seed) {
    const uint8_t * bytes = data;
    const size_t nblocks = size / 4;

    uint32_t h1 = seed;

    // Body: process 4-byte blocks
    const uint8_t * blocks = bytes;
    for (size_t i = 0; i < nblocks; i++) {
        uint32_t k1 = read_u32(blocks + i * 4);
        k1 = m3_32_mix_k(k1);
        h1 = m3_32_mix_h(h1, k1);
    }

    // Tail: process remaining bytes
    const uint8_t * tail = bytes + nblocks * 4;
    uint32_t k1 = 0;

    switch (size & 3) {
    case 3:
        k1 ^= (uint32_t)tail[2] << 16;
        [[fallthrough]];
    case 2:
        k1 ^= (uint32_t)tail[1] << 8;
        [[fallthrough]];
    case 1:
        k1 ^= tail[0];
        k1 = m3_32_mix_k(k1);
        h1 ^= k1;
    default:;
    }

    // Finalization
    h1 ^= (uint32_t)size;
    h1 = m3_32_fmix(h1);

    return h1;
}

// =================================================================================================
// Public API: MurmurHash2 64-bit (64A variant)
// =================================================================================================

extern uint64_t murmur64(const void * data, size_t size, uint64_t seed) {
    const uint8_t * bytes = data;
    const size_t nblocks = size / 8;

    uint64_t h = seed ^ (size * M2_64_M);

    // Body: process 8-byte blocks
    const uint8_t * blocks = bytes;
    for (size_t i = 0; i < nblocks; i++) {
        uint64_t k = read_u64(blocks + i * 8);

        k *= M2_64_M;
        k ^= k >> M2_64_R;
        k *= M2_64_M;

        h ^= k;
        h *= M2_64_M;
    }

    // Tail: process remaining bytes
    const uint8_t * tail = bytes + nblocks * 8;

    switch (size & 7) {
    case 7:
        h ^= (uint64_t)tail[6] << 48;
        [[fallthrough]];
    case 6:
        h ^= (uint64_t)tail[5] << 40;
        [[fallthrough]];
    case 5:
        h ^= (uint64_t)tail[4] << 32;
        [[fallthrough]];
    case 4:
        h ^= (uint64_t)tail[3] << 24;
        [[fallthrough]];
    case 3:
        h ^= (uint64_t)tail[2] << 16;
        [[fallthrough]];
    case 2:
        h ^= (uint64_t)tail[1] << 8;
        [[fallthrough]];
    case 1:
        h ^= tail[0];
        h *= M2_64_M;
    default:;
    }

    // Finalization
    h ^= h >> M2_64_R;
    h *= M2_64_M;
    h ^= h >> M2_64_R;

    return h;
}

// =================================================================================================
// Public API: MurmurHash3 128-bit (x64)
// =================================================================================================

extern hash128_t murmur128(const void * data, size_t size, uint64_t seed) {
    const uint8_t * bytes = (const uint8_t *)data;
    const size_t nblocks = size / 16;

    uint64_t h1 = seed;
    uint64_t h2 = seed;

    // Body: process 16-byte blocks
    const uint8_t * blocks = bytes;
    for (size_t i = 0; i < nblocks; i++) {
        uint64_t k1 = read_u64(blocks + i * 16);
        uint64_t k2 = read_u64(blocks + i * 16 + 8);

        k1 = m3_128_mix_k1(k1);
        h1 ^= k1;
        h1 = rotl64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52DCE729;

        k2 = m3_128_mix_k2(k2);
        h2 ^= k2;
        h2 = rotl64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495AB5;
    }

    // Tail: process remaining bytes
    const uint8_t * tail = bytes + nblocks * 16;
    uint64_t k1 = 0;
    uint64_t k2 = 0;

    switch (size & 15) {
    case 15:
        k2 ^= (uint64_t)tail[14] << 48;
        [[fallthrough]];
    case 14:
        k2 ^= (uint64_t)tail[13] << 40;
        [[fallthrough]];
    case 13:
        k2 ^= (uint64_t)tail[12] << 32;
        [[fallthrough]];
    case 12:
        k2 ^= (uint64_t)tail[11] << 24;
        [[fallthrough]];
    case 11:
        k2 ^= (uint64_t)tail[10] << 16;
        [[fallthrough]];
    case 10:
        k2 ^= (uint64_t)tail[9] << 8;
        [[fallthrough]];
    case 9:
        k2 ^= (uint64_t)tail[8];
        k2 = m3_128_mix_k2(k2);
        h2 ^= k2;
        [[fallthrough]];

    case 8:
        k1 ^= (uint64_t)tail[7] << 56;
        [[fallthrough]];
    case 7:
        k1 ^= (uint64_t)tail[6] << 48;
        [[fallthrough]];
    case 6:
        k1 ^= (uint64_t)tail[5] << 40;
        [[fallthrough]];
    case 5:
        k1 ^= (uint64_t)tail[4] << 32;
        [[fallthrough]];
    case 4:
        k1 ^= (uint64_t)tail[3] << 24;
        [[fallthrough]];
    case 3:
        k1 ^= (uint64_t)tail[2] << 16;
        [[fallthrough]];
    case 2:
        k1 ^= (uint64_t)tail[1] << 8;
        [[fallthrough]];
    case 1:
        k1 ^= tail[0];
        k1 = m3_128_mix_k1(k1);
        h1 ^= k1;
    default:;
    }

    // Finalization
    h1 ^= size;
    h2 ^= size;

    h1 += h2;
    h2 += h1;

    h1 = m3_128_fmix(h1);
    h2 = m3_128_fmix(h2);

    h1 += h2;
    h2 += h1;

    return (hash128_t){.h1 = h1, .h2 = h2};
}

// =================================================================================================
// Public API: xxHash64
// =================================================================================================

// xxHash64 prime constants
static constexpr uint64_t XX64_PRIME1 = 11400714785074694791ULL;
static constexpr uint64_t XX64_PRIME2 = 14029467366386228027ULL;
static constexpr uint64_t XX64_PRIME3 = 1609587929392839161ULL;
static constexpr uint64_t XX64_PRIME4 = 9650029242287828579ULL;
static constexpr uint64_t XX64_PRIME5 = 2870177450012600261ULL;

static uint64_t xx64_round(uint64_t acc, uint64_t lane) {
    acc += lane * XX64_PRIME2;
    acc = rotl64(acc, 31);
    acc *= XX64_PRIME1;
    return acc;
}

static uint64_t xx64_avalanche(uint64_t h64) {
    h64 ^= h64 >> 33;
    h64 *= XX64_PRIME2;
    h64 ^= h64 >> 29;
    h64 *= XX64_PRIME3;
    h64 ^= h64 >> 32;
    return h64;
}

extern uint64_t xxhash64(const void * data, size_t size, uint64_t seed) {
    const uint8_t * bytes = (const uint8_t *)data;
    uint64_t h64;

    // Process by 8-byte blocks if we have enough data
    if (size >= 32) {
        const uint8_t * limit = bytes + size - 32;
        uint64_t v1 = seed + XX64_PRIME1 + XX64_PRIME2;
        uint64_t v2 = seed + XX64_PRIME2;
        uint64_t v3 = seed;
        uint64_t v4 = seed - XX64_PRIME1;

        do {
            v1 = xx64_round(v1, read_u64(bytes));
            v2 = xx64_round(v2, read_u64(bytes + 8));
            v3 = xx64_round(v3, read_u64(bytes + 16));
            v4 = xx64_round(v4, read_u64(bytes + 24));
            bytes += 32;
        } while (bytes <= limit);

        h64 = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);

        h64 = (h64 ^ xx64_round(0, v1)) * XX64_PRIME1 + XX64_PRIME4;
        h64 = (h64 ^ xx64_round(0, v2)) * XX64_PRIME1 + XX64_PRIME4;
        h64 = (h64 ^ xx64_round(0, v3)) * XX64_PRIME1 + XX64_PRIME4;
        h64 = (h64 ^ xx64_round(0, v4)) * XX64_PRIME1 + XX64_PRIME4;
    } else {
        h64 = seed + XX64_PRIME5;
    }

    h64 += (uint64_t)size;

    // Process remaining 8-byte blocks
    while (size >= 8) {
        uint64_t k1 = read_u64(bytes);
        h64 ^= xx64_round(0, k1);
        h64 = rotl64(h64, 27) * XX64_PRIME1 + XX64_PRIME4;
        bytes += 8;
        size -= 8;
    }

    // Process remaining 4-byte block if present
    if (size >= 4) {
        h64 ^= (uint64_t)read_u32(bytes) * XX64_PRIME1;
        h64 = rotl64(h64, 23) * XX64_PRIME2 + XX64_PRIME3;
        bytes += 4;
        size -= 4;
    }

    // Process remaining 1-3 bytes
    while (size > 0) {
        h64 ^= ((uint64_t)*bytes) * XX64_PRIME5;
        h64 = rotl64(h64, 11) * XX64_PRIME1;
        bytes++;
        size--;
    }

    return xx64_avalanche(h64);
}
