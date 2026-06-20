#include "ownedc_arena.h"
#include "ownedc_env.h"
#include <stdint.h>

typedef struct arena_block {
    struct arena_block* next;
    size_t size;
    size_t used;
    uint8_t data[];
} arena_block_t;

struct ownedc_arena {
    arena_block_t* head;
    arena_block_t* current;
    size_t default_block_size;
};

static arena_block_t* allocate_block(size_t size) {
    // We allocate arena blocks using owner_malloc to track the huge block globally,
    // but the individual allocations within it are completely lock-free.
    arena_block_t* block = (arena_block_t*)owner_malloc(sizeof(arena_block_t) + size);
    if (!block) return NULL;
    block->next = NULL;
    block->size = size;
    block->used = 0;
    return block;
}

ownedc_arena_t* ownedc_arena_create(size_t block_size) {
    if (block_size < 1024) block_size = 1024;
    
    // The arena metadata is tracked by the global registry
    ownedc_arena_t* arena = (ownedc_arena_t*)owner_malloc(sizeof(ownedc_arena_t));
    if (!arena) return NULL;
    
    arena->default_block_size = block_size;
    
    arena_block_t* initial = allocate_block(block_size);
    arena->head = initial;
    arena->current = initial;
    
    return arena;
}

void* ownedc_arena_alloc(ownedc_arena_t* arena, size_t size) {
    if (!arena || !arena->current) return NULL;
    
    // 8-byte alignment
    size_t aligned_size = (size + 7) & ~7;
    
    if (arena->current->used + aligned_size <= arena->current->size) {
        void* ptr = arena->current->data + arena->current->used;
        arena->current->used += aligned_size;
        return ptr;
    }
    
    // Need a new block
    size_t new_size = arena->default_block_size;
    if (aligned_size > new_size) {
        new_size = aligned_size;
    }
    
    arena_block_t* new_block = allocate_block(new_size);
    if (!new_block) return NULL;
    
    arena->current->next = new_block;
    arena->current = new_block;
    
    void* ptr = new_block->data;
    new_block->used += aligned_size;
    
    return ptr;
}

void ownedc_arena_destroy(ownedc_arena_t* arena) {
    if (!arena) return;
    
    arena_block_t* curr = arena->head;
    while (curr) {
        arena_block_t* next = curr->next;
        owner_free(curr);
        curr = next;
    }
    
    owner_free(arena);
}
