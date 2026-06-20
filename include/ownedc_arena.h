#ifndef OWNEDC_ARENA_H
#define OWNEDC_ARENA_H

#include "ownedc.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ownedc_arena ownedc_arena_t;

/**
 * @brief Create a new lock-free bump arena with a specific block size.
 * The arena itself is allocated using the global owner_malloc, but all 
 * allocations inside the arena bypass global tracking for extreme performance.
 */
ownedc_arena_t* ownedc_arena_create(size_t block_size);

/**
 * @brief Allocate memory from the arena.
 * Fast path: bumps a pointer. No locks are acquired.
 */
void* ownedc_arena_alloc(ownedc_arena_t* arena, size_t size);

/**
 * @brief Destroy the arena and free all associated blocks instantly.
 * This drops the arena structure from global tracking as well.
 */
void ownedc_arena_destroy(ownedc_arena_t* arena);

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_ARENA_H
