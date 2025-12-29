#ifndef RUNE_MAP_H
#define RUNE_MAP_H

// sample map data
typedef struct {
    char * id;
    char * region;
    int type;
} instance;

#ifdef T
#error "T is already defined"
#else
#define T instance
#include "tree.h"
#undef T

typedef struct {
    RBT(instance) t;

} map_bucket;

#endif // T

#endif // RUNE_MAP_H
