/*
 * str tests.
 */

#include "../src/str.h"
#include "CUnit/Basic.h"

#include "test.h"

// =====================================================================================================================
// str() - Create managed string
// =====================================================================================================================

static void test_str_default() {
    char * s = str("Hello");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s));
    CU_ASSERT_EQUAL(str_len(s), 5);
    CU_ASSERT_STRING_EQUAL(s, "Hello");
    str_free(s);
}

static void test_str_with_max_len() {
    const str_opt custom_opt = {.max_len = 10};

    char * s1 = str("Hello", &custom_opt);
    CU_ASSERT_PTR_NOT_NULL(s1);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s1));
    CU_ASSERT_EQUAL(str_len(s1), 5);
    CU_ASSERT_STRING_EQUAL(s1, "Hello");
    str_free(s1);

    char * s2 = str("Hello World!", &custom_opt);
    CU_ASSERT_PTR_NOT_NULL(s2);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s2));
    CU_ASSERT_EQUAL(str_len(s2), 10);
    CU_ASSERT_EQUAL(strncmp(s2, "Hello Worl", 10), 0);
    str_free(s2);
}

static void test_str_empty() {
    char * s = str("");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s));
    CU_ASSERT_EQUAL(str_len(s), 0);
    CU_ASSERT_STRING_EQUAL(s, "");
    str_free(s);
}

static void test_str_null_input() {
    char * s = str(nullptr);
    CU_ASSERT_PTR_NULL(s);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// strf() - Create formatted string
// =====================================================================================================================

static void test_strf_basic() {
    char * s = strf("Hello, %s!", "World");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s));
    CU_ASSERT_STRING_EQUAL(s, "Hello, World!");
    str_free(s);
}

static void test_strf_multiple_args() {
    char * s = strf("%s %d %s", "test", 42, "end");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(s, "test 42 end");
    str_free(s);
}

static void test_strf_with_options() {
    const str_opt custom_opt = {.max_len = 10};
    char * s = strf(&custom_opt, "Hello, %s!", "World");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_EQUAL(str_len(s), 10);
    CU_ASSERT_EQUAL(strncmp(s, "Hello, Wor", 10), 0);
    str_free(s);
}

static void test_strf_null_format() {
    char * s = R_(strf)(&R_STR_OPTS_DEFAULT, nullptr);
    CU_ASSERT_PTR_NULL(s);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// str_free() - Free managed string
// =====================================================================================================================

static void test_str_free_valid() {
    char * s = str("test");
    CU_ASSERT_PTR_NOT_NULL(s);
    str_free(s);
}

static void test_str_free_null() {
    str_free(nullptr);
}

static void test_str_free_arr_valid() {
    char * s1 = str("one");
    char * s2 = str("two");
    char * s3 = str("three");
    char ** arr = mem_alloc((4) * sizeof(char *));
    arr[0] = s1;
    arr[1] = s2;
    arr[2] = s3;
    arr[3] = nullptr;
    CU_ASSERT_PTR_NOT_NULL(arr[0]);
    CU_ASSERT_PTR_NOT_NULL(arr[1]);
    CU_ASSERT_PTR_NOT_NULL(arr[2]);
    str_free_arr(arr);
}

static void test_str_free_arr_null() {
    str_free_arr(nullptr);
}

// =====================================================================================================================
// str_is() - Check if managed string
// =====================================================================================================================

static void test_str_is_managed() {
    char * s = str("test");
    CU_ASSERT_TRUE(str_is(s));
    str_free(s);
}

static void test_str_is_regular() {
    const char * s = "regular C string";
    CU_ASSERT_FALSE(str_is(s));
}

static void test_str_is_null() {
    CU_ASSERT_FALSE(str_is(nullptr));
}

// =====================================================================================================================
// str_len() - Get string length
// =====================================================================================================================

static void test_str_len_managed() {
    char * s = str("Hello");
    CU_ASSERT_EQUAL(str_len(s), 5);
    str_free(s);
}

static void test_str_len_regular() {
    const char * s = "Hello";
    CU_ASSERT_EQUAL(str_len(s), 5);
}

static void test_str_len_empty() {
    char * s = str("");
    CU_ASSERT_EQUAL(str_len(s), 0);
    str_free(s);
}

static void test_str_len_null() {
    CU_ASSERT_EQUAL(str_len(nullptr), 0);
}

static void test_str_len_with_max() {
    const str_opt opt = {.max_len = 3};
    const char * s = "Hello";
    CU_ASSERT_EQUAL(str_len(s, &opt), 3);
}

// =====================================================================================================================
// str_size() - Get allocation size
// =====================================================================================================================

static void test_str_size_managed() {
    char * s = str("Hello");
    size_t size = str_size(s);
    CU_ASSERT_TRUE(size > 0);
    CU_ASSERT_TRUE(size >= 5);
    str_free(s);
}

static void test_str_size_regular() {
    const char * s = "Hello";
    CU_ASSERT_EQUAL(str_size(s), 0);
}

static void test_str_size_null() {
    CU_ASSERT_EQUAL(str_size(nullptr), 0);
}

// =====================================================================================================================
// str_hash() - Get hash value
// =====================================================================================================================

static void test_str_hash_basic() {
    char * s = str("test");
    uint64_t hash = str_hash(s);
    CU_ASSERT_NOT_EQUAL(hash, 0);
    str_free(s);
}

static void test_str_hash_equal_strings() {
    char * s1 = str("test");
    char * s2 = str("test");
    CU_ASSERT_EQUAL(str_hash(s1), str_hash(s2));
    str_free(s1);
    str_free(s2);
}

static void test_str_hash_different_strings() {
    char * s1 = str("test1");
    char * s2 = str("test2");
    CU_ASSERT_NOT_EQUAL(str_hash(s1), str_hash(s2));
    str_free(s1);
    str_free(s2);
}

static void test_str_hash_null() {
    CU_ASSERT_EQUAL(str_hash(nullptr), 0);
}

// =====================================================================================================================
// str_cmp() - Compare strings
// =====================================================================================================================

static void test_str_cmp_equal() {
    char * s1 = str("test");
    char * s2 = str("test");
    CU_ASSERT_EQUAL(str_cmp(s1, s2), 0);
    str_free(s1);
    str_free(s2);
}

static void test_str_cmp_less() {
    char * s1 = str("abc");
    char * s2 = str("xyz");
    CU_ASSERT_TRUE(str_cmp(s1, s2) < 0);
    str_free(s1);
    str_free(s2);
}

static void test_str_cmp_greater() {
    char * s1 = str("xyz");
    char * s2 = str("abc");
    CU_ASSERT_TRUE(str_cmp(s1, s2) > 0);
    str_free(s1);
    str_free(s2);
}

static void test_str_cmp_null() {
    char * s = str("test");
    CU_ASSERT_TRUE(str_cmp(nullptr, s) < 0);
    CU_ASSERT_TRUE(str_cmp(s, nullptr) > 0);
    CU_ASSERT_EQUAL(str_cmp(nullptr, nullptr), 0);
    str_free(s);
}

// =====================================================================================================================
// str_eq() - Check string equality
// =====================================================================================================================

static void test_str_eq_equal() {
    char * s1 = str("test");
    char * s2 = str("test");
    CU_ASSERT_TRUE(str_eq(s1, s2));
    str_free(s1);
    str_free(s2);
}

static void test_str_eq_different() {
    char * s1 = str("test1");
    char * s2 = str("test2");
    CU_ASSERT_FALSE(str_eq(s1, s2));
    str_free(s1);
    str_free(s2);
}

static void test_str_eq_null() {
    char * s = str("test");
    CU_ASSERT_FALSE(str_eq(nullptr, s));
    CU_ASSERT_FALSE(str_eq(s, nullptr));
    CU_ASSERT_TRUE(str_eq(nullptr, nullptr));
    str_free(s);
}

// =====================================================================================================================
// str_find() - Find substring
// =====================================================================================================================

static void test_str_find_basic() {
    char * s = str("Hello World");
    const char * result = str_find(s, "World");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "World");
    str_free(s);
}

static void test_str_find_not_found() {
    char * s = str("Hello World");
    const char * result = str_find(s, "xyz");
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
    str_free(s);
}

static void test_str_find_empty_pattern() {
    char * s = str("Hello");
    const char * result = str_find(s, "");
    CU_ASSERT_PTR_EQUAL(result, s);
    CU_ASSERT_FALSE(err_has());
    str_free(s);
}

static void test_str_find_at_start() {
    char * s = str("Hello World");
    const char * result = str_find(s, "Hello");
    CU_ASSERT_PTR_EQUAL(result, s);
    str_free(s);
}

static void test_str_find_null() {
    const char * result = str_find(nullptr, "test");
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// str_rfind() - Find last substring occurrence
// =====================================================================================================================

static void test_str_rfind_basic() {
    char * s = str("Hello World World");
    const char * result = str_rfind(s, "World");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "World");
    CU_ASSERT_PTR_EQUAL(result, s + 12);
    str_free(s);
}

static void test_str_rfind_not_found() {
    char * s = str("Hello World");
    const char * result = str_rfind(s, "xyz");
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
    str_free(s);
}

static void test_str_rfind_empty_pattern() {
    char * s = str("Hello");
    const char * result = str_rfind(s, "");
    CU_ASSERT_PTR_EQUAL(result, s + 5);
    CU_ASSERT_FALSE(err_has());
    str_free(s);
}

// =====================================================================================================================
// str_cat() - Concatenate strings
// =====================================================================================================================

static void test_str_cat_basic() {
    char * s1 = str("Hello");
    char * s2 = str("World");
    char * result = str_cat(s1, " ", s2, nullptr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "Hello World");
    str_free(s1);
    str_free(s2);
    str_free(result);
}

static void test_str_cat_multiple() {
    char * result = str_cat("a", "b", "c", "d", nullptr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "abcd");
    str_free(result);
}

static void test_str_cat_with_options() {
    const str_opt opt = {.max_len = 5};
    char * result = str_cat(&opt, "Hello", " ", "World", nullptr);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

static void test_str_cat_null_first() {
    char * result = str_cat(nullptr, "test", nullptr);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// str_join() - Join string array
// =====================================================================================================================

static void test_str_join_basic() {
    const char * arr[] = {"one", "two", "three", nullptr};
    char * result = str_join(", ", arr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "one, two, three");
    str_free(result);
}

static void test_str_join_empty_delim() {
    const char * arr[] = {"a", "b", "c", nullptr};
    char * result = str_join("", arr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "abc");
    str_free(result);
}

static void test_str_join_single_element() {
    const char * arr[] = {"only", nullptr};
    char * result = str_join(", ", arr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "only");
    str_free(result);
}

static void test_str_join_null_delim() {
    const char * arr[] = {"one", "two", nullptr};
    char * result = str_join(nullptr, arr);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// str_repeat() - Repeat string
// =====================================================================================================================

static void test_str_repeat_basic() {
    char * s = str("ab");
    char * result = str_repeat(s, 3);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "ababab");
    str_free(s);
    str_free(result);
}

static void test_str_repeat_once() {
    char * s = str("test");
    char * result = str_repeat(s, 1);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "test");
    str_free(s);
    str_free(result);
}

static void test_str_repeat_zero() {
    char * s = str("test");
    char * result = str_repeat(s, 0);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
    str_free(s);
}

static void test_str_repeat_null() {
    char * result = str_repeat(nullptr, 3);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// str_replace() - Replace substring
// =====================================================================================================================

static void test_str_replace_basic() {
    char * s = str("Hello World");
    char * result = str_replace(s, "World", "Universe");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "Hello Universe");
    str_free(s);
    str_free(result);
}

static void test_str_replace_multiple() {
    char * s = str("foo bar foo");
    char * result = str_replace(s, "foo", "baz");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "baz bar baz");
    str_free(s);
    str_free(result);
}

static void test_str_replace_not_found() {
    char * s = str("Hello World");
    char * result = str_replace(s, "xyz", "abc");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "Hello World");
    str_free(s);
    str_free(result);
}

static void test_str_replace_empty_target() {
    char * s = str("test");
    char * result = str_replace(s, "", "x");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "test");
    str_free(s);
    str_free(result);
}

static void test_str_replace_null() {
    char * result = str_replace(nullptr, "a", "b");
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// str_split() - Split string
// =====================================================================================================================

static void test_str_split_basic() {
    char * s = str("one,two,three");
    char ** result = str_split(s, ",");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_PTR_NOT_NULL(result[0]);
    CU_ASSERT_PTR_NOT_NULL(result[1]);
    CU_ASSERT_PTR_NOT_NULL(result[2]);
    CU_ASSERT_PTR_NULL(result[3]);
    CU_ASSERT_STRING_EQUAL(result[0], "one");
    CU_ASSERT_STRING_EQUAL(result[1], "two");
    CU_ASSERT_STRING_EQUAL(result[2], "three");
    str_free(s);
    str_free_arr(result);
}

static void test_str_split_single() {
    char * s = str("single");
    char ** result = str_split(s, ",");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_PTR_NOT_NULL(result[0]);
    CU_ASSERT_PTR_NULL(result[1]);
    CU_ASSERT_STRING_EQUAL(result[0], "single");
    str_free(s);
    str_free_arr(result);
}

static void test_str_split_multi_char_delim() {
    char * s = str("one::two::three");
    char ** result = str_split(s, "::");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result[0], "one");
    CU_ASSERT_STRING_EQUAL(result[1], "two");
    CU_ASSERT_STRING_EQUAL(result[2], "three");
    str_free(s);
    str_free_arr(result);
}

static void test_str_split_null() {
    char ** result = str_split(nullptr, ",");
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// Test suite registration
// =====================================================================================================================

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // str() suite
    CU_pSuite suite_str = CU_add_suite("str()", nullptr, nullptr);
    if (suite_str == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str, test_str_default);
    ADD_TEST(suite_str, test_str_with_max_len);
    ADD_TEST(suite_str, test_str_empty);
    ADD_TEST(suite_str, test_str_null_input);

    // strf() suite
    CU_pSuite suite_strf = CU_add_suite("strf()", nullptr, nullptr);
    if (suite_strf == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_strf, test_strf_basic);
    ADD_TEST(suite_strf, test_strf_multiple_args);
    ADD_TEST(suite_strf, test_strf_with_options);
    ADD_TEST(suite_strf, test_strf_null_format);

    // str_free() suite
    CU_pSuite suite_str_free = CU_add_suite("str_free()", nullptr, nullptr);
    if (suite_str_free == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_free, test_str_free_valid);
    ADD_TEST(suite_str_free, test_str_free_null);
    ADD_TEST(suite_str_free, test_str_free_arr_valid);
    ADD_TEST(suite_str_free, test_str_free_arr_null);

    // str_is() suite
    CU_pSuite suite_str_is = CU_add_suite("str_is()", nullptr, nullptr);
    if (suite_str_is == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_is, test_str_is_managed);
    ADD_TEST(suite_str_is, test_str_is_regular);
    ADD_TEST(suite_str_is, test_str_is_null);

    // str_len() suite
    CU_pSuite suite_str_len = CU_add_suite("str_len()", nullptr, nullptr);
    if (suite_str_len == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_len, test_str_len_managed);
    ADD_TEST(suite_str_len, test_str_len_regular);
    ADD_TEST(suite_str_len, test_str_len_empty);
    ADD_TEST(suite_str_len, test_str_len_null);
    ADD_TEST(suite_str_len, test_str_len_with_max);

    // str_size() suite
    CU_pSuite suite_str_size = CU_add_suite("str_size()", nullptr, nullptr);
    if (suite_str_size == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_size, test_str_size_managed);
    ADD_TEST(suite_str_size, test_str_size_regular);
    ADD_TEST(suite_str_size, test_str_size_null);

    // str_hash() suite
    CU_pSuite suite_str_hash = CU_add_suite("str_hash()", nullptr, nullptr);
    if (suite_str_hash == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_hash, test_str_hash_basic);
    ADD_TEST(suite_str_hash, test_str_hash_equal_strings);
    ADD_TEST(suite_str_hash, test_str_hash_different_strings);
    ADD_TEST(suite_str_hash, test_str_hash_null);

    // str_cmp() suite
    CU_pSuite suite_str_cmp = CU_add_suite("str_cmp()", nullptr, nullptr);
    if (suite_str_cmp == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_cmp, test_str_cmp_equal);
    ADD_TEST(suite_str_cmp, test_str_cmp_less);
    ADD_TEST(suite_str_cmp, test_str_cmp_greater);
    ADD_TEST(suite_str_cmp, test_str_cmp_null);

    // str_eq() suite
    CU_pSuite suite_str_eq = CU_add_suite("str_eq()", nullptr, nullptr);
    if (suite_str_eq == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_eq, test_str_eq_equal);
    ADD_TEST(suite_str_eq, test_str_eq_different);
    ADD_TEST(suite_str_eq, test_str_eq_null);

    // str_find() suite
    CU_pSuite suite_str_find = CU_add_suite("str_find()", nullptr, nullptr);
    if (suite_str_find == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_find, test_str_find_basic);
    ADD_TEST(suite_str_find, test_str_find_not_found);
    ADD_TEST(suite_str_find, test_str_find_empty_pattern);
    ADD_TEST(suite_str_find, test_str_find_at_start);
    ADD_TEST(suite_str_find, test_str_find_null);

    // str_rfind() suite
    CU_pSuite suite_str_rfind = CU_add_suite("str_rfind()", nullptr, nullptr);
    if (suite_str_rfind == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_rfind, test_str_rfind_basic);
    ADD_TEST(suite_str_rfind, test_str_rfind_not_found);
    ADD_TEST(suite_str_rfind, test_str_rfind_empty_pattern);

    // str_cat() suite
    CU_pSuite suite_str_cat = CU_add_suite("str_cat()", nullptr, nullptr);
    if (suite_str_cat == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_cat, test_str_cat_basic);
    ADD_TEST(suite_str_cat, test_str_cat_multiple);
    ADD_TEST(suite_str_cat, test_str_cat_with_options);
    ADD_TEST(suite_str_cat, test_str_cat_null_first);

    // str_join() suite
    CU_pSuite suite_str_join = CU_add_suite("str_join()", nullptr, nullptr);
    if (suite_str_join == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_join, test_str_join_basic);
    ADD_TEST(suite_str_join, test_str_join_empty_delim);
    ADD_TEST(suite_str_join, test_str_join_single_element);
    ADD_TEST(suite_str_join, test_str_join_null_delim);

    // str_repeat() suite
    CU_pSuite suite_str_repeat = CU_add_suite("str_repeat()", nullptr, nullptr);
    if (suite_str_repeat == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_repeat, test_str_repeat_basic);
    ADD_TEST(suite_str_repeat, test_str_repeat_once);
    ADD_TEST(suite_str_repeat, test_str_repeat_zero);
    ADD_TEST(suite_str_repeat, test_str_repeat_null);

    // str_replace() suite
    CU_pSuite suite_str_replace = CU_add_suite("str_replace()", nullptr, nullptr);
    if (suite_str_replace == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_replace, test_str_replace_basic);
    ADD_TEST(suite_str_replace, test_str_replace_multiple);
    ADD_TEST(suite_str_replace, test_str_replace_not_found);
    ADD_TEST(suite_str_replace, test_str_replace_empty_target);
    ADD_TEST(suite_str_replace, test_str_replace_null);

    // str_split() suite
    CU_pSuite suite_str_split = CU_add_suite("str_split()", nullptr, nullptr);
    if (suite_str_split == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_split, test_str_split_basic);
    ADD_TEST(suite_str_split, test_str_split_single);
    ADD_TEST(suite_str_split, test_str_split_multi_char_delim);
    ADD_TEST(suite_str_split, test_str_split_null);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

#ifdef RUN_TESTS
#undef RUN_TESTS
#endif // RUN_TESTS