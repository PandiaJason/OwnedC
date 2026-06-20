# OwnedC: Ownership-Aware Memory Safety for C

OwnedC is a lightweight memory safety framework and static analysis toolkit that brings modern ownership semantics, thread safety, and region-based memory management to existing C codebases. It is designed to mitigate memory vulnerabilities without requiring a transition to a different programming language.

## Overview

OwnedC introduces ownership tracking, lifetime verification, and static analysis while remaining strictly compatible with C99 and C11 standards. It operates through a dual-layer approach: a lightweight runtime metadata registry for dynamic checks, and a static analyzer for compile-time validation.

### Core Features

- **Automated Memory Management (RAII):** Utilizes compiler-specific cleanup attributes to provide deterministic, scope-bound resource management, eliminating the need for manual `free` operations.
- **Dynamic Borrow Checking:** Enforces strict memory borrowing rules at runtime. Attempting to deallocate memory while active borrows exist will result in a controlled program termination, preventing use-after-free vulnerabilities.
- **Thread Ownership Verification:** Protects against data races by binding memory allocations to their originating thread. Cross-thread deallocation or borrowing is strictly prohibited unless explicitly authorized via `ownership_share()`.
- **Arena Allocators:** Features `safe_region`, a high-performance bump-pointer allocator for bulk memory management.
- **Safe Collections:** Includes natively bounded data structures (e.g., `safe_vector`) that utilize the ownership runtime to prevent out-of-bounds access.
- **CHERI Capability Architecture:** Provides hybrid integration for capability hardware (e.g., ARM Morello), allowing standard allocations to be automatically upgraded to hardware-enforced capabilities via `ownedc_cheri.h`.
- **Static Ownership Analysis:** Includes an integrated static analysis engine (`ownedc-analyzer.py`) capable of detecting unauthorized `malloc`/`free` calls and enforcing proper RAII macro usage during the build process.

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

## Included Demonstrations

The repository comes with a comprehensive suite of examples demonstrating the capabilities of OwnedC. After building the project, you can find the compiled executables in the `build/` directory.

### 1. RAII Auto-Cleanup (`demo_raii`)
Demonstrates how variables decorated with the `OWNED` macro are automatically tracked and deallocated when they fall out of scope, guaranteeing no memory leaks without explicit `owner_free()` calls.

### 2. Thread Safety & Ownership (`demo_threads`)
Shows the cross-thread ownership verification in action. The demonstration highlights how memory allocated on one thread safely aborts execution if modified by an unauthorized worker thread, and how to safely authorize cross-thread data via `ownership_share()`.

### 3. Safe Collections (`demo_vector`)
Highlights the `safe_vector_t` dynamically resizing data structure. It demonstrates type-agnostic insertions and memory-safe retrievals with automatic capacity management.

### 4. High-Performance Arenas (`demo_region`)
A demonstration of the `safe_region` allocator performing rapid bump-pointer allocations. It showcases how bulk allocations bypass the global runtime registry for speed, while the macro-region remains fully leak-protected.

### 5. CHERI Hybrid Architecture (`demo_hybrid_cheri`)
Demonstrates the `owner_malloc_cheri()` wrapper. On standard hardware, it utilizes software bounds checking; on capability hardware, it seamlessly generates `__capability` pointers.

### 6. The Full Showcase (`demo_full_showcase`)
A culmination of the entire project running sequentially through RAII, Safe Vectors, Region Arenas, and Thread Ownership cross-sharing.

## IDE Integration

OwnedC provides native integration for Visual Studio Code. The included `.vscode/tasks.json` invokes the static analysis engine and maps diagnostics directly to the editor's "Problems" panel.

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3). See the [LICENSE](LICENSE) file for full details.
