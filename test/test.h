//
// Created by Matt on 5/3/2025.
//

#ifndef RUNE_TEST_H
#define RUNE_TEST_H

#include "CUnit/Basic.h"

#define ADD_TEST(suite, test_func)                                                                 \
    if (NULL == CU_add_test(suite, #test_func, test_func)) {                                       \
        CU_cleanup_registry();                                                                     \
        return CU_get_error();                                                                     \
    }

#endif // RUNE_TEST_H
