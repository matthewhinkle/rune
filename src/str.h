/*
 * C string wrapper.
 *
 * All `const char *` data returned by this API are compatible with standard C string usage (ex: the
 * <string.h> functions) unless otherwise stated.
 */

#ifndef RUNE_STR_H
#define RUNE_STR_H

#include "std.h"

/**
 * Optionally override the max string size.
 */
#ifdef RUNE_CFG__STR_MAX_SIZE
static const size_t RUNE_STR_MAX_SIZE = RUNE_CFG__STR_MAX_SIZE;
#else
static const size_t RUNE_STR_MAX_SIZE = 4 * 1024; // 4 KiB
#endif

/**
 * Optionally override the max strings supported by `str_cat()`.
 */
#ifdef RUNE_CFG__STR_MAX_CAT
static const size_t RUNE_STR_MAX_CAT = RUNE_CFG__STR_MAX_CAT;
#else
static const size_t RUNE_STR_MAX_CAT = 1024;
#endif

/**
 * Initialize a string from an arbitrary char array. The max size of the string may be optionally
 * provided, otherwise, `RUNE_STR_MAX_SIZE` will be used.
 *
 * @param data the string data.
 * @param ... [optional] max size.
 * @return a copy of the string compatible with the `str_*` functions; must be freed with `str_free()`.
 */
#define str(data, ...) RUNE(str)((data)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_SIZE)
extern const char * RUNE(str)(const char * data, ...);

/**
 * Check if the string is a valid `str` string. The max size of the string to check may be
 * optionally provided, otherwise, `RUNE_STR_MAX_SIZE` will be used.
 *
 * @param data a C string.
 * @param ... [optional] max size.
 * @return `true` if the string is a valid `str` string, `false` otherwise.
 */
#define str_is(data, ...) RUNE(str_is)(data VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_SIZE)
extern bool RUNE(str_is)(const char * data, ...);

/**
 * Free the string data returned by `str()`. Additionally, supports freeing any C string. It is safe
 * to call this function with `nullptr`. Always returns `nullptr` to allow for chaining.
 *
 * @param data the string data to free.
 * @return `nullptr`.
 */
extern void * str_free(const char * data);

#define str_len(data, ...) RUNE(str_len)(data VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_SIZE)
extern size_t RUNE(str_len)(const char * data, ...);

#define str_size(data, ...) RUNE(str_size)(data VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_SIZE)
extern size_t RUNE(str_size)(const char * data, ...);

#define str_cat(...) RUNE(str_cat)(__VA_ARGS__, nullptr, RUNE_STR_MAX_SIZE)
extern const char * RUNE(str_cat)(const char * first, ...);

#define str_cmp(a, b, ...) RUNE(str_cmp)((a), (b)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_SIZE)
extern int RUNE(str_cmp)(const char * a, const char * b, ...);

#define str_sub(data, ...) RUNE(str_sub)((data)VA_ARGS(__VA_ARGS__), -1, -1)
extern const char * RUNE(str_sub)(const char * data, ssize_t start, ssize_t n, ...);

#endif // RUNE_STR_H
