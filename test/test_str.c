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

static void str__for_nullptr__should_return__nullptr() {
    CU_ASSERT_PTR_NULL(str(nullptr));
}

static void str__for_empty_cstr__should_return__nonnull() {
    const char * s = str("");
    CU_ASSERT_PTR_NOT_NULL(s);
    str_free(s);
}

static void str__for_empty_cstr__should_return__first_byte_of_cstr_data() {
    const char * s = str("");
    CU_ASSERT_STRING_EQUAL(s, "");
    str_free(s);
}

static void str__for_empty_cstr__should_return__empty_cstr() {
    const char * s = str("");
    CU_ASSERT_EQUAL(strlen(s), 0);
    str_free(s);
}

static void str__for_empty_cstr__should_return__cstr_with_nullterm() {
    const char * s = str("");
    CU_ASSERT_EQUAL(s[0], NULLTERM);
    str_free(s);
}

static void str__for_empty_cstr__should_return__cstr_within_buffer_starting_with_SOH() {
    const char * s = str("");

    // SOH | metadata | STX | c string ...
    const size_t header_size = 1 + sizeof(uint64_t) + 1;
    CU_ASSERT_EQUAL(*(s - header_size), 0x01); // SOH - "start of header"

    str_free(s);
}

static void str__for_empty_cstr__should_return__cstr_within_buffer_with_empty_size() {
    const char * s = str("");

    //  ... | metadata | STX | c string ...
    const size_t size_loc = sizeof(uint64_t) + 1;
    uint64_t val = 0;
    memcpy(&val, s - size_loc, sizeof(uint64_t));

    // size should be exactly 1, representing only the null term
    CU_ASSERT_EQUAL(val, 1);

    str_free(s);
}

static void str__for_empty_cstr__should_return__cstr_within_buffer_after_STX() {
    const char * s = str("");

    //  ... | STX | c string ...
    CU_ASSERT_EQUAL(*(s - 1), 0x02); // STX - "start of text"

    str_free(s);
}

static void str__for_nonempty_cstr__should_return__nonnull() {
    const char * s = str(HELLO);
    CU_ASSERT_PTR_NOT_NULL(s);
    str_free(s);
}

static void str__for_nonempty_cstr__should_return__first_byte_of_cstr_data() {
    const char * s = str(HELLO);
    CU_ASSERT_STRING_EQUAL(s, HELLO);
    str_free(s);
}

static void str__for_nonempty_cstr__should_return__cstr_with_same_len_as_input() {
    const char * s = str(HELLO);
    CU_ASSERT_EQUAL(strlen(s), HELLO_LEN);
    str_free(s);
}

static void str__for_nonempty_cstr__should_return__cstr_with_nullterm() {
    const char * s = str(HELLO);
    CU_ASSERT_EQUAL(s[HELLO_LEN], NULLTERM);
    str_free(s);
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

    ADD_TEST(suite, str__for_nullptr__should_return__nullptr);

    ADD_TEST(suite, str__for_empty_cstr__should_return__nonnull);
    ADD_TEST(suite, str__for_empty_cstr__should_return__first_byte_of_cstr_data);
    ADD_TEST(suite, str__for_empty_cstr__should_return__empty_cstr);
    ADD_TEST(suite, str__for_empty_cstr__should_return__cstr_with_nullterm);
    ADD_TEST(suite, str__for_empty_cstr__should_return__cstr_within_buffer_starting_with_SOH);
    ADD_TEST(suite, str__for_empty_cstr__should_return__cstr_within_buffer_with_empty_size);
    ADD_TEST(suite, str__for_empty_cstr__should_return__cstr_within_buffer_after_STX);

    ADD_TEST(suite, str__for_nonempty_cstr__should_return__nonnull);
    ADD_TEST(suite, str__for_nonempty_cstr__should_return__first_byte_of_cstr_data);
    ADD_TEST(suite, str__for_nonempty_cstr__should_return__cstr_with_same_len_as_input);
    ADD_TEST(suite, str__for_nonempty_cstr__should_return__cstr_with_nullterm);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

#ifdef RUN_TESTS
    #undef RUN_TESTS
#endif // RUN_TESTS
