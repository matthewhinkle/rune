#ifndef RUNE_TEST_H
#define RUNE_TEST_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ADD_TEST(suite, test_func)                                                                                     \
    if (NULL == CU_add_test(suite, #test_func, test_func)) {                                                           \
        CU_cleanup_registry();                                                                                         \
        return CU_get_error();                                                                                         \
    }

/**
 * Open a file securely with restricted permissions (owner only: 0600).
 * On Unix-like systems, uses open() with explicit mode to prevent world-writable files.
 * On Windows, uses fopen() (Windows uses ACLs, not Unix permissions).
 * Returns a FILE* that should be closed with fclose().
 * Returns NULL if opening fails.
 */
[[maybe_unused]]
static FILE * open_file_secure(const char * path, const char * mode) {
#ifdef _WIN32
    // Windows uses ACLs instead of Unix permissions
    // fopen is generally safe on Windows
    return fopen(path, mode);
#else
    // Unix-like systems: use open() with explicit restrictive permissions
    int oflags = 0;
    mode_t omode = S_IRUSR | S_IWUSR; // 0600: owner read/write only

    // Determine flags based on mode string
    if (strchr(mode, 'w')) {
        oflags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strchr(mode, 'a')) {
        oflags = O_WRONLY | O_CREAT | O_APPEND;
    } else {
        oflags = O_RDONLY;
        omode = 0; // No mode needed for read-only
    }

    const int fd = open(path, oflags, omode);
    if (fd < 0)
        return nullptr;

    FILE * fp = fdopen(fd, mode);
    if (!fp) {
        close(fd); // Clean up fd if fdopen fails
        return nullptr;
    }
    return fp;
#endif
}

/**
 * Open a null device for writing to suppress test output.
 * Returns a FILE* that should be closed with fclose().
 * Returns NULL if opening fails.
 */
[[maybe_unused]]
static FILE * open_null_stream(void) {
#ifdef _WIN32
    return open_file_secure("NUL", "w");
#else
    return open_file_secure("/dev/null", "w");
#endif
}

#endif // RUNE_TEST_H
