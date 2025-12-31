#ifndef RUNE_MAP_H
#define RUNE_MAP_H

// sample map data
typedef struct {
    const char * id;
    const char * region;
    const int type;
} instance;

#ifdef T
#error "T is already defined"
#else // T
#define T instance
#include "tree.h"
#undef T

// =====================================================================================================================
// buckets
// =====================================================================================================================

typedef struct {
    size_t size;
    RBT(instance) items;
} map_bucket;

static map_bucket bucket() {
    return (map_bucket){
        .size = 0,
        .items = rbt(instance),
    };
}

static bool bucket_add(instance val) {

    return false;
}

// =====================================================================================================================
// map
// =====================================================================================================================

typedef struct {
    size_t n_buckets;
    map_bucket * buckets;
} map;

#endif // T

#endif // RUNE_MAP_H
