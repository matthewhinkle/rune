# Testing Internals and r_test.h

The `r_test.h` header provides test-only access to internal runtime structures for testing purposes.

## When to Use r_test.h

Use `r_test.h` instead of `r.h` in test files when you need to:

- Access internal allocator stack state for validation
- Mock or override allocators in tests
- Verify allocator push/pop behavior
- Test error stack management in isolation

## Structure and APIs

### Allocator Stack Access

```c
/**
 * Test-only access to the thread-local allocator stack.
 * Should only be used by test code.
 */
extern _Thread_local struct {
    allocator stack[mem_alloc_STACK_MAX];
    int depth;
} mem_alloc_stack;
```

**Usage:**

```c
// Check allocator stack depth
CU_ASSERT_EQUAL(mem_alloc_stack.depth, 0);

// Verify allocator stack was cleared after scope
alloc_scope(custom_allocator) {
    CU_ASSERT_EQUAL(mem_alloc_stack.depth, 1);
}
CU_ASSERT_EQUAL(mem_alloc_stack.depth, 0);
```

### Default Allocator

```c
/**
 * Default allocator using malloc/realloc/free.
 * Only exposed for test code.
 */
extern const allocator r_default_allocator;
```

**Usage:**

```c
// Test that default allocator is used
allocator current = alloc_current();
CU_ASSERT_EQUAL(current.alloc, r_default_allocator.alloc);
CU_ASSERT_EQUAL(current.realloc, r_default_allocator.realloc);
CU_ASSERT_EQUAL(current.free, r_default_allocator.free);
```

## Example Test File Using r_test.h

```c
/*
 * Allocator tests with internal access.
 */

#include "../src/r_test.h"  // Use r_test.h instead of r.h
#include "CUnit/Basic.h"
#include "test.h"

// Test that allocator stack is cleaned up properly
static void alloc_scope__should_push_and_pop_allocator() {
    // Record initial depth
    int initial_depth = mem_alloc_stack.depth;

    // Mock allocator
    allocator mock = {
        .alloc = mock_alloc,
        .realloc = mock_realloc,
        .free = mock_free,
        .ctx = &mock_ctx,
    };

    // Allocator should be pushed
    alloc_scope(mock) {
        CU_ASSERT_EQUAL(mem_alloc_stack.depth, initial_depth + 1);
        CU_ASSERT_EQUAL(mem_alloc_stack.stack[mem_alloc_stack.depth - 1].alloc, mock.alloc);
    }

    // Allocator should be popped
    CU_ASSERT_EQUAL(mem_alloc_stack.depth, initial_depth);
}

// Test nested allocator scopes
static void alloc_scope__for_nested_scopes__should_maintain_stack() {
    int initial_depth = mem_alloc_stack.depth;

    allocator alloc1 = { /* ... */ };
    allocator alloc2 = { /* ... */ };

    alloc_scope(alloc1) {
        CU_ASSERT_EQUAL(mem_alloc_stack.depth, initial_depth + 1);

        alloc_scope(alloc2) {
            CU_ASSERT_EQUAL(mem_alloc_stack.depth, initial_depth + 2);
            // Inner allocator is active
            allocator current = alloc_current();
            CU_ASSERT_EQUAL(current.alloc, alloc2.alloc);
        }

        // Outer allocator is restored
        CU_ASSERT_EQUAL(mem_alloc_stack.depth, initial_depth + 1);
        allocator current = alloc_current();
        CU_ASSERT_EQUAL(current.alloc, alloc1.alloc);
    }

    CU_ASSERT_EQUAL(mem_alloc_stack.depth, initial_depth);
}

// Test that default allocator is used when stack is empty
static void alloc_current__for_empty_stack__should_return_default() {
    // Ensure stack is empty
    if (mem_alloc_stack.depth > 0) {
        while (mem_alloc_stack.depth > 0) {
            alloc_pop();
        }
    }

    allocator current = alloc_current();
    CU_ASSERT_EQUAL(current.alloc, r_default_allocator.alloc);
    CU_ASSERT_EQUAL(current.realloc, r_default_allocator.realloc);
    CU_ASSERT_EQUAL(current.free, r_default_allocator.free);
}

// ========================================================================
// main
// ========================================================================

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("allocator_internals_tests", nullptr, nullptr);
    if (suite == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(suite, alloc_scope__should_push_and_pop_allocator);
    ADD_TEST(suite, alloc_scope__for_nested_scopes__should_maintain_stack);
    ADD_TEST(suite, alloc_current__for_empty_stack__should_return_default);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
```

## Important Notes

### Thread Safety

- `mem_alloc_stack` is thread-local, so each thread has its own independent stack
- It's safe to access these internals from any thread's test code
- Each test runs in isolation (tests don't interfere with each other's allocator stacks)

### Not for Production Code

The `r_test.h` header and its exposed internals should **never** be used in production code:

```c
// ❌ WRONG - Production code should never use r_test.h
#include "r_test.h"
void do_something() {
    mem_alloc_stack.depth = 0;  // Don't do this in production!
}

// ✓ CORRECT - Use r.h in production
#include "r.h"
void do_something() {
    err_clear();
}
```

### Mock Allocators in Tests

Use the internal allocator stack for testing custom allocators:

```c
// Mock allocator implementation
static allocator * test_allocator = nullptr;

static void * mock_alloc(void * ctx, size_t size) {
    // Track allocation for test verification
    mock_allocations++;
    return malloc(size);
}

static void * mock_realloc(void * ctx, void * ptr, size_t old_size, size_t new_size) {
    mock_reallocations++;
    return realloc(ptr, new_size);
}

static void mock_free(void * ctx, void * ptr, size_t size) {
    mock_frees++;
    free(ptr);
}

// In test function
static void str_creation__with_custom_allocator__should_use_custom() {
    // Reset counters
    mock_allocations = 0;
    mock_frees = 0;

    allocator mock = {
        .alloc = mock_alloc,
        .realloc = mock_realloc,
        .free = mock_free,
        .ctx = nullptr,
    };

    alloc_scope(mock) {
        char * s = str("hello");
        CU_ASSERT(mock_allocations > 0);  // Verify custom allocator was used
        str_free(s);
    }

    CU_ASSERT(mock_frees > 0);  // Verify cleanup happened
}
```

## Accessing Error Stack (via r.h)

While `r_test.h` focuses on allocator internals, error stack details are accessible from `r.h`:

```c
#include "r.h"

// Check error stack state
if (err_has()) {
    const r_error_ctx * err = err_get();
    printf("Error: %s at %s:%d\n", err->message, err->file, err->line);
    err_clear();
}

// Check error stack depth
CU_ASSERT_EQUAL(err_depth(), 0);  // Verify clean state

// Pop specific errors
while (err_depth() > 0) {
    err_pop();
}
```
