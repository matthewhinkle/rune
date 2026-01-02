# Testing Guide

This document describes the testing conventions for the rune library.

## Framework

Tests use [CUnit](http://cunit.sourceforge.net/) for unit testing.

## File Structure

```
test/
  test.h           # Shared test utilities and macros
  test_r.c         # Core runtime tests (22 tests, 217 assertions)
  test_coll.c      # Collections tests (40 tests)
  test_str.c       # String module tests
```

### Test Coverage

- **test_r.c** — Tests for `r.h` module (error handling, allocators, memory functions)
- **test_coll.c** — Tests for `coll.h` module (LIST type and operations)
- **test_str.c** — Tests for `str.h` module (string creation, operations, hashing)

## Test File Layout

```c
/*
 * Module tests.
 */

#include "../src/str.h"
#include "CUnit/Basic.h"
#include "test.h"

// Test constants (if needed)
static const char *HELLO = "hello";
static const size_t HELLO_LEN = 5;

// Helper functions (if needed)
static uint64_t helper_func(...) { ... }

// =====================================================================================================================
// function_name() - Description of what this function does
// =====================================================================================================================

static void test_function_basic() {
    const char *s = function("input");
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT_FALSE(err_has());
    function_free(s);
}

static void test_function_edge_case() {
    const char *s = function(nullptr);
    CU_ASSERT_PTR_NULL(s);
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

    // function1() suite
    CU_pSuite suite_function1 = CU_add_suite("function1()", nullptr, nullptr);
    if (suite_function1 == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_function1, test_function1_basic);
    ADD_TEST(suite_function1, test_function1_edge_case);

    // function2() suite
    CU_pSuite suite_function2 = CU_add_suite("function2()", nullptr, nullptr);
    if (suite_function2 == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_function2, test_function2_basic);
    ADD_TEST(suite_function2, test_function2_null_input);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
```

## Naming Conventions

### Test Functions

Use descriptive names that clearly indicate what is being tested:

```c
// Format: test_<function>_<scenario>
static void test_str_default() { ... }
static void test_str_with_max_len() { ... }
static void test_str_empty() { ... }
static void test_str_null_input() { ... }
```

### Test Suites

**Important:** Create one dedicated CUnit test suite per function/macro being tested.

Each suite should be:

- Named after the function it tests (e.g., `"str()"`, `"strf()"`)
- Created with a unique suite variable (e.g., `suite_str`, `suite_strf`)
- Registered in `main()` with proper error handling

```c
// One suite per function - each gets its own variable
CU_pSuite suite_str = CU_add_suite("str()", nullptr, nullptr);
CU_pSuite suite_strf = CU_add_suite("strf()", nullptr, nullptr);
CU_pSuite suite_str_free = CU_add_suite("str_free()", nullptr, nullptr);
CU_pSuite suite_str_len = CU_add_suite("str_len()", nullptr, nullptr);
```

This organization provides:

- Clear separation of tests by function
- Better test reporting (suite name indicates which function is being tested)
- Easier navigation and debugging

## Test Organization

Group tests by function using section dividers with function names and descriptions:

```c
// =====================================================================================================================
// str() - Create managed string
// =====================================================================================================================

static void test_str_default() {
    const char *s = str("Hello");
    CU_ASSERT_PTR_NOT_NULL(s);
    str_free(s);
}

static void test_str_empty() {
    const char *s = str("");
    CU_ASSERT_PTR_NOT_NULL(s);
    str_free(s);
}

// =====================================================================================================================
// str_len() - Get string length
// =====================================================================================================================

static void test_str_len_managed() {
    const char *s = str("Hello");
    CU_ASSERT_EQUAL(str_len(s), 5);
    str_free(s);
}
```

## Helper Macros

The `test.h` header provides shared utilities:

```c
// ADD_TEST - registers a test and handles errors
ADD_TEST(suite, test_function_name);
```

## Memory Management

Always free allocated memory in tests:

```c
static void str__for_nonempty_cstr__should_return__nonnull() {
    char *s = str("hello");
    CU_ASSERT_PTR_NOT_NULL(s);
    str_free(s);  // Always clean up
}
```

## Common Assertions

```c
CU_ASSERT(expression)                    // Boolean check
CU_ASSERT_TRUE(value)                    // Expect true
CU_ASSERT_FALSE(value)                   // Expect false
CU_ASSERT_EQUAL(actual, expected)        // Equality
CU_ASSERT_NOT_EQUAL(actual, expected)    // Inequality
CU_ASSERT_PTR_NULL(pointer)              // Null pointer
CU_ASSERT_PTR_NOT_NULL(pointer)          // Non-null pointer
CU_ASSERT_STRING_EQUAL(actual, expected) // String equality
```

## Security Testing

All unit tests should include security-focused test cases to validate safe behavior. This is especially critical for
modules that handle user input, memory, or string operations.

### Key Security Test Areas

**Buffer Overflow Protection**

- Test functions with inputs at and beyond configured limits (`R_STR_MAX_LEN`, `R_STR_STACK_MAX`, etc.)
- Verify that `max_len` parameters are properly respected
- Test boundary conditions (exactly at limit, one byte over, significantly over)

**Null Termination**

- Verify all returned strings are properly null-terminated
- Test with strings containing embedded nulls
- Verify null termination on concatenation, splitting, and other transformation operations

**Format String Safety**

- For functions with format string parameters (e.g., `strf()`), validate against format string vulnerabilities
- Test with format string inputs from potentially untrusted sources
- Verify that format parameters are properly validated

**Edge Cases and Special Characters**

- Test with empty strings (`""`)
- Test with very long strings (at and beyond limits)
- Test with UTF-8 multi-byte sequences (don't assume 1 byte = 1 character)
- Test with special characters and control sequences
- Test with null bytes embedded in strings

**Search and Replace Operations**

- Test overlapping patterns in `str_replace()`
- Test when target is not found
- Test with zero-length search strings (if applicable)
- Test replacement resulting in strings larger than limits
- Test recursive replacement scenarios

**Memory Safety**

- Verify no heap overflows on large allocations
- Test cleanup of partially-allocated memory on failure
- Verify all allocated memory is freed properly (use memory leak detection tools)

### Example Security Test

```c
// Test buffer overflow protection
static void str__for_input_exceeding_maxlen__should_truncate() {
    const char *very_long = "aaaaaaaaaa..."; // longer than R_STR_MAX_LEN
    char *s = str(very_long, &(str_opt){.max_len = 10});
    CU_ASSERT_PTR_NOT_NULL(s);
    CU_ASSERT(str_len(s) <= 10);  // Should respect limit
    str_free(s);
}

// Test null termination
static void str__for_any_input__should_be_null_terminated() {
    char *s = str("test");
    CU_ASSERT_EQUAL(s[str_len(s)], '\0');
    str_free(s);
}
```

## Running Tests

```bash
# Build the project (CMake automatically builds and runs all tests)
cmake --build .

# Or run tests explicitly after building
cmake --build . --target run_tests

# Run individual test suites
./test_r
./test_coll
./test_str
```

### Build System Integration

The CMake build system automatically:

- Compiles all test executables (`test_r`, `test_coll`, `test_str`)
- Runs tests as part of the build process (via `run_tests` target)
- Links tests with CUnit library
- Dependencies: Tests for `str.h` include all necessary source files
