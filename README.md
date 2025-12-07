# rune

A modern C23 library providing foundational primitives with a clean, flexible API.

## Features

- **Strings** — Creation, manipulation, searching, hashing
- **Collections** — Generic lists, lock-free queues
- **Custom allocators** — Optional runtime context for all operations

## Quick Start

```c
#include "core/str.h"

int main(void) {
    char *s = str("hello");
    char *msg = str_cat(s, ", world!");

    stmem_free(msg);
    stmem_free(s);
    return 0;
}
```

## Requirements

- C23 compiler (GCC 14+, Clang 18+)
- CMake 3.31+

## Documentation

### Architecture & Design
- **[Architecture](docs/ARCHITECTURE.md)** — Project structure, module organization, and design principles
  - Project structure and module layout
  - Core module descriptions (`rune.h`, `str.h`, `coll.h`)
  - Dependency graph and module organization
  - Design principles and extensibility

### API & Patterns
- **[API Design](docs/API_DESIGN.md)** — Core API patterns and design philosophy
  - **Naming conventions** — `R_` prefixes, public API naming, module patterns
  - **Allocator interface** — Customizable memory operations
  - **Polymorphic macros** — `_Generic` based optional parameters
  - **Options structs** — Configuration with compound literals
  - **Constructor configuration** — Flexible argument ordering with `R_DISPATCH`
  - **Module structure** — Header and implementation patterns
  - **Memory management** — Allocation, deallocation, runtime consistency
  - **Error handling** — Thread-local error context design
  - **C23 features** — Modern language features used throughout

### Coding Standards
- **[Conventions](docs/CONVENTIONS.md)** — Coding style and standards
  - **Naming conventions** — Prefixes, casing rules, module naming, variable names
  - **Code style** — Function signatures, parameter qualifiers, declarations, constants
  - **Macro conventions** — Multi-statement, variadic args, type dispatch, formatting
  - **Memory management** — Allocation, deallocation, assertion policy
  - **Error handling** — Error setting, validation, clearing, documentation, return patterns
  - **Documentation** — Comment style, module-level docs, function docs, inline comments
  - **Type conventions** — Integer and pointer type usage
  - **Header guards and includes** — File organization

### Testing & Quality
- **[Testing](docs/TESTING.md)** — Testing framework and conventions
  - Framework (CUnit)
  - File structure and test layout
  - Test naming conventions and organization
  - Helper macros and common assertions
  - **Security testing** — Buffer overflow, null termination, format strings, edge cases
  - Running tests

### Development & Build
- **[Development Guide](docs/DEVELOPMENT.md)** — Contributing to rune
  - Understanding project patterns
  - Adding new modules
  - Modifying existing code

- **[Build System](docs/BUILD.md)** — Building, testing, and configuring the project
  - Compiler and dependency requirements
  - Installation instructions
  - Build targets and configurations
  - Running and debugging tests

- **[Testing Internals](docs/TESTING_INTERNALS.md)** — Advanced testing with r_test.h
  - Testing allocator internals
  - Mocking allocators in tests
  - Thread-local error and allocator stack access
