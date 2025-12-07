# Build System and External Dependencies

This document describes the build system, dependencies, and how to build and test the rune library.

## Requirements

### Compiler

- **C23 compiler** — GCC 14+, Clang 18+, or newer
- C23 features required:
  - `nullptr` — Null pointer constant
  - `_Generic` — Type-based dispatch in macros
  - `typeof`/`typeof_unqual` — Type inference
  - `auto` — Type inference for locals
  - `__VA_OPT__` — Conditional variadic expansion
  - `[[nodiscard]]` — Attribute to enforce result usage
  - `constexpr` — Compile-time constants
  - `_Thread_local` — Thread-local storage

### Build Tools

- **CMake** 3.31+ — Build configuration and generation
- **Python3** — Build utilities (header normalization)
- **pkg-config** — Finding optional dependencies

## External Dependencies

### Required

| Package | Version | Purpose             | Link |
|---------|---------|---------------------|------|
| CUnit   | 2.x+    | Unit testing        | http://cunit.sourceforge.net/ |

### Optional (for main executable)

| Package | Version | Purpose              | Link |
|---------|---------|----------------------|------|
| SDL3    | Latest  | Graphics/windowing   | https://www.libsdl.org/ |
| Vulkan  | Latest  | Graphics rendering   | https://www.vulkan.org/ |

**Note:** SDL3 and Vulkan are only required for the main rune executable. The core library (`r.h`, `str.h`, `coll.h`) has no external dependencies beyond C23.

## Installation

### macOS (Homebrew)

```bash
# Core dependencies
brew install cmake python3 cunit

# Optional graphics (for main executable)
brew install sdl3 vulkan-headers
```

### Ubuntu/Debian

```bash
# Core dependencies
sudo apt-get install cmake python3 libcunit1 libcunit1-dev

# Optional graphics (for main executable)
sudo apt-get install libsdl3-dev libvulkan-dev vulkan-tools
```

### Fedora

```bash
# Core dependencies
sudo dnf install cmake python3 CUnit-devel

# Optional graphics (for main executable)
sudo dnf install SDL3-devel vulkan-devel vulkan-tools
```

### Windows (vcpkg)

```bash
# Install vcpkg and dependencies
vcpkg install cunit:x64-windows
vcpkg install sdl3:x64-windows
vcpkg install vulkan:x64-windows

# Configure CMake with vcpkg toolchain
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
```

## Building

### Basic Build

```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .

# Tests run automatically after build
```

### Build with Custom Settings

```bash
# Build with specific generator
cmake -G "Ninja" ..

# Build in release mode with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# Build with verbose output
cmake --build . --verbose
```

### Build Targets

```bash
# Main executable
cmake --build . --target rune

# Individual test suites
cmake --build . --target test_r
cmake --build . --target test_coll
cmake --build . --target test_str

# Run all tests
cmake --build . --target run_tests

# Normalize header formatting
cmake --build . --target normalize_headers
```

## Testing

### Automatic Testing

Tests run automatically when building the project (via `run_tests` target added as dependency to main executable).

### Manual Testing

```bash
# Run individual test executables
./test_r
./test_coll
./test_str

# Run with verbose output
./test_r --verbose

# Run tests and capture output
./test_r > results.txt 2>&1
```

### Test Framework

- **Framework** — CUnit (C unit testing framework)
- **Test organization** — By module (`test_r.c`, `test_coll.c`, `test_str.c`)
- **Test naming** — `<function>__for_<input>__should_<expected>` pattern
- **Total coverage** — 62+ tests with 200+ assertions

### Debugging Tests

```bash
# With GDB
gdb ./test_r

# With Valgrind (memory leak detection)
valgrind --leak-check=full ./test_r

# With address sanitizer (ASAN)
cmake -DCMAKE_C_FLAGS="-fsanitize=address" ..
cmake --build .
./test_r
```

## Build Configuration

### CMakeLists.txt Structure

The build system:
1. Finds required packages (CUnit, Python3, optional: SDL3, Vulkan)
2. Normalizes header formatting (Python script)
3. Builds main executable with all core modules
4. Builds three separate test executables
5. Defines `run_tests` custom target that executes all tests
6. Makes main executable depend on `run_tests` for automatic testing

### Compile-Time Configuration

The library supports compile-time configuration via preprocessor macros. Define before including headers:

```c
// Custom error stack depth
#define RCFG__ERROR_STACK_MAX 16

// Custom allocator stack depth
#define RCFG__ALLOC_STACK_MAX 32

// Custom string limits
#define RCFG__STR_MAX_LEN 8192
#define RCFG__STR_STACK_MAX 16384
#define RCFG__STR_MAX_VARG 128

#include "r.h"
#include "str.h"
```

Default values:
- `RCFG__ERROR_STACK_MAX` — 8
- `RCFG__ALLOC_STACK_MAX` — 16
- `RCFG__STR_MAX_LEN` — 4096
- `RCFG__STR_STACK_MAX` — 8192
- `RCFG__STR_MAX_VARG` — 64

## Compiler Flags

### Recommended Debug Flags

```bash
cmake -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror" ..
```

### Recommended Release Flags

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3" ..
```

## Troubleshooting

### CMake Configuration Errors

**Error: "Could not find CUnit"**
- Install CUnit: See [Installation](#installation) section
- Or provide manual path: `cmake -DCUNIT_DIR=/path/to/cunit ..`

**Error: "Could not find SDL3/Vulkan"**
- These are optional; the core library builds without them
- For main executable, install SDL3 and Vulkan headers

### Build Errors

**"C23 features not supported"**
- Upgrade your compiler to GCC 14+ or Clang 18+
- Update cmake compiler: `export CC=gcc-14` or `export CC=clang-18`

**"undefined reference to `_Thread_local`"**
- Ensure C23 standard is set: Check CMakeLists.txt has `set(CMAKE_C_STANDARD 23)`
- Upgrade compiler if still failing

### Test Failures

**Tests fail immediately with "undefined symbol"**
- Ensure all source files are linked in CMakeLists.txt
- Rebuild with `cmake --build . --clean-first`

**Memory errors in tests**
- Run with Valgrind to detect leaks: `valgrind --leak-check=full ./test_r`
- Run with ASAN: `cmake -DCMAKE_C_FLAGS="-fsanitize=address" ..`

## Clean Build

```bash
# Remove build directory
rm -rf build

# Fresh configure and build
mkdir build && cd build
cmake ..
cmake --build .
```
