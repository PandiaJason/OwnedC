# OwnedC: Ownership-Aware Memory Safety for C

OwnedC is a lightweight memory safety framework and static analysis toolkit that brings modern ownership semantics, thread safety, and region-based memory management to existing C codebases. It is designed to mitigate memory vulnerabilities without requiring a transition to a different programming language.

## Overview

OwnedC introduces ownership tracking, lifetime verification, and static analysis while remaining strictly compatible with C99 and C11 standards.

### Key Capabilities

- **Automated Memory Management (RAII):** Utilizes compiler-specific cleanup attributes to provide deterministic, scope-bound resource management, eliminating the need for manual `free` operations.
- **Dynamic Borrow Checking:** Enforces strict memory borrowing rules at runtime. Attempting to deallocate memory while active borrows exist will result in a controlled program termination, preventing use-after-free vulnerabilities.
- **Thread Ownership Verification:** Protects against data races by binding memory allocations to their originating thread. Cross-thread deallocation or borrowing is strictly prohibited unless explicitly authorized.
- **Static Ownership Analysis:** Includes an integrated static analysis engine (`ownedc-analyzer.py`) capable of detecting unauthorized `malloc`/`free` calls and enforcing proper RAII macro usage during the build process.
- **Arena Allocators:** Features `safe_region`, a high-performance bump-pointer allocator for bulk memory management.

## Architecture

The framework operates through a dual-layer approach:
1. **Runtime Verification Layer:** Intercepts standard allocation routines, maintaining metadata in a thread-safe registry. It performs bounds checking, ownership validation, and memory poisoning dynamically.
2. **Compile-Time Static Analysis:** Statically verifies the Abstract Syntax Tree (AST) to ensure correct API usage before compilation.

## Getting Started

### Prerequisites
- GCC or Clang
- CMake 3.10+
- Python 3.x (for static analysis)

### Build Instructions

```bash
git clone https://github.com/PandiaJason/OwnedC.git
cd OwnedC
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

## IDE Integration

OwnedC provides native integration for Visual Studio Code. The included `.vscode/tasks.json` invokes the static analysis engine and maps diagnostics directly to the editor's problems panel.

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3). See the [LICENSE](LICENSE) file for full details.
