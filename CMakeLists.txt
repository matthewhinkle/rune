cmake_minimum_required(VERSION 3.31)
project(rune C)

set(CMAKE_C_STANDARD 17)

# -----------
# rune target
# -----------

# Find SDL3 package
find_package(SDL3 CONFIG REQUIRED)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find CUnit
find_package(unofficial-cunit CONFIG REQUIRED)

# Add the executable
add_executable(rune src/main.c
        src/std.h
        src/std.c
        src/coll.h
        src/str.c
        src/str.h
        test/test.h
)

# Link SDL3 and Vulkan libraries
target_link_libraries(rune PRIVATE SDL3::SDL3 Vulkan::Vulkan)

# ----------------
# rune_test target
# ----------------

# Add the test executable
add_executable(rune_test test/test_str.c
        src/std.h
        src/std.c
        src/coll.h
        src/str.c
        src/str.h
)

# Link CUnit library to the test executable
target_link_libraries(rune_test PRIVATE unofficial::cunit::cunit)

# Add a custom target to run the tests
add_custom_target(run_tests
        COMMAND rune_test
        DEPENDS rune_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running unit tests . . ."
)

# Make `run_tests` run after the build
add_dependencies(rune run_tests)

