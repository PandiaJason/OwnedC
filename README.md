<p align="center">
  <img src="assets/logo.jpg" alt="OwnedC Logo" width="200"/>
</p>

<h1 align="center">OwnedC</h1>
<p align="center"><i>Ownership verification and memory safety diagnostics for C — without rewriting your codebase.</i></p>

<p align="center">
  <a href="https://github.com/PandiaJason/OwnedC/actions/workflows/ci.yml"><img src="https://github.com/PandiaJason/OwnedC/actions/workflows/ci.yml/badge.svg" alt="CI"/></a>
  <img src="https://img.shields.io/badge/license-GPLv3-blue.svg" alt="License"/>
  <img src="https://img.shields.io/badge/standard-C99%20%2F%20C11-informational.svg" alt="C Standard"/>
  <img src="https://img.shields.io/badge/status-research%20prototype-orange.svg" alt="Status"/>
</p>

<p align="center"><b>Stop rewriting legacy C in Rust. Retrofit ownership verification, RAII, and memory safety diagnostics into SQLite, OpenSSL, and your own C codebases without modifying their source code—using standard compiler extensions.</b></p>

---

## Overview

OwnedC is an ownership verification framework for C that brings ownership tracking, scope-bound RAII, and dynamic ownership validation to existing codebases, without requiring a migration to a different language. It is implemented as a dual-layer system: a runtime metadata registry performs dynamic ownership and borrow verification, and a compiled static analyzer (`ownedc_lint`) performs best-effort compile-time linting against the same ownership rules.

OwnedC is a research prototype. It has not undergone external security review, and the performance characteristics of its core tracking path are documented, not hidden — see [Performance](#performance) before deciding where in your codebase to use it.

## Core Concept: Standard C with Dynamic Ownership Verification

OwnedC is **not a new programming language** or compiler fork. It is a library and tooling extension for standard C (C99/C11) that runs on standard compilers (GCC/Clang). It brings modern language-level safety paradigms directly into standard C:

* **Scope-Bound RAII**: Using compiler cleanup attributes (`__attribute__((cleanup))`), the `OWNED` macro automatically frees dynamic memory when variables leave their block scope. This replicates C++ destructors or Rust's automatic drop mechanisms.
* **Dynamic Ownership Verifier**: A thread-safe runtime metadata registry intercepts allocations and frees, dynamically verifying ownership boundaries (automatically flagging double-frees and use-after-free conditions).
* **Native Static Analyzer**: A compiled C binary (`ownedc_lint`) scans source files at build time to detect leaks, double-frees, and use-after-free issues statically, without requiring Python or external runtimes.
* **Zero-Rewrite Integration**: Unlike Rust or Zig, which require a complete codebase rewrite, OwnedC allows legacy C libraries (like SQLite, cJSON, libxml2, and OpenSSL) to integrate with ownership verification instantly by simply swapping their internal memory allocation hooks.

## Project Status

OwnedC is pre-1.0. APIs may change without notice between commits. The table below reflects maturity per subsystem as of this writing; it should be kept current as test coverage changes, and verified against actual CI results rather than taken at face value.

| Subsystem | Status | Notes |
|---|---|---|
| RAII (`OWNED`, `owner_malloc`) | Beta | Functionally correct; high per-call overhead (see Performance) |
| Dynamic Ownership Verification | Beta | Aborts on violation; covered by `tests/` |
| Static Lint (`ownedc_lint`) | Beta | Heuristic, not a sound type system — see Non-Goals |
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

- **Not a sound static analyzer.** `ownedc_lint` is heuristic. It catches common misuse patterns; it does not provide the compile-time guarantees of a type system like Rust's borrow checker.
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

**Methodology:** Apple Silicon (macOS), Clang 15, `-O3`-equivalent release build via CMake, 500,000 allocations of 32 bytes each, via `tests/benchmark.c`. Updated following registry optimization (prefixed headers and sharded locks); previous numbers reflected hash-map collision degradation at scale.

| Allocation Strategy | Single-Thread | Overhead | 8-Thread (Contention) | Overhead |
|---|---|---|---|---|
| Raw `malloc`/`free` | 0.0180s | 1.0x (baseline) | 0.0032s | 1.0x (baseline) |
| `owner_malloc`/`free` | 0.0534s | 2.96x | 0.2261s | 70.25x |
| `safe_region` (arena) | 0.0045s | 0.25x (4.0x faster) | N/A — single-threaded by design | — |

**Guidance:** `owner_malloc` is optimized using prefix headers for O(1) metadata resolution (avoiding global lookups) and sharded locks to distribute thread contention. For performance-critical hot paths, `safe_region` remains the optimal zero-overhead choice.

## Architecture

```
                  ┌─────────────────────────┐
                  │       ownedc_lint (CI)  │   compile-time, heuristic
                  └────────────┬─────────────┘
                               │
    source ──────────────────►│  build
                               │
                  ┌────────────▼─────────────┐
                  │   ownedc.h runtime core  │
                  │(registry + ownership chk)│
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
| Dynamic ownership verification | `ownedc.h` | — |
| Static lint | `ownedc_lint` | — |
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
| Bare-metal / kernel mode | `OWNEDC_NO_STDLIB` | Disables threading, sockets, file I/O |

## Comparison to Prior Art

| Project | Approach | Trade-off vs. OwnedC |
|---|---|---|
| Checked C (Microsoft Research) | New pointer syntax (`_Ptr`, `_Array_ptr`) | Requires source rewrite; OwnedC stays closer to standard C syntax |
| AddressSanitizer | Compile-time instrumentation, debug-build use | ~2-3x overhead, diagnostic-only; OwnedC's `safe_region` targets production hot paths |
| CHERI / Morello | Hardware-enforced capability pointers | OwnedC integrates with CHERI rather than competing with it |
| Rust | Compile-time ownership, zero-cost guarantees | Requires a complete codebase rewrite; OwnedC retrofits verification onto legacy C |

## Language Comparison Matrix

| Dimension | C / C++ | OwnedC | Rust |
|---|---|---|---|
| **Memory Safety** | Manual (unsafe) | Dynamic Tracking (diagnostics) | Compile-time Checked (safe) |
| **Runtime Overhead** | None | Low (Arenas) to Medium (Tracked Alloc) | Zero (Zero-cost abstraction) |
| **Compile-time Safety** | Weak | Heuristic Static Analysis (non-sound) | Strong (Static Borrow Checker) |
| **Concurrency Model** | Unsafe (races possible) | Thread ownership verification | Thread safety enforced at compile-time |
| **Retrofitting Legacy C** | Native | **Yes (Zero-code changes via allocator config)** | No (Requires complete rewrite) |
| **Binary size & VM** | Tiny | Small (Tiny runtime library) | Small |

## Real-World Showcase: Memory-Safe SQLite Integration

To prove that OwnedC is capable of bringing memory safety to industry-grade C tools, we integrated it directly into **SQLite (v3.47.0)**. 

By binding SQLite's pluggable memory allocator methods (`sqlite3_mem_methods`) to point directly to OwnedC's dynamic registry (`owner_malloc`, `owner_free`, `owner_realloc`, and `owner_malloc_usable_size`), all internal database allocations (such as page caches, node data, and parser buffers) are tracked.

```
                      ┌─────────────────────────┐
                      │    sqlite_ownedc (App)  │
                      └────────────┬────────────┘
                                   │
                         [Configures Allocator]
                                   │
                                   ▼
                      ┌─────────────────────────┐
                      │       SQLite Core       │
                      └────────────┬────────────┘
                                   │
                      [Redirects Heap Allocations]
                                   │
                                   ▼
                      ┌─────────────────────────┐
                      │   sqlite3_mem_methods   │
                      └────────────┬────────────┘
                                   │
                         [Allocator Interface]
                                   │
                                   ▼
                      ┌─────────────────────────┐
                      │ OwnedC Runtime Registry │
                      │  (Tracks size/borrows)  │
                      └──────┬────────────┬─────┘
                             │            │
             [Double-Frees]  │            │  [Statement Leaks]
                             ▼            ▼
                      ┌──────────┐   ┌──────────┐
                      │ Runtime  │   │   Leak   │
                      │  Abort   │   │  Report  │
                      └──────────┘   └──────────┘
```

### Running the SQLite Showcase

Build the project normally using CMake, then execute the following targets:

1. **Normal Run (0 Leaks)**:
   ```bash
   ./build/sqlite_ownedc
   ```
   *Expected Output*: Performs table creations, insertions, and SELECT queries. Prints:
   `OwnedC Stats: 0 active allocations, 196 freed allocations.` on clean exit.

2. **Simulated Leak Interception**:
   ```bash
   ./build/sqlite_ownedc --simulate-leak
   ```
   *Expected Output*: Simulates a bug where statement handles are unfinalized. OwnedC intercepts the unreleased statements on exit and dumps all 58 leaks:
   `OwnedC: Found 58 memory leaks. Terminating.`

3. **Double-Free Interception**:
   ```bash
   ./build/sqlite_ownedc --simulate-double-free
   ```
   *Expected Output*: Attempts to release an SQLite pointer twice. OwnedC's dynamic ownership verifier catches this instantly at runtime, printing:
   `OwnedC: Ownership Violation Detected: Double-free` and terminating safely before memory corruption occurs.

### Why This Proves OwnedC is Powerful

Integrating with a production-grade codebase like SQLite provides a definitive proof-of-concept for OwnedC's model:
- **Zero-Intrusion Legacy Safety**: We didn't modify a single line of SQLite's 240,000+ line codebase. By simply swapping the allocator configuration at runtime, the entire engine runs under memory-safe boundaries.
- **Deterministic Exploit Interception**: A double-free or use-after-free under standard C leads to silent heap corruption and security vulnerabilities. Under OwnedC, the runtime registry intercepts it immediately and panics safely.
- **Deep Allocation Visibility**: OwnedC successfully tracked, indexed, and freed all 196 dynamic memory allocations made during SQLite's table parsing, prepared statement executions, and connection cycles.

## Real-World Showcase: Production Library Integrations (cJSON, libxml2, OpenSSL)

To demonstrate how OwnedC scales across multiple distinct libraries in a single project, we built a showcase linking cJSON, libxml2, and OpenSSL simultaneously:

- **cJSON**: Configured using `cJSON_InitHooks` to point to OwnedC.
- **libxml2**: Swapped allocators via `xmlMemSetup`, capturing custom string duplications via `xmlStrdupFunc`.
- **OpenSSL**: Replaced allocator functions using `CRYPTO_set_mem_functions`. Since OpenSSL exposes the calling file name and line number in its allocator hooks, OwnedC prints the exact line inside OpenSSL's internal codebase where any leak occurs.

### Running the Multi-Library Showcase

Build the project using CMake, then execute the showcase binary:

1. **Normal Clean Run**:
   ```bash
   ./build/real_libs_ownedc
   ```
   *Expected Output*: Parses JSON, parses XML, and runs a SHA256 digest hash computation. It outputs a clean finished status. OpenSSL's lazy-initialized global structures are torn down automatically on exit before leak detection runs, resulting in 0 leaks reported.

2. **Simulated Leak Interception**:
   ```bash
   ./build/real_libs_ownedc --simulate-leak
   ```
   *Expected Output*: Simulates a leak where an OpenSSL message digest context is left unallocated/unfreed. OwnedC intercepts this at exit and outputs the exact source location of the leak:
   ```
   ========================================
   OwnedC: Memory Leak Detected
   ========================================
   Object Ptr: 0x1032ea900
   Size: 72 bytes
   Allocated at: crypto/evp/digest.c:131
   ========================================
   ```

### Key Findings from Multi-Library Integration

* **Unified Allocator Redirects Across Diverse Library APIs**: Swapping allocators on multiple production-grade libraries concurrently is fully functional using their native hook mechanisms (`cJSON_InitHooks`, `xmlMemSetup`, and `CRYPTO_set_mem_functions`).
* **Precise Internals Visibility (OpenSSL File/Line Context)**: Since OpenSSL allocator hooks forward the exact source file and line number of the calling location, OwnedC successfully traces leaks back to specific lines inside OpenSSL's internal source files (e.g., `crypto/evp/digest.c:131`).
* **OpenSSL/libxml2 Lazy Globals Cleanup**: Internal global static structures allocated dynamically on first use by OpenSSL and libxml2 are automatically cleaned up at process exit prior to OwnedC's exit checker, ensuring a clean 0-leak execution report.

## Building from Source

**Prerequisites:** GCC or Clang (required for `OWNED` RAII), CMake 3.10+.

```bash
git clone https://github.com/PandiaJason/OwnedC.git
cd OwnedC
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

Demonstration binaries are built to `build/`, covering each subsystem in the Feature Matrix above — run `./build/demo_raii`, `./build/demo_threads`, etc. individually, or `./build/demo_full_showcase` for a combined walkthrough.

## Comparison to Existing Safety Tools

If you are considering adopting or evaluating OwnedC, you may wonder how it compares to standard memory analysis tools:

### Why not AddressSanitizer (ASan)?
AddressSanitizer is a diagnostics-only compiler tool designed for debug/testing builds. It imposes a 2-3x CPU overhead and 2-4x memory overhead (due to massive shadow memory tables). It is not viable for production deployments. OwnedC's `safe_region` arenas run at zero overhead (often outperforming raw `malloc`), and its dynamic registry runs with minimal single-thread overhead (~3x), making it realistic to evaluate in production-like systems.

### Why not Valgrind?
Valgrind is an CPU-emulator-based analysis tool. Because it emulates instructions, it slows execution by 10x to 50x, making it entirely unusable outside of offline developer testing. OwnedC is a lightweight, natively compiled C library that links directly to the application.

### Why not Rust?
Rust provides compile-time safety guarantees and zero-cost abstractions, but adopting it requires rewriting millions of lines of legacy C codebases. OwnedC retrofits ownership verification and memory safety diagnostics onto legacy C applications in 5 minutes via pluggable allocator hooks, without changing the source code.

### What bugs can OwnedC detect that ASan cannot?
* **Thread-Ownership Violations**: ASan does not detect when Thread A frees memory allocated by Thread B. OwnedC validates thread ownership and aborts immediately.
* **Active Borrow Violations**: ASan cannot detect when you free an object that has active references/borrows. OwnedC tracks borrows and blocks premature deallocations.
* **Non-Heap Resource Leaks**: OwnedC's `OWNED` RAII model scales to non-heap system resources (like files and sockets), whereas ASan exclusively tracks memory.

## Security

OwnedC has not had an external security audit. Do not rely on it for memory safety guarantees in security-critical contexts until a 1.0 release. To report a suspected vulnerability, use GitHub's private vulnerability reporting (Security tab → Report a vulnerability) rather than a public issue.

## Contributing

Issues and pull requests are welcome. For anything beyond a small fix, please open an issue describing the change first — the ownership model has subtle invariants shared across the registry, arena, and CHERI backends, and a short design discussion up front avoids rework.

## Roadmap

- Multi-threaded arena support (currently single-thread by design).
- Promote Experimental subsystems to Beta as test coverage lands, starting with concurrency primitives.
- External security review ahead of a 1.0 release.

## License

GNU General Public License v3.0 (GPLv3). See [LICENSE](LICENSE) for full details.
