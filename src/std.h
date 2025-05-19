/*
 * Project-wide standard definitions and wrappers around C standard library behavior.
 *
 * Identifiers beginning with `R_` or `r_` are reserved for internal use.
 */

#ifndef RUNE_STD_H
#define RUNE_STD_H

#include <stddef.h>
#include <stdint.h>

#define GLUE_INTERNAL(a, b) a##b
#define GLUE(a, b) GLUE_INTERNAL(a, b)
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

#define R_IS_PTR(type) _Generic((type), void *: 1, const void *: 1, default: 0)

#define r_malloc_t(type) r_malloc(sizeof(type))
#define r_calloc_t(n, type) r_calloc((n), sizeof(type))
#define r_recalloc_t(ptr, n, type) r_recalloc(ptr, (n), sizeof(type))

static const char NULLTERM = '\0';

extern void * r_malloc(size_t size);
extern void * r_calloc(size_t n, size_t size);
extern void * r_recalloc(void * ptr, size_t n, size_t size);
extern void * r_realloc(void * ptr, size_t new_size);
extern void * r_free(void * ptr);

#endif // RUNE_STD_H
