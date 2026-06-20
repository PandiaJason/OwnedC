<p align="center">
  <img src="assets/logo.jpg" alt="OwnedC Logo" width="200"/>
</p>

# OwnedC: Rust-Inspired Memory Safety for C
[![C/C++ CI](https://github.com/PandiaJason/OwnedC/actions/workflows/ci.yml/badge.svg)](https://github.com/PandiaJason/OwnedC/actions/workflows/ci.yml)

> [!WARNING]
> **Project Maturity:** OwnedC is an early-stage research prototype. It has not been subjected to external security audits. It is intended for experimentation and is not yet recommended for mission-critical production environments.

OwnedC is a lightweight memory safety framework and static analysis toolkit that brings modern ownership semantics, thread safety, and RAII (Resource Acquisition Is Initialization) to existing C codebases. It is designed to mitigate use-after-free and data-race vulnerabilities without requiring a transition to a completely different programming language.

---

## The API at a Glance

Unlike traditional C where memory management relies on developer discipline, OwnedC enforces deterministic, scope-bound resource cleanup.

**Before (Raw C):**
```c
void process_data() {
    void* buffer = malloc(1024);
    if (!buffer) return;
    
    if (compute_something(buffer) < 0) {
        free(buffer); // Easy to forget on error paths
        return;
    }
    
    free(buffer);
}
```

**After (OwnedC):**
```c
#include "ownedc.h"

void process_data() {
    // Memory is automatically tracked and safely freed when 'buffer' leaves scope
    OWNED void* buffer = owner_malloc(1024);
    if (!buffer) return;
    
    if (compute_something(buffer) < 0) {
        return; // No memory leak! 'buffer' is automatically cleaned up.
    }
}
```

---

## Performance & Benchmarks

For a memory-safety tool, the primary concern is the overhead introduced by safety checks. OwnedC's dynamic registry requires taking a mutex and tracking metadata for standard allocations.

### Methodology
- **Hardware:** Apple Silicon (macOS)
- **Compiler:** Clang 15 (`-O3` equivalent release build via CMake)
- **Workload:** 500,000 allocations of 32 bytes each, executed via `tests/benchmark.c`.

| Allocation Strategy | Single-Thread Time | Overhead vs Raw | Multi-Thread Time (8 Threads) | Multi-Thread Overhead |
|---------------------|--------------------|-----------------|-------------------------------|-----------------------|
| Raw `malloc` / `free` | 0.0616s | **Baseline (1.0x)** | 0.0296s | **Baseline (1.0x)** |
| `owner_malloc` / `free` | 9.2519s | **150.1x** | 5.5906s | **188.7x** |
| `safe_region` (Arena) | 0.0097s | **0.15x (6.3x Faster)** | N/A | N/A |

*Takeaway:* Individual `owner_malloc` calls introduce significant overhead (~150x-188x), heavily exacerbated by global hash map collisions and mutex lock contention under highly concurrent loads. However, developers can switch to the `safe_region` Arena Allocator for bulk allocations. The arena completely bypasses the granular global hash-map, achieving speeds **~6.3x faster** than single-threaded raw `malloc` while fully retaining leak-protection and memory safety.

---

## Prior Art & Comparisons

The memory-safety landscape for C is extensive. OwnedC makes specific trade-offs compared to prior art:

- **Checked C (Microsoft Research):** Checked C introduces sweeping syntactic changes (`_Ptr`, `_Array_ptr`) requiring extensive codebase rewrites. OwnedC attempts to remain closer to standard C syntax via attributes and macros.
- **ASan (AddressSanitizer):** ASan is heavily instrumented and generally restricted to debug builds. While we have not performed extensive 1-to-1 memory bloat comparisons, OwnedC is designed conceptually as a production-capable runtime layer rather than an exclusively diagnostic tool.
- **CHERI / Morello:** CHERI provides hardware-enforced spatial memory safety. Rather than competing with CHERI, OwnedC **integrates** with it. When compiled on a CHERI toolchain, OwnedC automatically upgrades its pointers to `__capability` bounded hardware pointers via `ownedc_cheri.h`.
- **Rust:** Rust provides zero-cost abstractions and static safety. OwnedC is intended for legacy C codebases that simply cannot afford a full language rewrite, leaning on dynamic runtime checks as a compromise.

---

## Core Features

- **Automated Memory Management (RAII):** Utilizes `__attribute__((cleanup))` to provide deterministic resource management.
- **Dynamic Borrow Checking:** Enforces strict memory borrowing rules at runtime.
- **Thread Ownership Verification:** Protects against data races by prohibiting unauthorized cross-thread deallocation.
- **High-Performance Arenas:** Features `safe_region` for massive bump-pointer throughput.
- **Safe Collections:** Includes `safe_vector`, demonstrating generic data structures protected by the ownership runtime.
- **CHERI Integration:** `ownedc_cheri.h` transparently upgrades allocations to capability pointers on Morello hardware.
- **Static Ownership Analysis:** Includes `ownedc-analyzer.py` for build-time AST validation.

> **Note on Standard Compatibility:** The core dynamic registry is fully C99/C11 compliant. However, the RAII macro (`OWNED`) explicitly relies on the `__attribute__((cleanup))` extension. Therefore, while the library compiles universally, the automatic-cleanup feature specifically requires **GCC or Clang**.

---

## Getting Started

### Prerequisites
- GCC or Clang (Required for `OWNED` RAII features)
- CMake 3.10+

### Build Instructions

```bash
git clone https://github.com/PandiaJason/OwnedC.git
cd OwnedC
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

### Included Demonstrations
The repository comes with a comprehensive suite of examples. Run them to see the features in action:
- `build/owned_http_server`: **Real-World Use Case!** A multi-threaded web server demonstrating Thread Ownership and Region Arenas for request contexts.
- `build/demo_raii`: Auto-cleanup in action.
- `build/demo_vector`: Safe Collections and bounds-checking.
- `build/demo_region`: High-Performance Arenas.
- `build/demo_threads`: Thread Safety & Ownership.
- `build/demo_hybrid_cheri`: CHERI Hybrid Architecture mockup.
- `build/demo_full_showcase`: A comprehensive demonstration unifying all features.

## License
This project is licensed under the GNU General Public License v3.0 (GPLv3). See the [LICENSE](LICENSE) file for full details.
