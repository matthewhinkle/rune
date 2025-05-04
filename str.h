//
// Created by Matt on 4/27/2025.
//

#ifndef RUNE_STR_H
#define RUNE_STR_H

#include <stddef.h>

#include "std.h"

typedef struct str {
    const char * lit;
    char * chars;
    size_t size;
} str;

#define cstr(chars)                                                                                \
    _Generic((chars), char *: strc, const char *: strc, str *: strs, const str *: strs)(chars)

#define cnstr(chars, size)                                                                         \
    _Generic((chars), char *: snlit, const char *: strcn, str *: strsn, const str *: strsn)(       \
        chars, size                                                                                \
    )

#define slen(chars)                                                                                \
    _Generic((chars), char *: slen_c, const char *: slen_c, str *: slen_s, const str *: slen_s)(   \
        chars                                                                                      \
    )

#define snlen(chars, size)                                                                           \
    _Generic((chars), char *: snlen_c, const char *: snlen_c, str *: snlen_s, const str *: snlen_s)( \
        chars, size                                                                                  \
    )

#define sfirst(chars)                                                                                    \
    _Generic((chars), char *: sfirst_c, const char *: sfirst_c, str *: sfirst_s, const str *: sfirst_s)( \
        chars                                                                                            \
    )

#define slast(chars)                                                                                 \
    _Generic((chars), char *: slast_c, const char *: slast_c, str *: slast_s, const str *: slast_s)( \
        chars                                                                                        \
    )

#define snlast(chars)                                                                                    \
    _Generic((chars), char *: snlast_c, const char *: snlast_c, str *: snlast_s, const str *: snlast_s)( \
        chars                                                                                            \
    )

#define sadd(s, chars)                                                                                                                                     \
    _Generic((chars), char *: r_sadd_c, const char *: r_sadd_c, str *: r_sadd_s, const str *: r_sadd_s, struct str: r_sadd_v, const struct str: r_sadd_v)( \
        s, chars                                                                                                                                           \
    )

extern str slit(const char * chars);
extern str strc(const char * chars);
extern str strs(const str * s);

extern str snlit(const char * chars, size_t size);
extern str strcn(const char * chars, size_t size);
extern str strsn(const str * s);

extern void sfree(str * s);

extern const char * scstr(const str * s);

extern size_t slen_c(const char * chars);
extern size_t slen_s(const str * s);

extern size_t snlen_c(const char * chars, size_t max_size);
extern size_t snlen_s(const str * s, size_t max_size);

extern const char * sfirst_c(const char * chars);
extern const char * sfirst_s(const str * s);

extern const char * slast_c(const char * chars);
extern const char * slast_s(const str * s);

extern const char * snlast_c(const char * chars);
extern const char * snlast_s(const str * s);

extern str * ssub_s(const char * restrict s, size_t start, size_t end);

extern str * r_sadd_c(str * restrict s, const char * restrict chars);
extern str * r_sadd_s(str * restrict s, const str * restrict other);
extern str * r_sadd_v(str * restrict s, str other);

extern str * r_snadd_c(str * restrict s, const char * restrict chars, size_t size);
extern str * r_snadd_s(str * restrict s, const str * restrict other);
extern str * r_snadd_v(str * restrict s, str other);

extern str * r_srem_c(str * restrict s, const char * restrict chars);
extern str * r_srem_s(str * restrict s, const str * restrict other);
extern str * r_srem_v(str * restrict s, str other);

extern str * r_snrem_c(str * restrict s, const char * restrict chars, size_t size);
extern str * r_snrem_s(str * restrict s, const str * restrict other);
extern str * r_snrem_v(str * restrict s, str other);

extern ssize_t r_sidx_c(const str * restrict s, const char * restrict chars, size_t start);
extern ssize_t r_sidx_s(const str * restrict s, const str * restrict other, size_t start);
extern ssize_t r_sidx_v(const str * restrict s, str other, size_t start);

extern ssize_t r_sridx_c(const str * restrict s, const char * restrict chars, size_t rstart);
extern ssize_t r_sridx_s(const str * restrict s, const str * restrict other, size_t rstart);
extern ssize_t r_sridx_v(const str * restrict s, str other, size_t rstart);

#endif // RUNE_STR_H
