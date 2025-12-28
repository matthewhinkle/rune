/**
 * Hash module - Fast non-cryptographic hashing.
 *
 * Provides:
 *   - MurmurHash3 32-bit hash optimized for x86
 *   - MurmurHash2 64-bit hash (64A variant)
 *   - MurmurHash3 128-bit hash optimized for x64
 *   - xxHash64 ultra-fast 64-bit hash
 *   - Generic primitive hashing via _Generic dispatch
 *   - Convenience macros for common use cases
 *
 * Quick Reference:
 *
 *   Buffer Hashing
 *   --------------
 *   murmur32(data, len, seed)         32-bit MurmurHash3 (x86)
 *   murmur64(data, len, seed)         64-bit MurmurHash2 (64A)
 *   murmur128(data, len, seed)        128-bit MurmurHash3 (x64), returns hash128
 *   xxhash64(data, len, seed)         64-bit xxHash64 (ultra-fast)
 *   hash32(data, len)                 32-bit hash with default seed
 *   hash64(data, len)                 64-bit hash with default seed
 *   hash128(data, len)                128-bit hash with default seed
 *   xxhash64_default(data, len)       xxHash64 with default seed
 *
 *   Value Hashing
 *   -------------
 *   hash(x)                           Hash any value (primitives, pointers, structs)
 *   hash_mix(x)                       Direct 64-bit integer mixing function
 *
 *   Types
 *   -----
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
 *   uint32_t h32 = hash32(key, 5);
 *   uint64_t h64 = hash64(key, 5);
 *   uint64_t hxx = xxhash64_default(key, 5);
 *   hash128_t h128 = hash128(key, 5);
 *
 *   // Primitive hashing
 *   uint64_t hi = hash(42);
 *   uint64_t hf = hash(3.14);
 *   uint64_t hp = hash(ptr);
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

/*
 * ================================================================================================
 * TYPE DEFINITIONS
 * ================================================================================================
 */

// --------- Constants ---------

#ifdef RCFG__HASH_DEFAULT_SEED
static constexpr uint64_t R_HASH_DEFAULT_SEED = RCFG__HASH_DEFAULT_SEED;
#else  // Default seed for hash functions
static constexpr uint64_t R_HASH_DEFAULT_SEED = 0;
#endif // RCFG__HASH_DEFAULT_SEED

// --------- Hash types ---------

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
 * ================================================================================================
 * HASHING FUNCTIONS
 * ================================================================================================
 */

// --------- Buffer hashing ---------

/**
 * MurmurHash3 32-bit hash (x86 optimized).
 *
 * Fast 32-bit hash suitable for hash tables on 32-bit systems or when
 * a smaller hash is sufficient.
 *
 * @param data  Pointer to data to hash
 * @param size  Size of data in bytes
 * @param seed  Seed value for hash initialization
 * @return      32-bit hash value
 */
extern uint32_t murmur32(const void * data, size_t size, uint32_t seed);

/**
 * MurmurHash2 64-bit hash (64A variant).
 *
 * Fast 64-bit hash suitable for hash tables on 64-bit systems.
 * Uses the 64A variant which is optimized for 64-bit processors.
 *
 * @param data  Pointer to data to hash
 * @param size  Size of data in bytes
 * @param seed  Seed value for hash initialization
 * @return      64-bit hash value
 */
extern uint64_t murmur64(const void * data, size_t size, uint64_t seed);

/**
 * MurmurHash3 128-bit hash (x64 optimized).
 *
 * High-quality 128-bit hash suitable for applications requiring
 * very low collision probability or fingerprinting.
 *
 * @param data  Pointer to data to hash
 * @param size  Size of data in bytes
 * @param seed  Seed value for hash initialization
 * @return      128-bit hash value as hash128_t
 */
extern hash128_t murmur128(const void * data, size_t size, uint64_t seed);

/**
 * xxHash64 (fast 64-bit hash).
 *
 * Ultra-fast 64-bit hash function with excellent distribution.
 * Faster than MurmurHash on most modern systems, especially for larger data.
 * Suitable for hash tables and general-purpose hashing.
 *
 * @param data  Pointer to data to hash
 * @param size  Size of data in bytes
 * @param seed  Seed value for hash initialization
 * @return      64-bit hash value
 */
extern uint64_t xxhash64(const void * data, size_t size, uint64_t seed);

// --------- Convenience macros ---------

#define hash32(data, len) murmur32((data), (len), (uint32_t)R_HASH_DEFAULT_SEED)
#define hash64(data, len) murmur64((data), (len), R_HASH_DEFAULT_SEED)
#define hash128(data, len) murmur128((data), (len), R_HASH_DEFAULT_SEED)
#define xxhash64_default(data, len) xxhash64((data), (len), R_HASH_DEFAULT_SEED)

/*
 * ================================================================================================
 * PRIMITIVE HASHING
 * ================================================================================================
 */

// --------- Bit mixing ---------

/**
 * Bit mixing function for hash finalization.
 *
 * Uses the MurmurHash3 finalizer which provides excellent avalanche behavior.
 * A single bit change in input affects ~50% of output bits on average.
 *
 * @param x  64-bit value to mix
 * @return   Mixed 64-bit hash value
 */
// ReSharper disable once CppDFAUnreachableFunctionCall
static uint64_t hash_mix(uint64_t x) {
    x ^= x >> 33;
    x *= 0xFF51AFD7ED558CCDULL;
    x ^= x >> 33;
    x *= 0xC4CEB9FE1A85EC53ULL;
    x ^= x >> 33;
    return x;
}

// --------- Floating-point hashing ---------

/**
 * Hash a single-precision floating-point value.
 *
 * Handles special cases like +0.0 and -0.0 which compare equal but may have
 * different bit patterns. Normalizes them to +0.0 to ensure consistent hashing.
 *
 * @param x  Float value to hash
 * @return   64-bit hash value
 */
static uint64_t hash_float(float x) {
    if (x == 0.0f)
        x = 0.0f; // Normalize -0.0 to +0.0
    uint32_t b;
    memcpy(&b, &x, 4);
    return hash_mix(b);
}

/**
 * Hash a double-precision floating-point value.
 *
 * Handles special cases like +0.0 and -0.0 which compare equal but may have
 * different bit patterns. Normalizes them to +0.0 to ensure consistent hashing.
 *
 * @param x  Double value to hash
 * @return   64-bit hash value
 */
static uint64_t hash_double(double x) {
    if (x == 0.0)
        x = 0.0; // Normalize -0.0 to +0.0
    uint64_t b;
    memcpy(&b, &x, 8);
    return hash_mix(b);
}

// --------- Generic dispatch ---------

/**
 * Generic hash macro for primitive types.
 *
 * Hashes primitive values using compile-time type dispatch.
 * For pointers, structs, or buffers, use hash32(), hash64(), or hash128().
 *
 * Supported types:
 *   - All integer types (bool, char, short, int, long, long long and unsigned variants)
 *   - Floating-point types (float, double)
 *
 * Example:
 *   uint64_t h1 = hash(42);
 *   uint64_t h2 = hash(3.14);
 *   uint64_t h3 = hash64(buffer, len);     // For buffers, pointers, or other data
 */
#define hash(x) _Generic((x), float: hash_float(x), double: hash_double(x), default: hash_mix(x))

/*
 * ================================================================================================
 * HASH FUNCTION DISPATCH (for hash map auto-selection)
 * ================================================================================================
 */

/**
 * Hash dispatchers - Auto-select appropriate hash function by key type.
 * Used internally by hmap macro for convenient auto-selection.
 *
 * For strings: Uses FNV-1a hashing (cached for managed strings)
 * For primitives: Uses MurmurHash64
 * For pointers: Uses MurmurHash64 on pointer value
 * For custom types: Provide custom hash function via hmap_with()
 */

// String hash wrapper (for hash map dispatch)
// Uses default string options (nullptr)
static uint64_t R_HASH_FOR_STR(const char * key) {
    // Forward declare to avoid circular dependency
    extern uint64_t R_(str_hash)(const char * s, const void * opt);
    return R_(str_hash)(key, nullptr); // Use default string options
}

// Primitive hash wrappers (for hash map dispatch)
static uint64_t R_HASH_FOR_INT(int key) {
    return murmur64(&key, sizeof(int), R_HASH_DEFAULT_SEED);
}

static uint64_t R_HASH_FOR_LONG(long key) {
    return murmur64(&key, sizeof(long), R_HASH_DEFAULT_SEED);
}

static uint64_t R_HASH_FOR_UINT64(uint64_t key) {
    return murmur64(&key, sizeof(uint64_t), R_HASH_DEFAULT_SEED);
}

static uint64_t R_HASH_FOR_INT64(int64_t key) {
    return murmur64(&key, sizeof(int64_t), R_HASH_DEFAULT_SEED);
}

// Pointer hash wrapper
static uint64_t R_HASH_FOR_PTR(const void * key) {
    return murmur64(&key, sizeof(void *), R_HASH_DEFAULT_SEED);
}

// Generic bytes hash (fallback for custom types)
static uint64_t R_HASH_FOR_BYTES(const void * key, size_t size) {
    return xxhash64_default(key, size);
}

/*
 * ================================================================================================
 * UTILITY FUNCTIONS
 * ================================================================================================
 */

// --------- Hash128 operations ---------

/**
 * Compare two 128-bit hash values for equality.
 *
 * @param a  First hash value
 * @param b  Second hash value
 * @return   true if equal, false otherwise
 */
static bool hash128_eq(const hash128_t a, const hash128_t b) {
    return a.h1 == b.h1 && a.h2 == b.h2;
}

// --------- Hash combining ---------

/**
 * Combine two hash values into one.
 *
 * Useful for hashing composite keys or combining field hashes.
 * Uses the boost::hash_combine approach.
 *
 * @param h1  First hash value
 * @param h2  Second hash value
 * @return    Combined hash value
 */
static uint64_t hash_combine(const uint64_t h1, const uint64_t h2) {
    return h1 ^ (h2 + 0x9E3779B97F4A7C15ULL + (h1 << 6) + (h1 >> 2));
}

#endif // RUNE_HASH_H
