# Rune Architecture

This document describes the overall architecture and module organization of the rune library.

## Project Structure

```
rune/
├── docs/                    # Documentation
│   ├── API_DESIGN.md        # API patterns and philosophy
│   ├── ARCHITECTURE.md      # This file
│   ├── CONVENTIONS.md       # Coding standards
│   ├── TESTING.md           # Testing framework and conventions
│   └── DEVELOPMENT.md       # Contributing guide
├── src/
│   ├── r.h / r.c            # Core runtime, error handling, allocator (header + implementation)
│   ├── str.h / str.c        # String module
│   ├── coll.h               # Generic collections (list, queues)
│   ├── r_test.h             # Test-only internal APIs (for test code)
│   └── main.c               # Application entry point
├── test/
│   ├── test.h               # Shared test utilities and macros
│   ├── test_r.c             # Core runtime tests (22 tests)
│   ├── test_coll.c          # Collections tests (40 tests)
│   └── test_str.c           # String module tests
├── scripts/
│   └── normalize_headers.py  # Header normalization utility
└── CMakeLists.txt           # Build configuration
```

## Module Organization

### Module Layout

All core modules are located directly in `src/` for simplicity. As the library grows, domain-specific modules can be
organized into subdirectories while maintaining the same include pattern.

### Include Paths

Includes are straightforward, reflecting the module name:

```c
#include "r.h"          // Core runtime
#include "str.h"        // String module
#include "coll.h"       // Collections (requires T type definition)
#include "r_test.h"     // Test-only APIs (test code only)
```

## Core Modules

Foundational modules that provide the base of the library system.

### Dependency Graph

```
┌─────────────────────────────────────────────────┐
│                    r.h                          │
│    (error handling, allocator, core macros)     │
└─────────────────────────────────────────────────┘
          ▲              ▲
          │              │
    ┌─────┴─────┐  ┌─────┴─────┐
    │   str.h   │  │  coll.h   │
    │ (strings) │  │(generic   │
    │           │  │collections)
    └───────────┘  └───────────┘
```

### Module Descriptions

#### `r.h` / `r.c`

The foundation module providing core runtime utilities, thread-local error handling, and memory allocation.

**Structure:**

1. **System includes** — Standard C library headers
2. **Macros** — All preprocessing directives and macros grouped together:
    - Token manipulation: `R_GLUE`, `R_JOIN`, `R_UNIQUE`, `R_STRINGIFY`
    - Numeric utilities: `R_MAX`, `R_MIN`, `R_CLAMP`
    - Scope management: `scope()`, `alloc_scope()` for RAII-style resource management
    - Type adapters: `R_Atomic()` for atomic types
    - Function argument handling: `R_OPT()` for optional parameters
3. **Constants** — `NULLTERM`, `SOH`, `STX`, `ETX` (control characters)
4. **Type definitions**:
    - `allocator` — Customizable memory operations interface
    - `r_error_code` — Error code enumeration
    - `r_error_ctx` — Error context with location information
5. **Error stack** — Thread-local error management with automatic location capture
6. **Allocator stack** — Thread-local allocator stack with push/pop semantics
7. **Memory functions** — `mem_alloc`, `mem_alloc_zero`, `mem_realloc`, `mem_free`

**Core Features:**

- **Thread-local error handling** — Automatic file/line/function capture via macros
- **Error stack** — Full error history with configurable depth (default: 8)
- **Allocator interface** — Separate alloc/realloc/free function pointers
- **Allocator stack** — Scope-based custom allocator switching (via `alloc_scope()`)
- **Scoped resources** — Generic `scope()` macro for RAII-style management

```c
typedef struct {
    void * (*alloc)(void * ctx, size_t size);
    void * (*realloc)(void * ctx, void * ptr, size_t old_size, size_t new_size);
    void (*free)(void * ctx, void * ptr, size_t size);
    void * ctx;
} allocator;

typedef enum {
    R_ERR_OK = 0,
    // Memory errors (1-99)
    R_ERR_ALLOC_FAILED = 1,
    R_ERR_OUT_OF_MEMORY = 2,
    R_ERR_ALLOCATOR_STACK_OVERFLOW = 3,
    R_ERR_ALLOCATOR_STACK_UNDERFLOW = 4,
    // ... more error codes
} r_error_code;
```

**Quick Reference:**

```c
// Error API
err_set(code, message)       // Set error with auto location
err_get()                    // Get most recent error
err_code()                   // Get error code
err_msg()                    // Get error message
err_has()                    // Check if error is set
err_clear()                  // Clear error stack
err_print(stream)            // Print error to stream
err_print_stack(stream)      // Print full error stack

// Allocator API
alloc_push(a)                // Push custom allocator
alloc_pop()                  // Pop allocator
alloc_current()              // Get current allocator
mem_alloc(size)              // Allocate memory
mem_alloc_zero(count, size)  // Allocate zero-initialized memory
mem_realloc(ptr, old, new)   // Reallocate memory
mem_free(ptr, size)          // Free memory

// Scope management
alloc_scope(allocator) { ... }  // Automatically push/pop allocator
```

#### `str.h` / `str.c`

Managed string handling with efficient hashing and UTF-8 support. Strings are stored with metadata including length,
capacity, and cached hash value.

**Features:**

- **Creation** — `str()`, `strf()` (formatted strings)
- **Inspection** — `str_is()`, `str_len()`, `str_size()`, `str_hash()` (cached)
- **Comparison** — `str_cmp()`, `str_eq()` (uses hash for fast rejection)
- **Search** — `str_find()`, `str_rfind()`
- **Transformation** — `str_cat()`, `str_split()`, `str_join()`, `str_repeat()`, `str_replace()`
- **Memory** — `str_free()`, `str_free_arr()`

**Design:**

- All managed strings include header with metadata (length, capacity, hash)
- Strings are validated with boundary markers (SOH, STX, ETX) to detect corruption
- FNV-1a hashing for fast comparison and caching
- Configurable limits: `R_STR_MAX_LEN` (4096), `R_STR_STACK_MAX` (8KB), `R_STR_MAX_VARG` (64)

```c
char *s = str("hello");
char *msg = strf("Hello, %s!", "World");
char *combined = str_cat(s, " ", msg, nullptr);
str_free(combined);
str_free(msg);
str_free(s);
```

#### `coll.h`

Generic collections using the C "include trick" with `#ifdef T` pattern. Define a type macro `T` before including to
instantiate a collection type.

**Collections:**

- **`LIST(T)`** — Dynamic array with automatic growth, shrinking, and element access
    - Operations: `list()` (constructor), `list_add()`, `list_insert()`, `list_remove()`, `list_get()`, `list_resize()`,
      `list_grow()`, `list_shrink()`, `list_free()`
    - Automatic capacity doubling on overflow, halving on underutilization

**Usage Pattern:**

```c
// Define type before include
#define T int
#include "coll.h"

// Create and use list
LIST(int) nums = list(int, 1, 2, 3, 4, 5);
list_add(&nums, 6);
int value = list_get(&nums, 0);
list_remove(&nums, 0);
list_free(&nums);

// Use different type - requires new instantiation
#undef T
#define T char*
#include "coll.h"

LIST(char*) strings = list(char*, "hello", "world");
```

## Build Configuration

### CMake Structure

```cmake
# Requirements
cmake_minimum_required(VERSION 3.31)
project(rune C)
set(CMAKE_C_STANDARD 23)

# External dependencies
find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-cunit CONFIG REQUIRED)
find_package(Python3 REQUIRED)

# Header normalization (normalizes section dividers to 100 chars)
add_custom_target(normalize_headers
        COMMAND ${Python3_EXECUTABLE} scripts/normalize_headers.py
        --width 100
        src/r.h
)

# Main executable
add_executable(rune src/main.c src/r.h src/r.c src/str.h src/str.c src/coll.h src/r_test.h)
target_link_libraries(rune PRIVATE SDL3::SDL3 Vulkan::Vulkan)
add_dependencies(rune normalize_headers)

# Test executables
add_executable(test_rune test/test_r.c src/r.c)
target_link_libraries(test_rune PRIVATE unofficial::cunit::cunit)

add_executable(test_coll test/test_coll.c src/r.c)
target_link_libraries(test_coll PRIVATE unofficial::cunit::cunit)

add_executable(test_str test/test_str.c src/r.h src/r.c src/str.h src/str.c src/coll.h src/r_test.h)
target_link_libraries(test_str PRIVATE unofficial::cunit::cunit)

# Custom target to run all tests
add_custom_target(run_tests
        COMMAND test_rune
        COMMAND test_coll
        COMMAND test_str
        DEPENDS test_rune test_coll test_str
)

# Tests run automatically after build
add_dependencies(rune run_tests)
```

### Dependencies

| Dependency | Purpose            | Version |
|------------|--------------------|---------|
| SDL3       | Graphics/windowing | Latest  |
| Vulkan     | Graphics rendering | Latest  |
| CUnit      | Unit testing       | 2.x     |
| Python3    | Build utilities    | 3.x     |
| C standard | Core functionality | C23     |

## Design Principles

### 1. Minimal Dependencies

Core modules depend only on the C standard library. External dependencies are isolated to specific domains.

### 2. Optional Runtime

All modules accept an optional `runtime *` parameter. When `nullptr`, they use the default allocator. This allows:

- Simple usage without configuration
- Custom allocators when needed
- Arena allocators for performance
- Memory tracking for debugging

### 3. Compile-Time Configuration

Defaults can be overridden at compile time:

```c
#define RUNE_CFG__STR_MAX_LEN 8192  // Override default before include
#include "core/str.h"
```

### 4. Zero-Cost Abstractions

Macros provide ergonomic APIs without runtime overhead:

```c
// User writes:
char *s = str("hello");

// Expands to:
char *s = r_str_new(nullptr, "hello", nullptr);
```

### 5. Fail-Safe Defaults

- Functions accept `nullptr` gracefully
- Default options are sensible for most use cases
- Memory functions assert on failure (programming error)

## Extensibility

The architecture supports future expansion:

- **New modules** — Add `.h`/`.c` pairs in `src/` for new functionality
- **Domain organization** — As complexity grows, organize related modules into subdirectories
- **Collection instantiation** — New generic types can be created by defining `T` and including `coll.h`
- **Custom allocators** — The `allocator` interface supports custom memory management strategies
- **Error handling** — New error codes can be added to `r_error_code` enum with corresponding messages
- **Configuration** — Compile-time configuration via `RCFG__*` macros for limits and behavior
