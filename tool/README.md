# Rune Tools

Experimental tools for the Rune project. Not intended for production use yet.

## fmtheader

Normalizes those decorative `===` and `---` header lines in your source files to a consistent 120 characters. One less
thing to micromanage.

### What It Does

- Finds lines with 3+ repeated characters (e.g., `===`, `---`, `***`)
- Stretches them to exactly 120 characters
- Keeps any text between the decorations (e.g., `// --- Text ---`)
- Modifies files in-place
- Handles multiple files at once

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

### Implementation Details

- Written in C23
- No dependencies
- Handles lines up to 4096 characters
- Reads the whole file before writing (so it won't corrupt things if it crashes)
- Preserves everything else—just fixes the decorations
