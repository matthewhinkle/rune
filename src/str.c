/*
 * str module.
 */

#include "str.h"
#include "std.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// --------------
// string hashing
// --------------

/**
 * FNV-1a hash function implementation.
 * This is a non-cryptographic hash function suitable for strings.
 * It is fast and has a good distribution of hash values.
 *
 * @return the base hash value.
 */
static unsigned long long fnv1a_start() {
    return 0xCBF29CE484222325ULL; // FNV_OFFSET_BASIS
}

/**
 * FNV-1a hash function next step.
 * This function takes the current hash and the next byte of data,
 * and returns the updated hash value.
 *
 * @param hash the current hash value.
 * @param next_byte the next byte of data to hash.
 * @return the updated hash value.
 */
static unsigned long long fnv1a_next(const unsigned long long hash, const unsigned char next_byte) {
    return (hash ^ next_byte) * 0x100000001B3ULL; // FNV_PRIME
}

/**
 * FNV-1a hash function mix step.
 * This function takes two hash values and combines them into a single hash value.
 *
 * @param hash1 the first hash value.
 * @param hash2 the second hash value.
 * @return the combined hash value.
 */
static unsigned long long fnv1a_mix(const unsigned long long hash1, const unsigned long long hash2) {
    return (hash1 ^ hash2) * 0x100000001B3ULL; // FNV_PRIME
}

/**
 * FNV-1a hash function for a string.
 * This function takes a string and its maximum length,
 * and returns the hash value of the string.
 *
 * @param data the string to hash.
 * @param max_len the maximum length of the string.
 * @return the hash value of the string.
 */
static unsigned long long fnv1a_hash(const char * data, const size_t max_len) {
    unsigned long long hash = fnv1a_start();
    for (unsigned long long i = 0; i < max_len && data[i] != NULLTERM; ++i) {
        hash = fnv1a_next(hash, (unsigned char)data[i]);
    }
    return hash;
}

// -------------------
// internal string api
// -------------------

static const char SOH = 0x01; // "start of header": first byte of the "rune string"
static const char STX = 0x02; // "start of text": separator byte between header and C string data
static const char ETX = 0x03; // "end of text": final byte of the "rune string"

/**
 * Internal representation of a "rune string".
 * This structure is used to store strings with additional metadata,
 * such as length, capacity, and hash.
 *
 * The structure is designed to be allocated in a single block of memory,
 * with the string data immediately following the metadata.
 */
struct rstr {
    char soh;
    size_t len;
    size_t cap;
    unsigned long long hash;
    char stx;
    char * data;
    char etx;
};

/**
 * Check if the given data pointer points to a valid "rune string".
 * A valid "rune string" starts with SOH, followed by STX, and ends with ETX.
 *
 * @param data the pointer to the string data.
 * @return a pointer to the "rune string" structure if valid, or nullptr if invalid.
 */
[[nodiscard]]
static struct rstr * rstr(const char * data) {
    if (data == nullptr) {
        return nullptr;
    }

    struct rstr * r = (struct rstr *)(data - sizeof(struct rstr));
    if (r->soh == SOH && r->stx == STX && r->etx == ETX) {
        return r;
    }

    return nullptr;
}

/**
 * Create a new "rune string" with the given data, length, and hash.
 * The string is allocated in a single block of memory,
 * with the metadata followed by the string data.
 *
 * @param data the string data to store.
 * @param len the length of the string data.
 * @param hash the precomputed hash of the string data.
 * @return a pointer to the newly created "rune string".
 */
[[nodiscard]]
static struct rstr * rstr_new(const char * data, const size_t len, const unsigned long long hash) {
    const size_t cap = len + 1; // +1 for null terminator
    const size_t data_size = cap * sizeof(char);

    struct rstr * r = r_malloc(sizeof(struct rstr) + data_size);
    r->soh = SOH;
    r->len = len;
    r->cap = len;
    r->hash = hash;
    r->stx = STX;
    r->data = (char *)(r + 1);
    r->etx = ETX;
    if (data == nullptr) {
        memset(r->data, NULLTERM, data_size);
    } else {
        memcpy(r->data, data, len);
        r->data[len] = NULLTERM; // null-terminate the string
    }
    return r;
}

/**
 * Create a new "rune string" from the given data, up to a maximum length.
 * This function computes the length and hash of the string,
 * and returns a new "rune string" structure.
 *
 * @param data the string data to convert.
 * @param max_len the maximum length of the string data.
 * @return a pointer to the newly created "rune string", or nullptr if invalid.
 */
[[nodiscard]]
static struct rstr * rstr_of(const char * data, const size_t max_len) {
    // 1 - null check
    if (data == nullptr) {
        return nullptr;
    }

    // 2 - measure the length of the string while computing the hash
    size_t len = 0;
    unsigned long long hash = fnv1a_start();
    while (len < max_len && data[len] != NULLTERM) {
        hash = fnv1a_next(hash, (unsigned char)data[len]);
        len++;
    }

    // 3 - check if the data size is too large
    if (len > max_len) {
        return nullptr;
    }

    // 4 - allocate and initialize the string
    return rstr_new(data, len, hash);
}

/**
 * Resize a "rune string" to a new length and hash.
 * This function reallocates the memory if necessary,
 * and updates the metadata of the "rune string".
 *
 * @param r the "rune string" to resize.
 * @param new_len the new length of the string.
 * @param new_hash the new hash of the string.
 * @param realloc whether to always reallocate memory.
 * @return a pointer to the resized "rune string", or nullptr if invalid.
 */
[[nodiscard]]
static struct rstr *
rstr_resize(struct rstr * r, const size_t new_len, const size_t new_hash, const bool realloc) {
    if (r == nullptr) {
        return nullptr;
    }

    if (new_len == r->cap) {
        return r;
    }

    if (realloc || new_len > r->cap || new_len < r->cap >> 1) {
        // reallocate if growing the string or shrinking it too much.
        // `realloc` is used to force reallocation everytime.
        const size_t new_size = sizeof(struct rstr) + (new_len + 1) * sizeof(char);
        r = r_realloc(r, new_size);
        r->cap = new_len;
    }

    r->len = new_len;
    r->hash = new_hash;
    r->data = (char *)(r + 1);
    r->data[new_len] = NULLTERM;

    return r;
}

/**
 * Compute the hash of a string, either from a "rune string" or a regular C string.
 * If the input is a "rune string", it uses the precomputed hash; otherwise,
 * it computes the hash using the FNV-1a algorithm.
 *
 * @param str the string to hash.
 * @param max_len the maximum length of the string.
 * @return the hash value of the string.
 */
static unsigned long long rstr_hash(const char * str, const size_t max_len) {
    if (str == nullptr) {
        return 0;
    }
    const struct rstr * r = rstr(str);
    return r == nullptr ? fnv1a_hash(str, max_len) : r->hash;
}

/**
 * Compute the KMP (Knuth-Morris-Pratt) longest prefix-suffix (LPS) array for a pattern.
 *
 * @param pat the pattern to compute the LPS for.
 * @param max_len the maximum length of the pattern.
 * @param lps output array to store the LPS values.
 */
static void rstr_kmp_lps(const char * pat, const size_t max_len, int * lps) {
    int len = 0;
    lps[0] = 0;
    size_t i = 1;
    while (i < max_len) {
        if (pat[i] == pat[len]) {
            lps[i++] = ++len;
        } else if (len) {
            len = lps[len - 1];
        } else {
            lps[i++] = 0;
        }
    }
}

/**
 * Get the length of a "rune string" or a regular C string.
 * If the input is a "rune string", it uses the precomputed length; otherwise,
 * it computes the length using `strnlen`.
 *
 * @param str the string to get the length of.
 * @param max_len the maximum length to consider.
 * @return the length of the string, excluding the null terminator.
 */
static size_t rstr_len(const char * str, const size_t max_len) {
    const struct rstr * r = rstr(str);
    return r == nullptr ? strnlen(str, max_len) : r->len;
}

/**
 * Find a substring in a string using the KMP algorithm.
 * This function can find the first or last occurrence of the substring,
 * depending on the `rfind` parameter.
 *
 * @param str the string to search in.
 * @param key the substring to search for.
 * @param len the length of the string to search in.
 * @param key_len the length of the substring to search for.
 * @param rfind whether to find the last occurrence instead of the first.
 * @param lps pre-allocated array for the KMP longest prefix-suffix (LPS) values.
 * @return a pointer to the first or last occurrence of `key` in `str`, or nullptr if not found.
 */
static const char * rstr_find(
    const char * str,
    const char * key,
    const size_t len,
    const size_t key_len,
    const bool rfind,
    int * const restrict lps
) {
    rstr_kmp_lps(key, key_len, lps);

    ssize_t i = 0;
    ssize_t j = 0;
    ssize_t last = -1;
    while (i < len) {
        if (str[i] == key[j]) {
            i++;
            j++;
        }
        if (j == key_len) {
            if (rfind) {
                last = i - j;
                j = lps[j - 1];
            } else {
                return str + (i - j);
            }
        }
        if (i < len && str[i] != key[j]) {
            if (j == 0) {
                i++;
            } else {
                j = lps[j - 1];
            }
        }
    }
    if (rfind && last >= 0) {
        return str + last;
    }
    return nullptr;
}

/**
 * Replace all occurrences of a substring in a string with another substring.
 * This function uses the KMP algorithm to find occurrences and replaces them
 * with the specified replacement string.
 *
 * @param data the original string data.
 * @param find the substring to find and replace.
 * @param repl the replacement substring.
 * @param max_len the maximum length of the original string.
 * @param lps pre-allocated array for the KMP longest prefix-suffix (LPS) values.
 * @return a new "rune string" with all occurrences replaced, or nullptr if an error occurs.
 */
static struct rstr * rstr_replace(
    const char * data,
    const char * find,
    const char * repl,
    const size_t max_len,
    int * const restrict lps
) {
    // 1 - get the length of each string
    const size_t data_len = rstr_len(data, max_len);
    const size_t find_len = rstr_len(find, max_len);
    const size_t repl_len = rstr_len(repl, max_len);

    // 2 - compute how much the total length will increase for each substitution.
    // this will be 0 if `repl` is the length or shorter than `find`.
    const size_t increase = repl_len > find_len ? repl_len - find_len : 0;

    struct rstr * result = rstr_of(data, data_len);

    // 3 - iterate the string, finding all occurrences of `find` and counting
    // how many times it occurs.
    const size_t chunk_size = 128;

    const char * finds[chunk_size];
    const char * end = data + data_len;
    const char * src = data;
    char * dst = result->data;

    size_t n = 0;
    do {
        n = 0;
        for (const char *c = src, *idx = c; idx != nullptr && c < end && n < chunk_size;) {
            idx = rstr_find(c, find, end - c, find_len, false, lps);
            if (idx != nullptr) {
                finds[n++] = idx;
                c = idx + find_len; // move the pointer to the end of the found substring
            }
        }

        // flush
        const size_t total_increase = n * increase;
        result = rstr_resize(result, data_len + total_increase, 0, realloc);

        for (size_t i = 0; i < n; i++) {
            // copy the data before the match
            const size_t len = finds[i] - src;
            memcpy(dst, src, len);
            dst += len;
            src += len;

            // copy the replacement
            memcpy(dst, repl, repl_len);
            dst += repl_len;
            src += find_len;
        }
    } while (n > 0);

    const size_t tail_len = end - src;
    memcpy(dst, src, tail_len);
    dst += tail_len;
    *dst = NULLTERM;
    result->len = dst - result->data;
    result->hash = fnv1a_hash(result->data, result->cap);
    return result;
}

/**
 * Split a string into an array of strings using a delimiter.
 * This function uses the KMP algorithm to find occurrences of the delimiter
 * and splits the string accordingly.
 *
 * @param str the string to split.
 * @param str_len the length of the string to split.
 * @param delim the delimiter to use for splitting.
 * @param delim_len the length of the delimiter.
 * @param max_tokens the maximum number of tokens to return.
 * @param max_len the maximum length of each token.
 * @param result pre-allocated array to store the resulting tokens.
 * @return the number of tokens found, or 0 if none were found.
 */
static size_t rstr_split(
    const char * str,
    const size_t str_len,
    const char * delim,
    const size_t delim_len,
    const size_t max_tokens,
    const size_t max_len,
    char ** result
) {
    size_t count = 0;

    const char * start = str;
    const char * end;
    while ((size_t)(start - str) < str_len && count < max_tokens) {
        // Use rstr_find to locate the next delimiter
        if (delim_len < RUNE_STR_STACK_MAX_LEN) {
            int lps[delim_len];
            end = rstr_find(start, delim, str_len - (start - str), delim_len, false, lps);
        } else {
            int * lps = r_calloc_t(delim_len, int);
            end = rstr_find(start, delim, str_len - (start - str), delim_len, false, lps);
            free(lps);
        }
        if (end == nullptr) {
            end = str + str_len; // no more delimiters found
        }
        const size_t len = end - start;

        if (len > 0 && len <= max_len) {
            result[count] = r_malloc(len + 1);
            memcpy(result[count], start, len);
            result[count][len] = NULLTERM;
            count++;
        }
        start = end + delim_len;
    }

    result[count] = nullptr;
    return count;
}

// ------
// public
// ------

extern char * RUNE(str)(const char * data, const size_t max_len, ...) {
    const struct rstr * r = rstr_of(data, max_len);
    return r == nullptr ? nullptr : r->data;
}

extern bool RUNE(str_is)(const char * data, const size_t max_len, ... /* size */) {
    const struct rstr * r = rstr(data);
    return r != nullptr && r->len <= max_len;
}

extern void * str_free(char * data) {
    if (data != nullptr) {
        struct rstr * r = rstr(data);
        if (r == nullptr) {
            r_free(data);
        } else {
            r_free(r);
        }
    }
    return nullptr;
}

extern size_t RUNE(str_len)(const char * str, const size_t max_len, ...) {
    const struct rstr * r = rstr(str);
    return r == nullptr ? strnlen(str, max_len) : r->len;
}

extern size_t RUNE(str_size)(const char * str, const size_t max_len, ...) {
    return str_len(str, max_len) + 1 + sizeof(struct rstr); // +1 for nullterm
}

extern size_t RUNE(str_hash)(const char * str, const size_t len, ...) {
    return rstr_hash(str, len);
}

extern char * RUNE(str_cat)(const size_t max_len, const char * first, ...) {
    // 1 - empty args check
    if (first == nullptr) {
        return nullptr;
    }

    size_t n_strs = 0;

    // 2 - resolve optional `size` and verify the arg count is less than `RUNE_STR_MAX_VARG`
    va_list args;
    va_start(args, first);
    size_t total_len = 0;
    unsigned long long hash = fnv1a_start();
    const char * p = first;
    while (p != nullptr && n_strs++ < RUNE_STR_MAX_VARG) {
        const struct rstr * r = rstr(p);

        size_t len = 0;
        size_t new_hash = hash;

        if (r == nullptr) {
            size_t i = 0;
            while (i < max_len && p[i] != NULLTERM) {
                new_hash = fnv1a_next(new_hash, (unsigned char)p[i]);
                i++;
            }
            len = i;
        } else {
            new_hash = fnv1a_mix(hash, r->hash);
            len = r->len;
        }

        const size_t new_len = total_len + len;
        if (new_len > max_len) {
            // the total length is too large, break here.
            p = nullptr;
        } else {
            total_len = new_len;
            hash = new_hash;
            p = va_arg(args, const char *);
        }
    }
    va_end(args);

    // 3 - iterate again, copying into the result buffer
    const struct rstr * result = rstr_new(nullptr, total_len, hash);

    va_start(args, first);
    p = first;
    size_t pos = 0;
    while (p != nullptr) {
        const size_t p_len = str_len(p, max_len);
        const size_t new_pos = pos + p_len;

        if (new_pos > max_len) {
            // the total length is too large, break here.
            p = nullptr;
        } else {
            memcpy(result->data + pos, p, p_len);
            pos = new_pos;
            p = va_arg(args, const char *);
        }
    }
    va_end(args);

    result->data[total_len] = NULLTERM;
    return result->data;
}

extern int RUNE(str_cmp)(const char * a, const char * b, const size_t len, ...) {
    if (a == nullptr) {
        return b == nullptr ? 0 : -1;
    }

    if (b == nullptr) {
        return 1;
    }

    return strncmp(a, b, len);
}

extern bool RUNE(str_eq)(const char * a, const char * b, const size_t len, ...) {
    if (a == nullptr) {
        return b == nullptr;
    }

    if (b == nullptr) {
        return false;
    }

    return str_hash(a) == str_hash(b) && strncmp(a, b, len) == 0;
}

extern const char * RUNE(str_find)(const char * str, const char * key, const size_t len, ...) {
    if (str == nullptr || key == nullptr) {
        return nullptr;
    }

    const size_t n = str_len(str, len);
    const size_t m = str_len(key, len);

    if (m == 0) {
        return str;
    }

    if (n < m) {
        return nullptr;
    }

    if (m < RUNE_STR_STACK_MAX_LEN) {
        int lps[m];
        return rstr_find(str, key, n, m, false, lps);
    }

    int * lps = r_calloc_t(m, int);
    const char * result = rstr_find(str, key, n, m, false, lps);
    free(lps);
    return result;
}

extern char * RUNE(str_join_arr)(
    const char * delim,
    const char ** arr,
    const size_t max_join,
    const size_t max_len,
    ...
) {
    // 1 - null check
    if (delim == nullptr || arr == nullptr) {
        return nullptr;
    }

    const size_t delim_len = str_len(delim, max_len);
    size_t total_len = 0;
    unsigned long long hash = fnv1a_start();
    size_t n_strs = 0;

    // 2 - iterate the array, counting the lengths and validating against `max_len`
    for (const char * p = *arr; n_strs++ < max_join && p != nullptr; p++) {
        const struct rstr * r = rstr(p);

        size_t len = 0;
        unsigned long long new_hash = hash;

        if (r == nullptr) {
            len = strnlen(p, max_len);
            for (size_t j = 0; j < len && j < max_len; j++) {
                new_hash = fnv1a_next(new_hash, (unsigned char)p[j]);
            }
        } else {
            new_hash = fnv1a_mix(hash, r->hash);
            len = r->len;
        }

        const size_t new_len = total_len + len;
        if (new_len > max_len) {
            // the total length is too large, break here.
            break;
        }

        total_len = new_len;
        hash = new_hash;
    }

    if (n_strs <= 0) {
        return nullptr; // no strings to join
    }

    total_len += (n_strs - 1) * delim_len; // add the delimiters

    // 3 - allocate the result buffer
    const struct rstr * result = rstr_new(nullptr, total_len, hash);

    // 4 - iterate again, copying into the result buffer
    size_t pos = 0;
    for (size_t i = 0; i < n_strs && arr[i] != nullptr; i++) {
        const char * p = arr[i];
        const size_t p_len = str_len(p, max_len);
        memcpy(result->data + pos, p, p_len);
        pos += p_len;

        // add the delimiter if not the last string
        if (i < n_strs - 1) {
            memcpy(result->data + pos, delim, delim_len);
            pos += delim_len;
        }
    }

    result->data[total_len] = NULLTERM;
    return result->data;
}

extern char * RUNE(str_join_varg)(const char * delim, const size_t max_len, ...) {
    // 1 - null check
    if (delim == nullptr) {
        return nullptr;
    }

    size_t n_strs = 0;

    // 3 - reiterate, counting the lengths and validating against `max_len`
    va_list args;
    va_start(args, max_len);
    size_t total_len = 0;
    const size_t delim_len = str_len(delim, max_len);
    unsigned long long hash = fnv1a_start();
    const char * p = va_arg(args, const char *);
    while (p != nullptr && n_strs++ < RUNE_STR_MAX_VARG) {
        const struct rstr * r = rstr(p);

        size_t len = 0;
        unsigned long long new_hash = hash;

        if (r == nullptr) {
            size_t i = 0;
            while (i < max_len && p[i] != NULLTERM) {
                new_hash = fnv1a_next(new_hash, (unsigned char)p[i]);
                i++;
            }
            len = i;
        } else {
            new_hash = fnv1a_mix(hash, r->hash);
            len = r->len;
        }

        const size_t new_len = total_len + len;
        if (new_len > max_len) {
            // the total length is too large, break here.
            p = nullptr;
        } else {
            total_len = new_len;
            hash = new_hash;
            p = va_arg(args, const char *);
        }
    }
    va_end(args);

    total_len += (n_strs - 1) * delim_len; // add the delimiters

    // 4 - iterate again, copying into the result buffer
    const struct rstr * result = rstr_new(nullptr, total_len, hash);

    va_start(args, max_len);
    p = va_arg(args, const char *);

    size_t pos = 0;
    while (p != nullptr) {
        const size_t p_len = str_len(p, max_len);
        const size_t new_pos = pos + p_len;

        if (new_pos > max_len) {
            // the total length is too large, break here.
            p = nullptr;
        } else {
            memcpy(result->data + pos, p, p_len);
            pos = new_pos;

            // add the delimiter if not the last string
            p = va_arg(args, const char *);
            if (p != nullptr) {
                memcpy(result->data + pos, delim, delim_len);
                pos += delim_len;
            }
        }
    }
    va_end(args);

    result->data[total_len] = NULLTERM;
    return result->data;
}

extern const char * RUNE(str_rfind)(const char * str, const char * key, const size_t len, ...) {
    if (str == nullptr || key == nullptr) {
        return nullptr;
    }

    const size_t n = str_len(str, len);
    const size_t m = str_len(key, len);

    if (m == 0) {
        return str + n;
    }

    if (n < m) {
        return nullptr;
    }

    if (m < RUNE_STR_STACK_MAX_LEN) {
        int lps[m];
        return rstr_find(str, key, n, m, true, lps);
    }

    int * lps = r_calloc_t(m, int);
    const char * result = rstr_find(str, key, n, m, true, lps);
    free(lps);
    return result;
}

extern char * RUNE(str_replace)(
    const char * str,
    const char * target,
    const char * new_val,
    const size_t max_len,
    ...
) {
    if (str == nullptr || target == nullptr || new_val == nullptr) {
        return nullptr;
    }

    const size_t n = str_len(str, max_len);
    const size_t m = str_len(target, max_len);

    if (m == 0 || n < m) {
        return nullptr;
    }

    if (m < RUNE_STR_STACK_MAX_LEN) {
        int lps[m];
        const struct rstr * r = rstr_replace(str, target, new_val, max_len, lps);
        return r->data;
    }

    int * lps = r_calloc_t(m, int);
    const struct rstr * r = rstr_replace(str, target, new_val, max_len, lps);
    free(lps);
    return r->data;
}

extern char * RUNE(str_sub)(const char * str, ssize_t start, ssize_t n, ...) {
    // 1 - null check
    if (str == nullptr) {
        return nullptr;
    }

    // 2 - get the string length
    const ssize_t data_len = str_len(str);

    // 3 - clamp `start` to the string bounds
    start = CLAMP(start, 0, data_len);

    // 4 - clamp `n` to the string bounds
    if (n < 0) {
        n = data_len - start;
    }

    size_t i = start;
    unsigned long long hash = fnv1a_start();
    while (i < data_len && i < start + n) {
        hash = fnv1a_next(hash, (unsigned char)str[i]);
        i++;
    }
    const size_t len = i;

    // 5 - copy the substring into a new buffer
    const struct rstr * result = rstr_new(str + start, len, hash);
    return result->data;
}

char ** RUNE(str_split)(
    const char * str,
    const char * delim,
    const size_t max_tokens,
    const size_t max_len,
    ...
) {
    if (str == nullptr || delim == nullptr) {
        return nullptr;
    }

    const size_t n = str_len(str, max_len);
    const size_t m = str_len(delim, max_len);

    if (m == 0 || n < m) {
        return nullptr;
    }

    char * result[max_tokens + 1];
    const size_t count = rstr_split(str, n, delim, m, max_tokens, max_len, result);

    char ** result_alloc = r_calloc_t(count + 1, char *);
    memcpy(result_alloc, result, count * sizeof(char *));
    result_alloc[count] = nullptr; // null-terminate the array
    return result_alloc;
}

extern char * RUNE(str_trim)(char * str, const size_t max_len, const bool realloc, ...) {
    if (str == nullptr) {
        return nullptr;
    }

    const size_t len = str_len(str, max_len);
    if (len == 0) {
        return str;
    }

    size_t start = len;
    size_t end = 0;
    bool after_space = false;
    unsigned long long hash = fnv1a_start();
    for (size_t i = 0; i < len; i++) {
        const unsigned char c = (unsigned char)str[i];

        if (isspace(c)) {
            if (i && !after_space) {
                end = i;
                after_space = true;
            }
        } else {
            end = i;
            if (start == len) {
                start = i;
            }
            hash = fnv1a_next(hash, c);
            after_space = false;
        }
    }

    const size_t new_len = isspace((unsigned char)str[end]) ? end - start : end - start + 1;

    if (new_len != len) {
        struct rstr * r = rstr(str);
        if (r == nullptr) {
            r = rstr_of(str, max_len);
        }

        memmove(str, str + start, new_len);
        str[new_len] = NULLTERM;

        r = rstr_resize(r, new_len, hash, realloc);
        return r->data;
    }
    return str;
}

extern char * RUNE(str_ltrim)(char * str, const size_t max_len, const bool realloc, ...) {
    if (str == nullptr) {
        return nullptr;
    }

    const size_t len = str_len(str, max_len);
    if (len == 0) {
        return str;
    }

    size_t start = 0;
    unsigned long long hash = fnv1a_start();
    while (start < len && isspace((unsigned char)str[start])) {
        hash = fnv1a_next(hash, (unsigned char)str[start]);
        start++;
    }

    if (start > 0) {
        struct rstr * r = rstr(str);
        if (r == nullptr) {
            r = rstr_of(str, max_len);
        }

        memmove(str, str + start, len - start);
        str[len - start] = NULLTERM;

        r = rstr_resize(r, len - start, hash, realloc);
        return r->data;
    }
    return str;
}

extern char * RUNE(str_rtrim)(char * str, const size_t max_len, const bool realloc, ...) {
    if (str == nullptr) {
        return nullptr;
    }

    const size_t len = str_len(str, max_len);
    if (len == 0) {
        return str;
    }

    size_t end = len - 1;
    unsigned long long hash = fnv1a_start();
    while (end > 0 && isspace((unsigned char)str[end])) {
        hash = fnv1a_next(hash, (unsigned char)str[end]);
        end--;
    }

    if (end < len - 1) {
        struct rstr * r = rstr(str);
        if (r == nullptr) {
            r = rstr_of(str, max_len);
        }

        r->data[end + 1] = NULLTERM;
        r = rstr_resize(r, end + 1, hash, realloc);
        return r->data;
    }
    return str;
}
