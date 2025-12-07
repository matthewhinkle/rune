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
 * Module tests - brief description.
 */

#include "../src/r.h"        // Core runtime
#include "../src/str.h"      // Or other module being tested
#include "CUnit/Basic.h"
#include "test.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Test constants
static const char *HELLO = "hello";
static const size_t HELLO_LEN = 5;

// Helper functions (if needed)
static uint64_t helper_func(...) { ... }

// ========================================================================
// string creation tests
// ========================================================================

static void str__for_nullptr__should_return__nullptr() {
    // Arrange
    const char *input = nullptr;

    // Act
    char *result = str(input);

    // Assert
    CU_ASSERT_PTR_NULL(result);
}

static void str__for_empty_cstr__should_return__nonnull() {
    // Arrange
    const char *input = "";

    // Act
    char *result = str(input);

    // Assert
    CU_ASSERT_PTR_NOT_NULL(result);
    str_free(result);
}

// ========================================================================
// main
// ========================================================================

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("str_tests", nullptr, nullptr);
    if (suite == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(suite, str__for_nullptr__should_return__nullptr);
    ADD_TEST(suite, str__for_empty_cstr__should_return__nonnull);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
```

## Naming Conventions

### Test Functions

Follow the pattern: `<function>__for_<input>__should_return__<expected>`

```c
// Examples
str__for_nullptr__should_return__nullptr
str__for_empty_cstr__should_return__nonnull
str__for_nonempty_cstr__should_return__rstr_with_str_len
str_cat__for_two_strings__should_return__concatenated
```

### Test Suites

Name suites after the module: `<module>_tests`

```c
CU_add_suite("str_tests", nullptr, nullptr);
CU_add_suite("list_tests", nullptr, nullptr);
```

## Test Organization

Group related tests with section dividers:

```c
// ---------------
// empty str tests
// ---------------

static void str__for_empty_cstr__should_return__nonnull() { ... }
static void str__for_empty_cstr__should_return__valid_rstr() { ... }

// ----------------------
// non-empty string tests
// ----------------------

static void str__for_nonempty_cstr__should_return__nonnull() { ... }
static void str__for_nonempty_cstr__should_return__valid_rstr() { ... }
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
