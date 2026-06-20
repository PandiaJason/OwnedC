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
- **Static Ownership Analysis:** Includes `ownedc_lint.py`, acting as a heuristical "Borrow Checker" running offline at compile-time to intercept memory leaks, double-frees, and use-after-free before you run your executable.
- **Thread Ownership Verification:** Protects against data races by prohibiting unauthorized cross-thread deallocation.
- **First-Class C++ RAII Wrappers:** Use `ownedc.hpp` for `owned_ptr<T>` and `borrowed_ptr<T>` native C++ classes that automatically bridge C++ constructors/destructors to our robust C memory safety backend.
- **Concurrency Safety & Channels:** Provides `owned_mutex_t` for deadlocks, `ownedc_future.c` for Safe Async Promises & Futures, and `ownedc_channel.c` for Multi-Producer Single-Consumer (MPSC) Actor-model messaging passing without race conditions!
- **Resource Safety (Safe File I/O & Sockets):** Provides `safe_file_t` and `safe_socket_t` wrappers to automatically close descriptors, eliminating FD leaks.
- **Safe Managed Threads:** Implements `OWNED_THREAD` to automatically join or detach threads when handles drop out of scope, eliminating zombie threads.
- **Type-Safe Generics:** Macro-driven `OWNEDC_DEFINE_VECTOR(T)` natively generates type-safe arrays with completely transparent void* castings.
- **Kernel & Embedded Mode:** `OWNEDC_NO_STDLIB` totally abstracts standard libraries out of the library for bare-metal OS or embedded microcontroller execution.
- **Pluggable Allocators:** Integrates with `jemalloc`, `mimalloc`, or game-engine allocators via `ownedc_set_allocators()`.
- **High-Performance Lock-Free Arenas:** Features `ownedc_arena.h` for massive, lock-free bump-pointer throughput where 10,000 objects can be dropped instantly in O(1) time.
- **Shared Ownership & GC:** Implements `owned_rc_t` for reference counting, and advanced cycle detection via `owned_rc_collect_cycles()` to reclaim cyclic memory graphs.
- **Memory Profiler GUI:** Automatically exports the full ownership graph memory states using `ownership_dump_json()` and generates a beautiful HTML dashboard using `tools/ownedc_profiler.py`.
- **Rust-Like Error Handling:** Features `owned_result_t` (Result types) and the `TRY_UNWRAP` macro to force explicit error handling.
- **Deep-Freeing Arrays:** Provides `owned_array_t` for bounds-checked arrays that recursively free elements upon destruction.
- **CHERI Integration:** `ownedc_cheri.h` transparently upgrades allocations to capability pointers on Morello hardware.

> **Note on Standard Compatibility:** The core dynamic registry is fully C99/C11 compliant. However, the RAII macro (`OWNED`) explicitly relies on the `__attribute__((cleanup))` extension. Therefore, while the library compiles universally, the automatic-cleanup feature specifically requires **GCC or Clang**.

---

## Getting Started

### Prerequisites
- GCC or Clang (Required for `OWNED` RAII features)
- CMake 3.10+
- Python 3.x (Optional: For Static Linting and Profiler HTML Generation)

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
- `build/owned_http_server`: **Real-World Use Case!** A multi-threaded web server demonstrating Thread Ownership, Region Arenas, and Safe Strings.
- `build/demo_profiler`: Generates a massive JSON graph of your memory layout, visualizing leaks.
- `build/demo_lint`: See the offline Borrow Checker `tools/ownedc_lint.py` in action!
- `build/demo_cpp`: First-class C++ `owned_ptr<T>` usage in action.
- `build/demo_kernel`: Execution in a simulated `NO_STDLIB` embedded bare-metal setting.
- `build/demo_generics`: Creating strongly-typed Vectors with zero `void*` casts.
- `build/demo_future`: Spawning async tasks that safely transfer memory ownership across thread boundaries!
- `build/demo_channel`: Passing memory sequentially using MPSC channels between multiple threads.
- `build/demo_arena`: Dropping millions of Lock-Free Bump pointer objects simultaneously.
- `build/demo_allocator`: Enterprise Custom Allocator Integration.
- `build/demo_file` & `demo_socket`: Safe File I/O and Sockets.
- `build/demo_mutex` & `demo_thread_safe`: Concurrency Safety and Auto-joining.
- `build/demo_result`: Rust-Like Error Handling (`Result<T, E>`).
- `build/demo_raii`: Auto-cleanup in action.
- `build/demo_rc` & `demo_rc_cycle`: GC & Reference Counting.
- `build/demo_string`, `demo_array`, `demo_vector`: Bounds-checked Collections.
- `build/demo_hybrid_cheri`: CHERI Hybrid Architecture mockup.

## License
This project is licensed under the GNU General Public License v3.0 (GPLv3). See the [LICENSE](LICENSE) file for full details.
