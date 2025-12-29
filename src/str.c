/*
 * Core string module implementation.
 */

#include "str.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    typedef int64_t ssize_t;
#else
    #include <sys/types.h>
#endif

// =====================================================================================================================
// Internal: FNV-1a hashing
// =====================================================================================================================

static uint64_t fnv1a_start(void) {
    return 0xCBF29CE484222325ULL;
}

static uint64_t fnv1a_next(uint64_t hash, uint8_t byte) {
    return (hash ^ byte) * 0x100000001B3ULL;
}

static uint64_t fnv1a_mix(uint64_t h1, uint64_t h2) {
    return (h1 ^ h2) * 0x100000001B3ULL;
}

static uint64_t fnv1a_hash(const char * data, size_t max_len) {
    uint64_t hash = fnv1a_start();
    for (size_t i = 0; i < max_len && data[i] != NULLTERM; i++) {
        hash = fnv1a_next(hash, (uint8_t)data[i]);
    }
    return hash;
}

// =====================================================================================================================
// Internal: Managed string structure
// =====================================================================================================================

static constexpr char SOH = 0x01; // start of header
static constexpr char STX = 0x02; // start of text
static constexpr char ETX = 0x03; // end of text

typedef struct {
    char soh;
    size_t len;
    size_t cap;
    uint64_t hash;
    char stx;
    char data[];
} rstr;

// Get rstr from data pointer, or nullptr if not a managed string
static rstr * rstr_from(const char * data) {
    if (data == nullptr)
        return nullptr;

    rstr * r = (rstr *)(data - offsetof(rstr, data));
    if (r->soh == SOH && r->stx == STX) {
        // Check ETX marker at end
        if (r->data[r->cap] == ETX) {
            return r;
        }
    }
    return nullptr;
}

// Allocate new rstr with given length
static rstr * rstr_alloc(size_t len) {
    const size_t total = sizeof(rstr) + len + 2; // +1 for null, +1 for ETX
    rstr * r = mem_alloc(total);
    r->soh = SOH;
    r->len = len;
    r->cap = len;
    r->hash = 0L;
    r->stx = STX;
    r->data[len] = NULLTERM;
    r->data[len + 1] = ETX;
    return r;
}

// Create rstr from data, computing hash
static rstr * rstr_new(const char * data, size_t max_len) {
    if (data == nullptr)
        return nullptr;

    // Compute length and hash in one pass
    size_t len = 0;
    uint64_t hash = fnv1a_start();
    while (len < max_len && data[len] != NULLTERM) {
        hash = fnv1a_next(hash, (uint8_t)data[len]);
        len++;
    }

    rstr * r = rstr_alloc(len);
    r->hash = hash;
    memcpy(r->data, data, len);
    return r;
}

// Get length, using cached value for managed strings
static size_t rstr_len(const char * s, size_t max_len) {
    const rstr * r = rstr_from(s);
    return r ? r->len : strnlen(s, max_len);
}

// Get hash, using cached value for managed strings
static uint64_t rstr_hash(const char * s, size_t max_len) {
    if (s == nullptr)
        return 0;
    const rstr * r = rstr_from(s);
    return r ? r->hash : fnv1a_hash(s, max_len);
}

// =====================================================================================================================
// Internal: KMP string search
// =====================================================================================================================

static void kmp_lps(const char * pat, size_t len, int * lps) {
    int k = 0;
    lps[0] = 0;
    for (size_t i = 1; i < len;) {
        if (pat[i] == pat[k]) {
            lps[i++] = ++k;
        } else if (k) {
            k = lps[k - 1];
        } else {
            lps[i++] = 0;
        }
    }
}

static const char *
kmp_find(const char * text, size_t text_len, const char * pat, size_t pat_len, bool reverse, int * lps) {
    kmp_lps(pat, pat_len, lps);

    size_t i = 0, j = 0;
    ssize_t last = -1;

    while (i < text_len) {
        if (text[i] == pat[j]) {
            i++;
            j++;
        }
        if (j == pat_len) {
            if (reverse) {
                last = (ssize_t)(i - j);
                j = (size_t)lps[j - 1];
            } else {
                return text + (i - j);
            }
        }
        if (i < text_len && text[i] != pat[j]) {
            if (j == 0) {
                i++;
            } else {
                j = (size_t)lps[j - 1];
            }
        }
    }

    return reverse && last >= 0 ? text + last : nullptr;
}

// =====================================================================================================================
// Public API: Creation and destruction
// =====================================================================================================================

extern char * R_(str)(const char * restrict data, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (err_null(data))
        return nullptr;
    rstr * r = rstr_new(data, opt->max_len);
    if (r == nullptr) {
        err_set(R_ERR_ALLOC_FAILED, nullptr);
        return nullptr;
    }
    return r->data;
}

extern char * R_(strf)(const str_opt * opt, const char * fmt, ...) {
    if (err_null(fmt))
        return nullptr;

    if (opt == nullptr)
        opt = &r_str_opts_default;

    const size_t max_len = opt->max_len;
    va_list args;
    va_start(args, fmt);

    // Try stack buffer first
    if (max_len <= R_STR_STACK_MAX) {
        char tmp[max_len + 1];
        const int n = vsnprintf(tmp, max_len + 1, fmt, args);
        va_end(args);

        if (n < 0 || (size_t)n > max_len) {
            err_set(R_ERR_FORMAT_FAILED, nullptr);
            return nullptr;
        }

        rstr * r = rstr_new(tmp, (size_t)n);
        if (r == nullptr) {
            err_set(R_ERR_ALLOC_FAILED, nullptr);
            return nullptr;
        }
        return r->data;
    }

    // Fall back to heap
    char * tmp = mem_alloc(max_len + 1);
    const int n = vsnprintf(tmp, max_len + 1, fmt, args);
    va_end(args);

    char * result = nullptr;
    if (n >= 0 && (size_t)n <= max_len) {
        rstr * r = rstr_new(tmp, (size_t)n);
        if (r != nullptr) {
            result = r->data;
        } else {
            err_set(R_ERR_ALLOC_FAILED, nullptr);
        }
    } else {
        err_set(R_ERR_FORMAT_FAILED, nullptr);
    }

    mem_free(tmp, max_len + 1);
    return result;
}

extern void str_free(const char * s) {
    if (s == nullptr)
        return;

    rstr * r = rstr_from(s);
    if (r) {
        mem_free(r, sizeof(rstr) + r->cap + 2);
    }
}

// =====================================================================================================================
// Public API: Queries
// =====================================================================================================================

extern bool R_(str_is)(const char * s, const str_opt * opt) {
    return rstr_from(s) != nullptr;
}

extern size_t R_(str_len)(const char * s, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    return s ? rstr_len(s, opt->max_len) : 0;
}

extern size_t R_(str_size)(const char * s, const str_opt * opt) {
    const rstr * r = rstr_from(s);
    return r ? sizeof(rstr) + r->cap + 2 : 0;
}

extern uint64_t R_(str_hash)(const char * s, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    return rstr_hash(s, opt->max_len);
}

// =====================================================================================================================
// Public API: Comparison
// =====================================================================================================================

extern int R_(str_cmp)(const char * a, const char * b, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (a == nullptr)
        return b == nullptr ? 0 : -1;
    if (b == nullptr)
        return 1;
    return strncmp(a, b, opt->max_len);
}

extern bool R_(str_eq)(const char * a, const char * b, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (a == nullptr)
        return b == nullptr;
    if (b == nullptr)
        return false;

    // Fast path: compare hashes first
    if (rstr_hash(a, opt->max_len) != rstr_hash(b, opt->max_len)) {
        return false;
    }
    return strncmp(a, b, opt->max_len) == 0;
}

// =====================================================================================================================
// Public API: Search
// =====================================================================================================================

extern const char * R_(str_find)(const char * data, const char * target, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (err_null(data) || err_null(target))
        return nullptr;

    const size_t h_len = rstr_len(data, opt->max_len);
    const size_t n_len = rstr_len(target, opt->max_len);

    if (n_len == 0)
        return data;
    if (h_len < n_len) {
        err_set(R_ERR_PATTERN_NOT_FOUND, nullptr);
        return nullptr;
    }

    // Use stack buffer for small patterns (<=R_STR_STACK_MAX), heap for large patterns.
    // This avoids VLA overhead for common cases while protecting stack on systems with
    // limited stack space. Pattern length is naturally bounded by max_len (typically 4KB).
    if (n_len <= R_STR_STACK_MAX) {
        int lps[n_len];
        return kmp_find(data, h_len, target, n_len, false, lps);
    }

    int * lps = mem_alloc(n_len * sizeof(int));
    const char * result = kmp_find(data, h_len, target, n_len, false, lps);
    mem_free(lps, n_len * sizeof(int));
    return result;
}

extern const char * R_(str_rfind)(const char * data, const char * target, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (err_null(data) || err_null(target))
        return nullptr;

    const size_t h_len = rstr_len(data, opt->max_len);
    const size_t n_len = rstr_len(target, opt->max_len);

    if (n_len == 0)
        return data + h_len;
    if (h_len < n_len) {
        err_set(R_ERR_PATTERN_NOT_FOUND, nullptr);
        return nullptr;
    }

    // Use stack buffer for small patterns (<=R_STR_STACK_MAX), heap for large patterns.
    // This avoids VLA overhead for common cases while protecting stack on systems with
    // limited stack space. Pattern length is naturally bounded by max_len (typically 4KB).
    if (n_len <= R_STR_STACK_MAX) {
        int lps[n_len];
        return kmp_find(data, h_len, target, n_len, true, lps);
    }

    int * lps = mem_alloc(n_len * sizeof(int));
    const char * result = kmp_find(data, h_len, target, n_len, true, lps);
    mem_free(lps, n_len * sizeof(int));
    return result;
}

// =====================================================================================================================
// Public API: Transformation
// =====================================================================================================================

extern char * R_(str_cat)(const str_opt * opt, const char * first, ...) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (err_null(first))
        return nullptr;

    const size_t max_len = opt->max_len;

    // First pass: compute total length and hash
    va_list args;
    va_start(args, first);

    size_t total_len = 0;
    uint64_t hash = fnv1a_start();
    size_t count = 0;
    bool overflow = false;

    for (const char * p = first; p != nullptr && count < R_STR_MAX_VARG; p = va_arg(args, const char *)) {
        const rstr * r = rstr_from(p);
        size_t len;

        if (r) {
            len = r->len;
            hash = fnv1a_mix(hash, r->hash);
        } else {
            len = 0;
            while (len < max_len && p[len] != NULLTERM) {
                hash = fnv1a_next(hash, (uint8_t)p[len]);
                len++;
            }
        }

        if (total_len + len > max_len) {
            overflow = true;
            break;
        }
        total_len += len;
        count++;
    }
    va_end(args);

    if (overflow) {
        err_set(R_ERR_LENGTH_EXCEEDED, nullptr);
        return nullptr;
    }

    // Allocate result
    rstr * result = rstr_alloc(total_len);
    if (result == nullptr) {
        err_set(R_ERR_ALLOC_FAILED, nullptr);
        return nullptr;
    }
    result->hash = hash;

    // Second pass: copy data
    va_start(args, first);
    size_t pos = 0;

    for (const char * p = first; p != nullptr && pos < total_len; p = va_arg(args, const char *)) {
        size_t len = rstr_len(p, max_len);
        if (pos + len > total_len)
            len = total_len - pos;
        memcpy(result->data + pos, p, len);
        pos += len;
    }
    va_end(args);

    return result->data;
}

extern char * R_(str_join)(const char * delim, const char ** arr, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (err_null(delim) || err_null(arr))
        return nullptr;

    const size_t max_len = opt->max_len;
    const size_t delim_len = rstr_len(delim, max_len);

    // Count strings and compute total length
    size_t count = 0;
    size_t total_len = 0;
    uint64_t hash = fnv1a_start();

    for (const char ** p = arr; *p != nullptr; p++) {
        const rstr * r = rstr_from(*p);
        size_t len;

        if (r) {
            len = r->len;
            hash = fnv1a_mix(hash, r->hash);
        } else {
            len = strnlen(*p, max_len);
            for (size_t i = 0; i < len; i++) {
                hash = fnv1a_next(hash, (uint8_t)(*p)[i]);
            }
        }

        const size_t needed = total_len + len + (count > 0 ? delim_len : 0);
        if (needed > max_len)
            break;

        total_len = needed;
        count++;
    }

    if (count == 0) {
        err_set(R_ERR_EMPTY_INPUT, nullptr);
        return nullptr;
    }

    // Allocate and build result
    rstr * result = rstr_alloc(total_len);
    if (result == nullptr) {
        err_set(R_ERR_ALLOC_FAILED, nullptr);
        return nullptr;
    }
    result->hash = hash;

    size_t pos = 0;
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            memcpy(result->data + pos, delim, delim_len);
            pos += delim_len;
        }
        const size_t len = rstr_len(arr[i], max_len);
        memcpy(result->data + pos, arr[i], len);
        pos += len;
    }

    return result->data;
}

extern char * R_(str_repeat)(const char * s, size_t n, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (err_null(s) || err_check(n > 0, R_ERR_INVALID_ARGUMENT))
        return nullptr;

    const size_t max_len = opt->max_len;
    const size_t len = rstr_len(s, max_len);
    if (len == 0) {
        err_set(R_ERR_EMPTY_INPUT, nullptr);
        return nullptr;
    }
    if (n > max_len / len) {
        err_set(R_ERR_LENGTH_EXCEEDED, nullptr);
        return nullptr;
    }

    const size_t total = len * n;
    rstr * result = rstr_alloc(total);
    if (result == nullptr) {
        err_set(R_ERR_ALLOC_FAILED, nullptr);
        return nullptr;
    }
    result->hash = rstr_hash(s, max_len); // Simplified: not combining hashes

    for (size_t i = 0; i < n; i++) {
        memcpy(result->data + i * len, s, len);
    }

    return result->data;
}

extern char * R_(str_replace)(const char * s, const char * target, const char * replacement, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (err_null(s) || err_null(target) || err_null(replacement))
        return nullptr;

    const size_t max_len = opt->max_len;
    const size_t s_len = rstr_len(s, max_len);
    const size_t t_len = rstr_len(target, max_len);
    const size_t r_len = rstr_len(replacement, max_len);

    if (t_len == 0 || s_len < t_len) {
        // No match possible, return copy
        return R_(str)(s, opt);
    }

    // Count occurrences to compute result size
    // Note: str_find() sets R_ERR_PATTERN_NOT_FOUND when pattern is not found.
    // We clear this expected error after counting, as "not found" is normal for the loop exit.
    size_t count = 0;
    const char * p = s;
    while ((p = R_(str_find)(p, target, opt)) != nullptr) {
        count++;
        p += t_len;
    }
    err_clear(); // Clear R_ERR_PATTERN_NOT_FOUND from the final str_find() call

    if (count == 0) {
        return R_(str)(s, opt);
    }

    // Compute new length (handle both growth and shrinkage)
    size_t new_len;
    if (r_len >= t_len) {
        new_len = s_len + count * (r_len - t_len);
    } else {
        new_len = s_len - count * (t_len - r_len);
    }
    if (new_len > max_len) {
        err_set(R_ERR_LENGTH_EXCEEDED, nullptr);
        return nullptr;
    }

    rstr * result = rstr_alloc(new_len);
    if (result == nullptr) {
        err_set(R_ERR_ALLOC_FAILED, nullptr);
        return nullptr;
    }

    // Build result
    const char * src = s;
    char * dst = result->data;

    while ((p = R_(str_find)(src, target, opt)) != nullptr) {
        const size_t chunk = (size_t)(p - src);
        memcpy(dst, src, chunk);
        dst += chunk;
        memcpy(dst, replacement, r_len);
        dst += r_len;
        src = p + t_len;
    }
    err_clear(); // Clear R_ERR_PATTERN_NOT_FOUND from the final str_find() call

    // Copy remainder
    const size_t tail = s_len - (size_t)(src - s);
    memcpy(dst, src, tail);

    result->hash = fnv1a_hash(result->data, new_len);
    return result->data;
}

extern char ** R_(str_split)(const char * s, const char * delim, const str_opt * opt) {
    if (opt == nullptr)
        opt = &r_str_opts_default;
    if (err_null(s) || err_null(delim))
        return nullptr;

    const size_t max_len = opt->max_len;
    const size_t max_tokens = opt->max_tok > 0 ? opt->max_tok : R_STR_MAX_VARG;
    const size_t s_len = rstr_len(s, max_len);
    const size_t d_len = rstr_len(delim, max_len);

    if (d_len == 0 || s_len < d_len) {
        err_set(R_ERR_INVALID_ARGUMENT, nullptr);
        return nullptr;
    }

    // First pass: count tokens
    size_t count = 0;
    const char * start = s;
    const char * end;

    while (count < max_tokens && (size_t)(start - s) < s_len) {
        end = R_(str_find)(start, delim, opt);
        if (end == nullptr)
            end = s + s_len;

        if (end > start)
            count++;
        start = end + d_len;
    }
    err_clear(); // Clear error from str_find

    if (count == 0) {
        err_set(R_ERR_EMPTY_INPUT, nullptr);
        return nullptr;
    }

    // Allocate result array
    char ** result = mem_alloc_zero((count + 1) * sizeof(char *));
    if (result == nullptr) {
        err_set(R_ERR_ALLOC_FAILED, nullptr);
        return nullptr;
    }

    // Second pass: extract tokens
    start = s;
    size_t i = 0;

    while (i < count && (size_t)(start - s) < s_len) {
        end = R_(str_find)(start, delim, opt);
        if (end == nullptr)
            end = s + s_len;

        const size_t len = (size_t)(end - start);
        if (len > 0 && len <= max_len) {
            rstr * token = rstr_alloc(len);
            if (token == nullptr) {
                err_set(R_ERR_ALLOC_FAILED, nullptr);
                // Partial failure - free what we've allocated so far and return nullptr
                str_free_arr(result);
                return nullptr;
            }
            memcpy(token->data, start, len);
            token->hash = fnv1a_hash(token->data, len);
            result[i++] = token->data;
        }
        start = end + d_len;
    }
    err_clear(); // Clear error from str_find

    result[i] = nullptr;
    return result;
}

void str_free_arr(char ** arr) {
    if (arr == nullptr)
        return;

    // Count elements first
    size_t count = 0;
    for (char ** p = arr; *p != nullptr; p++)
        count++;

    // Free each string
    for (size_t i = 0; i < count; i++) {
        str_free(arr[i]);
    }

    mem_free(arr, (count + 1) * sizeof(char *));
}
