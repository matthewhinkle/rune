cmake_minimum_required(VERSION 3.31)
project(rune C)

set(CMAKE_C_STANDARD 23)

# ================================================================================================
# rune target
# ================================================================================================

# Find SDL3 package
find_package(SDL3 CONFIG REQUIRED)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find CUnit
find_package(unofficial-cunit CONFIG REQUIRED)

# Note: Old source files excluded - they will be replaced with new implementations
# set(RUNE_SRC
#         src/coll.h
#         src/str.c
#         src/str.h
#         src/std.c
#         src/std.h
# )

# Core rune header files
set(RUNE_SRC
        src/r.h
        src/r.c
        src/coll.h
        src/str.h
        src/str.c
        src/r_test.h
)

# Add the executable
add_executable(rune src/main.c ${RUNE_SRC})

# Link SDL3 and Vulkan libraries
target_link_libraries(rune PRIVATE SDL3::SDL3 Vulkan::Vulkan)

# ------------
# test targets
# ------------

# Test executable for rune.h core functionality (22 tests, 217 assertions)
add_executable(test_rune test/test_r.c src/r.c)
target_link_libraries(test_rune PRIVATE unofficial::cunit::cunit)

# Test executable for coll.h collections (40 tests)
add_executable(test_coll test/test_coll.c src/r.c)
target_link_libraries(test_coll PRIVATE unofficial::cunit::cunit)

# Test executable for str.h string module (coverage pending)
add_executable(test_str test/test_str.c ${RUNE_SRC})
target_link_libraries(test_str PRIVATE unofficial::cunit::cunit)

# Custom target to run all tests
add_custom_target(run_tests
        COMMAND test_rune
        COMMAND test_coll
        COMMAND test_str
        DEPENDS test_rune test_coll test_str
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running unit tests . . ."
)

# Run all tests automatically after build
add_dependencies(rune run_tests)

