# Rune Tools

This directory contains utility tools for the Rune project.

## fmtheader

A C-based tool that automatically formats repeated-character headers in source files to a consistent length of 120 characters.

### Features

- Detects lines with 3+ consecutive repeated characters (e.g., `===`, `---`, `***`, etc.)
- Extends them to exactly 120 characters
- Preserves text content between repeated characters (e.g., `// --- Text ---`)
- Modifies files in-place
- Processes multiple files in a single invocation

### Building

The tool is built automatically when you rebuild your CMake project in CLion:
- Right-click project → CMake → Build All
- Or use Build → Build Project

The executable will be placed in `cmake-build-debug/` or your configured build directory.

### Usage

```bash
./fmtheader <file1> [file2] ...
```

Examples:

```bash
# Format a single file
./fmtheader ../src/hash.h

# Format all header files in src
./fmtheader ../src/*.h

# Format all source and header files
./fmtheader ../src/*.h ../src/*.c
```

### Example Transformations

Before:
```c
// --- Type Definitions ---
// ====================================
* ========================================================================
```

After (all normalized to 120 characters):
```c
// --- Type Definitions ---
// ============================================================
* ==============================================================================================
```

### Implementation Notes

- Written in C23 standard
- No external dependencies
- Supports lines up to 4096 characters
- Preserves file structure and formatting
- Safe in-place modification with full file reading before writing
