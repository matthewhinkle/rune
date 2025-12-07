# Development Guide

This document provides guidance for contributing to the rune library.

## Before Making Changes

Review the following documentation to understand the project's patterns and conventions:

1. **[API_DESIGN.md](API_DESIGN.md)** — Core API patterns
    - `runtime` struct and optional parameter pattern
    - Polymorphic macros with `_Generic`
    - Options structs and compound literals

2. **[CONVENTIONS.md](CONVENTIONS.md)** — Coding standards
    - Naming conventions (`R_` prefix, `snake_case`)
    - Code style and formatting
    - C23 features used

3. **[ARCHITECTURE.md](ARCHITECTURE.md)** — Module organization
    - Directory structure (`src/core/`, etc.)
    - Module dependencies
    - Design principles

## Adding a New Module

1. Review existing modules for patterns (`core/str.h`, `core/rune.h`)
2. Place the module in the appropriate domain directory
3. Follow the header structure from CONVENTIONS.md
4. Ensure the module accepts optional `runtime *` as first parameter
5. Provide an options struct if configuration is needed
6. Mark allocating functions with `[[nodiscard]]`
7. Add corresponding tests

## Making Changes to Existing Code

1. Maintain consistency with surrounding code
2. Follow established patterns — don't introduce new conventions
3. Update documentation if behavior changes
4. Ensure tests pass after changes
