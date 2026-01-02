# API Design Patterns

This document outlines the core API patterns and philosophy used throughout the rune library.

## Naming Conventions

### Prefix Rules

| Prefix | Scope    | Usage                                          |
|--------|----------|------------------------------------------------|
| `R_`   | Internal | Uppercase macros, constants, and type adapters |
| `r_`   | Internal | Internal function implementations (use `R_()`) |
| (none) | Public   | All public APIs use module-based naming        |

**Internal constructs** use the `R_` prefix for uppercase and `r_` for lowercase implementation functions:

```c
// Internal uppercase macros (exposed in headers)
#define R_MAX(a, b) ((a) > (b) ? (a) : (b))
#define R_VA_ARGS(...) __VA_OPT__(, ) __VA_ARGS__
#define R_GLUE(a, b) R_GLUE_INTERN(a, b)

// Internal function implementation (hidden by R_() macro)
// User sees: str("hello") -> expands to -> R_(str)("hello", opts)
extern char * r__str(const char *data, const str_opt *opt);

// Convenience macro to call internal function
#define str(data, ...) R_(str)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))
```

**Public APIs** use module-based naming without global prefixes:

```c
// String module - public macros call internal implementations
char *s = str("hello");                           // String creation
size_t len = str_len(s);                          // String inspection
bool equal = str_eq(s1, s2);                      // Comparison
char *combined = str_cat(s1, s2, nullptr);       // Transformation
str_free(combined);                               // Memory cleanup

// Collections - generic types via include-trick pattern
#define T int
#include "coll.h"
LIST(int) nums = list(int, 1, 2, 3);
list_add(&nums, 4);
list_free(&nums);

// Error handling
if (err_has()) {
    err_print(stderr);
    err_clear();
}
```

### Module Naming Pattern

All public functions follow the pattern: `module_operation`

```c
str_new()      str_cat()      str_split()
list_new()     list_add()     list_remove()
json_parse()   json_get()     json_free()
```

## Allocator Interface

The allocator provides a customizable interface for system-level memory operations. This allows users to provide custom
allocators (arena, pool, tracking, etc.).

### Structure

```c
typedef struct {
    // Allocate memory: alloc(ctx, size) -> void*
    // Returns pointer to allocated memory, asserts on failure
    void * (*alloc)(void * ctx, size_t size);

    // Reallocate memory: realloc(ctx, ptr, old_size, new_size) -> void*
    // Returns pointer to reallocated memory, asserts on failure
    void * (*realloc)(void * ctx, void * ptr, size_t old_size, size_t new_size);

    // Free memory: free(ctx, ptr, size)
    // Releases memory associated with the pointer
    void (*free)(void * ctx, void * ptr, size_t size);

    // Context pointer passed to all functions
    // Allows stateful allocators (arena state, pool manager, memory tracker, etc.)
    void * ctx;
} allocator;
```

### Usage

Allocators are managed via a stack-based interface. The default allocator (malloc/realloc/free) is automatically used
when no custom allocator is active. Use `alloc_scope()` for automatic push/pop.

```c
// Default allocator - uses malloc/realloc/free
char *s1 = str("hello");

// Custom allocator - using scope for automatic cleanup
allocator arena = {
    .alloc = arena_alloc,
    .realloc = arena_realloc,
    .free = arena_free,
    .ctx = &my_arena,
};

// RAII-style scoped allocator (automatically popped)
alloc_scope(arena) {
    // All allocations within this scope use arena allocator
    char *s2 = str("hello");
    char *s3 = strf("Formatted: %s", s2);
    // Allocator automatically popped here
}
// Restore previous allocator (implicit)

// Manual push/pop also available
alloc_push(arena);
{
    char *temp = str("temporary");
    // ...
}
alloc_pop();  // Restore previous allocator
```

### Design Philosophy

- **Fail-fast on allocation failure** — Allocators assert when unable to fulfill requests. Programming errors should
  crash immediately, not silently fail.
- **No error codes** — The allocator interface doesn't report errors. If allocation fails, the program terminates. This
  keeps APIs clean.
- **Stack-based scoping** — Custom allocators are pushed/popped like a stack, enabling nested allocators and automatic
  cleanup.
- **Stateful allocators** — The `ctx` field supports stateful allocators like arenas, object pools, or memory tracking
  systems.

## Polymorphic Macros with `_Generic`

Public APIs use C11/C23 `_Generic` to enable optional parameters and type-based dispatch.

### Pattern

Each module provides a convenience macro that detects if options were passed:

```c
#define str(data, ...) R_(str)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))

#define strf(first, ...) \
    _Generic((first), \
        const str_opt *: R_(strf)((first)R_VA_ARGS(__VA_ARGS__)), \
        str_opt *: R_(strf)((first)R_VA_ARGS(__VA_ARGS__)), \
        default: R_(strf)(&R_STR_OPTS_DEFAULT, (first)R_VA_ARGS(__VA_ARGS__)) \
    )
```

### Supported Call Patterns

All modules support these calling conventions:

```c
// 1. Minimal - just required arguments (uses default options)
char *s = str("hello");

// 2. With custom options
char *s = str("hello", &(str_opt){.max_len = 256});

// 3. Formatted strings
char *s = strf("Value: %d", 42);

// 4. Formatted strings with custom options
char *s = strf(&(str_opt){.max_len = 256}, "Value: %d", 42);
```

## Options Structs

Each module that requires configuration provides an options struct.

### Pattern

```c
typedef struct {
    const size_t max_len;
    const size_t max_tok;
} str_opt;
```

### Usage with Compound Literals

Options use C99/C23 compound literals for clean inline configuration:

```c
// Inline options
char *s = str("hello", &(str_opt){.max_len = 256});

// Reusable options
str_opt my_opts = {.max_len = 1024};
char *s1 = str("foo", &my_opts);
char *s2 = str("bar", &my_opts);
```

### Default Behavior

When options are omitted, the module uses sensible defaults:

```c
// These are equivalent:
char *s = str("hello");
char *s = str("hello", &R_STR_OPTS_DEFAULT);

// R_STR_OPTS_DEFAULT provides reasonable limits:
// max_len = 4096  (maximum string length)
```

## Constructor Configuration Pattern

Constructor functions use a unified pattern for optional parameters based on `_Generic` type dispatch:

### Configuration Structs

Each collection type defines a configuration struct combining runtime and type-specific options:

```c
typedef struct {
    runtime *rt;                  // Runtime/allocator context (default: nullptr)
    size_t initial_capacity;      // Type-specific: initial capacity (default: 4)
} ListConfig;

#define LIST_CONFIG_DEFAULT { \
    .rt = nullptr, \
    .initial_capacity = 4 \
}

typedef struct {
    runtime *rt;
    size_t initial_capacity;
} MapConfig;

#define MAP_CONFIG_DEFAULT { \
    .rt = nullptr, \
    .initial_capacity = 16 \
}
```

### Configuration Helpers

Each type provides a helper macro that hides the compound literal syntax:

```c
#define listcfg(...) (ListConfig){LIST_CONFIG_DEFAULT, __VA_ARGS__}
#define mapcfg(...) (MapConfig){MAP_CONFIG_DEFAULT, __VA_ARGS__}
```

### Core Dispatch Macro

A single reusable macro handles all the `_Generic` complexity for flexible argument ordering:

```c
// Define once in a common header
#define R_DISPATCH(first, second, config_type, cfg_first, cfg_second, no_cfg) \
    _Generic((first), \
        config_type: (cfg_first), \
        default: _Generic((second), \
            config_type: (cfg_second), \
            default: (no_cfg) \
        ) \
    )
```

### Constructor Macros

Each constructor simply calls `R_DISPATCH` with its three cases:

```c
#define list(first, second, ...) \
    R_DISPATCH(first, second, ListConfig, \
        R_LIST_OF(second, (first).rt, (first).initial_capacity, __VA_ARGS__, R_END), \
        R_LIST_OF(first, (second).rt, (second).initial_capacity, __VA_ARGS__, R_END), \
        R_LIST_OF(first, nullptr, LIST_CONFIG_DEFAULT.initial_capacity, (second), __VA_ARGS__, R_END))

#define map(first, second, ...) \
    R_DISPATCH(first, second, MapConfig, \
        R_MAP_OF(second, (first).rt, (first).initial_capacity, __VA_ARGS__, R_END), \
        R_MAP_OF(first, (second).rt, (second).initial_capacity, __VA_ARGS__, R_END), \
        R_MAP_OF(first, nullptr, MAP_CONFIG_DEFAULT.initial_capacity, (second), __VA_ARGS__, R_END))
```

### User API

Users enjoy flexible argument ordering and clean syntax:

```c
// Type first, no config (defaults)
LIST(int) nums = list(int, 1, 2, 3, 4, 5);

// Type first, config second
LIST(int) optimized = list(int, listcfg(.initial_capacity = 100, .rt = &arena), 1, 2, 3);

// Config first, type second (flexible ordering)
LIST(int) flexible = list(listcfg(.initial_capacity = 100, .rt = &arena), int, 1, 2, 3);

// Same pattern for maps
MAP(str, int) data = map(str, int, mapcfg(.initial_capacity = 256), ...);
MAP(str, int) data = map(mapcfg(.initial_capacity = 256), str, int, ...);
```

### Key Benefits

- **Single dispatch macro** — `R_DISPATCH` eliminates boilerplate across all types
- **Flexible argument ordering** — Config and type can appear in either order
- **Clean constructor definitions** — Each type's macro is concise and DRY
- **Type-safe dispatch** — `_Generic` handles all type detection
- **Unified configuration** — Consistent config pattern across library
- **Hidden syntax** — Helpers (`listcfg`, `mapcfg`) hide complexity from users
- **Extensible** — Adding new types requires minimal code

This pattern scales across all collection and container types in the library.

## Module Structure

Each module follows a consistent file organization:

```
src/
  str.h      // Public API declarations
  str.c      // Implementation
  list.h     // Public API declarations
  list.c     // Implementation
```

### Header Structure

```c
#ifndef RUNE_STR_H
#define RUNE_STR_H

#include "core/rune.h"

// Options struct
typedef struct { ... } str_opts;

// Core function declarations
[[nodiscard]]
extern char *r_str_new(runtime *rt, const char *s, const str_opts *opts);

// Convenience macros
#define str(first, ...) _Generic((first), ...)

// Additional operations
#define str_cat(first, ...) ...
#define str_len(s, ...) ...

#endif // RUNE_STR_H
```

## Memory Management

### Allocation

All allocating functions are marked `[[nodiscard]]` to prevent accidentally ignoring allocation failures:

```c
[[nodiscard]]
extern char * R_(str)(const char * restrict data, const str_opt * opt);

// Compiler warning if return value is ignored:
str("data");  // ⚠️ Warning: ignoring return value marked nodiscard
```

### Deallocation

Each module provides free functions that:

- Accept `nullptr` safely (no-op)
- Handle cleanup of module-specific metadata
- Don't require a return value assignment

```c
// String module
str_free(s);              // Free single string
str_free_arr(arr);        // Free nullptr-terminated array

// Collections
list_free(&lst);          // Free list in-place
```

### Allocator Consistency

Objects must be freed using the same allocator they were allocated with. The active allocator at free time should match
the allocator at allocation time:

```c
allocator arena = arena_create(4096);

alloc_scope(arena) {
    char *s = str("hello");
    // ... use s ...
    str_free(s);  // Frees via arena allocator
    // allocator automatically popped here
}

// All memory automatically freed when arena is destroyed
arena_destroy(&arena);
```

## Error Handling

The rune library uses **thread-local error context** to decouple error details from function signatures. This approach
keeps APIs clean while providing rich diagnostic information for debugging.

### Design Philosophy

- **No error return codes** — Functions return meaningful values (nullptr, or actual results)
- **No out-parameters** — Error details don't clutter function signatures
- **Optional diagnostics** — Consumers can check error details if needed
- **Thread-safe** — Each thread has its own error stack
- **Zero cost** — Can be disabled in performance-critical paths

### Error Codes

Error codes are organized by category for easy identification:

```c
// Success
R_ERR_OK = 0

// Memory errors (1-99)
R_ERR_ALLOC_FAILED = 1
R_ERR_OUT_OF_MEMORY = 2
R_ERR_ALLOCATOR_STACK_OVERFLOW = 3
R_ERR_ALLOCATOR_STACK_UNDERFLOW = 4

// Validation errors (100-199)
R_ERR_NULL_POINTER = 100
R_ERR_INVALID_ARGUMENT = 101
R_ERR_INVALID_LENGTH = 102
R_ERR_EMPTY_INPUT = 103

// Overflow/capacity errors (200-299)
R_ERR_OVERFLOW = 200
R_ERR_BUFFER_OVERFLOW = 201
R_ERR_LENGTH_EXCEEDED = 202
R_ERR_CAPACITY_EXCEEDED = 203

// Not found/empty errors (300-399)
R_ERR_NOT_FOUND = 300
R_ERR_PATTERN_NOT_FOUND = 301
R_ERR_INDEX_OUT_OF_BOUNDS = 302

// Collection errors (400-499)
R_ERR_QUEUE_FULL = 400
R_ERR_QUEUE_EMPTY = 401
R_ERR_LIST_EMPTY = 402

// Format/parse errors (500-599)
R_ERR_FORMAT_FAILED = 500
R_ERR_PARSE_FAILED = 501

// String operation errors (600-699)
R_ERR_STRING_TOO_LONG = 600
R_ERR_INVALID_UTF8 = 601
```

### Basic Usage

Check for errors after operations that can fail:

```c
char *s = str("hello");
if (s == nullptr) {
    fprintf(stderr, "Error: %s\n", err_msg());
    return 1;
}
```

### Detailed Error Information

Get full context when needed:

```c
char *s = str_cat("a", "b", "c", nullptr);
if (s == nullptr) {
    const r_error_ctx *err = err_get();
    fprintf(stderr, "Error [%d]: %s\n", err->code, err->message);
    fprintf(stderr, "  at %s:%d in %s()\n", err->file, err->line, err->func);
    return 1;
}
```

### Error Stack Traces

For nested operations, view the full error chain:

```c
if (process_data() == nullptr) {
    err_print_stack(stderr);  // Shows full error history
    return 1;
}
```

### Checking Error Codes

Handle different error types appropriately:

```c
char *s = str_repeat("x", 1000000);
if (s == nullptr) {
    if (err_code() == R_ERR_LENGTH_EXCEEDED) {
        fprintf(stderr, "String too long, using shorter version\n");
        s = str_repeat("x", 100);  // Retry with smaller size
    } else if (err_code() == R_ERR_OUT_OF_MEMORY) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }
}
```

### Clearing Errors

Clear error state when handling or recovering from errors:

```c
char *data = load_config("config.txt");
if (data == nullptr) {
    err_clear();  // Clear error - we handled it
    data = get_default_config();  // Use fallback
}
```

### Performance-Critical Code

Disable error tracking in hot paths for zero overhead:

```c
err_enable(false);

for (int i = 0; i < 1000000; i++) {
    // Fast path without error tracking overhead
    process_item(i);
}

err_enable(true);
```

### API Consistency

All error-returning functions follow the same pattern:

- **String functions** — Return nullptr on error
- **Allocator functions** — Set error and assert (fail-fast for bugs)
- **Collection operations** — Return zero-initialized value on error (e.g., empty struct)

No error codes are part of the API signature. All error context is thread-local and optional.

## C23 Features Used

The library leverages modern C23 features:

| Feature                    | Usage                          |
|----------------------------|--------------------------------|
| `nullptr`                  | Null pointer constant          |
| `[[nodiscard]]`            | Enforce checking return values |
| `typeof` / `typeof_unqual` | Type inference in macros       |
| `auto`                     | Type inference for locals      |
| `__VA_OPT__`               | Conditional variadic expansion |
| `_Generic`                 | Type-based dispatch            |
| `constexpr`                | Compile-time constants         |
| Compound literals          | Inline struct initialization   |

## Example: Complete Module Usage

```c
#include "r.h"
#include "str.h"
#include "coll.h"

#include <stdio.h>

int main(void) {
    // Create strings
    char *greeting = str("Hello, World!");
    char *name = str("Alice");

    // String operations
    char *message = str_cat(greeting, " My name is ", name, nullptr);
    char **words = str_split(message, " ");

    // String with custom options
    char *limited = str("very long string", &(str_opt){.max_len = 10});

    // Custom allocator with RAII-style scope
    allocator arena = {/* custom allocator functions */};
    alloc_scope(arena) {
        char *temp = str("temporary");
        // Uses arena allocator
        // Allocator automatically popped on scope exit
    }

    // Collections example
    #define T int
    #include "coll.h"

    LIST(int) numbers = list(int, 1, 2, 3, 4, 5);
    list_add(&numbers, 6);

    printf("First number: %d\n", list_get(&numbers, 0));

    list_free(&numbers);

    // Error checking
    if (err_has()) {
        err_print(stderr);
        err_clear();
    }

    // Cleanup
    str_free(greeting);
    str_free(name);
    str_free(message);
    str_free_arr(words);
    str_free(limited);

    return 0;
}
```
