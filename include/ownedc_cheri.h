#ifndef OWNEDC_CHERI_H
#define OWNEDC_CHERI_H

#include "ownedc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CHERI (Capability Hardware Enhanced RISC Instructions) Integration
 * 
 * When compiled on a CHERI-enabled architecture (like Morello), 
 * OwnedC automatically upgrades standard allocations into hardware-enforced 
 * capabilities with strict bounds and permissions.
 */

#if defined(__CHERI__) || defined(__CHERI_PURE_CAPABILITY__)

#include <cheriintrin.h>

/* CHERI Hardware Capabilities */
static inline void* owner_malloc_cheri(size_t size) {
    void* ptr = owner_malloc(size);
    if (!ptr) return NULL;
    // Set precise hardware bounds on the capability
    return cheri_bounds_set(ptr, size);
}

static inline void* owner_calloc_cheri(size_t num, size_t size) {
    void* ptr = owner_calloc(num, size);
    if (!ptr) return NULL;
    return cheri_bounds_set(ptr, num * size);
}

#else

/* 
 * Standard Hardware Fallback 
 * 
 * If CHERI hardware is not available, we fall back to standard software-based
 * bounds checking and runtime analysis provided by OwnedC.
 */
static inline void* owner_malloc_cheri(size_t size) {
    return owner_malloc(size);
}

static inline void* owner_calloc_cheri(size_t num, size_t size) {
    return owner_calloc(num, size);
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* OWNEDC_CHERI_H */
