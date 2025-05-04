//
// Created by Matt on 5/3/2025.
//

#include "../src/str.h"
#include "CUnit/Basic.h"

#include "test.h"

static void slit__should_return_str() {
    str s = slit("hello");
    CU_ASSERT_PTR_NOT_NULL(s.lit);
    CU_ASSERT_STRING_EQUAL(s.lit, "hello");
    CU_ASSERT_EQUAL(s.size, 6); // "hello" + NULL terminator
}

void str_creation(void) {
    str s = cstr("hello");
    CU_ASSERT_PTR_NOT_NULL(s.chars);
    CU_ASSERT_STRING_EQUAL(s.chars, "hello");
    CU_ASSERT_EQUAL(s.size, 6); // "hello" + NULL terminator
    sfree(&s);
}

void str_concatenation(void) {
    str s = cstr("hello");
    sadd(&s, cstr(" world"));
    CU_ASSERT_STRING_EQUAL(s.chars, "hello world");
    CU_ASSERT_EQUAL(s.size, 12); // "hello world" + NULL terminator
    sfree(&s);
}

void str_free(void) {
    str s = cstr("test");
    sfree(&s);
    CU_ASSERT_PTR_NULL(s.chars);
    CU_ASSERT_EQUAL(s.size, 0);
}

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("str_tests", 0, 0);
    if (NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(suite, slit__should_return_str);
    ADD_TEST(suite, str_creation);
    ADD_TEST(suite, str_concatenation);
    ADD_TEST(suite, str_free);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}