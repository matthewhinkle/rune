cmake_minimum_required(VERSION 3.31)
project(rune C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Suppress macro naming convention warnings across all compilers
if (MSVC)
    add_compile_options(/wd4668)  # Undefined preprocessor identifier
else ()
    add_compile_options(-Wno-reserved-identifier)  # GCC/Clang: macro names like 'scope'
endif ()

# ================================================================================================
# Build options
# ================================================================================================

option(ENABLE_SDL3 "Enable SDL3 support" OFF)
option(ENABLE_VULKAN "Enable Vulkan support" OFF)

# ================================================================================================
# rune target
# ================================================================================================

# Find SDL3 package (optional)
if (ENABLE_SDL3)
    find_package(SDL3 CONFIG REQUIRED)
endif ()

# Find Vulkan (optional)
if (ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
endif ()

# Find CUnit - portable across platforms
find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h)
find_library(CUNIT_LIBRARY NAMES cunit)

if (CUNIT_INCLUDE_DIR AND CUNIT_LIBRARY)
    set(CUNIT_INCLUDE_DIRS ${CUNIT_INCLUDE_DIR})
    set(CUNIT_LIBRARIES ${CUNIT_LIBRARY})
else ()
    message(FATAL_ERROR "CUnit not found. Please install it:\n"
            "  macOS: brew install cunit\n"
            "  Linux: sudo apt-get install libcunit1-dev (Ubuntu) or equivalent\n"
            "  Windows: vcpkg install cunit or download from https://cunit.sourceforge.net/")
endif ()

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
        src/hash.h
        src/hash.c
)

# Add the executable
add_executable(rune src/main.c ${RUNE_SRC}
        src/hash.h
        src/hash.c
        src/tree.h
        src/map.c
        src/map.h
)

# Link optional libraries
if (ENABLE_SDL3)
    target_link_libraries(rune PRIVATE SDL3::SDL3)
endif ()

if (ENABLE_VULKAN)
    target_link_libraries(rune PRIVATE Vulkan::Vulkan)
endif ()

# ================================================================================================
# tool targets
# ================================================================================================

# Header formatting tool
add_executable(fmtheader tool/build/fmtheader.c)

# Expand glob patterns for cross-platform compatibility
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Auto-format headers on build (optional - doesn't fail the build if it errors)
if (NOT WIN32)
    add_custom_target(fmtheader_run ALL
            COMMAND bash -c "$<TARGET_FILE:fmtheader> ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c || echo 'Warning: fmtheader failed (non-fatal)'"
            DEPENDS fmtheader
            COMMENT "Formatting source file headers (optional)..."
    )
else ()
    add_custom_target(fmtheader_run ALL
            COMMAND $<TARGET_FILE:fmtheader> ${HEADER_FILES} ${SOURCE_FILES}
            DEPENDS fmtheader
            COMMENT "Formatting source file headers (optional)..."
    )
endif ()

# Make sure fmtheader_run runs early
add_dependencies(rune fmtheader_run)

# ------------
# test targets
# ------------

# Test executable for rune.h core functionality (22 tests, 217 assertions)
add_executable(test_rune test/test_r.c src/r.c)
target_include_directories(test_rune PRIVATE ${CUNIT_INCLUDE_DIRS})
target_link_libraries(test_rune PRIVATE ${CUNIT_LIBRARIES})

# Test executable for coll.h collections (40 tests)
add_executable(test_coll test/test_coll.c src/r.c)
target_include_directories(test_coll PRIVATE ${CUNIT_INCLUDE_DIRS})
target_link_libraries(test_coll PRIVATE ${CUNIT_LIBRARIES})

# Test executable for tree.h red-black tree module
add_executable(test_tree test/test_tree.c src/r.c)
target_include_directories(test_tree PRIVATE ${CUNIT_INCLUDE_DIRS})
target_link_libraries(test_tree PRIVATE ${CUNIT_LIBRARIES})
# Suppress shadow warnings from complex rbt_remove macro expansions
target_compile_options(test_tree PRIVATE -Wno-shadow)

# Test executable for str.h string module (coverage pending)
add_executable(test_str test/test_str.c ${RUNE_SRC})
target_include_directories(test_str PRIVATE ${CUNIT_INCLUDE_DIRS})
target_link_libraries(test_str PRIVATE ${CUNIT_LIBRARIES})

# Test executable for hash.h hash module
add_executable(test_hash test/test.h test/test_hash.c src/r.c src/str.c src/hash.c)
target_include_directories(test_hash PRIVATE ${CUNIT_INCLUDE_DIRS})
target_link_libraries(test_hash PRIVATE ${CUNIT_LIBRARIES})

# Custom target to run all tests
add_custom_target(run_tests
        COMMAND test_rune
        COMMAND test_coll
        COMMAND test_tree
        COMMAND test_str
        COMMAND test_hash
        DEPENDS test_rune test_coll test_tree test_str test_hash
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running unit tests . . ."
)

# Run all tests automatically after build
add_dependencies(rune run_tests)

