/*
 * Test-only header for accessing internal runtime state.
 *
 * This header exposes internal allocator and error state for testing purposes.
 * It should ONLY be included by test files, not by production code.
 *
 * Include this instead of r.h in test files:
 *   #include "r_test.h"
 */

#ifndef RUNE_R_TEST_H
#define RUNE_R_TEST_H

#include "../src/r.h"

// =================================================================================================
// Test-only allocator internals
// =================================================================================================

/**
 * Test-only access to the thread-local allocator stack.
 * @internal - Should only be used by test code
 */
extern _Thread_local struct {
    allocator stack[mem_alloc_STACK_MAX];
    int depth;
} mem_alloc_stack;

/**
 * Default allocator using malloc/realloc/free.
 * @internal - Only exposed for test code
 */
extern const allocator r_default_allocator;

#endif // RUNE_R_TEST_H
