/*
 * C string wrapper.
 *
 * All `char *` and `const char *` data returned by this API are compatible with standard C string
 * usage (ex: the <string.h> functions) unless otherwise stated.
 */

#ifndef RUNE_STR_H
#define RUNE_STR_H

#include "std.h"

/**
 * Optionally override the max string size.
 */
#ifdef RUNE_CFG__STR_MAX_LEN
static const size_t RUNE_STR_MAX_LEN = RUNE_CFG__STR_MAX_LEN;
#else  // default to 4 KiB
static const size_t RUNE_STR_MAX_LEN = 4 * 1024; // 4 KiB
#endif // RUNE_CFG__STR_MAX_LEN

/**
 * Optionally override the max stack-allocated string size.
 */
#ifdef RUNE_CFG__STR_STACK_MAX_LEN
static const size_t RUNE_STR_STACK_MAX_LEN = RUNE_CFG__STR_STACK_MAX_LEN;
#else  // default to 8 KiB
static const size_t RUNE_STR_STACK_MAX_LEN = 8 * 1024; // 8 KiB
#endif // RUNE_CFG__STR_STACK_MAX_LEN

/**
 * Optionally override the max tokens supported by `str_split()`.
 */
#ifdef RUNE_CFG__STR_MAX_TOK
static const size_t RUNE_STR_MAX_TOK = RUNE_CFG__STR_MAX_TOK;
#else  // default to 1024
static const size_t RUNE_STR_MAX_TOK = 1024;
#endif // RUNE_CFG__STR_MAX_TOK

/**
 * Optionally override the max size of the string used by `str_join_arr()`.
 */
#ifdef RUNE_CFG__STR_MAX_ARR
static const size_t RUNE_STR_MAX_ARR = RUNE_CFG__STR_MAX_ARR;
#else  // default to 1024
static const size_t RUNE_STR_MAX_ARR = 1024;
#endif // RUNE_CFG__STR_MAX_ARR

/**
 * Optionally override the max size of the string used by `str_join_varg()`.
 */
#ifdef RUNE_CFG__STR_MAX_VARG
static const size_t RUNE_STR_MAX_VARG = RUNE_CFG__STR_MAX_VARG;
#else  // default to 1024
static const size_t RUNE_STR_MAX_VARG = 1024;
#endif // RUNE_CFG__STR_MAX_VARG

/**
 * Initialize a string from an arbitrary char array. The max size of the string may be optionally
 * provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param data the string data.
 * @param max_len [optional] the max size of the string to initialize.
 * @return a copy of the string compatible with the `str_*` functions; must be freed with `str_free()`.
 */
#define str(data, ...) RUNE(str)((data)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
[[nodiscard]]
extern char * RUNE(str)(const char * data, size_t max_len, ...);

/**
 * Format a string similar to `printf` with multiple format arguments. The max size of the string
 * may be optionally provided as the first argument, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * formatted = strf("Hello, %s! %d", "World", 42);
 * ...
 * str_free(formatted);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * formatted = strf(100, "Hello, %s! %d", "World", 42);
 * ...
 * str_free(formatted);
 * ```
 *
 * @param arg the first argument, which can be a format string or a size_t for max length.
 * @param ... additional arguments for formatting.
 * @return a new formatted string; must be freed with `str_free()`.
 */
#define strf(arg, ...)                                                                             \
    _Generic(                                                                                      \
        (arg),                                                                                     \
        GENERIC_INTS(RUNE(strf)((size_t)(arg)VA_ARGS(__VA_ARGS__))),                               \
        default: RUNE(strf)((RUNE_STR_MAX_LEN), (const char *)(arg)VA_ARGS(__VA_ARGS__))           \
    )
extern char * RUNE(strf)(size_t max_len, const char * fmt, ...);

/**
 * Free the string data returned by `str()`. Additionally, supports freeing any C string. It is safe
 * to call this function with `nullptr`. Always returns `nullptr` to allow for chaining.
 *
 * @param data the string data to free.
 * @return `nullptr`.
 */
extern void * str_free(char * data);

// -------
// getters
// -------

/**
 * Check if the string is a valid `str` string. The max size of the string to check may be
 * optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param data a C string.
 * @param max_len [optional] the max size of the string to check.
 * @return `true` if the string is a valid `str` string, `false` otherwise.
 */
#define str_is(data, ...) RUNE(str_is)(data VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern bool RUNE(str_is)(const char * data, size_t max_len, ...);

/**
 * Get the length of the string. The max size of the string to check may be optionally provided,
 * otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param str a C string.
 * @param max_len [optional] the max chars of the string to check.
 * @return the length of the string, excluding the null terminator.
 */
#define str_len(str, ...) RUNE(str_len)((str)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern size_t RUNE(str_len)(const char * str, size_t max_len, ...);

/**
 * Get the total size of the string and its metadata.
 *
 * @param str a C string.
 * @param max_len [optional] the max chars of the string to check.
 * @return the total size of the string, including the null terminator and metadata.
 */
#define str_size(str, ...) RUNE(str_size)(str VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern size_t RUNE(str_size)(const char * str, size_t max_len, ...);

// ---------------------------
// immutable string operations
// ---------------------------

/**
 * Concatenate multiple strings into a single string. The max size of the string to create may be
 * optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * cat = str_cat("foo", "bar", "baz");
 * ...
 * str_free(cat);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * cat = str_cat(8, "foo", "bar", "baz");
 * ...
 * str_free(cat);
 * ```
 *
 * @param max_len [optional] the max size of the string to create.
 * @param first the first string to concatenate.
 * @param ... additional strings to concatenate, ending with `nullptr`.
 * @return a new string containing the concatenated strings; must be freed with `str_free()`.
 */
#define str_cat(first, ...)                                                                        \
    _Generic(                                                                                      \
        (first),                                                                                   \
        GENERIC_INTS(RUNE(str_cat)((size_t)(first)VA_ARGS(__VA_ARGS__), nullptr)),                 \
        default: RUNE(str_cat)((RUNE_STR_MAX_LEN), (first)VA_ARGS(__VA_ARGS__), nullptr)           \
    )
[[nodiscard]]
extern char * RUNE(str_cat)(size_t max_len, const char * first, ...);

/**
 * Compare two strings for equality. The max size of the strings to compare may be optionally
 * provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param a the first string to compare.
 * @param b the second string to compare.
 * @param max_len [optional] the max size of the strings to compare.
 * @return greater than 0 if `a` is greater than `b`, less than 0 if `a` is less than `b`, or 0 if
 * they are equal.
 */
#define str_cmp(a, b, ...) RUNE(str_cmp)((a), (b)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern int RUNE(str_cmp)(const char * a, const char * b, size_t len, ...);

/**
 * Check if two strings are equal. The max size of the strings to compare may be optionally
 * provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param a the first string to compare.
 * @param b the second string to compare.
 * @param max_len [optional] the max size of the strings to compare.
 * @return `true` if the strings are equal, `false` otherwise.
 */
#define str_eq(a, b, ...) RUNE(str_eq)((a), (b)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern bool RUNE(str_eq)(const char * a, const char * b, size_t len, ...);

/**
 * Find the first occurrence of a substring in a string. The max size of the strings to search may
 * be optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param str the string to search in.
 * @param key the substring to search for.
 * @param max_len [optional] the max size of the strings to search.
 * @return a pointer to the first occurrence of `key` in `str`, or `nullptr` if not found.
 */
#define str_find(a, b, ...) RUNE(str_find)((a), (b)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern const char * RUNE(str_find)(const char * str, const char * key, size_t len, ...);

/**
 * Find the last occurrence of a substring in a string. The max size of the strings to search may
 * be optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param str the string to search in.
 * @param key the substring to search for.
 * @param max_len [optional] the max size of the strings to search.
 * @return a pointer to the last occurrence of `key` in `str`, or `nullptr` if not found.
 */
#define str_rfind(a, b, ...) RUNE(str_rfind)((a), (b)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern const char * RUNE(str_rfind)(const char * str, const char * key, size_t len, ...);

/**
 * Compute the hash of a string. The max size of the string to hash may be optionally provided,
 * otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param str the string to hash.
 * @param max_len [optional] the max size of the string to hash.
 * @return the hash of the string.
 */
#define str_hash(str, ...) RUNE(str_hash)((str)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern unsigned long long RUNE(str_hash)(const char * str, size_t len, ...);

/**
 * Join an array of strings into a single string with a delimiter. The max size of the string to
 * create may be optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * const char * strs[] = {"hello", "world", "this", "is", "a", "test", nullptr};
 * char * result = str_join(" !:! ", strs);
 * ...
 * str_free(result);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * const char * strs[] = {"hello", "world", "this", "is", "a", "test", nullptr};
 * char * result = str_join(8, " !:! ", strs);
 * ...
 * str_free(result);
 * ```
 *
 * @param delim the delimiter to use between strings.
 * @param arg the first string or array of strings to join.
 * @param ... additional strings to join, ending with `nullptr`.
 * @return a new string containing the joined strings; must be freed with `str_free()`.
 */
#define str_join(delim, arg, ...)                                                                         \
    _Generic(                                                                                             \
        &*(arg),                                                                                          \
        char *: RUNE(str_join_varg)((delim), RUNE_STR_MAX_LEN, (arg)VA_ARGS(__VA_ARGS__), nullptr),       \
        const char *: RUNE(str_join_varg)((delim), RUNE_STR_MAX_LEN, (arg)VA_ARGS(__VA_ARGS__), nullptr), \
        const char **: RUNE(str_join_arr)(                                                                \
            (delim),                                                                                      \
            (const char **)(arg),                                                                         \
            (RUNE_STR_MAX_ARR),                                                                           \
            RUNE_STR_MAX_VARG,                                                                            \
            RUNE_STR_MAX_LEN                                                                              \
        ),                                                                                                \
        default: RUNE(str_join_varg)((delim), (size_t)(arg)VA_ARGS(__VA_ARGS__), nullptr)                 \
    )

/**
 * Join an array of strings into a single string with a delimiter. The max size of the string to
 * create may be optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param delim the delimiter to use between strings.
 * @param arr the array of strings to join.
 * @param max_join [optional] the max number of strings to join.
 * @param max_len [optional] the max size of the string to create.
 * @param ... unused, for compatibility with `str_join_varg()`.
 * @return a new string containing the joined strings; must be freed with `str_free()`.
 */
[[nodiscard]]
extern char *
    RUNE(str_join_arr)(const char * delim, const char ** arr, size_t max_join, size_t max_len, ...);

/**
 * Join multiple strings into a single string with a delimiter. The max size of the string to create
 * may be optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * @param delim the delimiter to use between strings.
 * @param max_len [optional] the max size of the string to create.
 * @param ... the strings to join, ending with `nullptr`.
 * @return a new string containing the joined strings; must be freed with `str_free()`.
 */
[[nodiscard]]
extern char * RUNE(str_join_varg)(const char * delim, size_t max_len, ...);

/**
 * Repeat a string a specified number of times. The max size of the string to create may be
 * optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * repeated = str_repeat("hello", 3);
 * ...
 * str_free(repeated);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * repeated = str_repeat("hello", 3, 20);
 * ...
 * str_free(repeated);
 * ```
 *
 * @param str the string to repeat.
 * @param n the number of times to repeat the string.
 * @param max_len [optional] the max size of the string to create.
 * @return a new string with the repeated content; must be freed with `str_free()`.
 */
#define str_repeat(str, n, ...) RUNE(str_repeat)((str), (n)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
[[nodiscard]]
extern char * RUNE(str_repeat)(const char * str, size_t n, size_t max_len, ...);

/**
 * Replace all occurrences of a substring in a string with another substring. The max size of the
 * string to create may be optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * result = str_replace("hello world", "world", "everyone");
 * ...
 * str_free(result);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * result = str_replace("hello world", "world", "everyone", 20);
 * ...
 * str_free(result);
 * ```
 *
 * @param str the string to modify.
 * @param target the substring to replace.
 * @param new_val the substring to replace with.
 * @param max_len [optional] the max size of the string to create.
 * @return a new string with the replacements; must be freed with `str_free()`.
 */
#define str_replace(str, target, ...)                                                              \
    RUNE(str_replace)((str), (target)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
[[nodiscard]]
extern char *
    RUNE(str_replace)(const char * str, const char * target, const char * new_val, size_t max_len, ...);

/**
 * Extract a substring from a string. The start index and number of characters to extract may be
 * optionally provided; if not, the entire string is returned. Negative values for `start` or `n`
 * are interpreted as offsets from the end of the string or as "to the end", respectively.
 *
 * Example extracting a substring:
 * ```c
 * char * sub = str_sub("hello world", 6, 5); // "world"
 * ...
 * str_free(sub);
 * ```
 *
 * Example extracting from a start index to the end:
 * ```c
 * char * sub = str_sub("hello world", 6); // "world"
 * ...
 * str_free(sub);
 * ```
 *
 * Example extracting the whole string:
 * ```c
 * char * sub = str_sub("hello world");
 * ...
 * str_free(sub);
 * ```
 *
 * @param str the source string.
 * @param start [optional] the start index (0-based). Negative values count from the end.
 * @param n [optional] the number of characters to extract. Negative means "to the end".
 * @return a new string containing the substring; must be freed with `str_free()`.
 */
#define str_sub(str, ...) RUNE(str_sub)((str)VA_ARGS(__VA_ARGS__), -1, -1)
[[nodiscard]]
extern char * RUNE(str_sub)(const char * str, ssize_t start, ssize_t n, ...);

/**
 * Split a string into an array of strings using a delimiter. The max number of tokens and the max
 * length of each token may be optionally provided, otherwise, `RUNE_STR_MAX_TOK` and
 * `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_tokens` and `max_len`:
 * ```c
 * char ** tokens = str_split("0, 1, 2, 3, 4, 5", ", ");
 * ...
 * str_free(tokens);
 * ```
 *
 * Example using custom `max_tokens` and `max_len`:
 * ```c
 * char ** tokens = str_split("0, 1, 2, 3, 4, 5", ", ", 10, 5);
 * ...
 * str_free(tokens);
 * ```
 *
 * @param str the string to split.
 * @param delim the delimiter to use for splitting.
 * @param max_tokens [optional] the maximum number of tokens to return.
 * @param max_len [optional] the maximum length of each token.
 * @return an array of strings; must be freed with `str_free()`.
 */
#define str_split(str, delim, ...)                                                                 \
    RUNE(str_split)((str), (delim)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_TOK, RUNE_STR_MAX_LEN)
[[nodiscard]]
extern char **
    RUNE(str_split)(const char * str, const char * delim, size_t max_tokens, size_t max_len, ...);

// --------------------------
// mutating string operations
// --------------------------

/**
 * Capitalize the first non-blank character in-place. The max size of the string to convert may be
 * optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * s = str("Hello, World");
 * char * capped = str_capitalize(s);
 * ...
 * // `str_capitalize()` modifies the string in-place, only `s` or `capped` needs to be freed.
 * str_free(s);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * s = str("Hello, World");
 * char * capped = str_capitalize(s, 20);
 * ...
 * // `str_lower()` modifies the string in-place, only `s` or `capped` needs to be freed.
 * str_free(s);
 * ```
 *
 * @param str the string to convert to lowercase.
 * @param max_len [optional] the max size of the string to convert.
 * @return modified `str`.
 */
#define str_capitalize(str, ...) RUNE(str_capitalize)((str), __VA_ARGS__)
extern char * RUNE(str_capitalize)(char * str, size_t max_len, ...);

/**
 * Convert a string to lowercase in-place. The max size of the string to convert may be optionally
 * provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * `str` is mutated in-place, this function does not allocate additional memory for the result.
 *
 * Example using default `max_len`:
 * ```c
 * char * s = str("Hello World");
 * char * lower = str_lower(s);
 * ...
 * // `str_lower()` modifies the string in-place, only `s` or `lower` needs to be freed.
 * str_free(s);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * s = str("Hello World");
 * char * lower = str_lower("Hello World", 20);
 * ...
 * // `str_lower()` modifies the string in-place, only `s` or `lower` needs to be freed.
 * str_free(s);
 * ```
 *
 * @param str the string to convert to lowercase.
 * @param max_len [optional] the max size of the string to convert.
 * @return `str` modified with all characters converted to lowercase.
 */
#define str_lower(str, ...) RUNE(str_lower)((str)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern char * RUNE(str_lower)(char * str, size_t max_len, ...);

/**
 * Convert a string to uppercase in-place. The max size of the string to convert may be optionally
 * provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * s = str("Hello World");
 * char * upper = str_upper(s);
 * ...
 * // `str_upper()` modifies the string in-place, only `s` or `upper` needs to be freed.
 * str_free(s);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * s = str("Hello World");
 * char * upper = str_upper(s, 20);
 * ...
 * // `str_upper()` modifies the string in-place, only `s` or `upper` needs to be freed.
 * str_free(upper);
 * ```
 *
 * @param str the string to convert to uppercase.
 * @param max_len [optional] the max size of the string to convert.
 * @return a new string with all characters converted to uppercase; must be freed with `str_free()`.
 */
#define str_upper(str, ...) RUNE(str_upper)((str)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN)
extern char * RUNE(str_upper)(char * str, size_t max_len, ...);

/**
 * Reverse a string in place. The max size of the string to reverse may be optionally provided,
 * otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * reversed = str_reverse("Hello World");
 * ...
 * str_free(reversed);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * reversed = str_reverse("Hello World", 20);
 * ...
 * str_free(reversed);
 * ```
 *
 * @param str the string to reverse.
 * @param max_len [optional] the max size of the string to reverse.
 * @return a new string with the characters reversed; must be freed with `str_free()`.
 */
#define str_reverse(str, ...) RUNE(str_reverse)((str), __VA_ARGS__, RUNE_STR_MAX_LEN)
extern char * RUNE(str_reverse)(char * str, size_t max_len, ...);

/**
 * Pad a string with a character to a specified length. The max size of the string to pad may be
 * optionally provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * padded = str_pad("Hello", '*', true, 10);
 * ...
 * str_free(padded);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * padded = str_pad("Hello", '*', true, 20);
 * ...
 * str_free(padded);
 * ```
 *
 * @param str the string to pad.
 * @param pad_char the character to use for padding.
 * @param left_pad if `true`, pads on the left; if `false`, pads on the right.
 * @param max_len [optional] the max size of the string to pad.
 * @return a new string with padding applied; must be freed with `str_free()`.
 */
#define str_pad(str, len, ...)                                                                     \
    RUNE(str_pad)((str), (len)VA_ARGS(__VA_ARGS__), ' ', true, RUNE_STR_MAX_LEN)
extern char * RUNE(str_pad)(char * str, size_t len, char pad_char, bool left, size_t max_len, ...);

/**
 * Trim blankspace from both ends of a string. The max size of the string to trim may be optionally
 * provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * trimmed = str_trim("  hello world  ");
 * ...
 * str_free(trimmed);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * trimmed = str_trim("  hello world  ", 20);
 * ...
 * str_free(trimmed);
 * ```
 *
 * @param str the string to trim.
 * @param max_len [optional] the max size of the string to trim.
 * @return a new string with blankspace trimmed; must be freed with `str_free()`.
 */
#define str_trim(str, ...) RUNE(str_trim)((str)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN, false)
[[nodiscard]]
extern char * RUNE(str_trim)(char * str, size_t max_len, bool realloc, ...);

/**
 * Left trim blankspace from a string. The max size of the string to trim may be optionally
 * provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * trimmed = str_ltrim("  hello world  ");
 * ...
 * str_free(trimmed);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * trimmed = str_ltrim("  hello world  ", 20);
 * ...
 * str_free(trimmed);
 * ```
 *
 * @param str the string to left trim.
 * @param max_len [optional] the max size of the string to trim.
 * @return a new string with blankspace left trimmed; must be freed with `str_free()`.
 */
#define str_ltrim(str, ...) RUNE(str_ltrim)((str)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN, false)
[[nodiscard]]
extern char * RUNE(str_ltrim)(char * str, size_t max_len, bool realloc, ...);

/**
 * Right trim blankspace from a string. The max size of the string to trim may be optionally
 * provided, otherwise, `RUNE_STR_MAX_LEN` will be used.
 *
 * Example using default `max_len`:
 * ```c
 * char * trimmed = str_rtrim("  hello world  ");
 * ...
 * str_free(trimmed);
 * ```
 *
 * Example using custom `max_len`:
 * ```c
 * char * trimmed = str_rtrim("  hello world  ", 20);
 * ...
 * str_free(trimmed);
 * ```
 *
 * @param str the string to right trim.
 * @param max_len [optional] the max size of the string to trim.
 * @return a new string with blankspace right trimmed; must be freed with `str_free()`.
 */
#define str_rtrim(str, ...) RUNE(str_rtrim)((str)VA_ARGS(__VA_ARGS__), RUNE_STR_MAX_LEN, false)
[[nodiscard]]
extern char * RUNE(str_rtrim)(char * str, size_t max_len, bool realloc, ...);

#endif // RUNE_STR_H
