/**
 * Core runtime with thread-local error handling and allocator support.
 *
 * Provides:
 *   - Thread-local error stack with automatic location capture (file/line/function)
 *   - Flexible allocator system with push/pop stack semantics
 *   - RAII-style allocator scopes via alloc_scope() macro
 *   - Type-safe memory allocation macros (mem_alloc, mem_alloc_zero, mem_realloc)
 *   - Default malloc/realloc/free allocator with thread-local stack fallback
 *
 * Quick Reference:
 *
 *   Error API
 *   -------------------------------------------------------------------------------------------------------------------
 *   err_set(code, message)       Set error with automatic location capture
 *   err_get()                    Get most recent error context
 *   err_code()                   Get error code of most recent error
 *   err_msg()                    Get error message of most recent error
 *   err_has()                    Check if error is currently set
 *   err_depth()                  Get number of errors in stack
 *   err_at(index)                Get error at specific stack index
 *   err_pop()                    Remove most recent error
 *   err_clear()                  Clear all errors
 *   err_enable(enabled)          Enable/disable error tracking
 *   err_is_enabled()             Check if error tracking is enabled
 *   err_print(stream)            Print most recent error
 *   err_print_stack(stream)      Print all errors in stack
 *   r_error_message(code)        Get message for error code
 *
 *   Allocator API
 *   -------------------------------------------------------------------------------------------------------------------
 *   alloc_push(a)                Push allocator onto stack
 *   alloc_pop()                  Pop allocator from stack
 *   alloc_current()              Get current allocator
 *   mem_alloc(type)              Allocate single instance
 *   mem_alloc_zero(count, type)  Allocate zero-initialized array
 *   mem_realloc(
 *       ptr, old_n, new_n, type
 *   )                            Reallocate typed array
 *   mem_free(ptr, size)          Free allocated memory
 *
 * Example:
 *   // Allocate and use default allocator
 *   int * x = mem_alloc(int);
 *   if (err_has()) { err_print(stderr); return; }
 *   mem_free(x, sizeof(int));
 *
 *   // Scoped custom allocator (automatically popped on exit)
 *   alloc_scope(my_custom_allocator) {
 *       int * y = mem_alloc(int);  // Uses my_custom_allocator
 *   }
 *
 * Requires C11 for _Thread_local support.
 * Identifiers beginning with `R_` or `r_` are reserved for internal use.
 */

#ifndef RUNE_H
#define RUNE_H

// ReSharper disable once CppUnusedIncludeDirective
#include <stdatomic.h>
#include <stdio.h>

/*
 * =====================================================================================================================
 * MACROS
 * =====================================================================================================================
 */

// ------------------------------------------------ Token manipulation -------------------------------------------------

#define R_GLUE_INTERN(a, b) a##b
#define R_GLUE(a, b) R_GLUE_INTERN(a, b)
#define R_JOIN(a, b, sym) R_GLUE(R_GLUE(a, sym), b)
#define R_UNIQUE(name) R_GLUE(name, __LINE__)
#define R_STRINGIFY(a) #a
#define R_VA_ARGS(...) __VA_OPT__(, ) __VA_ARGS__
#define R_(name) R_JOIN(r, name, __)

// ------------------------------------------------- Numeric utilities -------------------------------------------------

#define R_MAX(a, b) ((a) > (b) ? (a) : (b))
#define R_MIN(a, b) ((a) < (b) ? (a) : (b))
#define R_CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

// --------------------------------------------------- Type adapters ---------------------------------------------------

#if defined(__clang__) || defined(__GNUC__)
#define R_Atomic(type) _Atomic(type) // Use real _Atomic on Clang/GCC
#else
#define R_Atomic(type) type // Fallback for MSVC where _Atomic may not be available
#endif

// ---------------------------------------------- Function wrapper macros ----------------------------------------------

#define R_OPT(def, ...) __VA_OPT__(__VA_ARGS__) __VA_OPT__(def)

// -------------------------------------------- Scope and allocator macros ---------------------------------------------
// These macros implement RAII-style resource management using push/pop semantics.
// The resource is pushed on entry and automatically popped on exit, even when exiting
// via break, continue, or return.
//
// scope(push_fn, pop_fn, resource)  - Generic scoped resource
//   push_fn: function that takes one argument to push the resource.
//   pop_fn:  function that takes no arguments to pop the resource.
//   resource: the resource to push
//
//   Example:
//     scope(custom_push, custom_pop, my_resource) {
//         // my_resource is active here
//     }
//     // my_resource is now popped
//
// alloc_scope(a) - Specialized allocator scope (shorthand for scope(alloc_push, alloc_pop, a))
//
//   Example:
//     alloc_scope(my_arena_allocator) {
//         int * x = mem_alloc(int);  // Uses my_arena_allocator
//     }
//     // my_arena_allocator is now popped

#define scope(push_fn, pop_fn, resource)                                                                               \
    for (int R_UNIQUE(_scope_once) = (push_fn(resource), 1); R_UNIQUE(_scope_once); R_UNIQUE(_scope_once) = 0, pop_fn())

#define alloc_scope(a) scope(alloc_push, alloc_pop, a)

/*
 * =====================================================================================================================
 * TYPE DEFINITIONS
 * =====================================================================================================================
 */

// ----------------------------------------------------- Constants -----------------------------------------------------

static constexpr char NULLTERM = '\0';

// ------------------------------------------------ Allocator interface ------------------------------------------------

/**
 * Allocator interface with separate function pointers for clarity.
 *
 * Provides customizable memory allocation with separate alloc, realloc, and free functions.
 * The `ctx` field allows stateful allocators (e.g., arena, pool allocators).
 *
 * @param alloc   Allocate memory: alloc(ctx, size) - Returns pointer or asserts
 * @param realloc Reallocate memory: realloc(ctx, ptr, old_size, new_size) - Returns pointer or asserts
 * @param free    Free memory: free(ctx, ptr, size) - No return value
 * @param ctx     Optional context passed to all three functions
 */
typedef struct {
    void * (*alloc)(void * ctx, size_t size);
    void * (*realloc)(void * ctx, void * ptr, size_t old_size, size_t new_size);
    void (*free)(void * ctx, void * ptr, size_t size);
    void * ctx;
} allocator;

// ---------------------------------------------------- Error codes ----------------------------------------------------

typedef enum {
    // Success
    R_ERR_OK = 0,

    // Memory errors (1-99)
    R_ERR_ALLOC_FAILED = 1,
    R_ERR_OUT_OF_MEMORY = 2,
    R_ERR_ALLOCATOR_STACK_OVERFLOW = 3,
    R_ERR_ALLOCATOR_STACK_UNDERFLOW = 4,

    // Validation errors (100-199)
    R_ERR_NULL_POINTER = 100,
    R_ERR_INVALID_ARGUMENT = 101,
    R_ERR_INVALID_LENGTH = 102,
    R_ERR_EMPTY_INPUT = 103,

    // Overflow/capacity errors (200-299)
    R_ERR_OVERFLOW = 200,
    R_ERR_BUFFER_OVERFLOW = 201,
    R_ERR_LENGTH_EXCEEDED = 202,
    R_ERR_CAPACITY_EXCEEDED = 203,

    // Not found/empty errors (300-399)
    R_ERR_NOT_FOUND = 300,
    R_ERR_PATTERN_NOT_FOUND = 301,
    R_ERR_INDEX_OUT_OF_BOUNDS = 302,

    // Collection errors (400-499)
    R_ERR_QUEUE_FULL = 400,
    R_ERR_QUEUE_EMPTY = 401,
    R_ERR_LIST_EMPTY = 402,

    // Format/parse errors (500-599)
    R_ERR_FORMAT_FAILED = 500,
    R_ERR_PARSE_FAILED = 501,

    // String operation errors (600-699)
    R_ERR_STRING_TOO_LONG = 600,
    R_ERR_INVALID_UTF8 = 601,
} r_error_code;

// --------------------------------------------------- Error context ---------------------------------------------------

typedef struct {
    const r_error_code code;    // Error code (immutable)
    const char * const message; // Static error message (immutable)
    const char * const file;    // Source file where error occurred (immutable)
    const int line;             // Line number where error occurred (immutable)
    const char * const func;    // Function name where error occurred (immutable)
} r_error_ctx;

// --------------------------------------------- Error stack configuration ---------------------------------------------

#ifdef RCFG__ERROR_STACK_MAX
static constexpr int R_ERROR_STACK_MAX = RCFG__ERROR_STACK_MAX;
#else  // Default max error stack depth
static constexpr int R_ERROR_STACK_MAX = 8;
#endif // RCFG__ERROR_STACK_MAX

typedef struct {
    r_error_ctx stack[R_ERROR_STACK_MAX];
    int depth;
    bool enabled;
} r_error_stack_t;

extern _Thread_local r_error_stack_t r_error_stack;

/*
 * =====================================================================================================================
 * ERROR HANDLING
 * =====================================================================================================================
 */

// -------------------------------------------------- Message lookup ---------------------------------------------------

extern const char * r_error_message(r_error_code code);

// ------------------------------------------- Error API: Setup & Reporting --------------------------------------------

#define err_set(code, message) R_(err_set)((code), (message), __FILE__, __LINE__, __func__)
#define err_print(stream) R_(err_print)(stream)
#define err_print_stack(stream) R_(err_print_stack)(stream)

extern bool R_(err_set)(r_error_code code, const char * message, const char * file, int line, const char * func);
extern void R_(err_print)(FILE * stream);
extern void R_(err_print_stack)(FILE * stream);

// ----------------------------------------------- Error API: Inspection -----------------------------------------------

#define err_get() R_(err_get)()
#define err_code() R_(err_code)()
#define err_msg() R_(err_msg)()
#define err_has() R_(err_has)()
#define err_depth() R_(err_depth)()
#define err_at(index) R_(err_at)(index)

extern const r_error_ctx * R_(err_get)(void);
extern r_error_code R_(err_code)(void);
extern const char * R_(err_msg)(void);
extern bool R_(err_has)(void);
extern int R_(err_depth)(void);

// ----------------------------------------------- Error API: Management -----------------------------------------------

#define err_pop() R_(err_pop)()
#define err_clear() R_(err_clear)()
#define err_enable(enabled) R_(err_enable)(enabled)
#define err_is_enabled() R_(err_is_enabled)()

extern void R_(err_pop)(void);
extern void R_(err_clear)(void);
extern void R_(err_enable)(bool enabled);
extern bool R_(err_is_enabled)(void);

// ------------------------------------------------ Convenience macros -------------------------------------------------

/**
 * Check if a pointer is null and set error if so.
 * Returns true if pointer was null (error condition).
 *
 * Usage:
 *   if (err_null(data)) return nullptr;
 *   if (err_null(buffer)) { handle_error(); return 1; }
 *
 * @param ptr pointer to check
 * @return true if null (error set), false if valid
 */
#define err_null(ptr) ((ptr) == nullptr && err_set(R_ERR_NULL_POINTER, #ptr " is null"))

/**
 * Check a condition and set error if false.
 * Returns true if condition failed (error condition).
 *
 * Usage:
 *   if (err_check(len > 0, R_ERR_INVALID_LENGTH)) return nullptr;
 *   if (err_check(size <= max, R_ERR_CAPACITY_EXCEEDED)) return false;
 *
 * @param cond condition to check (true = success)
 * @param code error code if condition fails
 * @return true if condition failed (error set), false if condition passed
 */
#define err_check(cond, code) (!(cond) && err_set((code), #cond " failed"))

/**
 * Return nullptr with error set.
 * For pointer-returning functions that encounter errors.
 *
 * Usage:
 *   if (allocation_failed) return err_ret(nullptr, R_ERR_ALLOC_FAILED);
 *
 * @param val value to return (typically nullptr)
 * @param code error code to set
 * @return the value passed in (with error set as side effect)
 */
#define err_ret(val, code) (err_set((code)), (val))

/**
 * Return void with error set.
 * For void-returning functions that encounter errors.
 *
 * Usage:
 *   if (invalid_input) { err_ret_void(R_ERR_INVALID_ARGUMENT); return; }
 *
 * @param code error code to set
 */
#define err_ret_void(code)                                                                                             \
    do {                                                                                                               \
        err_set((code));                                                                                               \
        return;                                                                                                        \
    } while (0)

/*
 * =====================================================================================================================
 * ALLOCATOR INTERFACE
 * =====================================================================================================================
 */

// ------------------------------------------------ Stack configuration ------------------------------------------------

#ifdef RCFG__ALLOC_STACK_MAX
static constexpr int mem_alloc_STACK_MAX = RCFG__ALLOC_STACK_MAX;
#else  // Default max allocator stack depth
static constexpr int mem_alloc_STACK_MAX = 16;
#endif // RCFG__ALLOC_STACK_MAX

// ------------------------------------------ Allocator API: Stack management ------------------------------------------

extern void alloc_push(allocator a);
extern void alloc_pop(void);
extern allocator alloc_current(void);

// ----------------------------------------- Allocator API: Memory operations ------------------------------------------

[[nodiscard]] extern void * mem_alloc(size_t size);
[[nodiscard]] extern void * mem_alloc_zero(size_t size);
[[nodiscard]] extern void * mem_realloc(void * ptr, size_t old_size, size_t new_size);
extern void mem_free(void * ptr, size_t size);

#endif // RUNE_H
