/*
 * rune tests.
 */

#include "../src/r.h"
#include "CUnit/Basic.h"
#include "test.h"

// CUnit's macro casts function pointers to void*, which violates strict ISO C
// MinGW's GCC is stricter about this on Windows, so we suppress it there
#ifdef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

// Test-only access to internal allocator state
extern _Thread_local struct {
    allocator stack[mem_alloc_STACK_MAX];
    int depth;
} mem_alloc_stack;

// Default allocator using malloc/realloc/free
extern const allocator r_default_allocator;

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Helper allocator state for testing
typedef struct {
    size_t alloc_count;
    size_t free_count;
    size_t realloc_count;
    size_t total_allocated;
    size_t total_freed;
} alloc_stats;

static alloc_stats test_stats;

// Custom allocator functions that track statistics
static void * test_alloc(void * ctx, size_t size) {
    alloc_stats * stats = (alloc_stats *)ctx;
    stats->alloc_count++;
    stats->total_allocated += size;
    void * ptr = malloc(size);
    assert(ptr != NULL);
    return ptr;
}

static void * test_realloc(void * ctx, void * ptr, size_t old_size, size_t new_size) {
    alloc_stats * stats = (alloc_stats *)ctx;
    stats->realloc_count++;
    stats->total_allocated += new_size;
    stats->total_freed += old_size;
    void * new_ptr = realloc(ptr, new_size);
    assert(new_ptr != NULL);
    return new_ptr;
}

static void test_free(void * ctx, void * ptr, size_t size) {
    alloc_stats * stats = ctx;
    if (ptr != NULL) {
        stats->free_count++;
        stats->total_freed += size;
        free(ptr);
    }
}

static allocator test_allocator;

static void setup_test_allocator(void) {
    memset(&test_stats, 0, sizeof(test_stats));
    test_allocator.alloc = test_alloc;
    test_allocator.realloc = test_realloc;
    test_allocator.free = test_free;
    test_allocator.ctx = &test_stats;
}

// =====================================================================================================================
// Macro tests
// =====================================================================================================================

static void R_GLUE__should__concatenate_tokens(void) {
// Test token concatenation using R_GLUE
#define TEST_A hello
#define TEST_B world
    constexpr int R_GLUE(TEST_A, TEST_B) = 42;
    CU_ASSERT_EQUAL(helloworld, 42);
    (void)helloworld; // Suppress unused warning
#undef TEST_A
#undef TEST_B
}

// =====================================================================================================================
// mem_alloc() - Allocate memory with default allocator
// =====================================================================================================================

static void mem_alloc__for_default_allocator__should_return__nonnull(void) {
    void * ptr = mem_alloc(100);
    CU_ASSERT_PTR_NOT_NULL(ptr);
    mem_free(ptr, 100);
}

static void mem_alloc__for_size__should_return__writable_memory(void) {
    char * ptr = mem_alloc(10);
    CU_ASSERT_PTR_NOT_NULL(ptr);

    // Write to memory
    for (int i = 0; i < 10; i++) {
        ptr[i] = (char)('a' + i);
    }

    // Verify writes
    for (int i = 0; i < 10; i++) {
        CU_ASSERT_EQUAL(ptr[i], (char)('a' + i));
    }

    mem_free(ptr, 10);
}

// =====================================================================================================================
// mem_alloc_zero() - Allocate zeroed memory
// =====================================================================================================================

static void mem_alloc_zero__for_count_and_size__should_return__zeroed_memory(void) {
    constexpr size_t count = 10;
    int * ptr = mem_alloc_zero(count * sizeof(int));
    CU_ASSERT_PTR_NOT_NULL(ptr);

    // Verify all elements are zero
    for (size_t i = 0; i < count; i++) {
        CU_ASSERT_EQUAL(ptr[i], 0);
    }

    mem_free(ptr, count * sizeof(int));
}

static void mem_alloc_zero__for_small_size__should_return__zeroed_byte(void) {
    void * ptr = mem_alloc_zero(1);
    CU_ASSERT_PTR_NOT_NULL(ptr);
    CU_ASSERT_EQUAL(*(char *)ptr, 0);
    mem_free(ptr, 1);
}

// =====================================================================================================================
// mem_alloc() - Typed allocation tests
// =====================================================================================================================

static void mem_alloc_t__for_type__should_return__typed_pointer(void) {
    typedef struct {
        int x;
        int y;
    } Point;
    Point * p = mem_alloc(sizeof(Point));
    CU_ASSERT_PTR_NOT_NULL(p);

    p->x = 10;
    p->y = 20;
    CU_ASSERT_EQUAL(p->x, 10);
    CU_ASSERT_EQUAL(p->y, 20);

    mem_free(p, sizeof(Point));
}

static void mem_alloc_zero_t__for_count_and_type__should_return__zeroed_array(void) {
    typedef struct {
        int a;
        int b;
    } Pair;
    constexpr size_t count = 5;
    Pair * arr = mem_alloc_zero(count * sizeof(Pair));
    CU_ASSERT_PTR_NOT_NULL(arr);

    // Verify all elements are zero
    for (size_t i = 0; i < count; i++) {
        CU_ASSERT_EQUAL(arr[i].a, 0);
        CU_ASSERT_EQUAL(arr[i].b, 0);
    }

    mem_free(arr, count * sizeof(Pair));
}

// =====================================================================================================================
// mem_realloc() - Reallocate memory
// =====================================================================================================================

static void mem_realloc__for_larger_size__should_return__grown_allocation(void) {
    constexpr size_t old_size = 10;
    constexpr size_t new_size = 20;

    char * ptr = (char *)mem_alloc(old_size);
    CU_ASSERT_PTR_NOT_NULL(ptr);

    // Fill initial memory
    for (size_t i = 0; i < old_size; i++) {
        ptr[i] = (char)('A' + i);
    }

    // Reallocate to larger size
    char * new_ptr = (char *)mem_realloc(ptr, old_size, new_size);
    CU_ASSERT_PTR_NOT_NULL(new_ptr);

    // Verify old data is preserved
    for (size_t i = 0; i < old_size; i++) {
        CU_ASSERT_EQUAL(new_ptr[i], (char)('A' + i));
    }

    mem_free(new_ptr, new_size);
}

static void mem_realloc__for_smaller_size__should_return__shrunk_allocation(void) {
    constexpr size_t old_size = 20;
    constexpr size_t new_size = 10;

    char * ptr = (char *)mem_alloc(old_size);
    CU_ASSERT_PTR_NOT_NULL(ptr);

    // Fill initial memory
    for (size_t i = 0; i < old_size; i++) {
        ptr[i] = (char)('A' + i);
    }

    // Reallocate to smaller size
    char * new_ptr = (char *)mem_realloc(ptr, old_size, new_size);
    CU_ASSERT_PTR_NOT_NULL(new_ptr);

    // Verify data within new size is preserved
    for (size_t i = 0; i < new_size; i++) {
        CU_ASSERT_EQUAL(new_ptr[i], (char)('A' + i));
    }

    mem_free(new_ptr, new_size);
}

static void mem_realloc_t__for_larger_count__should_return__grown_typed_array(void) {
    typedef struct {
        int value;
    } Item;
    constexpr size_t old_count = 5;
    constexpr size_t new_count = 10;

    Item * arr = mem_alloc_zero(old_count * sizeof(Item));
    CU_ASSERT_PTR_NOT_NULL(arr);

    // Fill initial array
    for (size_t i = 0; i < old_count; i++) {
        arr[i].value = (int)i;
    }

    // Reallocate
    Item * new_arr = mem_realloc(arr, old_count * sizeof(Item), new_count * sizeof(Item));
    CU_ASSERT_PTR_NOT_NULL(new_arr);

    // Verify old data preserved
    for (size_t i = 0; i < old_count; i++) {
        CU_ASSERT_EQUAL(new_arr[i].value, (int)i);
    }

    mem_free(new_arr, new_count * sizeof(Item));
}

// =====================================================================================================================
// alloc_push() / alloc_pop() - Thread-local allocator stack tests
// =====================================================================================================================

static void alloc_push__should_add_allocator_to_stack(void) {
    setup_test_allocator();
    // Initially using default allocator
    allocator current = alloc_current();
    CU_ASSERT_PTR_EQUAL(current.alloc, r_default_allocator.alloc);

    // Push custom allocator
    alloc_push(test_allocator);
    current = alloc_current();
    CU_ASSERT_PTR_EQUAL(current.alloc, test_allocator.alloc);
    CU_ASSERT_PTR_EQUAL(current.ctx, &test_stats);

    // Pop it back
    alloc_pop();
    current = alloc_current();
    CU_ASSERT_PTR_EQUAL(current.alloc, r_default_allocator.alloc);
}

static void alloc_push__with_nested_allocators__should_use_topmost(void) {
    setup_test_allocator();

    allocator test_alloc_1 = test_allocator;
    allocator test_alloc_2 = test_allocator;
    test_alloc_1.ctx = (void *)1;
    test_alloc_2.ctx = (void *)2;

    alloc_push(test_alloc_1);
    allocator current = alloc_current();
    CU_ASSERT_PTR_EQUAL(current.ctx, (void *)1);

    alloc_push(test_alloc_2);
    current = alloc_current();
    CU_ASSERT_PTR_EQUAL(current.ctx, (void *)2);

    alloc_pop();
    current = alloc_current();
    CU_ASSERT_PTR_EQUAL(current.ctx, (void *)1);

    alloc_pop();
}

// =====================================================================================================================
// alloc_scope() - Scoped allocator tests
// =====================================================================================================================

static void alloc_scope__should_push_and_pop_allocator(void) {
    setup_test_allocator();
    const allocator before = alloc_current();
    CU_ASSERT_PTR_EQUAL(before.alloc, r_default_allocator.alloc);

    alloc_scope(test_allocator) {
        const allocator inside = alloc_current();
        CU_ASSERT_PTR_EQUAL(inside.alloc, test_allocator.alloc);
    }

    const allocator after = alloc_current();
    CU_ASSERT_PTR_EQUAL(after.alloc, r_default_allocator.alloc);
}

static void alloc_scope__with_allocation__should_use_scoped_allocator(void) {
    setup_test_allocator();

    alloc_scope(test_allocator) {
        void * ptr = mem_alloc(100);
        CU_ASSERT_PTR_NOT_NULL(ptr);
        CU_ASSERT_EQUAL(test_stats.alloc_count, 1);
        CU_ASSERT_EQUAL(test_stats.total_allocated, 100);
        mem_free(ptr, 100);
        CU_ASSERT_EQUAL(test_stats.free_count, 1);
    }
}

static void alloc_scope__with_nested_scopes__should_respect_nesting(void) {
    setup_test_allocator();
    alloc_stats stats2 = {0};
    const allocator test_alloc_2 = (allocator){
        .alloc = test_alloc,
        .realloc = test_realloc,
        .free = test_free,
        .ctx = &stats2,
    };

    alloc_scope(test_allocator) {
        void * p1 = mem_alloc(100);
        CU_ASSERT_EQUAL(test_stats.alloc_count, 1);

        alloc_scope(test_alloc_2) {
            void * p2 = mem_alloc(200);
            CU_ASSERT_EQUAL(test_stats.alloc_count, 1); // Still 1
            CU_ASSERT_EQUAL(stats2.alloc_count, 1);     // New allocator used
            mem_free(p2, 200);
        }

        mem_free(p1, 100);
    }
}

static void alloc_scope__with_break__should_still_pop_allocator(void) {
    setup_test_allocator();

    for (int i = 0; i < 1; i++) {
        alloc_scope(test_allocator) {
            const allocator inside = alloc_current();
            CU_ASSERT_PTR_EQUAL(inside.alloc, test_allocator.alloc);
            break; // Exit with break
        }
        const allocator outside = alloc_current();
        CU_ASSERT_PTR_EQUAL(outside.alloc, r_default_allocator.alloc);
    }
}

// =====================================================================================================================
// Custom allocator tests
// =====================================================================================================================

static void mem_alloc__with_custom_allocator__should__call_custom_allocator(void) {
    setup_test_allocator();

    alloc_scope(test_allocator) {
        void * ptr = mem_alloc(100);
        CU_ASSERT_PTR_NOT_NULL(ptr);
        CU_ASSERT_EQUAL(test_stats.alloc_count, 1);
        CU_ASSERT_EQUAL(test_stats.total_allocated, 100);

        mem_free(ptr, 100);
        CU_ASSERT_EQUAL(test_stats.free_count, 1);
        CU_ASSERT_EQUAL(test_stats.total_freed, 100);
    }
}

static void mem_realloc__with_custom_allocator__should__call_custom_reallocator(void) {
    setup_test_allocator();

    alloc_scope(test_allocator) {
        void * ptr = mem_alloc(50);
        CU_ASSERT_EQUAL(test_stats.alloc_count, 1);

        void * new_ptr = mem_realloc(ptr, 50, 100);
        CU_ASSERT_PTR_NOT_NULL(new_ptr);
        CU_ASSERT_EQUAL(test_stats.realloc_count, 1);
        CU_ASSERT_EQUAL(test_stats.total_allocated, 150);
        CU_ASSERT_EQUAL(test_stats.total_freed, 50);

        mem_free(new_ptr, 100);
    }
}

static void mem_alloc_zero__with_custom_allocator__should_return__zeroed_memory(void) {
    setup_test_allocator();

    alloc_scope(test_allocator) {
        int * arr = mem_alloc_zero(10 * sizeof(int));
        CU_ASSERT_PTR_NOT_NULL(arr);
        CU_ASSERT_EQUAL(test_stats.alloc_count, 1);

        for (int i = 0; i < 10; i++) {
            CU_ASSERT_EQUAL(arr[i], 0);
        }

        mem_free(arr, 10 * sizeof(int));
    }
}

static void mem_alloc__with_custom_allocator__should__track_multiple_allocations(void) {
    setup_test_allocator();

    alloc_scope(test_allocator) {
        void * p1 = mem_alloc(100);
        void * p2 = mem_alloc(200);
        void * p3 = mem_alloc(300);

        CU_ASSERT_EQUAL(test_stats.alloc_count, 3);
        CU_ASSERT_EQUAL(test_stats.total_allocated, 600);

        mem_free(p1, 100);
        mem_free(p2, 200);
        mem_free(p3, 300);

        CU_ASSERT_EQUAL(test_stats.free_count, 3);
        CU_ASSERT_EQUAL(test_stats.total_freed, 600);
    }
}

// =====================================================================================================================
// Stress tests
// =====================================================================================================================

static void mem_alloc__for_100_cycles__should__succeed(void) {
    for (int i = 0; i < 100; i++) {
        void * ptr = mem_alloc(100);
        CU_ASSERT_PTR_NOT_NULL(ptr);
        mem_free(ptr, 100);
    }
}

static void mem_alloc__for_1mb__should_return__nonnull(void) {
    constexpr size_t large_size = 1024 * 1024;
    void * ptr = mem_alloc(large_size);
    CU_ASSERT_PTR_NOT_NULL(ptr);
    mem_free(ptr, large_size);
}

static void mem_alloc__with_many_scope_entries__should__succeed(void) {
    setup_test_allocator();

    for (int i = 0; i < 10; i++) {
        alloc_scope(test_allocator) {
            void * ptr = mem_alloc(1024);
            CU_ASSERT_PTR_NOT_NULL(ptr);
            mem_free(ptr, 1024);
        }
    }

    CU_ASSERT_EQUAL(test_stats.alloc_count, 10);
    CU_ASSERT_EQUAL(test_stats.free_count, 10);
}

// =====================================================================================================================
// Error handling tests
// =====================================================================================================================

static void err_set__should_set_error_with_location(void) {
    err_clear();
    CU_ASSERT_FALSE(err_has());

    err_set(R_ERR_NULL_POINTER, "Test error");
    CU_ASSERT_TRUE(err_has());
    CU_ASSERT_EQUAL(err_code(), R_ERR_NULL_POINTER);
    CU_ASSERT_STRING_EQUAL(err_msg(), "Test error");

    err_clear();
}

static void err_set__with_null_message__should_use_default_message(void) {
    err_clear();
    err_set(R_ERR_OUT_OF_MEMORY, nullptr);
    CU_ASSERT_STRING_EQUAL(err_msg(), "Out of memory");
    err_clear();
}

static void err_set__with_empty_message__should_use_default_message(void) {
    err_clear();
    err_set(R_ERR_INVALID_ARGUMENT, "");
    CU_ASSERT_STRING_EQUAL(err_msg(), "Invalid argument");
    err_clear();
}

static void err_get__with_no_error__should_return_null(void) {
    err_clear();
    const r_error_ctx * ctx = err_get();
    CU_ASSERT_PTR_NULL(ctx);
}

static void err_get__with_error__should_return_context(void) {
    err_clear();
    err_set(R_ERR_BUFFER_OVERFLOW, "Test overflow");
    const r_error_ctx * ctx = err_get();
    CU_ASSERT_PTR_NOT_NULL(ctx);
    CU_ASSERT_EQUAL(ctx->code, R_ERR_BUFFER_OVERFLOW);
    CU_ASSERT_STRING_EQUAL(ctx->message, "Test overflow");
    CU_ASSERT_PTR_NOT_NULL(ctx->file);
    CU_ASSERT_PTR_NOT_NULL(ctx->func);
    CU_ASSERT(ctx->line > 0);
    err_clear();
}

static void err_code__with_no_error__should_return_ok(void) {
    err_clear();
    CU_ASSERT_EQUAL(err_code(), R_ERR_OK);
}

static void err_msg__with_no_error__should_return_no_error(void) {
    err_clear();
    CU_ASSERT_STRING_EQUAL(err_msg(), "No error");
}

static void err_has__with_no_error__should_return_false(void) {
    err_clear();
    CU_ASSERT_FALSE(err_has());
}

static void err_has__with_error__should_return_true(void) {
    err_clear();
    err_set(R_ERR_NOT_FOUND, nullptr);
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

static void err_depth__with_no_errors__should_return_zero(void) {
    err_clear();
    CU_ASSERT_EQUAL(err_depth(), 0);
}

static void err_depth__with_multiple_errors__should_return_count(void) {
    err_clear();
    err_set(R_ERR_NULL_POINTER, "Error 1");
    CU_ASSERT_EQUAL(err_depth(), 1);
    err_set(R_ERR_INVALID_ARGUMENT, "Error 2");
    CU_ASSERT_EQUAL(err_depth(), 2);
    err_set(R_ERR_OVERFLOW, "Error 3");
    CU_ASSERT_EQUAL(err_depth(), 3);
    err_clear();
}

static void err_at__with_valid_index__should_return_error(void) {
    err_clear();
    err_set(R_ERR_NULL_POINTER, "First");
    err_set(R_ERR_INVALID_ARGUMENT, "Second");
    err_set(R_ERR_OVERFLOW, "Third");

    const r_error_ctx * err0 = err_at(0);
    CU_ASSERT_PTR_NOT_NULL(err0);
    CU_ASSERT_EQUAL(err0->code, R_ERR_NULL_POINTER);
    CU_ASSERT_STRING_EQUAL(err0->message, "First");

    const r_error_ctx * err1 = err_at(1);
    CU_ASSERT_PTR_NOT_NULL(err1);
    CU_ASSERT_EQUAL(err1->code, R_ERR_INVALID_ARGUMENT);

    const r_error_ctx * err2 = err_at(2);
    CU_ASSERT_PTR_NOT_NULL(err2);
    CU_ASSERT_EQUAL(err2->code, R_ERR_OVERFLOW);

    err_clear();
}

static void err_at__with_negative_index__should_return_null(void) {
    err_clear();
    err_set(R_ERR_NULL_POINTER, "Error");
    const r_error_ctx * err = err_at(-1);
    CU_ASSERT_PTR_NULL(err);
    err_clear();
}

static void err_at__with_out_of_bounds_index__should_return_null(void) {
    err_clear();
    err_set(R_ERR_NULL_POINTER, "Error");
    const r_error_ctx * err = err_at(10);
    CU_ASSERT_PTR_NULL(err);
    err_clear();
}

static void err_pop__should_remove_most_recent_error(void) {
    err_clear();
    err_set(R_ERR_NULL_POINTER, "First");
    err_set(R_ERR_INVALID_ARGUMENT, "Second");
    CU_ASSERT_EQUAL(err_depth(), 2);
    CU_ASSERT_EQUAL(err_code(), R_ERR_INVALID_ARGUMENT);

    err_pop();
    CU_ASSERT_EQUAL(err_depth(), 1);
    CU_ASSERT_EQUAL(err_code(), R_ERR_NULL_POINTER);

    err_pop();
    CU_ASSERT_EQUAL(err_depth(), 0);
    CU_ASSERT_FALSE(err_has());
}

static void err_pop__with_no_errors__should_not_crash(void) {
    err_clear();
    err_pop(); // Should be safe
    CU_ASSERT_EQUAL(err_depth(), 0);
}

static void err_clear__should_remove_all_errors(void) {
    err_clear();
    err_set(R_ERR_NULL_POINTER, "Error 1");
    err_set(R_ERR_INVALID_ARGUMENT, "Error 2");
    err_set(R_ERR_OVERFLOW, "Error 3");
    CU_ASSERT_EQUAL(err_depth(), 3);

    err_clear();
    CU_ASSERT_EQUAL(err_depth(), 0);
    CU_ASSERT_FALSE(err_has());
}

static void err_enable__should_control_error_tracking(void) {
    err_clear();
    CU_ASSERT_TRUE(err_is_enabled());

    // Disable error tracking
    err_enable(false);
    CU_ASSERT_FALSE(err_is_enabled());

    // Try to set error while disabled
    err_set(R_ERR_NULL_POINTER, "Should not be set");
    CU_ASSERT_FALSE(err_has());
    CU_ASSERT_EQUAL(err_depth(), 0);

    // Re-enable error tracking
    err_enable(true);
    CU_ASSERT_TRUE(err_is_enabled());

    // Now errors should be set
    err_set(R_ERR_NULL_POINTER, "Should be set");
    CU_ASSERT_TRUE(err_has());
    err_clear();
}

static void err_set__when_disabled__should_return_false(void) {
    err_clear();
    err_enable(false);
    bool result = err_set(R_ERR_NULL_POINTER, "Test");
    CU_ASSERT_FALSE(result);
    err_enable(true);
}

static void err_set__when_stack_full__should_return_false(void) {
    err_clear();
    // Fill the error stack to maximum
    for (int i = 0; i < R_ERROR_STACK_MAX; i++) {
        bool result = err_set(R_ERR_NULL_POINTER, "Error");
        CU_ASSERT_TRUE(result);
    }
    CU_ASSERT_EQUAL(err_depth(), R_ERROR_STACK_MAX);

    // Try to add one more - should fail
    bool result = err_set(R_ERR_OVERFLOW, "Should fail");
    CU_ASSERT_FALSE(result);
    CU_ASSERT_EQUAL(err_depth(), R_ERROR_STACK_MAX);

    err_clear();
}

static void r_error_message__for_all_error_codes__should_return_messages(void) {
    // Test all defined error codes
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_OK), "No error");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_ALLOC_FAILED), "Memory allocation failed");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_OUT_OF_MEMORY), "Out of memory");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_ALLOCATOR_STACK_OVERFLOW), "Allocator stack overflow");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_ALLOCATOR_STACK_UNDERFLOW), "Allocator stack underflow");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_NULL_POINTER), "Null pointer argument");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_INVALID_ARGUMENT), "Invalid argument");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_INVALID_LENGTH), "Invalid length");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_EMPTY_INPUT), "Empty input");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_OVERFLOW), "Numeric overflow");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_BUFFER_OVERFLOW), "Buffer overflow");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_LENGTH_EXCEEDED), "Maximum length exceeded");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_CAPACITY_EXCEEDED), "Capacity exceeded");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_NOT_FOUND), "Item not found");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_PATTERN_NOT_FOUND), "Pattern not found in string");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_INDEX_OUT_OF_BOUNDS), "Index out of bounds");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_QUEUE_FULL), "Queue is full");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_QUEUE_EMPTY), "Queue is empty");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_LIST_EMPTY), "List is empty");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_FORMAT_FAILED), "Format operation failed");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_PARSE_FAILED), "Parse operation failed");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_STRING_TOO_LONG), "String exceeds maximum length");
    CU_ASSERT_STRING_EQUAL(r_error_message(R_ERR_INVALID_UTF8), "Invalid UTF-8 sequence");
}

static void r_error_message__for_unknown_code__should_return_unknown(void) {
    CU_ASSERT_STRING_EQUAL(r_error_message((r_error_code)9999), "Unknown error");
}

static void err_print__with_no_error__should_print_no_error(void) {
    err_clear();
    FILE * null_stream = open_null_stream();
    if (null_stream) {
        err_print(null_stream);
        fclose(null_stream);
    }
    CU_ASSERT_TRUE(true); // If we got here, it didn't crash
}

static void err_print__with_error__should_not_crash(void) {
    err_clear();
    err_set(R_ERR_NULL_POINTER, "Test error");
    FILE * null_stream = open_null_stream();
    if (null_stream) {
        err_print(null_stream);
        fclose(null_stream);
    }
    CU_ASSERT_TRUE(true); // If we got here, it didn't crash
    err_clear();
}

static void err_print_stack__with_no_errors__should_not_crash(void) {
    err_clear();
    FILE * null_stream = open_null_stream();
    if (null_stream) {
        err_print_stack(null_stream);
        fclose(null_stream);
    }
    CU_ASSERT_TRUE(true);
}

static void err_print_stack__with_multiple_errors__should_not_crash(void) {
    err_clear();
    err_set(R_ERR_NULL_POINTER, "Error 1");
    err_set(R_ERR_INVALID_ARGUMENT, "Error 2");
    err_set(R_ERR_OVERFLOW, "Error 3");
    FILE * null_stream = open_null_stream();
    if (null_stream) {
        err_print_stack(null_stream);
        fclose(null_stream);
    }
    CU_ASSERT_TRUE(true);
    err_clear();
}

static void mem_free__with_null_pointer__should_not_crash(void) {
    mem_free(nullptr, 100);
    CU_ASSERT_TRUE(true); // If we got here, it didn't crash
}

// =====================================================================================================================
// Test suite registration
// =====================================================================================================================

int main(void) {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // R macros suite
    CU_pSuite suite_macros = CU_add_suite("R macros", nullptr, nullptr);
    if (suite_macros == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_macros, R_GLUE__should__concatenate_tokens);

    // mem_alloc() suite
    CU_pSuite suite_mem_alloc = CU_add_suite("mem_alloc()", nullptr, nullptr);
    if (suite_mem_alloc == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_mem_alloc, mem_alloc__for_default_allocator__should_return__nonnull);
    ADD_TEST(suite_mem_alloc, mem_alloc__for_size__should_return__writable_memory);
    ADD_TEST(suite_mem_alloc, mem_alloc_t__for_type__should_return__typed_pointer);
    ADD_TEST(suite_mem_alloc, mem_alloc__with_custom_allocator__should__call_custom_allocator);
    ADD_TEST(suite_mem_alloc, mem_alloc__with_custom_allocator__should__track_multiple_allocations);

    // mem_alloc_zero() suite
    CU_pSuite suite_mem_alloc_zero = CU_add_suite("mem_alloc_zero()", nullptr, nullptr);
    if (suite_mem_alloc_zero == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_mem_alloc_zero, mem_alloc_zero__for_count_and_size__should_return__zeroed_memory);
    ADD_TEST(suite_mem_alloc_zero, mem_alloc_zero__for_small_size__should_return__zeroed_byte);
    ADD_TEST(suite_mem_alloc_zero, mem_alloc_zero_t__for_count_and_type__should_return__zeroed_array);
    ADD_TEST(suite_mem_alloc_zero, mem_alloc_zero__with_custom_allocator__should_return__zeroed_memory);

    // mem_realloc() suite
    CU_pSuite suite_mem_realloc = CU_add_suite("mem_realloc()", nullptr, nullptr);
    if (suite_mem_realloc == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_mem_realloc, mem_realloc__for_larger_size__should_return__grown_allocation);
    ADD_TEST(suite_mem_realloc, mem_realloc__for_smaller_size__should_return__shrunk_allocation);
    ADD_TEST(suite_mem_realloc, mem_realloc_t__for_larger_count__should_return__grown_typed_array);
    ADD_TEST(suite_mem_realloc, mem_realloc__with_custom_allocator__should__call_custom_reallocator);

    // alloc_push() / alloc_pop() suite
    CU_pSuite suite_alloc_push_pop = CU_add_suite("alloc_push() / alloc_pop()", nullptr, nullptr);
    if (suite_alloc_push_pop == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_alloc_push_pop, alloc_push__should_add_allocator_to_stack);
    ADD_TEST(suite_alloc_push_pop, alloc_push__with_nested_allocators__should_use_topmost);

    // alloc_scope() suite
    CU_pSuite suite_alloc_scope = CU_add_suite("alloc_scope()", nullptr, nullptr);
    if (suite_alloc_scope == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_alloc_scope, alloc_scope__should_push_and_pop_allocator);
    ADD_TEST(suite_alloc_scope, alloc_scope__with_allocation__should_use_scoped_allocator);
    ADD_TEST(suite_alloc_scope, alloc_scope__with_nested_scopes__should_respect_nesting);
    ADD_TEST(suite_alloc_scope, alloc_scope__with_break__should_still_pop_allocator);

    // Stress tests suite
    CU_pSuite suite_stress = CU_add_suite("Stress tests", nullptr, nullptr);
    if (suite_stress == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_stress, mem_alloc__for_100_cycles__should__succeed);
    ADD_TEST(suite_stress, mem_alloc__for_1mb__should_return__nonnull);
    ADD_TEST(suite_stress, mem_alloc__with_many_scope_entries__should__succeed);

    // Error handling suite
    CU_pSuite suite_error = CU_add_suite("Error handling", nullptr, nullptr);
    if (suite_error == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_error, err_set__should_set_error_with_location);
    ADD_TEST(suite_error, err_set__with_null_message__should_use_default_message);
    ADD_TEST(suite_error, err_set__with_empty_message__should_use_default_message);
    ADD_TEST(suite_error, err_set__when_disabled__should_return_false);
    ADD_TEST(suite_error, err_set__when_stack_full__should_return_false);
    ADD_TEST(suite_error, err_get__with_no_error__should_return_null);
    ADD_TEST(suite_error, err_get__with_error__should_return_context);
    ADD_TEST(suite_error, err_code__with_no_error__should_return_ok);
    ADD_TEST(suite_error, err_msg__with_no_error__should_return_no_error);
    ADD_TEST(suite_error, err_has__with_no_error__should_return_false);
    ADD_TEST(suite_error, err_has__with_error__should_return_true);
    ADD_TEST(suite_error, err_depth__with_no_errors__should_return_zero);
    ADD_TEST(suite_error, err_depth__with_multiple_errors__should_return_count);
    ADD_TEST(suite_error, err_at__with_valid_index__should_return_error);
    ADD_TEST(suite_error, err_at__with_negative_index__should_return_null);
    ADD_TEST(suite_error, err_at__with_out_of_bounds_index__should_return_null);
    ADD_TEST(suite_error, err_pop__should_remove_most_recent_error);
    ADD_TEST(suite_error, err_pop__with_no_errors__should_not_crash);
    ADD_TEST(suite_error, err_clear__should_remove_all_errors);
    ADD_TEST(suite_error, err_enable__should_control_error_tracking);
    ADD_TEST(suite_error, r_error_message__for_all_error_codes__should_return_messages);
    ADD_TEST(suite_error, r_error_message__for_unknown_code__should_return_unknown);
    ADD_TEST(suite_error, err_print__with_no_error__should_print_no_error);
    ADD_TEST(suite_error, err_print__with_error__should_not_crash);
    ADD_TEST(suite_error, err_print_stack__with_no_errors__should_not_crash);
    ADD_TEST(suite_error, err_print_stack__with_multiple_errors__should_not_crash);
    ADD_TEST(suite_error, mem_free__with_null_pointer__should_not_crash);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

#ifdef _WIN32
#pragma GCC diagnostic pop
#endif
