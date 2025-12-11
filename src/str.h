/**
 * Core string module - Unicode-safe byte string operations.
 *
 * Provides:
 *   - Managed string creation with automatic memory handling
 *   - Safe UTF-8 operations (all functions work with byte sequences, not character counts)
 *   - Efficient hashing and comparison with optional caching for managed strings
 *   - String searching, transformation, and manipulation
 *   - Configurable limits and optional parameters for each operation
 *
 * Quick Reference:
 *
 *   Creation & Destruction
 *   ----------------------
 *   str(data, ...)           Create managed string from C string
 *   strf(fmt, ...)           Create formatted string (printf-style)
 *   stmem_free(s)              Free a managed string (nullptr-safe)
 *   stmem_free_arr(arr)        Free nullptr-terminated array of strings
 *
 *   Inspection
 *   ----------
 *   str_is(data, ...)        Check if pointer is managed string
 *   str_len(data, ...)       Get string byte length
 *   str_size(data, ...)      Get allocation size (metadata included)
 *   str_hash(data, ...)      Get FNV-1a hash (cached for managed strings)
 *
 *   Comparison
 *   ----------
 *   str_cmp(a, b, ...)       Lexicographic comparison (<0, 0, >0)
 *   str_eq(a, b, ...)        Check equality (uses hash for fast rejection)
 *
 *   Search
 *   ------
 *   str_find(data, target)   Find first occurrence (returns pointer or nullptr)
 *   str_rfind(data, target)  Find last occurrence (returns pointer or nullptr)
 *
 *   Transformation (returns new strings - free with stmem_free)
 *   -------------------------------------------------------
 *   str_cat(first, ...)      Concatenate strings (nullptr-terminated args)
 *   str_join(delim, arr)     Join nullptr-terminated array with delimiter
 *   str_repeat(s, n)         Repeat string n times
 *   str_replace(
 *       s,
 *       target,
 *       replacement
 *   )                        Replace all occurrences
 *   str_split(s, delim)      Split by delimiter (returns nullptr-terminated array)
 *
 * Example:
 *   char * greeting = str("Hello");
 *   if (err_has()) { err_print(stderr); return; }
 *
 *   char * greeting2 = strf("Hello, %s!", "World");
 *   char * combined = str_cat(greeting, " ", greeting2, nullptr);
 *
 *   stmem_free(greeting);
 *   stmem_free(greeting2);
 *   stmem_free(combined);
 *
 * Requires C11 for _Generic support.
 */

#ifndef RUNE_CORE_STR_H
#define RUNE_CORE_STR_H

#include <stdint.h>

#include "r.h"

// =================================================================================================
// Configuration
// =================================================================================================

// String operation limits
// -------------------------------------------------------------------------------------------------

#ifdef RCFG__STR_MAX_LEN
static constexpr size_t R_STR_MAX_LEN = RCFG__STR_MAX_LEN;
#else  // RCFG__STR_MAX_LEN
static constexpr size_t R_STR_MAX_LEN = 4096;
#endif // RCFG__STR_MAX_LEN

#ifdef RCFG__STR_STACK_MAX
static constexpr size_t R_STR_STACK_MAX = RCFG__STR_STACK_MAX;
#else  // RCFG__STR_STACK_MAX
static constexpr size_t R_STR_STACK_MAX = 8 * 1024;
#endif // RCFG__STR_STACK_MAX

#ifdef RCFG__STR_MAX_VARG
static constexpr size_t R_STR_MAX_VARG = RCFG__STR_MAX_VARG;
#else  // RCFG__STR_MAX_VARG
static constexpr size_t R_STR_MAX_VARG = 64;
#endif // RCFG__STR_MAX_VARG

// String options structure
// -------------------------------------------------------------------------------------------------

typedef struct {
    const size_t max_len;
    const size_t max_tok;
} str_opt;

static const str_opt R_STR_OPTS_DEFAULT = {
    .max_len = R_STR_MAX_LEN,
};

// =================================================================================================
// Creation & Destruction
// =================================================================================================

// Creation
// -------------------------------------------------------------------------------------------------

#define str(data, ...) R_(str)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
[[nodiscard]]
extern char * R_(str)(const char * restrict data, const str_opt * opt);

#define strf(first, ...)                                                                                               \
    _Generic(                                                                                                          \
        (first),                                                                                                       \
        const str_opt *: R_(strf)((first)R_VA_ARGS(__VA_ARGS__)),                                                      \
        str_opt *: R_(strf)((first)R_VA_ARGS(__VA_ARGS__)),                                                            \
        default: R_(strf)(&R_STR_OPTS_DEFAULT, (first)R_VA_ARGS(__VA_ARGS__))                                          \
    )
[[nodiscard]]
extern char * R_(strf)(const str_opt * opt, const char * fmt, ...);

// Destruction
// -------------------------------------------------------------------------------------------------

extern void str_free(const char * s);
extern void str_free_arr(char ** arr);

// =================================================================================================
// Inspection
// =================================================================================================

#define str_is(data, ...) R_(str_is)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
extern bool R_(str_is)(const char * s, const str_opt * opt);

#define str_len(data, ...) R_(str_len)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
extern size_t R_(str_len)(const char * s, const str_opt * opt);

#define str_size(data, ...) R_(str_size)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
extern size_t R_(str_size)(const char * s, const str_opt * opt);

#define str_hash(data, ...) R_(str_hash)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
extern uint64_t R_(str_hash)(const char * s, const str_opt * opt);

// =================================================================================================
// Comparison
// =================================================================================================

#define str_cmp(data, ...) R_(str_cmp)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
extern int R_(str_cmp)(const char * a, const char * b, const str_opt * opt);

#define str_eq(data, ...) R_(str_eq)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
extern bool R_(str_eq)(const char * a, const char * b, const str_opt * opt);

// =================================================================================================
// Search
// =================================================================================================

#define str_find(data, target, ...) R_(str_find)((data), (target), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
extern const char * R_(str_find)(const char * data, const char * target, const str_opt * opt);

#define str_rfind(data, target, ...) R_(str_rfind)((data), (target), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
extern const char * R_(str_rfind)(const char * data, const char * target, const str_opt * opt);

// =================================================================================================
// Transformation
// =================================================================================================

#define str_cat(first, ...)                                                                                            \
    _Generic(                                                                                                          \
        (first),                                                                                                       \
        const str_opt *: R_(str_cat)((first)R_VA_ARGS(__VA_ARGS__)),                                                   \
        str_opt *: R_(str_cat)((first)R_VA_ARGS(__VA_ARGS__)),                                                         \
        default: R_(str_cat)(&R_STR_OPTS_DEFAULT, (first)R_VA_ARGS(__VA_ARGS__))                                       \
    )
[[nodiscard]]
extern char * R_(str_cat)(const str_opt * opt, const char * first, ...);

#define str_join(delim, arr, ...) R_(str_join)((delim), (arr), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
[[nodiscard]]
extern char * R_(str_join)(const char * delim, const char ** arr, const str_opt * opt);

#define str_repeat(s, n, ...) R_(str_repeat)((s), (n), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
[[nodiscard]]
extern char * R_(str_repeat)(const char * s, size_t n, const str_opt * opt);

#define str_replace(s, target, replacement, ...)                                                                       \
    R_(str_replace)((s), (target), (replacement), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
[[nodiscard]]
extern char * R_(str_replace)(const char * s, const char * target, const char * replacement, const str_opt * opt);

#define str_split(s, delim, ...) R_(str_split)((s), (delim), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
[[nodiscard]]
extern char ** R_(str_split)(const char * s, const char * delim, const str_opt * opt);

#endif // RUNE_CORE_STR_H
