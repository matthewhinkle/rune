//
// Created by Matt on 5/3/2025.
//

#ifndef RUNE_TEST_H
#define RUNE_TEST_H

#include "CUnit/Basic.h"
#include <stdio.h>

#define ADD_TEST(suite, test_func)                                                                 \
    if (NULL == CU_add_test(suite, #test_func, test_func)) {                                       \
        CU_cleanup_registry();                                                                     \
        return CU_get_error();                                                                     \
    }

/**
 * Open a null device for writing to suppress test output.
 * Returns a FILE* that should be closed with fclose().
 * Returns NULL if opening fails.
 */
static inline FILE * open_null_stream(void) {
#ifdef _WIN32
    return fopen("NUL", "w");
#else
    return fopen("/dev/null", "w");  // codeql [cpp/world-writable-file-creation] - /dev/null is a system device, not a created file
#endif
}

#endif // RUNE_TEST_H
