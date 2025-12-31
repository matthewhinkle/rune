# rune

A C23 library experimenting with foundational primitives found in higher-level languages.

[![Build Status](https://github.com/matthewhinkle/rune/actions/workflows/coverage.yml/badge.svg)](https://github.com/matthewhinkle/rune/actions/workflows/coverage.yml)
[![Language: C23](https://img.shields.io/badge/Language-C23-blue)](https://en.cppreference.com/w/c/23)
[![Platforms: Linux | macOS | Windows](https://img.shields.io/badge/Platforms-Linux%20%7C%20macOS%20%7C%20Windows-success)](https://github.com/matthewhinkle/rune/actions/workflows/coverage.yml)
[![codecov](https://codecov.io/gh/matthewhinkle/rune/branch/master/graph/badge.svg)](https://codecov.io/gh/matthewhinkle/rune)
[![CodeQL](https://github.com/matthewhinkle/rune/actions/workflows/codeql.yml/badge.svg)](https://github.com/matthewhinkle/rune/actions/workflows/codeql.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Features

- **Strings** — Creation, manipulation, searching, hashing
- **Collections** — Generic lists, lock-free queues
- **Custom allocators** — Optional runtime context for all operations

## Quick Start

### Strings

```c
#include "str.h"

char * s = str("Hello");
char * msg = str_cat(
    s,
    ", ",
    "world!"
);  // Concatenate multiple strings
printf("%s\n", msg);  // "Hello, world!"

char ** parts = str_split("a,b,c", ",");  // Split into array
char * joined = str_join("-", parts);     // Join with delimiter
printf("%s\n", joined);                   // "a-b-c"

str_free(s);
str_free(msg);
str_free(joined);
str_free_arr(parts);
```

### Collections (Type-Safe Generics)

```c
#define T int
#include "coll.h"  // Instantiate list for int
#undef T

// Create and manipulate
LIST(int) nums = list(int, 1, 2, 3, 4, 5);
list_add(&nums, 6);
list_insert(&nums, 0, 0);  // Insert at beginning
printf("Size: %zu\n", nums.size);  // 7

// Works with custom types too
typedef struct {
    const char * name;
    int age;
} Person;

#define T Person
#include "coll.h"
#undef T

LIST(Person) people = list(Person);
list_add(&people, (Person){"Alice", 30});
list_add(&people, (Person){"Bob", 25});
printf(
    "%s is %d\n",
    people.data[0].name,
    people.data[0].age
);  // Alice is 30

list_free(&nums);
list_free(&people);
```

### Custom Allocators (Arena Example)

```c
#include "r.h"
#include <string.h>

// Arena allocator context
typedef struct {
    void * buffer;
    size_t capacity;
    size_t used;
} arena_ctx;

static void * arena_alloc(void * ctx, size_t size) {
    arena_ctx * a = (arena_ctx *)ctx;
    if (a->used + size > a->capacity) {
        return NULL;  // Out of memory
    }
    void * ptr = (char *)a->buffer + a->used;
    a->used += size;
    return ptr;
}

static void * arena_realloc(void * ctx, void * ptr, size_t old_size, size_t new_size) {
    arena_ctx * a = (arena_ctx *)ctx;
    if (new_size <= old_size) {
        return ptr;
    }
    void * new_ptr = arena_alloc(ctx, new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, old_size);
    }
    return new_ptr;
}

static void arena_free(void * ctx, void * ptr, size_t size) {
    // No-op: arena frees all at once
    (void)ctx;
    (void)ptr;
    (void)size;
}

// Usage
int main(void) {
    char buffer[10240];
    arena_ctx ctx = {
        .buffer = buffer,
        .capacity = sizeof(buffer),
        .used = 0,
    };
    allocator arena = {
        .alloc = arena_alloc,
        .realloc = arena_realloc,
        .free = arena_free,
        .ctx = &ctx,
    };

    alloc_scope(arena) {
        // All allocations use the arena
        char * s = str("From arena");

        #define T int
        #include "coll.h"
        #undef T

        LIST(int) nums = list(int, 1, 2, 3);
        list_add(&nums, 4);

        // Memory is automatically freed when scope exits
    }
    return 0;
}
```

## Requirements

- C23 compiler (GCC 14+, Clang 18+)
- CMake 3.31+

## Documentation

- **[Architecture](doc/ARCHITECTURE.md)** — Project structure and design
- **[API Design](doc/API_DESIGN.md)** — Core API patterns and philosophy
- **[Conventions](doc/CONVENTIONS.md)** — Coding standards
- **[Testing](doc/TESTING.md)** — Testing framework
- **[Development Guide](doc/DEVELOPMENT.md)** — Contributing to rune
- **[Build System](doc/BUILD.md)** — Building and testing
- **[Testing Internals](doc/TESTING_INTERNALS.md)** — Advanced testing utilities
