/*
 * str tests.
 */

#include "../src/str.h"
#include "CUnit/Basic.h"

#include "test.h"

static void noop() {
}

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // ReSharper disable once CppLocalVariableMayBeConst
    CU_pSuite suite = CU_add_suite("str_tests", nullptr, nullptr);
    if (suite == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(suite, noop);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

#ifdef RUN_TESTS
#undef RUN_TESTS
#endif // RUN_TESTS
