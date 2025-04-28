//
// Created by Matt on 4/27/2025.
//

#ifndef RUNE_STR_H
#define RUNE_STR_H

#include <stddef.h>
#include <string.h>

#ifndef RUNE_CONST_STR_MAX_SIZE
#define RUNE_CONST_STR_MAX_SIZE 1024
#endif // RUNE_CONST_STR_MAX_SIZE

#ifndef RUNE_STR_MAX_SIZE
#define RUNE_STR_MAX_SIZE (128 * 1024)
#endif // RUNE_STR_MAX_SIZE

typedef struct str {
    const char * chars;
    const size_t size;
} str;

extern str str_const(const char * chars);

#endif // RUNE_STR_H
