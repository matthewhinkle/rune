//
// Created by Matt on 4/27/2025.
//

#ifndef RUNE_STD_H
#define RUNE_STD_H

#include <stddef.h>

#define GLUE_INTERNAL(a, b) a##b
#define GLUE(a, b) GLUE_INTERNAL(a, b)
#define JOIN(a, b, sym) GLUE(GLUE(a, sym), b)
#define STRINGIFY(a) #a

#define FUNC(block)                                                            \
    do {                                                                       \
        block                                                                  \
    } while (0)
#define RETURN(expr) ({ expr; })

#define VA_ARGS(...) , ##__VA_ARGS__
#define R_END 0x52554e45

#define r_malloc_t(type) (type *)r_malloc(sizeof(type))
#define r_calloc_t(n, type) (type *)r_calloc((n), sizeof(type))
#define r_recalloc_t(ptr, n, type) (type *)r_recalloc(ptr, (n), sizeof(type))

extern void * r_malloc(size_t size);
extern void * r_calloc(size_t n, size_t size);
extern void * r_recalloc(void * ptr, size_t n, size_t size);
extern void * r_realloc(void * ptr, size_t new_size);
extern void * r_free(void * ptr);

#endif // RUNE_STD_H
