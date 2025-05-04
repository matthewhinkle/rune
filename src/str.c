//
// Created by Matt on 4/27/2025.
//

#include "str.h"

#include "std.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const int r_str_max_size = 128 * 1024;

extern str slit(const char * chars) {
    const size_t max_size = 1024;
    return snlit(chars, max_size);
}

extern str strc(const char * chars) {
    return strcn(chars, r_str_max_size);
}

extern str snlit(const char * chars, size_t size) {
    return (str){
        .chars = NULL,
        .lit = chars,
        .size = 1 + strnlen(chars, size),
    };
}

extern str strcn(const char * chars, const size_t size) {
    const size_t buf_len = strnlen(chars, size);
    const size_t buf_size = buf_len + 1;
    char * buf = r_calloc_t(buf_size, char);

    strncpy(buf, chars, buf_len);
    buf[buf_len] = NULLTERM;

    return (str){
        .chars = buf,
        .size = buf_size,
        .lit = NULL,
    };
}

extern void sfree(str * s) {
    if (s != NULL) {
        if (s->chars != NULL) {
            free(s->chars);
            s->chars = NULL;
        }
        s->size = 0;
    }
}

extern const char * scstr(const str * s) {
    assert(s != NULL);

    return s->chars == NULL ? s->lit : s->chars;
}

extern size_t slen_s(const str * s) {
    return !(s->lit == NULL && s->chars == NULL) ? s->size : 0;
}

extern const char * sfirst_s(const str * s) {
    assert(s != NULL);

    return s->lit == NULL ? s->chars : s->lit;
}

extern const char * slast_s(const str * s) {
    assert(s != NULL);

    if (s->size < 1) {
        return NULL;
    }

    return s->lit == NULL ? s->chars + s->size - 2 : s->lit + s->size - 2;
}

extern str * r_sadd_c(str * restrict s, const char * restrict chars) {
    return r_snadd_c(s, chars, r_str_max_size);
}

extern str * r_sadd_s(str * restrict s, const str * restrict other) {
    assert(s != NULL);
    assert(other != NULL);

    return r_snadd_c(s, scstr(other), other->size);
}

extern str * r_sadd_v(str * restrict s, str other) {
    assert(s != NULL);

    return r_snadd_c(s, scstr(&other), other.size);
}

extern str * r_snadd_c(str * restrict s, const char * restrict chars, const size_t max_size) {
    assert(s != NULL);
    assert(chars != NULL);

    const size_t chars_len = strnlen(chars, max_size);
    const size_t buf_size = s->size + chars_len;

    char * buf = r_recalloc_t(s->chars, buf_size, char);

    const size_t dst_idx = s->size > 0 ? s->size - 1 : 0;
    strncpy(buf + dst_idx, chars, chars_len + 1);
    buf[buf_size - 1] = NULLTERM;

    s->size = buf_size;
    s->chars = buf;
    return s;
}
