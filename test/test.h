//
// Created by Matt on 5/3/2025.
//

#ifndef RUNE_TEST_H
#define RUNE_TEST_H

#include "CUnit/Basic.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

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
    int fd = open("/dev/null", O_WRONLY);
    if (fd >= 0) {
        FILE * null_stream = fdopen(fd, "w");
        if (null_stream) {
            return null_stream;
        }
        close(fd);
    }
    return NULL;
}

#endif // RUNE_TEST_H
