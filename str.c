//
// Created by Matt on 4/27/2025.
//

#include "str.h"

#include <string.h>

extern str str_const(const char chars[]) {
    return (str){
        .chars = chars,
        .size = strnlen(chars, RUNE_CONST_STR_MAX_SIZE),
    };
}
