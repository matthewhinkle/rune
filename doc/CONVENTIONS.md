# Rune Coding Conventions

This document defines the coding standards and conventions used throughout the rune library.

## Naming Conventions

### Prefixes

| Prefix | Scope                               | Example                                 |
|--------|-------------------------------------|-----------------------------------------|
| `R_`   | Internal uppercase macros/constants | `R_MAX`, `R_VA_ARGS`, `R_CLAMP`         |
| `r_`   | Internal lowercase functions/macros | `mem_alloc`, `r_str_new`, `mem_alloc_t` |
| (none) | Public macros                       | `str`, `str_cat`, `list_new`            |
| (none) | Public types                        | `runtime`, `str_opts`                   |

### Casing Rules

| Type              | Convention             | Example                       |
|-------------------|------------------------|-------------------------------|
| Macros (internal) | `SCREAMING_SNAKE_CASE` | `R_VA_ARGS`, `R_GENERIC_INTS` |
| Macros (public)   | `snake_case`           | `str_new`, `list_add`         |
| Functions         | `snake_case`           | `r_str_new`, `fnv1a_hash`     |
| Types             | `snake_case`           | `runtime`, `str_opts`         |
| Constants         | `SCREAMING_SNAKE_CASE` | `RUNE_STR_MAX_LEN`            |
| Variables         | `snake_case`           | `max_len`, `total_len`        |
| Struct fields     | `snake_case`           | `.alloc`, `.ctx`, `.max_len`  |

### Module Naming

Public APIs follow the pattern `module_operation`:

```c
// String module
str()           str_cat()       str_len()       str_find()
str_split()     str_join()      str_replace()   str_free()

// List module
list()          list_new()      list_add()      list_get()
list_remove()   list_free()

// JSON module
json_parse()    json_get()      json_free()
```

### Conventional Variable Names

| Variable | Type        | Usage                     |
|----------|-------------|---------------------------|
| `rt`     | `runtime *` | Runtime context parameter |
| `opts`   | `*_opts *`  | Options struct parameter  |
| `len`    | `size_t`    | Length of data            |
| `n`      | `size_t`    | Count or number of items  |
| `i`, `j` | `size_t`    | Loop indices              |
| `result` | varies      | Return value being built  |
| `ctx`    | `void *`    | Generic context pointer   |

## Code Style

### Function Signatures

Parameters on separate lines when signature exceeds ~100 characters:

```c
// Short - single line
extern char * R_(str)(const char * restrict data, const str_opt * opt);

// Long - split across lines
static rstr * rstr_replace(
    const char * data,
    const char * find,
    const char * repl,
    const size_t max_len,
    int * const restrict lps
);
```

### Parameter Qualifiers

Use `const` for parameters that aren't modified:

```c
extern size_t R_(str_len)(const char * s, const str_opt * opt);
```

Use `restrict` for pointers that don't alias:

```c
extern char * R_(str)(const char * restrict data, const str_opt * restrict opt);
```

### Function Declarations

- Use `extern` for public function declarations in headers
- Use `static` for file-internal functions
- Use `[[nodiscard]]` for all allocating functions
- Use `R_()` macro for internal function names (hides `r_` prefix)

```c
// Header - public macro dispatches to internal implementation
[[nodiscard]]
extern char * R_(str)(const char * restrict data, const str_opt * opt);

#define str(data, ...) R_(str)((data), R_OPT(&R_STR_OPTS_DEFAULT, __VA_ARGS__))

// Implementation (static helper)
[[nodiscard]]
static rstr * rstr_alloc(size_t len);
```

### Constants

Use `static constexpr` for compile-time constants (C23):

```c
static constexpr char SOH = 0x01;
static constexpr size_t MAX_LEN = 4 * 1024;
```

Use `static const` when `constexpr` isn't applicable:

```c
static const char NULLTERM = '\0';
```

## Macro Conventions

### Multi-Statement Macros

Use the `do { ... } while (0)` pattern:

```c
#define R_FUNC(block) \
    do { \
        block \
    } while (0)
```

### Optional Variadic Arguments

Use `__VA_OPT__` (C23) or `, ##__VA_ARGS__` for optional trailing args:

```c
// C23 style (preferred)
#define str_len(s, ...) r_str_len((s) __VA_OPT__(, __VA_ARGS__))

// Legacy style (for compatibility)
#define R_VA_ARGS(...) , ##__VA_ARGS__
```

### Type-Based Dispatch

Use `_Generic` for polymorphic macros:

```c
#define str(first, ...) _Generic((first), \
    runtime*: r_str_new((first), __VA_ARGS__), \
    const runtime*: r_str_new((first), __VA_ARGS__), \
    char*: r_str_new(nullptr, (first) __VA_OPT__(, __VA_ARGS__)), \
    const char*: r_str_new(nullptr, (first) __VA_OPT__(, __VA_ARGS__)), \
    default: r_str_new(nullptr, (first) __VA_OPT__(, __VA_ARGS__)) \
)
```

### Optional Constructor Parameters

All constructor functions support optional configuration parameters with flexible argument ordering using a unified
dispatch pattern.

**1. Define configuration structs for each type:**

Each collection type has a corresponding config struct containing both runtime and type-specific options:

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

**2. Define configuration helper macros:**

Each type has a module-specific helper that hides the compound literal cast:

```c
#define listcfg(...) (ListConfig){LIST_CONFIG_DEFAULT, __VA_ARGS__}
#define mapcfg(...) (MapConfig){MAP_CONFIG_DEFAULT, __VA_ARGS__}
```

**3. Use the core dispatch macro for flexible argument ordering:**

Define a reusable core dispatch macro that handles the nested `_Generic` logic:

```c
// Core dispatch macro - handles flexible argument ordering
#define R_DISPATCH(first, second, config_type, cfg_first, cfg_second, no_cfg) \
    _Generic((first), \
        config_type: (cfg_first), \
        default: _Generic((second), \
            config_type: (cfg_second), \
            default: (no_cfg) \
        ) \
    )

// Each constructor just specifies its three cases
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

**Usage with flexible argument ordering:**

```c
// Type first, no config
list(int, 1, 2, 3, 4)

// Type first, config second
list(int, listcfg(.initial_capacity = 100, .rt = &arena), 1, 2, 3)

// Config first, type second
list(listcfg(.initial_capacity = 100, .rt = &arena), int, 1, 2, 3)

// Same flexibility for maps
map(str, int, mapcfg(.initial_capacity = 256), "key", 42)
map(mapcfg(.initial_capacity = 256), str, int, "key", 42)
```

**Design rationale:**

- **Single dispatch macro** — `R_DISPATCH` handles all the `_Generic` complexity once
- **Flexible argument order** — Config and type can appear in either order
- **DRY pattern** — Each constructor definition is concise, avoiding code duplication
- **Type-safe** — `_Generic` detects types at compile-time
- **Unified configuration** — Both system (runtime) and type-specific options in one struct
- **Hidden casting** — Config helpers (`listcfg`, `mapcfg`) hide compound literal syntax
- **Extensible** — New types just call `R_DISPATCH` with their parameters

**Apply this pattern to all constructor functions** to provide a consistent, maintainable interface across the library.

### Formatting Control

Use clang-format directives for complex macros:

```c
// clang-format off
#define R_GENERIC_INTS(expr) \
    signed char: expr, \
    char: expr, \
    short: expr, \
    int: expr, \
    ...
// clang-format on
```

## Memory Management

### Allocation Functions

- First parameter is always `runtime *rt` (can be `nullptr`)
- Return `nullptr` on failure (don't crash)
- Mark with `[[nodiscard]]`

```c
[[nodiscard]]
extern void *mem_alloc(runtime *rt, size_t size);
```

### Free Functions

- Accept `nullptr` safely (no-op)
- Return `nullptr` for assignment chaining

```c
extern void *mem_free(runtime *rt, void *ptr, size_t size);

// Usage
ptr = mem_free(rt, ptr, size);  // ptr is now nullptr
```

### Assertion Policy

Use `assert()` for programming errors (not runtime errors):

```c
void *mem_alloc(runtime *rt, const size_t size) {
    void *result = rt->alloc(rt->ctx, nullptr, 0, size);
    assert(result != nullptr);  // Programming error if allocator fails
    return result;
}
```

## Error Handling

### Design Principle

The rune library uses **thread-local error context** to decouple error details from function signatures:

- **No return codes** — Functions return meaningful values (nullptr, or actual results)
- **No out-parameters** — Error details don't clutter function signatures
- **Thread-local stack** — Each thread maintains its own error stack
- **Optional diagnostics** — Consumers can check error details if needed
- **Zero overhead** — Can be disabled in performance-critical paths

### Setting Errors

Use the `err_set()` macro to set an error. This automatically captures file, line, and function:

```c
// Set error with default message (derived from error code)
if (allocation_failed) {
    err_set(R_ERR_ALLOC_FAILED);
    return nullptr;
}

// Set error with custom message
if (length_exceeded) {
    err_set(R_ERR_LENGTH_EXCEEDED, "concatenation would exceed max_len");
    return nullptr;
}

// Set error and return in one expression
if (invalid_input) {
    return err_ret(nullptr, R_ERR_INVALID_ARGUMENT);
}
```

### Checking Inputs with Error Context

Use convenience macros for common input validation patterns:

```c
// Check for null pointer
if (err_null(data))
    return nullptr;

// Check a condition
if (err_check(len > 0, R_ERR_INVALID_LENGTH))
    return nullptr;

// Combine checks
if (err_null(a) || err_null(b) || err_check(c > 0, R_ERR_INVALID_ARGUMENT))
    return nullptr;
```

### Function Implementation Pattern

Follow this pattern for functions that can fail:

```c
extern char * R_(str_cat)(const str_opt * opt, const char * first, ...) {
    // 1. Normalize options
    if (opt == nullptr)
        opt = &R_STR_OPTS_DEFAULT;

    // 2. Validate critical inputs (set error on failure)
    if (err_null(first))
        return nullptr;

    // 3. Allocate or process
    rstr * result = rstr_alloc(total_len);
    if (result == nullptr) {
        err_set(R_ERR_ALLOC_FAILED);
        return nullptr;
    }

    // 4. Build result
    // ... copy data ...

    return result->data;
}
```

### Clearing Error State

Clear errors after handling them to avoid stale error context:

```c
// When you recover from an error
char * data = load_config("config.txt");
if (data == nullptr) {
    err_clear();  // Clear the error - we've handled it
    data = get_default_config();  // Use fallback
}
```

In functions that call other functions that may set errors but treat those errors as non-fatal:

```c
// str_replace() calls str_find() which sets R_ERR_PATTERN_NOT_FOUND
// But not finding the pattern is normal, so we clear the error
while ((p = R_(str_find)(p, target, opt)) != nullptr) {
    count++;
    p += t_len;
}
err_clear();  // Not finding pattern is OK, clear it
```

### Documenting Error Conditions

Document which errors a function can set in its comment:

```c
/**
 * Create a string by concatenating multiple strings.
 *
 * Sets error context on failure:
 * - R_ERR_NULL_POINTER: first argument is nullptr
 * - R_ERR_LENGTH_EXCEEDED: result would exceed max_len
 * - R_ERR_ALLOC_FAILED: memory allocation failed
 *
 * @param opt options struct (nullptr uses defaults)
 * @param first first string (required)
 * @param ... additional strings (variadic, terminated by nullptr)
 * @return concatenated string, or nullptr on error
 */
extern char *R_(str_cat)(const str_opt *opt, const char *first, ...);
```

### Error Handling in Consumer Code

Three patterns for consumers:

```c
// 1. Simple check - just check for nullptr
char *s = str("hello");
if (s == nullptr) {
    fprintf(stderr, "Failed to create string\n");
    return 1;
}

// 2. Check error code for recovery
char *s = str_repeat("x", huge_number);
if (s == nullptr) {
    if (r_err_code() == R_ERR_LENGTH_EXCEEDED) {
        fprintf(stderr, "String too long, using shorter version\n");
        s = str_repeat("x", 100);
    }
}

// 3. Print detailed diagnostic
if (complex_operation() == nullptr) {
    fprintf(stderr, "Operation failed:\n");
    r_err_print(stderr);  // Prints: Error [code]: message at file:line in func()
    return 1;
}

// 4. Full error stack trace for nested failures
if (high_level_operation() == nullptr) {
    r_err_print_stack(stderr);  // Shows all errors in the call stack
    return 1;
}
```

### Error Codes

Error codes are organized by category. Common codes:

```c
// Memory errors (1-99)
R_ERR_ALLOC_FAILED              // Allocation returned nullptr
R_ERR_OUT_OF_MEMORY             // Malloc/realloc failed
R_ERR_ALLOCATOR_STACK_OVERFLOW  // Allocator stack depth exceeded
R_ERR_ALLOCATOR_STACK_UNDERFLOW // Allocator stack empty on pop

// Validation errors (100-199)
R_ERR_NULL_POINTER              // Required argument was nullptr
R_ERR_INVALID_ARGUMENT          // Invalid argument value
R_ERR_INVALID_LENGTH            // Invalid length parameter
R_ERR_EMPTY_INPUT               // Input was empty when not allowed

// Overflow errors (200-299)
R_ERR_LENGTH_EXCEEDED           // Result length exceeds max_len
R_ERR_CAPACITY_EXCEEDED         // Capacity exceeded

// Not found errors (300-399)
R_ERR_PATTERN_NOT_FOUND         // Pattern not found in search
R_ERR_NOT_FOUND                 // Item not found

// Collection errors (400-499)
R_ERR_QUEUE_FULL                // Queue has no capacity
R_ERR_QUEUE_EMPTY               // Queue has no items
R_ERR_LIST_EMPTY                // List has no items

// Format errors (500-599)
R_ERR_FORMAT_FAILED             // vsnprintf failed or exceeded limit

// String errors (600-699)
R_ERR_STRING_TOO_LONG           // String exceeds maximum length
R_ERR_INVALID_UTF8              // Invalid UTF-8 sequence
```

### Performance: Disabling Error Tracking

For performance-critical code paths, disable error tracking temporarily:

```c
r_err_enable(false);

// Hot loop with no error tracking overhead
for (int i = 0; i < 1000000; i++) {
    process_item(i);  // No error context overhead
}

r_err_enable(true);
```

### Return Patterns by Type

Different return types follow consistent patterns:

**Pointer-returning functions:**

```c
// Return nullptr on error, set error context
if (condition_invalid) {
    r_err_set(R_ERR_CODE);
    return nullptr;
}
```

**Void-returning functions:**

```c
// Set error and return early
if (condition_invalid) {
    r_err_ret_void(R_ERR_CODE);
    return;  // Part of r_err_ret_void macro
}
```

**Value-returning functions (collections, etc.):**

```c
// Return zero-initialized value on error
if (queue_full) {
    r_err_set(R_ERR_QUEUE_FULL);
    return (typeof(item)){0};  // Zero-initialized
}
```

### Thread Safety

Error context is completely thread-local:

- Each thread has its own error stack
- No locking required
- Errors in one thread don't affect others
- Safe for use in concurrent code

## Documentation

Documentation philosophy: **Only comment where necessary**. Well-named functions and modules are self-documenting.
Comments should explain the "why" and edge cases, not the obvious "what".

### Comment Style

Three styles serve different purposes:

| Style        | Purpose                             | Usage                                                                 |
|--------------|-------------------------------------|-----------------------------------------------------------------------|
| `/** ... */` | Public module/API documentation     | Module-level contract; function parameters with esoteric requirements |
| `//`         | Implementation notes for developers | Section headers, inline explanations, design rationale                |
| (No comment) | Self-documenting code               | Default for well-named functions and clear logic                      |

### Module-Level Documentation

Each header starts with `/** ... */` describing what the module provides. This is the **public API contract** and should
be comprehensive—it's what users read to understand the module.

```c
/**
 * C string wrapper.
 *
 * Provides:
 *   - Safe string allocation with configurable max length
 *   - Efficient string operations (concatenation, splitting, searching)
 *   - UTF-8 validation and manipulation
 *
 * Quick Reference:
 *   str_new()       Create a new string
 *   str_cat()       Concatenate multiple strings
 *   str_len()       Get string length
 *   stmem_free()      Free allocated string
 *
 * All `char *` and `const char *` data returned by this API are compatible
 * with standard C string usage unless otherwise stated.
 */
```

### Function Documentation

Use `/** ... */` for functions when:

- Parameters have esoteric or non-obvious requirements
- Return values need explanation beyond the type
- Error conditions should be documented

Omit doc comments for functions where:

- The name clearly describes what it does
- Parameters are straightforward
- No special error handling is needed

Example with documentation (parameters need explanation):

```c
/**
 * Find pattern in string starting at given position.
 *
 * @param str the string to search (non-nullptr)
 * @param pattern the pattern to find (non-nullptr)
 * @param start search position; if greater than str length, returns nullptr (no error)
 * @return pointer to first occurrence, or nullptr if not found (no error set)
 */
extern const char *str_find(const char *str, const char *pattern, size_t start);
```

Example without documentation (obvious from name and signature):

```c
[[nodiscard]]
extern char *str_new(const char *s, const str_opts *opts);

extern void stmem_free(char *s);

extern size_t str_len(const char *s);
```

### Inline Comments

Use `//` for section headers, design rationale, and notes for developers reading the source code:

```c
// Convert input to normalized form before processing
// This ensures consistent behavior regardless of input format
for (int i = 0; i < len; i++) {
    data[i] = normalize(data[i]);
}

// O(n) search: could optimize with KMP, but profiling shows
// the simple approach is fast enough for typical strings
const char *p = strstr(haystack, needle);
```

### Section Dividers

Use comment dividers to organize code sections. Top-level section dividers MUST use the '=' character repeated to a
width of 120 characters. For smaller subsections you may use '-' repeated to 120 characters; both styles should span the
full 120-character width so files have a consistent visual rhythm.

Example (120 '=' characters):

```c
// ========================================================================================================================
// public api
// ========================================================================================================================

// ------------------------------------------------------------------------------------------------------------------------
// string hashing
// ------------------------------------------------------------------------------------------------------------------------
```

## C23 Features

### Required Features

The library requires C23 and uses these features:

| Feature                    | Usage                                    |
|----------------------------|------------------------------------------|
| `nullptr`                  | Null pointer constant (never use `NULL`) |
| `[[nodiscard]]`            | Mark allocating functions                |
| `constexpr`                | Compile-time constants                   |
| `auto`                     | Type inference in local variables        |
| `typeof` / `typeof_unqual` | Type inference in macros                 |
| `__VA_OPT__`               | Conditional variadic expansion           |
| `_Generic`                 | Type-based dispatch                      |
| Compound literals          | Inline struct initialization             |

### Examples

```c
// nullptr instead of NULL
if (ptr == nullptr) { ... }

// [[nodiscard]] for allocations
[[nodiscard]] extern char *str_new(...);

// constexpr for compile-time constants
static constexpr size_t MAX_LEN = 4096;

// auto for type inference
auto result = str_new("hello");

// Compound literals for options
str("hello", &(str_opts){.max_len = 256});
```

## Type Conventions

### Integer Types

| Type      | Usage                                                       |
|-----------|-------------------------------------------------------------|
| `size_t`  | Sizes, lengths, counts (always non-negative)                |
| `ssize_t` | Sizes where negative has meaning (e.g., -1 for "until end") |
| `int`     | General integers, loop counters when size_t not needed      |
| `bool`    | Boolean values (never use `int` for true/false)             |

### Pointer Types

| Type           | Usage                                          |
|----------------|------------------------------------------------|
| `void *`       | Generic pointers (allocator returns, contexts) |
| `const char *` | String input parameters                        |
| `char *`       | Mutable string / string return values          |
| `const T *`    | Read-only struct/data parameters               |

## Header Guards

Use `#ifndef` style with `RUNE_` prefix:

```c
#ifndef RUNE_STR_H
#define RUNE_STR_H

// ... content ...

#endif // RUNE_STR_H
```

## Include Order

1. Corresponding header (for .c files)
2. Project headers
3. System/standard library headers

```c
// str.c
#include "str.h"           // 1. Corresponding header
#include "r.h"             // 2. Project headers (core runtime)

#include <ctype.h>         // 3. Standard library
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
```
