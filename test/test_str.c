/*
 * str tests.
 */

#include "../src/str.h"
#include "CUnit/Basic.h"

#include "test.h"

#include <stdint.h>
#include <stdlib.h>

static const char * HELLO = "hello";
static const size_t HELLO_LEN = 5;

static uint64_t fnv1a_hash(const char * data) {
    uint64_t hash = 0xCBF29CE484222325ULL; // FNV_OFFSET_BASIS
    for (size_t i = 0; i < strlen(data); ++i) {
        hash ^= (uint8_t)data[i];
        hash *= 0x100000001B3ULL; // FNV_PRIME
    }
    return hash;
}

// ---------------
// empty str tests
// ---------------

#if 0

static void str__for_nullptr__should_return__nullptr() {
    CU_ASSERT_PTR_NULL(str(nullptr));
}

static void str__for_empty_cstr__should_return__nonnull() {
    char * s = str("");
    CU_ASSERT_PTR_NOT_NULL(s);
    stmem_free(s);
}

static void str__for_empty_cstr__should_return__first_byte_of_cstr_data() {
    char * s = str("");
    CU_ASSERT_STRING_EQUAL(s, "");
    stmem_free(s);
}

static void str__for_empty_cstr__should_return__empty_cstr() {
    char * s = str("");
    CU_ASSERT_EQUAL(strlen(s), 0);
    stmem_free(s);
}

static void str__for_empty_cstr__should_return__cstr_with_nullterm() {
    char * s = str("");
    CU_ASSERT_EQUAL(s[0], NULLTERM);
    stmem_free(s);
}

static void str__for_empty_cstr__should_return__valid_rstr() {
    char * s = str("");
    CU_ASSERT(str_is(s));
    stmem_free(s);
}

static void str__for_empty_cstr__should_return__rstr_with_zero_len() {
    char * s = str("");
    CU_ASSERT_EQUAL(str_len(s), 0);
    stmem_free(s);
}

static void str__for_empty_cstr__should_return__rstr_with_fnv1a_offset_basis() {
    char * s = str("");
    CU_ASSERT_EQUAL(str_hash(s), 0xCBF29CE484222325); // FNV_OFFSET_BASIS
    stmem_free(s);
}

// ----------------------
// non-empty string tests
// ----------------------

static void str__for_nonempty_cstr__should_return__nonnull() {
    char * s = str(HELLO);
    CU_ASSERT_PTR_NOT_NULL(s);
    stmem_free(s);
}

static void str__for_nonempty_cstr__should_return__first_byte_of_cstr_data() {
    char * s = str(HELLO);
    CU_ASSERT_STRING_EQUAL(s, HELLO);
    stmem_free(s);
}

static void str__for_nonempty_cstr__should_return__cstr_with_same_len_as_input() {
    char * s = str(HELLO);
    CU_ASSERT_EQUAL(strlen(s), HELLO_LEN);
    stmem_free(s);
}

static void str__for_nonempty_cstr__should_return__cstr_with_nullterm() {
    char * s = str(HELLO);
    CU_ASSERT_EQUAL(s[HELLO_LEN], NULLTERM);
    stmem_free(s);
}

static void str__for_nonempty_cstr__should_return__valid_rstr() {
    char * s = str(HELLO);
    CU_ASSERT(str_is(s));
    stmem_free(s);
}

static void str__for_nonempty_cstr__should_return__rstr_with_str_len() {
    char * s = str(HELLO);
    CU_ASSERT_EQUAL(str_len(s), HELLO_LEN);
    stmem_free(s);
}

static void str__for_nonempty_cstr__should_return__rstr_with_fnv1a_hash() {
    char * s = str(HELLO);
    CU_ASSERT_EQUAL(str_hash(s), fnv1a_hash(HELLO));
    stmem_free(s);
}

#endif

static void noop() {
}

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // ReSharper disable once CppLocalVariableMayBeConst
    CU_pSuite suite = CU_add_suite("str_tests", nullptr, nullptr);
    if (suite == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }

#if 0
    ADD_TEST(suite, str__for_nullptr__should_return__nullptr);

    ADD_TEST(suite, str__for_empty_cstr__should_return__nonnull);
    ADD_TEST(suite, str__for_empty_cstr__should_return__first_byte_of_cstr_data);
    ADD_TEST(suite, str__for_empty_cstr__should_return__empty_cstr);
    ADD_TEST(suite, str__for_empty_cstr__should_return__cstr_with_nullterm);
    ADD_TEST(suite, str__for_empty_cstr__should_return__valid_rstr);
    ADD_TEST(suite, str__for_empty_cstr__should_return__rstr_with_zero_len);
    ADD_TEST(suite, str__for_empty_cstr__should_return__rstr_with_fnv1a_offset_basis);

    ADD_TEST(suite, str__for_nonempty_cstr__should_return__nonnull);
    ADD_TEST(suite, str__for_nonempty_cstr__should_return__first_byte_of_cstr_data);
    ADD_TEST(suite, str__for_nonempty_cstr__should_return__cstr_with_same_len_as_input);
    ADD_TEST(suite, str__for_nonempty_cstr__should_return__cstr_with_nullterm);
    ADD_TEST(suite, str__for_nonempty_cstr__should_return__valid_rstr);
    ADD_TEST(suite, str__for_nonempty_cstr__should_return__rstr_with_str_len);
    ADD_TEST(suite, str__for_nonempty_cstr__should_return__rstr_with_fnv1a_hash);
#endif

    ADD_TEST(suite, noop);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

#ifdef RUN_TESTS
#undef RUN_TESTS
#endif // RUN_TESTS
