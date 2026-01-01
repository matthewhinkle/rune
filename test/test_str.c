/*
 * str tests.
 */

#include "../src/str.h"
#include "CUnit/Basic.h"

#include "test.h"

// =====================================================================================================================
// str() - Create managed string
// =====================================================================================================================

static void str__default() {
    const char * s = str("Hello");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s));
    CU_ASSERT_EQUAL(str_len(s), 5);
    CU_ASSERT_STRING_EQUAL(s, "Hello");
    str_free(s);
}

static void str__with_max_len() {
    const str_opt custom_opt = {.max_len = 10};

    const char * s1 = str("Hello", &custom_opt);
    CU_ASSERT_PTR_NOT_NULL(s1);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s1));
    CU_ASSERT_EQUAL(str_len(s1), 5);
    CU_ASSERT_STRING_EQUAL(s1, "Hello");
    str_free(s1);

    const char * s2 = str("Hello World!", &custom_opt);
    CU_ASSERT_PTR_NOT_NULL(s2);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s2));
    CU_ASSERT_EQUAL(str_len(s2), 10);
    CU_ASSERT_EQUAL(strncmp(s2, "Hello Worl", 10), 0);
    str_free(s2);
}

static void str__empty() {
    const char * s = str("");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s));
    CU_ASSERT_EQUAL(str_len(s), 0);
    CU_ASSERT_STRING_EQUAL(s, "");
    str_free(s);
}

static void str__null_input() {
    const char * s = str(nullptr);
    CU_ASSERT_PTR_NULL(s);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// strf() - Create formatted string
// =====================================================================================================================

static void strf__basic() {
    const char * s = strf("Hello, %s!", "World");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_TRUE(str_is(s));
    CU_ASSERT_STRING_EQUAL(s, "Hello, World!");
    str_free(s);
}

static void strf__multiple_args() {
    const char * s = strf("%s %d %s", "test", 42, "end");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(s, "test 42 end");
    str_free(s);
}

static void strf__with_options() {
    const str_opt custom_opt = {.max_len = 10};
    const char * s = strf(&custom_opt, "Hello, %s!", "World");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_EQUAL(str_len(s), 10);
    CU_ASSERT_EQUAL(strncmp(s, "Hello, Wor", 10), 0);
    str_free(s);
}

static void strf__null_format() {
    const char * s = R_(strf)(&R_STR_OPTS_DEFAULT, nullptr);
    CU_ASSERT_PTR_NULL(s);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

static void strf__large_heap_allocation() {
    // Test heap allocation path (> R_STR_STACK_MAX = 8192)
    const str_opt large_opt = {.max_len = 10000};
    char large_pattern[9000];
    memset(large_pattern, 'X', sizeof(large_pattern) - 1);
    large_pattern[sizeof(large_pattern) - 1] = '\0';

    const char * s = strf(&large_opt, "%s", large_pattern);
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_EQUAL(str_len(s), sizeof(large_pattern) - 1);
    str_free(s);
}

// =====================================================================================================================
// str_free() - Free managed string
// =====================================================================================================================

static void str_free__valid() {
    const char * s = str("test");
    CU_ASSERT_PTR_NOT_NULL(s);
    str_free(s);
}

static void str_free__null() {
    str_free(nullptr);
}

static void str_free__arr_valid() {
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

static void str_free__arr_null() {
    str_free_arr(nullptr);
}

static void str_free__unmanaged() {
    // Test that str_free safely handles non-managed strings
    const char * unmanaged = "regular C string";
    str_free(unmanaged); // Should safely do nothing
}

// =====================================================================================================================
// str_is() - Check if managed string
// =====================================================================================================================

static void str_is__managed() {
    const char * s = str("test");
    CU_ASSERT_TRUE(str_is(s));
    str_free(s);
}

static void str_is__regular() {
    const char * s = "regular C string";
    CU_ASSERT_FALSE(str_is(s));
}

static void str_is__null() {
    CU_ASSERT_FALSE(str_is(nullptr));
}

// =====================================================================================================================
// str_len() - Get string length
// =====================================================================================================================

static void str_len__managed() {
    const char * s = str("Hello");
    CU_ASSERT_EQUAL(str_len(s), 5);
    str_free(s);
}

static void str_len__regular() {
    const char * s = "Hello";
    CU_ASSERT_EQUAL(str_len(s), 5);
}

static void str_len__empty() {
    const char * s = str("");
    CU_ASSERT_EQUAL(str_len(s), 0);
    str_free(s);
}

static void str_len__null() {
    CU_ASSERT_EQUAL(str_len(nullptr), 0);
}

static void str_len__with_max() {
    const str_opt opt = {.max_len = 3};
    const char * s = "Hello";
    CU_ASSERT_EQUAL(str_len(s, &opt), 3);
}

// =====================================================================================================================
// str_size() - Get allocation size
// =====================================================================================================================

static void str_size__managed() {
    const char * s = str("Hello");
    const size_t size = str_size(s);
    CU_ASSERT_TRUE(size > 0);
    CU_ASSERT_TRUE(size >= 5);
    str_free(s);
}

static void str_size__regular() {
    const char * s = "Hello";
    CU_ASSERT_EQUAL(str_size(s), 0);
}

static void str_size__null() {
    CU_ASSERT_EQUAL(str_size(nullptr), 0);
}

// =====================================================================================================================
// str_hash() - Get hash value
// =====================================================================================================================

static void str_hash__basic() {
    const char * s = str("test");
    const uint64_t hash = str_hash(s);
    CU_ASSERT_NOT_EQUAL(hash, 0);
    str_free(s);
}

static void str_hash__equal_strings() {
    const char * s1 = str("test");
    const char * s2 = str("test");
    CU_ASSERT_EQUAL(str_hash(s1), str_hash(s2));
    str_free(s1);
    str_free(s2);
}

static void str_hash__different_strings() {
    const char * s1 = str("test1");
    const char * s2 = str("test2");
    CU_ASSERT_NOT_EQUAL(str_hash(s1), str_hash(s2));
    str_free(s1);
    str_free(s2);
}

static void str_hash__null() {
    CU_ASSERT_EQUAL(str_hash(nullptr), 0);
}

static void str_hash__unmanaged_strings() {
    // Test hashing works with regular C strings
    const char * s1 = "test";
    const char * s2 = "test";
    CU_ASSERT_EQUAL(str_hash(s1), str_hash(s2));
}

// =====================================================================================================================
// str_cmp() - Compare strings
// =====================================================================================================================

static void str_cmp__equal() {
    const char * s1 = str("test");
    const char * s2 = str("test");
    CU_ASSERT_EQUAL(str_cmp(s1, s2), 0);
    str_free(s1);
    str_free(s2);
}

static void str_cmp__less() {
    const char * s1 = str("abc");
    const char * s2 = str("xyz");
    CU_ASSERT_TRUE(str_cmp(s1, s2) < 0);
    str_free(s1);
    str_free(s2);
}

static void str_cmp__greater() {
    const char * s1 = str("xyz");
    const char * s2 = str("abc");
    CU_ASSERT_TRUE(str_cmp(s1, s2) > 0);
    str_free(s1);
    str_free(s2);
}

static void str_cmp__null() {
    const char * s = str("test");
    CU_ASSERT_TRUE(str_cmp(nullptr, s) < 0);
    CU_ASSERT_TRUE(str_cmp(s, nullptr) > 0);
    CU_ASSERT_EQUAL(str_cmp(nullptr, nullptr), 0);
    str_free(s);
}

// =====================================================================================================================
// str_eq() - Check string equality
// =====================================================================================================================

static void str_eq__equal() {
    const char * s1 = str("test");
    const char * s2 = str("test");
    CU_ASSERT_TRUE(str_eq(s1, s2));
    str_free(s1);
    str_free(s2);
}

static void str_eq__different() {
    const char * s1 = str("test1");
    const char * s2 = str("test2");
    CU_ASSERT_FALSE(str_eq(s1, s2));
    str_free(s1);
    str_free(s2);
}

static void str_eq__null() {
    const char * s = str("test");
    CU_ASSERT_FALSE(str_eq(nullptr, s));
    CU_ASSERT_FALSE(str_eq(s, nullptr));
    CU_ASSERT_TRUE(str_eq(nullptr, nullptr));
    str_free(s);
}

// =====================================================================================================================
// str_find() - Find substring
// =====================================================================================================================

static void str_find__basic() {
    const char * s = str("Hello World");
    const char * result = str_find(s, "World");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "World");
    str_free(s);
}

static void str_find__not_found() {
    const char * s = str("Hello World");
    const char * result = str_find(s, "xyz");
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
    str_free(s);
}

static void str_find__empty_pattern() {
    const char * s = str("Hello");
    const char * result = str_find(s, "");
    CU_ASSERT_PTR_EQUAL(result, s);
    CU_ASSERT_FALSE(err_has());
    str_free(s);
}

static void str_find__at_start() {
    const char * s = str("Hello World");
    const char * result = str_find(s, "Hello");
    CU_ASSERT_PTR_EQUAL(result, s);
    str_free(s);
}

static void str_find__null() {
    const char * result = str_find(nullptr, "test");
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

static void str_find__large_pattern() {
    // Test heap allocation for lps array (pattern > R_STR_STACK_MAX = 8192)
    const str_opt large_opt = {.max_len = 20000};

    // Create a large haystack
    char * haystack = mem_alloc(16000);
    memset(haystack, 'A', 15999);
    haystack[15999] = '\0';

    // Create a large pattern
    char * pattern = mem_alloc(9000);
    memset(pattern, 'A', 8999);
    pattern[8999] = '\0';

    char * s = str(haystack, &large_opt);
    char * p = str(pattern, &large_opt);

    const char * result = str_find(s, p, &large_opt);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_PTR_EQUAL(result, s);

    str_free(s);
    str_free(p);
    mem_free(haystack, 16000);
    mem_free(pattern, 9000);
}

// =====================================================================================================================
// str_rfind() - Find last substring occurrence
// =====================================================================================================================

static void str_rfind__basic() {
    const char * s = str("Hello World World");
    const char * result = str_rfind(s, "World");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "World");
    CU_ASSERT_PTR_EQUAL(result, s + 12);
    str_free(s);
}

static void str_rfind__not_found() {
    const char * s = str("Hello World");
    const char * result = str_rfind(s, "xyz");
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
    str_free(s);
}

static void str_rfind__empty_pattern() {
    const char * s = str("Hello");
    const char * result = str_rfind(s, "");
    CU_ASSERT_PTR_EQUAL(result, s + 5);
    CU_ASSERT_FALSE(err_has());
    str_free(s);
}

static void str_rfind__large_pattern() {
    // Test heap allocation for lps array (pattern > R_STR_STACK_MAX = 8192)
    const str_opt large_opt = {.max_len = 20000};

    // Create a large haystack with pattern at the end
    char * haystack = mem_alloc(16000);
    memset(haystack, 'B', 6000);
    memset(haystack + 6000, 'A', 9999);
    haystack[15999] = '\0';

    // Create a large pattern
    char * pattern = mem_alloc(9000);
    memset(pattern, 'A', 8999);
    pattern[8999] = '\0';

    char * s = str(haystack, &large_opt);
    char * p = str(pattern, &large_opt);

    const char * result = str_rfind(s, p, &large_opt);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());

    str_free(s);
    str_free(p);
    mem_free(haystack, 16000);
    mem_free(pattern, 9000);
}

// =====================================================================================================================
// str_cat() - Concatenate strings
// =====================================================================================================================

static void str_cat__basic() {
    char * s1 = str("Hello");
    char * s2 = str("World");
    const char * result = str_cat(s1, " ", s2, nullptr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "Hello World");
    str_free(s1);
    str_free(s2);
    str_free(result);
}

static void str_cat__multiple() {
    const char * result = str_cat("a", "b", "c", "d", nullptr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "abcd");
    str_free(result);
}

static void str_cat__with_options() {
    const str_opt opt = {.max_len = 5};
    const char * result = str_cat(&opt, "Hello", " ", "World", nullptr);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

static void str_cat__null_first() {
    const char * result = str_cat(nullptr, "test", nullptr);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

static void str_cat__mixed_managed_unmanaged() {
    char * managed = str("Hello");
    const char * unmanaged = " World";
    const char * result = str_cat(managed, unmanaged, nullptr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "Hello World");
    str_free(managed);
    str_free(result);
}

// =====================================================================================================================
// str_join() - Join string array
// =====================================================================================================================

static void str_join__basic() {
    const char * arr[] = {"one", "two", "three", nullptr};
    const char * result = str_join(", ", arr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "one, two, three");
    str_free(result);
}

static void str_join__empty_delim() {
    const char * arr[] = {"a", "b", "c", nullptr};
    const char * result = str_join("", arr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "abc");
    str_free(result);
}

static void str_join__single_element() {
    const char * arr[] = {"only", nullptr};
    const char * result = str_join(", ", arr);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "only");
    str_free(result);
}

static void str_join__null_delim() {
    const char * arr[] = {"one", "two", nullptr};
    const char * result = str_join(nullptr, arr);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// str_repeat() - Repeat string
// =====================================================================================================================

static void str_repeat__basic() {
    const char * s = str("ab");
    const char * result = str_repeat(s, 3);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "ababab");
    str_free(s);
    str_free(result);
}

static void str_repeat__once() {
    const char * s = str("test");
    const char * result = str_repeat(s, 1);
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "test");
    str_free(s);
    str_free(result);
}

static void str_repeat__zero() {
    const char * s = str("test");
    const char * result = str_repeat(s, 0);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
    str_free(s);
}

static void str_repeat__null() {
    const char * result = str_repeat(nullptr, 3);
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// str_replace() - Replace substring
// =====================================================================================================================

static void str_replace__basic() {
    const char * s = str("Hello World");
    const char * result = str_replace(s, "World", "Universe");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_STRING_EQUAL(result, "Hello Universe");
    str_free(s);
    str_free(result);
}

static void str_replace__multiple() {
    const char * s = str("foo bar foo");
    const char * result = str_replace(s, "foo", "baz");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "baz bar baz");
    str_free(s);
    str_free(result);
}

static void str_replace__not_found() {
    const char * s = str("Hello World");
    const char * result = str_replace(s, "xyz", "abc");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "Hello World");
    str_free(s);
    str_free(result);
}

static void str_replace__empty_target() {
    const char * s = str("test");
    const char * result = str_replace(s, "", "x");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result, "test");
    str_free(s);
    str_free(result);
}

static void str_replace__null() {
    const char * result = str_replace(nullptr, "a", "b");
    CU_ASSERT_PTR_NULL(result);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

// =====================================================================================================================
// str_split() - Split string
// =====================================================================================================================

static void str_split__basic() {
    const char * s = str("one,two,three");
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

static void str_split__single() {
    const char * s = str("single");
    char ** result = str_split(s, ",");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_PTR_NOT_NULL(result[0]);
    CU_ASSERT_PTR_NULL(result[1]);
    CU_ASSERT_STRING_EQUAL(result[0], "single");
    str_free(s);
    str_free_arr(result);
}

static void str_split__multi_char_delim() {
    const char * s = str("one::two::three");
    char ** result = str_split(s, "::");
    CU_ASSERT_PTR_NOT_NULL(result);
    CU_ASSERT_STRING_EQUAL(result[0], "one");
    CU_ASSERT_STRING_EQUAL(result[1], "two");
    CU_ASSERT_STRING_EQUAL(result[2], "three");
    str_free(s);
    str_free_arr(result);
}

static void str_split__null() {
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
    // ReSharper disable CppLocalVariableMayBeConst
    CU_pSuite suite_str = CU_add_suite("str()", nullptr, nullptr);
    if (suite_str == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str, str__default);
    ADD_TEST(suite_str, str__with_max_len);
    ADD_TEST(suite_str, str__empty);
    ADD_TEST(suite_str, str__null_input);

    // strf() suite
    CU_pSuite suite_strf = CU_add_suite("strf()", nullptr, nullptr);
    if (suite_strf == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_strf, strf__basic);
    ADD_TEST(suite_strf, strf__multiple_args);
    ADD_TEST(suite_strf, strf__with_options);
    ADD_TEST(suite_strf, strf__null_format);
    ADD_TEST(suite_strf, strf__large_heap_allocation);

    // str_free() suite
    CU_pSuite suite_str_free = CU_add_suite("str_free()", nullptr, nullptr);
    if (suite_str_free == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_free, str_free__valid);
    ADD_TEST(suite_str_free, str_free__null);
    ADD_TEST(suite_str_free, str_free__arr_valid);
    ADD_TEST(suite_str_free, str_free__arr_null);
    ADD_TEST(suite_str_free, str_free__unmanaged);

    // str_is() suite
    CU_pSuite suite_str_is = CU_add_suite("str_is()", nullptr, nullptr);
    if (suite_str_is == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_is, str_is__managed);
    ADD_TEST(suite_str_is, str_is__regular);
    ADD_TEST(suite_str_is, str_is__null);

    // str_len() suite
    CU_pSuite suite_str_len = CU_add_suite("str_len()", nullptr, nullptr);
    if (suite_str_len == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_len, str_len__managed);
    ADD_TEST(suite_str_len, str_len__regular);
    ADD_TEST(suite_str_len, str_len__empty);
    ADD_TEST(suite_str_len, str_len__null);
    ADD_TEST(suite_str_len, str_len__with_max);

    // str_size() suite
    CU_pSuite suite_str_size = CU_add_suite("str_size()", nullptr, nullptr);
    if (suite_str_size == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_size, str_size__managed);
    ADD_TEST(suite_str_size, str_size__regular);
    ADD_TEST(suite_str_size, str_size__null);

    // str_hash() suite
    CU_pSuite suite_str_hash = CU_add_suite("str_hash()", nullptr, nullptr);
    if (suite_str_hash == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_hash, str_hash__basic);
    ADD_TEST(suite_str_hash, str_hash__equal_strings);
    ADD_TEST(suite_str_hash, str_hash__different_strings);
    ADD_TEST(suite_str_hash, str_hash__null);
    ADD_TEST(suite_str_hash, str_hash__unmanaged_strings);

    // str_cmp() suite
    CU_pSuite suite_str_cmp = CU_add_suite("str_cmp()", nullptr, nullptr);
    if (suite_str_cmp == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_cmp, str_cmp__equal);
    ADD_TEST(suite_str_cmp, str_cmp__less);
    ADD_TEST(suite_str_cmp, str_cmp__greater);
    ADD_TEST(suite_str_cmp, str_cmp__null);

    // str_eq() suite
    CU_pSuite suite_str_eq = CU_add_suite("str_eq()", nullptr, nullptr);
    if (suite_str_eq == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_eq, str_eq__equal);
    ADD_TEST(suite_str_eq, str_eq__different);
    ADD_TEST(suite_str_eq, str_eq__null);

    // str_find() suite
    CU_pSuite suite_str_find = CU_add_suite("str_find()", nullptr, nullptr);
    if (suite_str_find == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_find, str_find__basic);
    ADD_TEST(suite_str_find, str_find__not_found);
    ADD_TEST(suite_str_find, str_find__empty_pattern);
    ADD_TEST(suite_str_find, str_find__at_start);
    ADD_TEST(suite_str_find, str_find__null);
    ADD_TEST(suite_str_find, str_find__large_pattern);

    // str_rfind() suite
    CU_pSuite suite_str_rfind = CU_add_suite("str_rfind()", nullptr, nullptr);
    if (suite_str_rfind == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_rfind, str_rfind__basic);
    ADD_TEST(suite_str_rfind, str_rfind__not_found);
    ADD_TEST(suite_str_rfind, str_rfind__empty_pattern);
    ADD_TEST(suite_str_rfind, str_rfind__large_pattern);

    // str_cat() suite
    CU_pSuite suite_str_cat = CU_add_suite("str_cat()", nullptr, nullptr);
    if (suite_str_cat == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_cat, str_cat__basic);
    ADD_TEST(suite_str_cat, str_cat__multiple);
    ADD_TEST(suite_str_cat, str_cat__with_options);
    ADD_TEST(suite_str_cat, str_cat__null_first);
    ADD_TEST(suite_str_cat, str_cat__mixed_managed_unmanaged);

    // str_join() suite
    CU_pSuite suite_str_join = CU_add_suite("str_join()", nullptr, nullptr);
    if (suite_str_join == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_join, str_join__basic);
    ADD_TEST(suite_str_join, str_join__empty_delim);
    ADD_TEST(suite_str_join, str_join__single_element);
    ADD_TEST(suite_str_join, str_join__null_delim);

    // str_repeat() suite
    CU_pSuite suite_str_repeat = CU_add_suite("str_repeat()", nullptr, nullptr);
    if (suite_str_repeat == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_repeat, str_repeat__basic);
    ADD_TEST(suite_str_repeat, str_repeat__once);
    ADD_TEST(suite_str_repeat, str_repeat__zero);
    ADD_TEST(suite_str_repeat, str_repeat__null);

    // str_replace() suite
    CU_pSuite suite_str_replace = CU_add_suite("str_replace()", nullptr, nullptr);
    if (suite_str_replace == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_replace, str_replace__basic);
    ADD_TEST(suite_str_replace, str_replace__multiple);
    ADD_TEST(suite_str_replace, str_replace__not_found);
    ADD_TEST(suite_str_replace, str_replace__empty_target);
    ADD_TEST(suite_str_replace, str_replace__null);

    // str_split() suite
    CU_pSuite suite_str_split = CU_add_suite("str_split()", nullptr, nullptr); // NOLINT(*-misplaced-const)
    if (suite_str_split == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_str_split, str_split__basic);
    ADD_TEST(suite_str_split, str_split__single);
    ADD_TEST(suite_str_split, str_split__multi_char_delim);
    ADD_TEST(suite_str_split, str_split__null);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
    // ReSharper restore CppLocalVariableMayBeConst
}

#ifdef RUN_TESTS
#undef RUN_TESTS
#endif // RUN_TESTS
