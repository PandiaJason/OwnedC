<p align="center">
  <img src="assets/logo.jpg" alt="OwnedC Logo" width="200"/>
</p>

<h1 align="center">OwnedC</h1>
<p align="center"><i>Ownership-aware memory safety for C — without rewriting your codebase.</i></p>

<p align="center">
  <a href="https://github.com/PandiaJason/OwnedC/actions/workflows/ci.yml"><img src="https://github.com/PandiaJason/OwnedC/actions/workflows/ci.yml/badge.svg" alt="CI"/></a>
  <img src="https://img.shields.io/badge/license-GPLv3-blue.svg" alt="License"/>
  <img src="https://img.shields.io/badge/standard-C99%20%2F%20C11-informational.svg" alt="C Standard"/>
  <img src="https://img.shields.io/badge/status-research%20prototype-orange.svg" alt="Status"/>
</p>

---

## Overview

OwnedC is a memory safety framework for C that brings ownership tracking, scope-bound RAII, and dynamic borrow checking to existing codebases, without requiring a migration to a different language. It is implemented as a dual-layer system: a runtime metadata registry performs dynamic ownership and borrow checks, and a static analyzer (`ownedc_lint.py`) performs best-effort compile-time linting against the same ownership rules.

OwnedC is a research prototype. It has not undergone external security review, and the performance characteristics of its core tracking path are documented, not hidden — see [Performance](#performance) before deciding where in your codebase to use it.

## Project Status

OwnedC is pre-1.0. APIs may change without notice between commits. The table below reflects maturity per subsystem as of this writing; it should be kept current as test coverage changes, and verified against actual CI results rather than taken at face value.

| Subsystem | Status | Notes |
|---|---|---|
| RAII (`OWNED`, `owner_malloc`) | Beta | Functionally correct; high per-call overhead (see Performance) |
| Dynamic Borrow Checking | Beta | Aborts on violation; covered by `tests/` |
| Static Lint (`ownedc_lint.py`) | Beta | Heuristic, not a sound type system — see Non-Goals |
| Thread Ownership Verification | Beta | Single global registry; not yet stress-tested at high thread counts |
| Arenas (`safe_region`) | Beta | Benchmarked; single-threaded by design |
| Safe Collections (vector / string / array) | Beta | |
| CHERI Integration (`ownedc_cheri.h`) | Experimental | Limited hardware test surface (Morello) |
| C++ Wrappers (`ownedc.hpp`) | Experimental | |
| Concurrency: Mutex (`owned_mutex_t`) | Beta | |
| Concurrency: Managed Threads (`OWNED_THREAD`) | Experimental | |
| Concurrency: Futures / Channels | Experimental | |
| Resource Safety: File / Socket wrappers | Experimental | |
| Pluggable Allocators (jemalloc / mimalloc) | Experimental | |
| Shared Ownership / Rc + cycle collection | Experimental | |
| Memory Profiler GUI | Experimental | |
| Kernel / Embedded mode (`NO_STDLIB`) | Experimental | Disables concurrency, sockets, file I/O |

> **Beta**: implemented, tested, behavior is documented and benchmarked where relevant.
> **Experimental**: implemented, not yet stress-tested; API may change; not recommended outside of evaluation.

## Design Goals

- Add memory and ownership safety to existing C codebases without a full rewrite.
- Make the cost of safety explicit and measurable, not assumed.
- Provide a zero-overhead path (`safe_region`) for hot loops that cannot absorb tracking cost.
- Allow hardware-enforced safety (CHERI) to be adopted incrementally where available, without requiring it.
- Interoperate with existing allocators and C++ call sites rather than requiring isolation.

## Non-Goals

- **Not a sound static analyzer.** `ownedc_lint.py` is heuristic. It catches common misuse patterns; it does not provide the compile-time guarantees of a type system like Rust's borrow checker.
- **Not a general-purpose garbage collector.** `owned_rc_t` targets cycle reclamation within bounded, explicitly-owned reference graphs — not whole-heap GC.
- **Not a high-throughput general allocator.** `owner_malloc` trades throughput for traceability; `safe_region` is the allocator for performance-sensitive paths.
- **Not production-hardened.** No external audit has been performed. Treat all safety guarantees as best-effort until a 1.0 release.

## Quick Start

```c
#include "ownedc.h"

void process_data() {
    // Memory is automatically tracked and safely freed when 'buffer' leaves scope
    OWNED void* buffer = owner_malloc(1024);
    if (!buffer) return;

    if (compute_something(buffer) < 0) {
        return; // No leak: 'buffer' is automatically cleaned up.
    }
}
```

Compare to the equivalent raw C, where the `free()` on the error path is easy to omit:

```c
void process_data() {
    void* buffer = malloc(1024);
    if (!buffer) return;

    if (compute_something(buffer) < 0) {
        free(buffer); // easy to forget
        return;
    }
    free(buffer);
}
```

## Performance

OwnedC's dynamic registry takes a mutex and tracks metadata on every `owner_malloc`/`owner_free` call. That cost is real and is reported here directly rather than abstracted away.

**Methodology:** Apple Silicon (macOS), Clang 15, `-O3`-equivalent release build via CMake, 500,000 allocations of 32 bytes each, via `tests/benchmark.c`.

| Allocation Strategy | Single-Thread | Overhead | 8-Thread | Overhead |
|---|---|---|---|---|
| Raw `malloc`/`free` | 0.0616s | 1.0x (baseline) | 0.0296s | 1.0x (baseline) |
| `owner_malloc`/`free` | 9.2519s | 150.1x | 5.5906s | 188.7x |
| `safe_region` (arena) | 0.0097s | 0.15x (6.3x faster) | N/A — single-threaded by design | — |

**Guidance:** `owner_malloc` is for cold paths, setup code, and diagnostics — not hot loops. `safe_region` is the production-viable allocator for performance-sensitive code; for concurrent workloads, use one arena per thread. The overhead gap is attributed to global hash-map collisions and mutex contention in the registry and is tracked as a roadmap item (see below) rather than treated as a fixed cost of the design.

## Architecture

```
                 ┌─────────────────────────┐
                 │   ownedc_lint.py (CI)    │   compile-time, heuristic
                 └────────────┬─────────────┘
                              │
   source ──────────────────►│  build
                              │
                 ┌────────────▼─────────────┐
                 │   ownedc.h runtime core   │
                 │  (registry + borrow check)│
                 └──────┬─────────────┬──────┘
                         │             │
              ┌──────────▼───┐   ┌─────▼──────────┐
              │ owner_malloc │   │  safe_region    │
              │ (tracked,    │   │  (arena, single-│
              │  mutex-bound)│   │   thread, O(1)  │
              └──────────────┘   │   bulk free)    │
                                  └─────────────────┘
                         │
               optional hardware backend
                         │
                 ┌───────▼────────┐
                 │  CHERI/Morello  │
                 └─────────────────┘
```

## Feature Matrix

| Feature | Header / Tool | Requires |
|---|---|---|
| RAII auto-cleanup | `ownedc.h` (`OWNED`) | GCC or Clang (`__attribute__((cleanup))`) |
| Dynamic borrow checking | `ownedc.h` | — |
| Static lint | `ownedc_lint.py` | Python 3.x |
| Thread ownership verification | `ownedc.h` | OS threading |
| Arenas | `ownedc.h` (`safe_region`) | — |
| Safe collections (vector/string/array) | `ownedc.h` | — |
| CHERI capability upgrade | `ownedc_cheri.h` | CHERI toolchain / hardware |
| C++ RAII wrappers | `ownedc.hpp` | C++ compiler |
| Mutex / deadlock safety | `ownedc.h` (`owned_mutex_t`) | OS threading |
| Managed threads | `ownedc.h` (`OWNED_THREAD`) | OS threading |
| Futures / MPSC channels | `ownedc_future.c`, `ownedc_channel.c` | OS threading |
| Safe file / socket I/O | `ownedc.h` (`safe_file_t`, `safe_socket_t`) | OS support |
| Pluggable allocators | `ownedc_set_allocators()` | jemalloc / mimalloc (optional) |
| Reference counting + cycle GC | `ownedc.h` (`owned_rc_t`) | — |
| Profiler dashboard | `tools/ownedc_profiler.py` | Python 3.x |
| Bare-metal / kernel mode | `OWNEDC_NO_STDLIB` | Disables threading, sockets, file I/O |

## Comparison to Prior Art

| Project | Approach | Trade-off vs. OwnedC |
|---|---|---|
| Checked C (Microsoft Research) | New pointer syntax (`_Ptr`, `_Array_ptr`) | Requires source rewrite; OwnedC stays closer to standard C syntax |
| AddressSanitizer | Compile-time instrumentation, debug-build use | ~2-3x overhead, diagnostic-only; OwnedC's `safe_region` targets production hot paths |
| CHERI / Morello | Hardware-enforced capability pointers | OwnedC integrates with CHERI rather than competing with it |
| Rust | Compile-time ownership, zero-cost | Requires a rewrite; OwnedC targets legacy C that can't absorb one |

## Building from Source

**Prerequisites:** GCC or Clang (required for `OWNED` RAII), CMake 3.10+, Python 3.x (optional — static lint and profiler HTML generation).

```bash
git clone https://github.com/PandiaJason/OwnedC.git
cd OwnedC
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

Demonstration binaries are built to `build/`, covering each subsystem in the Feature Matrix above — run `./build/demo_raii`, `./build/demo_threads`, etc. individually, or `./build/demo_full_showcase` for a combined walkthrough.

## Security

OwnedC has not had an external security audit. Do not rely on it for memory safety guarantees in security-critical contexts until a 1.0 release. To report a suspected vulnerability, use GitHub's private vulnerability reporting (Security tab → Report a vulnerability) rather than a public issue.

## Contributing

Issues and pull requests are welcome. For anything beyond a small fix, please open an issue describing the change first — the ownership model has subtle invariants shared across the registry, arena, and CHERI backends, and a short design discussion up front avoids rework.

## Roadmap

- Profile and fix the hash-map collision behavior driving `owner_malloc` overhead at scale.
- Multi-threaded arena support (currently single-thread by design).
- Promote Experimental subsystems to Beta as test coverage lands, starting with concurrency primitives.
- External security review ahead of a 1.0 release.

## License

GNU General Public License v3.0 (GPLv3). See [LICENSE](LICENSE) for full details.
