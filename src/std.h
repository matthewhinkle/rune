/*
 * Project-wide standard definitions and wrappers around C standard library behavior.
 *
 * Identifiers beginning with `R_` or `r_` are reserved for internal use.
 */

#ifndef RUNE_STD_H
#define RUNE_STD_H

#include <stddef.h>
#include <stdint.h>

#define R_GLUE(a, b) a##b
#define GLUE(a, b) R_GLUE(a, b)
#define JOIN(a, b, sym) GLUE(GLUE(a, sym), b)
#define STRINGIFY(a) #a
#define RUNE(name) JOIN(r, name, __)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

#define FUNC(block)                                                                                \
    do {                                                                                           \
        block                                                                                      \
    } while (0)

#define VA_ARGS(...) , ##__VA_ARGS__
#define R_END 0x52554e45

// clang-format off
#define GENERIC_INTS(expr)                                                                         \
    signed char: expr,                                                                             \
    char: expr,                                                                                    \
    short: expr,                                                                                   \
    int: expr,                                                                                     \
    long: expr,                                                                                    \
    long long: expr,                                                                               \
    unsigned char: expr,                                                                           \
    unsigned short: expr,                                                                          \
    unsigned int: expr,                                                                            \
    unsigned long : expr,                                                                          \
    unsigned long long : expr
// clang-format on

#define r_malloc_t(type) r_malloc(sizeof(type))
#define r_calloc_t(n, type) r_calloc((n), sizeof(type))
#define r_recalloc_t(ptr, n, type) r_recalloc(ptr, (n), sizeof(type))

static const char NULLTERM = '\0';

/**
 * Allocate memory of the given size. This is a wrapper around `malloc()` that asserts the result is
 * not `nullptr`.
 *
 * @param size the size of memory to allocate.
 * @return a pointer to the allocated memory; must be freed with `r_free()`.
 */
[[nodiscard]]
extern void * r_malloc(size_t size);

/**
 * Allocate memory for an array of `n` elements, each of size `size`. This is a wrapper around
 * `calloc()` that asserts the result is not `nullptr`.
 *
 * @param n the number of elements to allocate.
 * @param size the size of each element.
 * @return a pointer to the allocated memory; must be freed with `r_free()`.
 */
[[nodiscard]]
extern void * r_calloc(size_t n, size_t size);

/**
 * Reallocate memory for an array of `n` elements, each of size `size`. If `ptr` is `nullptr`, this
 * behaves like `r_calloc()`. This is a wrapper around `realloc()` that asserts the result is not
 * `nullptr`.
 *
 * @param ptr the pointer to the existing memory block, or `nullptr`.
 * @param n the number of elements to allocate.
 * @param size the size of each element.
 * @return a pointer to the reallocated memory; must be freed with `r_free()`.
 */
[[nodiscard]]
extern void * r_recalloc(void * ptr, size_t n, size_t size);

/**
 * Reallocate memory to a new size. If `ptr` is `nullptr`, this behaves like `r_malloc()`. This is a
 * wrapper around `realloc()` that asserts the result is not `nullptr`.
 *
 * @param ptr the pointer to the existing memory block, or `nullptr`.
 * @param new_size the new size of the memory block.
 * @return a pointer to the reallocated memory; must be freed with `r_free()`.
 */
[[nodiscard]]
extern void * r_realloc(void * ptr, size_t new_size);

/**
 * Free memory allocated with `r_malloc()`, `r_calloc()`, or `r_recalloc()`. This is a wrapper
 * around `free()` that returns `nullptr` after freeing the memory.
 *
 * @param ptr the pointer to the memory block to free.
 * @return `nullptr`.
 */
extern void * r_free(void * ptr);

#endif // RUNE_STD_H
