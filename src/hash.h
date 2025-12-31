/**
 * Hash module - Fast non-cryptographic hashing.
 *
 * Provides:
 *   - MurmurHash3 32-bit hash optimized for x86
 *   - MurmurHash2 64-bit hash (64A variant)
 *   - MurmurHash3 128-bit hash optimized for x64
 *   - xxHash64 ultra-fast 64-bit hash
 *   - Primitive value hashing (integers, floats, doubles)
 *   - Convenience macros for common use cases
 *
 * Quick Reference:
 *
 *   Buffer Hashing
 *   -------------------------------------------------------------------------------------------------------------------
 *   murmur32(data, len, seed)         32-bit MurmurHash3 (x86)
 *   murmur64(data, len, seed)         64-bit MurmurHash2 (64A)
 *   murmur128(data, len, seed)        128-bit MurmurHash3 (x64), returns hash128
 *   xxhash64(data, len, seed)         64-bit xxHash64 (ultra-fast)
 *   hash32(data, len)                 32-bit hash with default seed (MurmurHash3)
 *   hash64(data, len)                 64-bit hash with default seed (xxHash64)
 *   hash128(data, len)                128-bit hash with default seed (MurmurHash3)
 *
 *   Value Hashing
 *   -------------------------------------------------------------------------------------------------------------------
 *   hash_mix(x)                       64-bit integer mixing (for ints, longs, pointers)
 *   hash_float(x)                     Hash a float value
 *   hash_double(x)                    Hash a double value
 *
 *   Types
 *   -------------------------------------------------------------------------------------------------------------------
 *   hash128_t                         128-bit hash result (two uint64_t values)
 *
 * Performance Notes:
 *   - xxHash64 is fastest for large buffers (1KB+), especially on 64-bit systems
 *   - MurmurHash64 offers good balance across all sizes
 *   - MurmurHash128 best for fingerprinting and collision-sensitive use
 *   - Primitive hashing uses inline bit mixing with zero function call overhead
 *
 * Example:
 *   // Buffer hashing
 *   const char * key = "hello";
 *   uint32_t h32 = hash32(key, 5);                // MurmurHash3 32-bit
 *   uint64_t h64 = hash64(key, 5);                // xxHash64 (default 64-bit)
 *   uint64_t hxx = xxhash64(key, 5, 42);          // xxHash64 with custom seed
 *   hash128_t h128 = hash128(key, 5);             // MurmurHash3 128-bit
 *
 *   // Value hashing
 *   uint64_t hi = hash_mix(42);
 *   uint64_t hf = hash_float(3.14f);
 *   uint64_t hd = hash_double(3.14);
 *   uint64_t hp = hash_mix((uintptr_t)ptr);
 *
 * These are non-cryptographic hash functions suitable for:
 *   - Hash tables and hash maps
 *   - Checksums and fingerprinting
 *   - Load balancing and sharding
 *   - Bloom filters
 *
 * NOT suitable for:
 *   - Cryptographic purposes
 *   - Password hashing
 *   - Security-sensitive applications
 */

// ReSharper disable CppRedundantParentheses
#ifndef RUNE_HASH_H
#define RUNE_HASH_H

#include "r.h"

#include <stdint.h>
#include <string.h>

// =====================================================================================================================
// TYPE DEFINITIONS
// =====================================================================================================================

// ----------------------------------------------------- Constants -----------------------------------------------------

#ifdef RCFG__HASH_DEFAULT_SEED
static constexpr uint64_t R_HASH_DEFAULT_SEED = RCFG__HASH_DEFAULT_SEED;
#else  // Default seed for hash functions
static constexpr uint64_t R_HASH_DEFAULT_SEED = 0;
#endif // RCFG__HASH_DEFAULT_SEED

// ---------------------------------------------------- Hash types -----------------------------------------------------

/**
 * 128-bit hash result.
 *
 * Contains two 64-bit values that together form the complete hash.
 * For quick comparisons, h1 alone provides good distribution.
 */
typedef struct {
    uint64_t h1;
    uint64_t h2;
} hash128_t;

/*
 * =====================================================================================================================
 * HASHING FUNCTIONS
 * =====================================================================================================================
 */

// -------------------------------------------------- Buffer hashing ---------------------------------------------------

extern uint32_t murmur32(const void * data, size_t size, uint32_t seed);
extern uint64_t murmur64(const void * data, size_t size, uint64_t seed);
extern hash128_t murmur128(const void * data, size_t size, uint64_t seed);
extern uint64_t xxhash64(const void * data, size_t size, uint64_t seed);

// ------------------------------------------------ Convenience macros -------------------------------------------------

#define hash32(data, len) murmur32((data), (len), (uint32_t)R_HASH_DEFAULT_SEED)
#define hash64(data, len) xxhash64((data), (len), R_HASH_DEFAULT_SEED)
#define hash128(data, len) murmur128((data), (len), R_HASH_DEFAULT_SEED)

// =====================================================================================================================
// PRIMITIVE HASHING
// =====================================================================================================================

// ---------------------------------------------------- Bit mixing -----------------------------------------------------

// ReSharper disable once CppDFAUnreachableFunctionCall
static uint64_t hash_mix(uint64_t x) {
    x ^= x >> 33;
    x *= 0xFF51AFD7ED558CCDULL;
    x ^= x >> 33;
    x *= 0xC4CEB9FE1A85EC53ULL;
    x ^= x >> 33;
    return x;
}

// ---------------------------------------------- Floating-point hashing -----------------------------------------------

static uint64_t hash_float(float x) {
    if (x == 0.0f)
        x = 0.0f; // Normalize -0.0 to +0.0
    uint32_t b;
    memcpy(&b, &x, 4);
    return hash_mix(b);
}

static uint64_t hash_double(double x) {
    if (x == 0.0)
        x = 0.0; // Normalize -0.0 to +0.0
    uint64_t b;
    memcpy(&b, &x, 8);
    return hash_mix(b);
}

// =====================================================================================================================
// UTILITY FUNCTIONS
// =====================================================================================================================

// ------------------------------------------------ Hash128 operations -------------------------------------------------

static bool hash128_eq(const hash128_t a, const hash128_t b) {
    return a.h1 == b.h1 && a.h2 == b.h2;
}

// -------------------------------------------------- Hash combining ---------------------------------------------------

static uint64_t hash_combine(const uint64_t h1, const uint64_t h2) {
    return h1 ^ (h2 + 0x9E3779B97F4A7C15ULL + (h1 << 6) + (h1 >> 2));
}

#endif // RUNE_HASH_H
