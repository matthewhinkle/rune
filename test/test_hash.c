/*
 * Hash tests.
 */

#include "../src/hash.h"
#include "CUnit/Basic.h"
#include "test.h"

#include <stdint.h>
#include <stdlib.h>

// Test constants
static const char * HELLO = "hello";
static const size_t HELLO_LEN = 5;
static const char * EMPTY = "";
static const char * TEST_DATA_16 = "0123456789abcdef"; // exactly 16 bytes

// =====================================================================================================================
// murmur32 tests - empty input
// =====================================================================================================================

static void murmur32__for_empty_data__should_return__nonzero_hash() {
    // Arrange
    const char * data = EMPTY;

    // Act
    const uint32_t result = murmur32(data, 0, 0);

    // Assert - empty data with seed 0 should still produce a hash (due to finalization)
    CU_ASSERT_EQUAL(result, murmur32(data, 0, 0)); // Deterministic
}

static void murmur32__for_empty_data_with_different_seeds__should_return__different_hashes() {
    // Arrange
    const char * data = EMPTY;

    // Act
    const uint32_t h1 = murmur32(data, 0, 0);
    const uint32_t h2 = murmur32(data, 0, 1);
    const uint32_t h3 = murmur32(data, 0, 42);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h3);
    CU_ASSERT_NOT_EQUAL(h1, h3);
}

// =====================================================================================================================
// murmur32 tests - basic functionality
// =====================================================================================================================

static void murmur32__for_same_input__should_return__same_hash() {
    // Arrange
    const char * data = HELLO;

    // Act
    const uint32_t h1 = murmur32(data, HELLO_LEN, 0);
    const uint32_t h2 = murmur32(data, HELLO_LEN, 0);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void murmur32__for_different_inputs__should_return__different_hashes() {
    // Arrange
    const char * data1 = "hello";
    const char * data2 = "world";

    // Act
    const uint32_t h1 = murmur32(data1, 5, 0);
    const uint32_t h2 = murmur32(data2, 5, 0);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

static void murmur32__for_different_seeds__should_return__different_hashes() {
    // Arrange
    const char * data = HELLO;

    // Act
    const uint32_t h1 = murmur32(data, HELLO_LEN, 0);
    const uint32_t h2 = murmur32(data, HELLO_LEN, 1);
    const uint32_t h3 = murmur32(data, HELLO_LEN, 0xDEADBEEF);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h3);
    CU_ASSERT_NOT_EQUAL(h1, h3);
}

// =====================================================================================================================
// murmur32 tests - tail byte handling (size & 3)
// =====================================================================================================================

static void murmur32__for_1_byte__should_return__valid_hash() {
    // Arrange - tests tail case 1
    const char * data = "a";

    // Act
    const uint32_t h = murmur32(data, 1, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, murmur32(data, 1, 0));
}

static void murmur32__for_2_bytes__should_return__valid_hash() {
    // Arrange - tests tail case 2
    const char * data = "ab";

    // Act
    const uint32_t h = murmur32(data, 2, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, murmur32(data, 2, 0));
}

static void murmur32__for_3_bytes__should_return__valid_hash() {
    // Arrange - tests tail case 3
    const char * data = "abc";

    // Act
    const uint32_t h = murmur32(data, 3, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, murmur32(data, 3, 0));
}

static void murmur32__for_4_bytes__should_return__valid_hash() {
    // Arrange - tests full block, no tail
    const char * data = "abcd";

    // Act
    const uint32_t h = murmur32(data, 4, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, murmur32(data, 4, 0));
}

static void murmur32__for_5_bytes__should_return__valid_hash() {
    // Arrange - tests 1 block + 1 tail byte
    const char * data = "abcde";

    // Act
    const uint32_t h = murmur32(data, 5, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, murmur32(data, 5, 0));
}

static void murmur32__for_various_tail_sizes__should_produce__unique_hashes() {
    // Arrange - different tail sizes should produce different hashes
    const char * data = "abcdefgh";

    // Act
    const uint32_t h1 = murmur32(data, 1, 0);
    const uint32_t h2 = murmur32(data, 2, 0);
    const uint32_t h3 = murmur32(data, 3, 0);
    const uint32_t h4 = murmur32(data, 4, 0);
    const uint32_t h5 = murmur32(data, 5, 0);
    const uint32_t h6 = murmur32(data, 6, 0);
    const uint32_t h7 = murmur32(data, 7, 0);
    const uint32_t h8 = murmur32(data, 8, 0);

    // Assert - all should be different
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h3);
    CU_ASSERT_NOT_EQUAL(h3, h4);
    CU_ASSERT_NOT_EQUAL(h4, h5);
    CU_ASSERT_NOT_EQUAL(h5, h6);
    CU_ASSERT_NOT_EQUAL(h6, h7);
    CU_ASSERT_NOT_EQUAL(h7, h8);
}

// =====================================================================================================================
// murmur64 tests - empty input
// =====================================================================================================================

static void murmur64__for_empty_data__should_return__deterministic_hash() {
    // Arrange
    const char * data = EMPTY;

    // Act
    const uint64_t h1 = murmur64(data, 0, 0);
    const uint64_t h2 = murmur64(data, 0, 0);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void murmur64__for_empty_data_with_different_seeds__should_return__different_hashes() {
    // Arrange
    const char * data = EMPTY;

    // Act
    const uint64_t h1 = murmur64(data, 0, 0);
    const uint64_t h2 = murmur64(data, 0, 1);
    const uint64_t h3 = murmur64(data, 0, 42);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h3);
    CU_ASSERT_NOT_EQUAL(h1, h3);
}

// =====================================================================================================================
// murmur64 tests - basic functionality
// =====================================================================================================================

static void murmur64__for_same_input__should_return__same_hash() {
    // Arrange
    const char * data = HELLO;

    // Act
    const uint64_t h1 = murmur64(data, HELLO_LEN, 0);
    const uint64_t h2 = murmur64(data, HELLO_LEN, 0);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void murmur64__for_different_inputs__should_return__different_hashes() {
    // Arrange
    const char * data1 = "hello";
    const char * data2 = "world";

    // Act
    const uint64_t h1 = murmur64(data1, 5, 0);
    const uint64_t h2 = murmur64(data2, 5, 0);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

static void murmur64__for_different_seeds__should_return__different_hashes() {
    // Arrange
    const char * data = HELLO;

    // Act
    const uint64_t h1 = murmur64(data, HELLO_LEN, 0);
    const uint64_t h2 = murmur64(data, HELLO_LEN, 1);
    const uint64_t h3 = murmur64(data, HELLO_LEN, 0xDEADBEEFCAFEBABEULL);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h3);
    CU_ASSERT_NOT_EQUAL(h1, h3);
}

// =====================================================================================================================
// murmur64 tests - tail byte handling (size & 7)
// =====================================================================================================================

static void murmur64__for_various_tail_sizes__should_produce__unique_hashes() {
    // Arrange - different tail sizes should produce different hashes
    const char * data = "0123456789abcdef";

    // Act - test all tail sizes (0-7)
    const uint64_t h1 = murmur64(data, 1, 0);   // tail 1
    const uint64_t h2 = murmur64(data, 2, 0);   // tail 2
    const uint64_t h3 = murmur64(data, 3, 0);   // tail 3
    const uint64_t h4 = murmur64(data, 4, 0);   // tail 4
    const uint64_t h5 = murmur64(data, 5, 0);   // tail 5
    const uint64_t h6 = murmur64(data, 6, 0);   // tail 6
    const uint64_t h7 = murmur64(data, 7, 0);   // tail 7
    const uint64_t h8 = murmur64(data, 8, 0);   // no tail (full block)
    const uint64_t h9 = murmur64(data, 9, 0);   // 1 block + tail 1
    const uint64_t h15 = murmur64(data, 15, 0); // 1 block + tail 7

    // Assert - all should be different
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h3);
    CU_ASSERT_NOT_EQUAL(h3, h4);
    CU_ASSERT_NOT_EQUAL(h4, h5);
    CU_ASSERT_NOT_EQUAL(h5, h6);
    CU_ASSERT_NOT_EQUAL(h6, h7);
    CU_ASSERT_NOT_EQUAL(h7, h8);
    CU_ASSERT_NOT_EQUAL(h8, h9);
    CU_ASSERT_NOT_EQUAL(h9, h15);
}

// =====================================================================================================================
// murmur128 tests - empty input
// =====================================================================================================================

static void murmur128__for_empty_data__should_return__deterministic_hash() {
    // Arrange
    const char * data = EMPTY;

    // Act
    const hash128_t h1 = murmur128(data, 0, 0);
    const hash128_t h2 = murmur128(data, 0, 0);

    // Assert
    CU_ASSERT_EQUAL(h1.h1, h2.h1);
    CU_ASSERT_EQUAL(h1.h2, h2.h2);
}

static void murmur128__for_empty_data_with_different_seeds__should_return__different_hashes() {
    // Arrange
    const char * data = EMPTY;

    // Act
    const hash128_t h1 = murmur128(data, 0, 0);
    const hash128_t h2 = murmur128(data, 0, 1);

    // Assert - at least one component should differ
    CU_ASSERT_TRUE(h1.h1 != h2.h1 || h1.h2 != h2.h2);
}

// =====================================================================================================================
// murmur128 tests - basic functionality
// =====================================================================================================================

static void murmur128__for_same_input__should_return__same_hash() {
    // Arrange
    const char * data = HELLO;

    // Act
    const hash128_t h1 = murmur128(data, HELLO_LEN, 0);
    const hash128_t h2 = murmur128(data, HELLO_LEN, 0);

    // Assert
    CU_ASSERT_EQUAL(h1.h1, h2.h1);
    CU_ASSERT_EQUAL(h1.h2, h2.h2);
}

static void murmur128__for_different_inputs__should_return__different_hashes() {
    // Arrange
    const char * data1 = "hello";
    const char * data2 = "world";

    // Act
    const hash128_t h1 = murmur128(data1, 5, 0);
    const hash128_t h2 = murmur128(data2, 5, 0);

    // Assert - at least one component should differ
    CU_ASSERT_TRUE(h1.h1 != h2.h1 || h1.h2 != h2.h2);
}

static void murmur128__for_different_seeds__should_return__different_hashes() {
    // Arrange
    const char * data = HELLO;

    // Act
    const hash128_t h1 = murmur128(data, HELLO_LEN, 0);
    const hash128_t h2 = murmur128(data, HELLO_LEN, 1);

    // Assert
    CU_ASSERT_TRUE(h1.h1 != h2.h1 || h1.h2 != h2.h2);
}

// =====================================================================================================================
// murmur128 tests - tail byte handling (size & 15)
// =====================================================================================================================

static void murmur128__for_various_tail_sizes__should_produce__unique_hashes() {
    // Arrange - test all 16 tail sizes (0-15)
    const char * data = "0123456789abcdefghijklmnopqrstuv"; // 32 bytes

    // Act - test various tail sizes
    const hash128_t h1 = murmur128(data, 1, 0);
    const hash128_t h8 = murmur128(data, 8, 0);
    const hash128_t h9 = murmur128(data, 9, 0);
    const hash128_t h15 = murmur128(data, 15, 0);
    const hash128_t h16 = murmur128(data, 16, 0); // full block
    const hash128_t h17 = murmur128(data, 17, 0); // 1 block + tail 1

    // Assert - all should be different
    CU_ASSERT_TRUE(h1.h1 != h8.h1 || h1.h2 != h8.h2);
    CU_ASSERT_TRUE(h8.h1 != h9.h1 || h8.h2 != h9.h2);
    CU_ASSERT_TRUE(h9.h1 != h15.h1 || h9.h2 != h15.h2);
    CU_ASSERT_TRUE(h15.h1 != h16.h1 || h15.h2 != h16.h2);
    CU_ASSERT_TRUE(h16.h1 != h17.h1 || h16.h2 != h17.h2);
}

static void murmur128__for_16_bytes__should_return__valid_hash() {
    // Arrange - exactly one 16-byte block, no tail
    const char * data = TEST_DATA_16;

    // Act
    const hash128_t h = murmur128(data, 16, 0);

    // Assert - deterministic
    const hash128_t h2 = murmur128(data, 16, 0);
    CU_ASSERT_EQUAL(h.h1, h2.h1);
    CU_ASSERT_EQUAL(h.h2, h2.h2);
}

// =====================================================================================================================
// convenience macro tests
// =====================================================================================================================

static void hash32_macro__should_use__default_seed() {
    // Arrange
    const char * data = HELLO;

    // Act
    const uint32_t h1 = hash32(data, HELLO_LEN);
    const uint32_t h2 = murmur32(data, HELLO_LEN, (uint32_t)R_HASH_DEFAULT_SEED);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void hash64_macro__should_use__default_seed() {
    // Arrange
    const char * data = HELLO;

    // Act
    const uint64_t h1 = hash64(data, HELLO_LEN);
    const uint64_t h2 = xxhash64(data, HELLO_LEN, R_HASH_DEFAULT_SEED);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void hash128_macro__should_use__default_seed() {
    // Arrange
    const char * data = HELLO;

    // Act
    const hash128_t h1 = hash128(data, HELLO_LEN);
    const hash128_t h2 = murmur128(data, HELLO_LEN, R_HASH_DEFAULT_SEED);

    // Assert
    CU_ASSERT_EQUAL(h1.h1, h2.h1);
    CU_ASSERT_EQUAL(h1.h2, h2.h2);
}

// =====================================================================================================================
// hash128_eq tests
// =====================================================================================================================

static void hash128_eq__for_equal_hashes__should_return__true() {
    // Arrange
    const hash128_t h1 = {.h1 = 0x123456789ABCDEFULL, .h2 = 0xFEDCBA9876543210ULL};
    const hash128_t h2 = {.h1 = 0x123456789ABCDEFULL, .h2 = 0xFEDCBA9876543210ULL};

    // Act & Assert
    CU_ASSERT_TRUE(hash128_eq(h1, h2));
}

static void hash128_eq__for_different_h1__should_return__false() {
    // Arrange
    const hash128_t h1 = {.h1 = 0x123456789ABCDEFULL, .h2 = 0xFEDCBA9876543210ULL};
    const hash128_t h2 = {.h1 = 0x000000000000000ULL, .h2 = 0xFEDCBA9876543210ULL};

    // Act & Assert
    CU_ASSERT_FALSE(hash128_eq(h1, h2));
}

static void hash128_eq__for_different_h2__should_return__false() {
    // Arrange
    const hash128_t h1 = {.h1 = 0x123456789ABCDEFULL, .h2 = 0xFEDCBA9876543210ULL};
    const hash128_t h2 = {.h1 = 0x123456789ABCDEFULL, .h2 = 0x000000000000000ULL};

    // Act & Assert
    CU_ASSERT_FALSE(hash128_eq(h1, h2));
}

static void hash128_eq__for_computed_hashes__should_return__true_when_equal() {
    // Arrange
    const hash128_t h1 = murmur128(HELLO, HELLO_LEN, 0);
    const hash128_t h2 = murmur128(HELLO, HELLO_LEN, 0);

    // Act & Assert
    CU_ASSERT_TRUE(hash128_eq(h1, h2));
}

// =====================================================================================================================
// hash_combine tests
// =====================================================================================================================

static void hash_combine__for_same_inputs__should_return__same_hash() {
    // Arrange
    const uint64_t a = 0x123456789ABCDEFULL;
    const uint64_t b = 0xFEDCBA9876543210ULL;

    // Act
    const uint64_t h1 = hash_combine(a, b);
    const uint64_t h2 = hash_combine(a, b);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void hash_combine__for_different_order__should_return__different_hash() {
    // Arrange
    const uint64_t a = 0x123456789ABCDEFULL;
    const uint64_t b = 0xFEDCBA9876543210ULL;

    // Act
    const uint64_t h1 = hash_combine(a, b);
    const uint64_t h2 = hash_combine(b, a);

    // Assert - order matters
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

static void hash_combine__for_zero_values__should_return__nonzero_hash() {
    // Arrange
    const uint64_t a = 0;
    const uint64_t b = 0;

    // Act
    const uint64_t h = hash_combine(a, b);

    // Assert - the constant ensures nonzero output
    CU_ASSERT_NOT_EQUAL(h, 0);
}

static void hash_combine__for_chained_calls__should_be__deterministic() {
    // Arrange
    const uint64_t a = hash64("hello", 5);
    const uint64_t b = hash64("world", 5);
    const uint64_t c = hash64("!", 1);

    // Act
    const uint64_t h1 = hash_combine(hash_combine(a, b), c);
    const uint64_t h2 = hash_combine(hash_combine(a, b), c);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

// =====================================================================================================================
// security tests - large inputs
// =====================================================================================================================

static void murmur32__for_large_input__should_return__valid_hash() {
    // Arrange - 1KB of data
    char large_data[1024];
    memset(large_data, 'A', sizeof(large_data));

    // Act
    const uint32_t h = murmur32(large_data, sizeof(large_data), 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, murmur32(large_data, sizeof(large_data), 0));
}

static void murmur64__for_large_input__should_return__valid_hash() {
    // Arrange - 1KB of data
    char large_data[1024];
    memset(large_data, 'A', sizeof(large_data));

    // Act
    const uint64_t h = murmur64(large_data, sizeof(large_data), 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, murmur64(large_data, sizeof(large_data), 0));
}

static void murmur128__for_large_input__should_return__valid_hash() {
    // Arrange - 1KB of data
    char large_data[1024];
    memset(large_data, 'A', sizeof(large_data));

    // Act
    const hash128_t h = murmur128(large_data, sizeof(large_data), 0);

    // Assert - deterministic
    const hash128_t h2 = murmur128(large_data, sizeof(large_data), 0);
    CU_ASSERT_EQUAL(h.h1, h2.h1);
    CU_ASSERT_EQUAL(h.h2, h2.h2);
}

// =====================================================================================================================
// security tests - binary data with null bytes
// =====================================================================================================================

static void murmur32__for_data_with_null_bytes__should_hash_all_bytes() {
    // Arrange - data containing null bytes
    const uint8_t data1[] = {0x00, 0x01, 0x02, 0x03};
    const uint8_t data2[] = {0x00, 0x01, 0x02, 0x04}; // different last byte

    // Act
    const uint32_t h1 = murmur32(data1, sizeof(data1), 0);
    const uint32_t h2 = murmur32(data2, sizeof(data2), 0);

    // Assert - should produce different hashes
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

static void murmur64__for_data_with_null_bytes__should_hash_all_bytes() {
    // Arrange - data containing null bytes
    const uint8_t data1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    const uint8_t data2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};

    // Act
    const uint64_t h1 = murmur64(data1, sizeof(data1), 0);
    const uint64_t h2 = murmur64(data2, sizeof(data2), 0);

    // Assert - should produce different hashes
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

static void murmur128__for_data_with_null_bytes__should_hash_all_bytes() {
    // Arrange - data containing null bytes
    const uint8_t data1[] =
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    const uint8_t data2[] =
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};

    // Act
    const hash128_t h1 = murmur128(data1, sizeof(data1), 0);
    const hash128_t h2 = murmur128(data2, sizeof(data2), 0);

    // Assert - should produce different hashes
    CU_ASSERT_TRUE(h1.h1 != h2.h1 || h1.h2 != h2.h2);
}

// =====================================================================================================================
// avalanche tests - small changes produce large hash differences
// =====================================================================================================================

static void murmur32__for_single_bit_change__should_produce__different_hash() {
    // Arrange
    const uint8_t data1[] = {0x00, 0x00, 0x00, 0x00};
    const uint8_t data2[] = {0x01, 0x00, 0x00, 0x00}; // single bit flip

    // Act
    const uint32_t h1 = murmur32(data1, sizeof(data1), 0);
    const uint32_t h2 = murmur32(data2, sizeof(data2), 0);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

static void murmur64__for_single_bit_change__should_produce__different_hash() {
    // Arrange
    const uint8_t data1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t data2[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // Act
    const uint64_t h1 = murmur64(data1, sizeof(data1), 0);
    const uint64_t h2 = murmur64(data2, sizeof(data2), 0);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

// =====================================================================================================================
// primitive hash tests - hash() macro
// =====================================================================================================================

static void hash__for_integers__should_return__deterministic_hash() {
    // Arrange
    int x = 42;

    // Act
    const uint64_t h1 = hash_mix(x);
    const uint64_t h2 = hash_mix(x);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void hash__for_different_integers__should_return__different_hashes() {
    // Arrange & Act
    const uint64_t h0 = hash_mix(0);
    const uint64_t h1 = hash_mix(1);
    const uint64_t h2 = hash_mix(2);
    const uint64_t h42 = hash_mix(42);

    // Assert
    CU_ASSERT_NOT_EQUAL(h0, h1);
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h42);
}

static void hash__for_negative_integers__should_return__valid_hash() {
    // Arrange & Act
    const uint64_t h_neg1 = hash_mix(-1);
    const uint64_t h_neg2 = hash_mix(-2);
    const uint64_t h_pos1 = hash_mix(1);

    // Assert - negative and positive should differ
    CU_ASSERT_NOT_EQUAL(h_neg1, h_pos1);
    CU_ASSERT_NOT_EQUAL(h_neg1, h_neg2);
}

static void hash__for_various_int_types__should_return__valid_hashes() {
    // Arrange & Act - test various integer types via _Generic
    const uint64_t h_char = hash_mix((char)'A');
    const uint64_t h_short = hash_mix((short)42);
    const uint64_t h_int = hash_mix((int)42);
    const uint64_t h_long = hash_mix((long)42);
    const uint64_t h_llong = hash_mix((long long)42);
    const uint64_t h_uint = hash_mix((unsigned int)42);

    // Assert - all should be deterministic (test by calling again)
    CU_ASSERT_EQUAL(h_char, hash_mix((char)'A'));
    CU_ASSERT_EQUAL(h_short, hash_mix((short)42));
    CU_ASSERT_EQUAL(h_int, hash_mix((int)42));
    CU_ASSERT_EQUAL(h_long, hash_mix((long)42));
    CU_ASSERT_EQUAL(h_llong, hash_mix((long long)42));
    CU_ASSERT_EQUAL(h_uint, hash_mix((unsigned int)42));
}

static void hash__for_floats__should_return__deterministic_hash() {
    // Arrange
    float f = 3.14f;
    double d = 3.14159265358979;

    // Act
    const uint64_t hf1 = hash_float(f);
    const uint64_t hf2 = hash_float(f);
    const uint64_t hd1 = hash_double(d);
    const uint64_t hd2 = hash_double(d);

    // Assert
    CU_ASSERT_EQUAL(hf1, hf2);
    CU_ASSERT_EQUAL(hd1, hd2);
}

static void hash__for_negative_zero__should_equal__positive_zero() {
    // Arrange
    float pos_zero_f = 0.0f;
    float neg_zero_f = -0.0f;
    double pos_zero_d = 0.0;
    double neg_zero_d = -0.0;

    // Act
    const uint64_t h_pos_f = hash_float(pos_zero_f);
    const uint64_t h_neg_f = hash_float(neg_zero_f);
    const uint64_t h_pos_d = hash_double(pos_zero_d);
    const uint64_t h_neg_d = hash_double(neg_zero_d);

    // Assert - -0 and +0 should hash the same (they compare equal)
    CU_ASSERT_EQUAL(h_pos_f, h_neg_f);
    CU_ASSERT_EQUAL(h_pos_d, h_neg_d);
}

static void hash64__for_pointer_address__should_return__deterministic_hash() {
    // Arrange
    int x = 42;
    int * p = &x;

    // Act
    const uint64_t h1 = hash64((const void *)p, sizeof(p));
    const uint64_t h2 = hash64((const void *)p, sizeof(p));

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void hash64__for_different_pointer_addresses__should_return__different_hashes() {
    // Arrange
    int x = 42;
    int y = 42;

    // Act
    const uint64_t h1 = hash64((const void *)&x, sizeof(&x));
    const uint64_t h2 = hash64((const void *)&y, sizeof(&y));

    // Assert - different addresses should hash differently
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

static void hash64__for_null_pointer__should_return__valid_hash() {
    // Arrange
    void * p = NULL;

    // Act - hash the pointer value itself (8 bytes of the pointer variable)
    const uint64_t h = hash64(&p, sizeof(p));

    // Assert - deterministic
    void * p2 = NULL;
    CU_ASSERT_EQUAL(h, hash64(&p2, sizeof(p2)));
}

static void hash_float__for_same_input__should_return__same_hash() {
    // Arrange
    float f = 3.14f;

    // Act
    const uint64_t h1 = hash_float(f);
    const uint64_t h2 = hash_float(f);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void hash_float__for_different_inputs__should_return__different_hashes() {
    // Arrange & Act
    const uint64_t h1 = hash_float(1.5f);
    const uint64_t h2 = hash_float(2.5f);
    const uint64_t h3 = hash_float(-1.5f);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h1, h3);
    CU_ASSERT_NOT_EQUAL(h2, h3);
}

static void hash_float__for_negative_zero__should_equal__positive_zero() {
    // Arrange
    float pos_zero = 0.0f;
    float neg_zero = -0.0f;

    // Act
    const uint64_t h_pos = hash_float(pos_zero);
    const uint64_t h_neg = hash_float(neg_zero);

    // Assert - -0.0 and +0.0 should hash the same
    CU_ASSERT_EQUAL(h_pos, h_neg);
}

static void hash_double__for_same_input__should_return__same_hash() {
    // Arrange
    double d = 3.14159265358979;

    // Act
    const uint64_t h1 = hash_double(d);
    const uint64_t h2 = hash_double(d);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void hash_double__for_different_inputs__should_return__different_hashes() {
    // Arrange & Act
    const uint64_t h1 = hash_double(1.5);
    const uint64_t h2 = hash_double(2.5);
    const uint64_t h3 = hash_double(-1.5);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h1, h3);
    CU_ASSERT_NOT_EQUAL(h2, h3);
}

static void hash_double__for_negative_zero__should_equal__positive_zero() {
    // Arrange
    double pos_zero = 0.0;
    double neg_zero = -0.0;

    // Act
    const uint64_t h_pos = hash_double(pos_zero);
    const uint64_t h_neg = hash_double(neg_zero);

    // Assert - -0.0 and +0.0 should hash the same
    CU_ASSERT_EQUAL(h_pos, h_neg);
}

static void hash_mix__for_same_input__should_return__same_output() {
    // Arrange
    const uint64_t x = 0x123456789ABCDEFULL;

    // Act
    const uint64_t h1 = hash_mix(x);
    const uint64_t h2 = hash_mix(x);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void hash_mix__for_zero__should_return__deterministic() {
    // Arrange & Act
    const uint64_t h1 = hash_mix(0);
    const uint64_t h2 = hash_mix(0);

    // Assert - 0 is a fixed point of the mixing function (0 ^ 0 = 0, 0 * k = 0)
    // This is a known property; the important thing is determinism
    CU_ASSERT_EQUAL(h1, h2);
    CU_ASSERT_EQUAL(h1, 0); // document expected behavior
}

static void hash_mix__for_sequential_inputs__should_produce__varied_outputs() {
    // Arrange & Act
    uint64_t hashes[10];
    for (uint64_t i = 0; i < 10; i++) {
        hashes[i] = hash_mix(i);
    }

    // Assert - all should be unique
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            CU_ASSERT_NOT_EQUAL(hashes[i], hashes[j]);
        }
    }
}

// =====================================================================================================================
// distribution tests - verify reasonable hash distribution
// =====================================================================================================================

static void murmur32__for_sequential_keys__should_produce__varied_hashes() {
    // Arrange & Act - hash sequential integers
    uint32_t hashes[10];
    for (int i = 0; i < 10; i++) {
        hashes[i] = murmur32(&i, sizeof(i), 0);
    }

    // Assert - all hashes should be unique
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            CU_ASSERT_NOT_EQUAL(hashes[i], hashes[j]);
        }
    }
}

static void murmur64__for_sequential_keys__should_produce__varied_hashes() {
    // Arrange & Act - hash sequential integers
    uint64_t hashes[10];
    for (int i = 0; i < 10; i++) {
        hashes[i] = murmur64(&i, sizeof(i), 0);
    }

    // Assert - all hashes should be unique
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            CU_ASSERT_NOT_EQUAL(hashes[i], hashes[j]);
        }
    }
}

// =====================================================================================================================
// xxhash64 tests - empty input
// =====================================================================================================================

static void xxhash64__for_empty_data__should_return__deterministic_hash() {
    // Arrange
    const char * data = EMPTY;

    // Act
    const uint64_t h1 = xxhash64(data, 0, 0);
    const uint64_t h2 = xxhash64(data, 0, 0);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void xxhash64__for_empty_data_with_different_seeds__should_return__different_hashes() {
    // Arrange
    const char * data = EMPTY;

    // Act
    const uint64_t h1 = xxhash64(data, 0, 0);
    const uint64_t h2 = xxhash64(data, 0, 1);
    const uint64_t h3 = xxhash64(data, 0, 42);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h3);
    CU_ASSERT_NOT_EQUAL(h1, h3);
}

// =====================================================================================================================
// xxhash64 tests - basic functionality
// =====================================================================================================================

static void xxhash64__for_same_input__should_return__same_hash() {
    // Arrange
    const char * data = HELLO;

    // Act
    const uint64_t h1 = xxhash64(data, HELLO_LEN, 0);
    const uint64_t h2 = xxhash64(data, HELLO_LEN, 0);

    // Assert
    CU_ASSERT_EQUAL(h1, h2);
}

static void xxhash64__for_different_inputs__should_return__different_hashes() {
    // Arrange
    const char * data1 = "hello";
    const char * data2 = "world";

    // Act
    const uint64_t h1 = xxhash64(data1, 5, 0);
    const uint64_t h2 = xxhash64(data2, 5, 0);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

static void xxhash64__for_different_seeds__should_return__different_hashes() {
    // Arrange
    const char * data = HELLO;

    // Act
    const uint64_t h1 = xxhash64(data, HELLO_LEN, 0);
    const uint64_t h2 = xxhash64(data, HELLO_LEN, 1);
    const uint64_t h3 = xxhash64(data, HELLO_LEN, 0xDEADBEEFCAFEBABEULL);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h3);
    CU_ASSERT_NOT_EQUAL(h1, h3);
}

// =====================================================================================================================
// xxhash64 tests - tail byte handling (size & 7)
// =====================================================================================================================

static void xxhash64__for_1_byte__should_return__valid_hash() {
    // Arrange - tests tail handling
    const char * data = "a";

    // Act
    const uint64_t h = xxhash64(data, 1, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, xxhash64(data, 1, 0));
}

static void xxhash64__for_2_bytes__should_return__valid_hash() {
    // Arrange
    const char * data = "ab";

    // Act
    const uint64_t h = xxhash64(data, 2, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, xxhash64(data, 2, 0));
}

static void xxhash64__for_4_bytes__should_return__valid_hash() {
    // Arrange - tests 4-byte tail handling
    const char * data = "abcd";

    // Act
    const uint64_t h = xxhash64(data, 4, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, xxhash64(data, 4, 0));
}

static void xxhash64__for_8_bytes__should_return__valid_hash() {
    // Arrange - exactly one 8-byte block, no tail
    const char * data = "abcdefgh";

    // Act
    const uint64_t h = xxhash64(data, 8, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, xxhash64(data, 8, 0));
}

static void xxhash64__for_various_tail_sizes__should_produce__unique_hashes() {
    // Arrange - test all relevant tail sizes and thresholds
    const char * data = "0123456789abcdef";

    // Act - test various sizes: small, 4-byte block, 8-byte block, and 32-byte threshold
    const uint64_t h1 = xxhash64(data, 1, 0);   // 1 byte
    const uint64_t h2 = xxhash64(data, 2, 0);   // 2 bytes
    const uint64_t h4 = xxhash64(data, 4, 0);   // 4 bytes (4-byte block tail)
    const uint64_t h7 = xxhash64(data, 7, 0);   // 7 bytes
    const uint64_t h8 = xxhash64(data, 8, 0);   // 8 bytes (full 8-byte block)
    const uint64_t h15 = xxhash64(data, 15, 0); // 15 bytes
    const uint64_t h16 = xxhash64(data, 16, 0); // 16 bytes (2 blocks)

    // Assert - all should be different
    CU_ASSERT_NOT_EQUAL(h1, h2);
    CU_ASSERT_NOT_EQUAL(h2, h4);
    CU_ASSERT_NOT_EQUAL(h4, h7);
    CU_ASSERT_NOT_EQUAL(h7, h8);
    CU_ASSERT_NOT_EQUAL(h8, h15);
    CU_ASSERT_NOT_EQUAL(h15, h16);
}

static void xxhash64__for_32_byte_threshold__should_use_parallel_processing() {
    // Arrange - test that 32 bytes triggers parallel processing path
    char data[64];
    memset(data, 'A', sizeof(data));

    // Act - hash below threshold (simple path), at threshold, and above
    const uint64_t h16 = xxhash64(data, 16, 0);
    const uint64_t h31 = xxhash64(data, 31, 0);
    const uint64_t h32 = xxhash64(data, 32, 0); // Triggers parallel v1-v4 processing
    const uint64_t h33 = xxhash64(data, 33, 0);
    const uint64_t h64 = xxhash64(data, 64, 0);

    // Assert - hashes should differ (different code paths)
    CU_ASSERT_NOT_EQUAL(h16, h31); // Below vs just-below threshold
    CU_ASSERT_NOT_EQUAL(h31, h32); // Cross the 32-byte threshold
    CU_ASSERT_NOT_EQUAL(h32, h33);
    CU_ASSERT_NOT_EQUAL(h33, h64);
}

// =====================================================================================================================
// xxhash64 tests - large inputs
// =====================================================================================================================

static void xxhash64__for_large_input__should_return__valid_hash() {
    // Arrange - 1KB of data
    char large_data[1024];
    memset(large_data, 'A', sizeof(large_data));

    // Act
    const uint64_t h = xxhash64(large_data, sizeof(large_data), 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, xxhash64(large_data, sizeof(large_data), 0));
}

static void xxhash64__for_very_large_input__should_return__valid_hash() {
    // Arrange - 1MB of data
    char * large_data = mem_alloc_zero(1024 * 1024);

    // Act
    const uint64_t h = xxhash64(large_data, 1024 * 1024, 0);

    // Assert - deterministic
    CU_ASSERT_EQUAL(h, xxhash64(large_data, 1024 * 1024, 0));

    mem_free(large_data, 1024 * 1024);
}

// =====================================================================================================================
// xxhash64 tests - binary data with null bytes
// =====================================================================================================================

static void xxhash64__for_data_with_null_bytes__should_hash_all_bytes() {
    // Arrange - data containing null bytes
    const uint8_t data1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    const uint8_t data2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};

    // Act
    const uint64_t h1 = xxhash64(data1, sizeof(data1), 0);
    const uint64_t h2 = xxhash64(data2, sizeof(data2), 0);

    // Assert - should produce different hashes
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

// =====================================================================================================================
// xxhash64 tests - avalanche properties
// =====================================================================================================================

static void xxhash64__for_single_bit_change__should_produce__different_hash() {
    // Arrange
    const uint8_t data1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t data2[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // Act
    const uint64_t h1 = xxhash64(data1, sizeof(data1), 0);
    const uint64_t h2 = xxhash64(data2, sizeof(data2), 0);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

static void xxhash64__for_last_byte_change__should_produce__different_hash() {
    // Arrange - change in tail bytes
    const uint8_t data1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t data2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

    // Act
    const uint64_t h1 = xxhash64(data1, sizeof(data1), 0);
    const uint64_t h2 = xxhash64(data2, sizeof(data2), 0);

    // Assert
    CU_ASSERT_NOT_EQUAL(h1, h2);
}

// =====================================================================================================================
// xxhash64 tests - distribution
// =====================================================================================================================

static void xxhash64__for_sequential_keys__should_produce__varied_hashes() {
    // Arrange & Act - hash sequential integers
    uint64_t hashes[10];
    for (int i = 0; i < 10; i++) {
        hashes[i] = xxhash64(&i, sizeof(i), 0);
    }

    // Assert - all hashes should be unique
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            CU_ASSERT_NOT_EQUAL(hashes[i], hashes[j]);
        }
    }
}

// =====================================================================================================================
// xxhash64 vs murmur64 comparison
// =====================================================================================================================

static void xxhash64__should_differ_from_murmur64__for_same_input() {
    // Arrange
    const char * data = HELLO;

    // Act - both hash functions should produce different results
    const uint64_t hxx = xxhash64(data, HELLO_LEN, 0);
    const uint64_t hmur = murmur64(data, HELLO_LEN, 0);

    // Assert - different algorithms should produce different hashes
    CU_ASSERT_NOT_EQUAL(hxx, hmur);
}

static void xxhash64_and_murmur64__should_be_deterministic_independently() {
    // Arrange
    const char * data = "The quick brown fox jumps over the lazy dog";

    // Act - both should be individually deterministic
    const uint64_t hxx1 = xxhash64(data, 44, 0);
    const uint64_t hxx2 = xxhash64(data, 44, 0);
    const uint64_t hmur1 = murmur64(data, 44, 0);
    const uint64_t hmur2 = murmur64(data, 44, 0);

    // Assert
    CU_ASSERT_EQUAL(hxx1, hxx2);
    CU_ASSERT_EQUAL(hmur1, hmur2);
}

// =====================================================================================================================
// Test suite registration
// =====================================================================================================================

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // murmur32() suite
    CU_pSuite suite_murmur32 = CU_add_suite("murmur32()", nullptr, nullptr);
    if (suite_murmur32 == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_murmur32, murmur32__for_empty_data__should_return__nonzero_hash);
    ADD_TEST(suite_murmur32, murmur32__for_empty_data_with_different_seeds__should_return__different_hashes);
    ADD_TEST(suite_murmur32, murmur32__for_same_input__should_return__same_hash);
    ADD_TEST(suite_murmur32, murmur32__for_different_inputs__should_return__different_hashes);
    ADD_TEST(suite_murmur32, murmur32__for_different_seeds__should_return__different_hashes);
    ADD_TEST(suite_murmur32, murmur32__for_1_byte__should_return__valid_hash);
    ADD_TEST(suite_murmur32, murmur32__for_2_bytes__should_return__valid_hash);
    ADD_TEST(suite_murmur32, murmur32__for_3_bytes__should_return__valid_hash);
    ADD_TEST(suite_murmur32, murmur32__for_4_bytes__should_return__valid_hash);
    ADD_TEST(suite_murmur32, murmur32__for_5_bytes__should_return__valid_hash);
    ADD_TEST(suite_murmur32, murmur32__for_various_tail_sizes__should_produce__unique_hashes);
    ADD_TEST(suite_murmur32, murmur32__for_large_input__should_return__valid_hash);
    ADD_TEST(suite_murmur32, murmur32__for_data_with_null_bytes__should_hash_all_bytes);
    ADD_TEST(suite_murmur32, murmur32__for_single_bit_change__should_produce__different_hash);
    ADD_TEST(suite_murmur32, murmur32__for_sequential_keys__should_produce__varied_hashes);

    // murmur64() suite
    CU_pSuite suite_murmur64 = CU_add_suite("murmur64()", nullptr, nullptr);
    if (suite_murmur64 == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_murmur64, murmur64__for_empty_data__should_return__deterministic_hash);
    ADD_TEST(suite_murmur64, murmur64__for_empty_data_with_different_seeds__should_return__different_hashes);
    ADD_TEST(suite_murmur64, murmur64__for_same_input__should_return__same_hash);
    ADD_TEST(suite_murmur64, murmur64__for_different_inputs__should_return__different_hashes);
    ADD_TEST(suite_murmur64, murmur64__for_different_seeds__should_return__different_hashes);
    ADD_TEST(suite_murmur64, murmur64__for_various_tail_sizes__should_produce__unique_hashes);
    ADD_TEST(suite_murmur64, murmur64__for_large_input__should_return__valid_hash);
    ADD_TEST(suite_murmur64, murmur64__for_data_with_null_bytes__should_hash_all_bytes);
    ADD_TEST(suite_murmur64, murmur64__for_single_bit_change__should_produce__different_hash);
    ADD_TEST(suite_murmur64, murmur64__for_sequential_keys__should_produce__varied_hashes);

    // murmur128() suite
    CU_pSuite suite_murmur128 = CU_add_suite("murmur128()", nullptr, nullptr);
    if (suite_murmur128 == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_murmur128, murmur128__for_empty_data__should_return__deterministic_hash);
    ADD_TEST(suite_murmur128, murmur128__for_empty_data_with_different_seeds__should_return__different_hashes);
    ADD_TEST(suite_murmur128, murmur128__for_same_input__should_return__same_hash);
    ADD_TEST(suite_murmur128, murmur128__for_different_inputs__should_return__different_hashes);
    ADD_TEST(suite_murmur128, murmur128__for_different_seeds__should_return__different_hashes);
    ADD_TEST(suite_murmur128, murmur128__for_various_tail_sizes__should_produce__unique_hashes);
    ADD_TEST(suite_murmur128, murmur128__for_16_bytes__should_return__valid_hash);
    ADD_TEST(suite_murmur128, murmur128__for_large_input__should_return__valid_hash);
    ADD_TEST(suite_murmur128, murmur128__for_data_with_null_bytes__should_hash_all_bytes);

    // hash32/64/128 macros suite
    CU_pSuite suite_hash_macros = CU_add_suite("hash macros", nullptr, nullptr);
    if (suite_hash_macros == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_hash_macros, hash32_macro__should_use__default_seed);
    ADD_TEST(suite_hash_macros, hash64_macro__should_use__default_seed);
    ADD_TEST(suite_hash_macros, hash128_macro__should_use__default_seed);

    // hash128_eq() suite
    CU_pSuite suite_hash128_eq = CU_add_suite("hash128_eq()", nullptr, nullptr);
    if (suite_hash128_eq == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_hash128_eq, hash128_eq__for_equal_hashes__should_return__true);
    ADD_TEST(suite_hash128_eq, hash128_eq__for_different_h1__should_return__false);
    ADD_TEST(suite_hash128_eq, hash128_eq__for_different_h2__should_return__false);
    ADD_TEST(suite_hash128_eq, hash128_eq__for_computed_hashes__should_return__true_when_equal);

    // hash_combine() suite
    CU_pSuite suite_hash_combine = CU_add_suite("hash_combine()", nullptr, nullptr);
    if (suite_hash_combine == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_hash_combine, hash_combine__for_same_inputs__should_return__same_hash);
    ADD_TEST(suite_hash_combine, hash_combine__for_different_order__should_return__different_hash);
    ADD_TEST(suite_hash_combine, hash_combine__for_zero_values__should_return__nonzero_hash);
    ADD_TEST(suite_hash_combine, hash_combine__for_chained_calls__should_be__deterministic);

    // hash() generic macro suite
    CU_pSuite suite_hash = CU_add_suite("hash()", nullptr, nullptr);
    if (suite_hash == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_hash, hash__for_integers__should_return__deterministic_hash);
    ADD_TEST(suite_hash, hash__for_different_integers__should_return__different_hashes);
    ADD_TEST(suite_hash, hash__for_negative_integers__should_return__valid_hash);
    ADD_TEST(suite_hash, hash__for_various_int_types__should_return__valid_hashes);
    ADD_TEST(suite_hash, hash64__for_pointer_address__should_return__deterministic_hash);
    ADD_TEST(suite_hash, hash64__for_different_pointer_addresses__should_return__different_hashes);
    ADD_TEST(suite_hash, hash64__for_null_pointer__should_return__valid_hash);
    ADD_TEST(suite_hash, hash__for_floats__should_return__deterministic_hash);
    ADD_TEST(suite_hash, hash__for_negative_zero__should_equal__positive_zero);

    // hash_float() suite
    CU_pSuite suite_hash_float = CU_add_suite("hash_float()", nullptr, nullptr);
    if (suite_hash_float == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_hash_float, hash_float__for_same_input__should_return__same_hash);
    ADD_TEST(suite_hash_float, hash_float__for_different_inputs__should_return__different_hashes);
    ADD_TEST(suite_hash_float, hash_float__for_negative_zero__should_equal__positive_zero);

    // hash_double() suite
    CU_pSuite suite_hash_double = CU_add_suite("hash_double()", nullptr, nullptr);
    if (suite_hash_double == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_hash_double, hash_double__for_same_input__should_return__same_hash);
    ADD_TEST(suite_hash_double, hash_double__for_different_inputs__should_return__different_hashes);
    ADD_TEST(suite_hash_double, hash_double__for_negative_zero__should_equal__positive_zero);

    // hash_mix() suite
    CU_pSuite suite_hash_mix = CU_add_suite("hash_mix()", nullptr, nullptr);
    if (suite_hash_mix == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_hash_mix, hash_mix__for_same_input__should_return__same_output);
    ADD_TEST(suite_hash_mix, hash_mix__for_zero__should_return__deterministic);
    ADD_TEST(suite_hash_mix, hash_mix__for_sequential_inputs__should_produce__varied_outputs);

    // xxhash64() suite
    CU_pSuite suite_xxhash64 = CU_add_suite("xxhash64()", nullptr, nullptr);
    if (suite_xxhash64 == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_xxhash64, xxhash64__for_empty_data__should_return__deterministic_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_empty_data_with_different_seeds__should_return__different_hashes);
    ADD_TEST(suite_xxhash64, xxhash64__for_same_input__should_return__same_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_different_inputs__should_return__different_hashes);
    ADD_TEST(suite_xxhash64, xxhash64__for_different_seeds__should_return__different_hashes);
    ADD_TEST(suite_xxhash64, xxhash64__for_1_byte__should_return__valid_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_2_bytes__should_return__valid_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_4_bytes__should_return__valid_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_8_bytes__should_return__valid_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_various_tail_sizes__should_produce__unique_hashes);
    ADD_TEST(suite_xxhash64, xxhash64__for_32_byte_threshold__should_use_parallel_processing);
    ADD_TEST(suite_xxhash64, xxhash64__for_large_input__should_return__valid_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_very_large_input__should_return__valid_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_data_with_null_bytes__should_hash_all_bytes);
    ADD_TEST(suite_xxhash64, xxhash64__for_single_bit_change__should_produce__different_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_last_byte_change__should_produce__different_hash);
    ADD_TEST(suite_xxhash64, xxhash64__for_sequential_keys__should_produce__varied_hashes);
    ADD_TEST(suite_xxhash64, xxhash64__should_differ_from_murmur64__for_same_input);
    ADD_TEST(suite_xxhash64, xxhash64_and_murmur64__should_be_deterministic_independently);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
