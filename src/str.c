/*
 * str module.
 */

#include "str.h"
#include "std.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// string memory format:
// [ SOH | metadata | STX | C string with `\0` | ETX ]
static const char SOH = 0x01; // "start of header": first byte of the "rune string"
static const char STX = 0x02; // "start of text": separator byte between header and C string data
static const char ETX = 0x03; // "end of text": final byte of the "rune string"

static const size_t SIZE_HEADER = sizeof(SOH) + sizeof(uint64_t) + sizeof(STX);
static const size_t SIZE_FOOTER = sizeof(ETX);

//
// private api
//

static size_t total_size(const size_t size) {
    return SIZE_HEADER + size + SIZE_FOOTER;
}

static const char * first_byte(const char * str) {
    // 1 - null check
    if (str == nullptr) {
        return nullptr;
    }

    // 2 - check if str is already a "rune string"
    if (*str == SOH) {
        // nothing else to do
        return str;
    }

    // 3 - check if str is within a "rune string"
    const char * p = str - SIZE_HEADER;

    // 4 - verify pointer underflow, and that the char is SOH
    return p < str && *p == SOH ? p : str;
}

static char * init(const size_t data_len) {
    // 1 - resolve the total size of the "rune string"
    const size_t data_size = data_len + 1; // +1 for nullterm

    // 2 - setup a pointer to iteratively append metadata and C string data
    char * dst = r_malloc(total_size(data_size) * sizeof(char));
    char * p = dst;

    // 3 - write "start of header"
    *p++ = SOH;

    // 4 - write the metadata
    const uint64_t data_size_u64 = data_size;
    memcpy(p, &data_size_u64, sizeof(uint64_t));
    p += sizeof(uint64_t);

    // 5 - write "start of text"
    *p++ = STX;

    // 6 - copy the location of the C string data
    char * str = p;

    // 7 - init the C string data to 0x0
    memset(p, NULLTERM, data_size);
    p += data_size;

    // 9 - write "end of text"
    *p = ETX;

    // 10 - return the location of the C string data
    // ReSharper disable once CppDFAMemoryLeak
    return str;
}

static const char * cstr(const char * data) {
    const char * first = first_byte(data);
    if (first == nullptr) {
        return nullptr;
    }

    if (*first == SOH && *(first + SIZE_HEADER - 1) == STX) {
        // the string has a valid "rune string" header
        return first + SIZE_HEADER;
    }

    return data;
}

static bool is_rstr(const char * data, const size_t max_size) {
    // 1 - null check
    if (data == nullptr) {
        return nullptr;
    }

    // 2 - set up a pointer to iteratively check the format of the data
    const char * p = first_byte(data);

    // 3 - check if the first byte is SOH
    if (*p++ != SOH) {
        // no "start of header"
        return false;
    }

    // 4 - extract the metadata
    uint64_t data_size = 0;
    memcpy(&data_size, p, sizeof(uint64_t));
    if (data_size > max_size) {
        return false;
    }

    // 5 - increment past the metadata
    p += sizeof(uint64_t);

    // 6 - check if the next byte is STX
    if (*p++ != STX) {
        // no "start of text"
        return false;
    }

    // 7 - increment past the c string data, -1 for nullterm
    p += data_size - 1;

    // 8 - check for nullterm
    if (*p++ != NULLTERM) {
        return false;
    }

    // 9 - check if the last byte is ETX
    if (*p != ETX) {
        return false;
    }

    // 10 - the provided string is a valid "rune string"
    return true;
}

static size_t len(const char * data, const size_t max_size) {
    const char * first = first_byte(data);
    if (first == nullptr) {
        return 0;
    }

    if (*first == SOH && *(first + SIZE_HEADER - 1) == STX) {
        // the string has a valid "rune string" header
        const char * p = first + sizeof(SOH);
        uint64_t size = 0;
        memcpy(&size, p, sizeof(uint64_t));
        return size > 0 ? size - 1 : 0; // -1 for null term
    }

    return max_size > 0 ? strnlen(data, max_size - 1) : 0; // -1 for null term
}

//
// public api
//

extern const char * RUNE(str)(const char * data, ... /* size */) {
    // 1 - null check
    if (data == nullptr) {
        return nullptr;
    }

    // 2 - resolve optional `size` argument from ...
    va_list args;
    va_start(args, data);
    const size_t max_size = va_arg(args, size_t);
    va_end(args);

    // 3 - resolve the c string data length and size (+1 for nullterm)
    const char * str_data = cstr(data);
    const size_t data_len = strnlen(str_data, max_size);

    // 4 - check if the data size is too large
    if (data_len > max_size - 1) { // -1 for nullterm
        return nullptr;
    }

    // 5 - allocate and format the result buffer
    char * result = init(data_len);
    memcpy(result, str_data, data_len);
    return result;
}

extern bool RUNE(str_is)(const char * data, ... /* size */) {
    va_list args;
    va_start(args, data);
    const size_t max_size = va_arg(args, size_t);
    va_end(args);

    return is_rstr(data, max_size);
}

extern void * str_free(const char * data) {
    return r_free((void *)first_byte(data));
}

extern size_t RUNE(str_len)(const char * data, ...) {
    va_list args;
    va_start(args, data);
    const size_t max_size = va_arg(args, size_t);
    va_end(args);
    return len(data, max_size);
}

extern size_t RUNE(str_size)(const char * data, ...) {
    va_list args;
    va_start(args, data);
    const size_t max_size = va_arg(args, size_t);
    va_end(args);

    return total_size(len(data, max_size) + 1);
}

extern const char * RUNE(str_cat)(const char * first, ...) {
    // 1 - empty args check
    if (first == nullptr) {
        return nullptr;
    }

    size_t n_strs = 0;

    // 2 - resolve optional `size` and verify the arg count is less than `RUNE_STR_MAX_CAT`
    va_list args;
    va_start(args, first);
    const char * p = first;
    while (p != nullptr && n_strs++ < RUNE_STR_MAX_CAT) {
        p = va_arg(args, const char *);
    }
    assert(p == nullptr);
    const size_t max_size = va_arg(args, size_t);
    va_end(args);

    // 3 - reiterate, counting the lengths and validating against `max_size`
    size_t total_len = 0;
    va_start(args, first);
    p = first;
    while (p != nullptr) {
        const size_t new_len = total_len + len(p, max_size);

        if (new_len > max_size) {
            // the total length is too large, break here.
            p = nullptr;
        } else {
            total_len = new_len;
            p = va_arg(args, const char *);
        }
    }
    va_end(args);

    // 4 - iterate again, copying into the result buffer
    char * result = init(total_len);

    va_start(args, first);
    p = first;
    size_t pos = 0;
    while (p != nullptr) {
        const size_t p_len = len(p, max_size);
        const size_t new_pos = pos + p_len;

        if (new_pos > max_size) {
            // the total length is too large, break here.
            p = nullptr;
        } else {
            memcpy(result + pos, p, p_len);
            pos = new_pos;
            p = va_arg(args, const char *);
        }
    }
    va_end(args);

    return result;
}

extern int RUNE(str_cmp)(const char * a, const char * b, ... /* `size` */) {
    va_list args;
    va_start(args, b);
    const size_t max_size = va_arg(args, size_t);
    va_end(args);

    if (a == nullptr) {
        return b == nullptr ? 0 : -1;
    }

    if (b == nullptr) {
        return 1;
    }

    return strncmp(cstr(a), cstr(b), max_size);
}

extern const char * RUNE(str_sub)(const char * data, ssize_t start, ssize_t n, ...) {
    // 1 - null check
    if (data == nullptr) {
        return nullptr;
    }

    // 2 - get the string length
    const ssize_t data_len = str_len(data);

    // 3 - clamp `start` to the string bounds
    start = CLAMP(start, 0, data_len);

    // 4 - clamp `n` to the string bounds
    if (n < 0) {
        n = data_len - start;
    }

    // 5 - copy the substring into a new buffer
    char * result = init(start + n);
    memcpy(result, data + start, n);
    return result;
}
