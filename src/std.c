/*
 * std module.
 */

#include "std.h"

#include <assert.h>
#include <stdlib.h>

extern void * r_malloc(const size_t size) {
    void * result = malloc(size);
    assert(result != nullptr);
    return result;
}

extern void * r_calloc(const size_t n, const size_t size) {
    void * result = calloc(n, size);
    assert(result != nullptr);
    return result;
}

extern void * r_recalloc(void * ptr, const size_t n, const size_t size) {
    void * result = ptr == nullptr ? calloc(n, size) : realloc(ptr, n * size);
    assert(result != nullptr);
    return result;
}

extern void * r_realloc(void * ptr, const size_t new_size) {
    void * result = realloc(ptr, new_size);
    assert(result != nullptr);
    return result;
}

extern void * r_free(void * ptr) {
    if (ptr != nullptr) {
        free(ptr);
    }
    return nullptr;
}
