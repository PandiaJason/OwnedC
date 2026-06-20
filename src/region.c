#include "ownedc_region.h"


struct safe_region_chunk {
    size_t used;
    size_t capacity;
    struct safe_region_chunk* next;
    unsigned char data[]; // Flexible array member
};

safe_region_t* safe_region_create(size_t chunk_size) {
    if (chunk_size == 0) chunk_size = 4096;
    
    safe_region_t* region = (safe_region_t*)owner_malloc(sizeof(safe_region_t));
    if (!region) return NULL;
    
    safe_region_chunk_t* first_chunk = (safe_region_chunk_t*)owner_malloc(sizeof(safe_region_chunk_t) + chunk_size);
    if (!first_chunk) {
        owner_free(region);
        return NULL;
    }
    
    first_chunk->used = 0;
    first_chunk->capacity = chunk_size;
    first_chunk->next = NULL;
    
    region->current_chunk = first_chunk;
    region->chunk_size = chunk_size;
    
    return region;
}

void* safe_region_alloc(safe_region_t* region, size_t size) {
    if (!region || size == 0) return NULL;
    
    // Ensure proper alignment (e.g., 8 bytes)
    size = (size + 7) & ~7;
    
    safe_region_chunk_t* chunk = region->current_chunk;
    
    // Check if we need a new chunk
    if (chunk->used + size > chunk->capacity) {
        size_t new_cap = (size > region->chunk_size) ? size : region->chunk_size;
        safe_region_chunk_t* new_chunk = (safe_region_chunk_t*)owner_malloc(sizeof(safe_region_chunk_t) + new_cap);
        if (!new_chunk) return NULL;
        
        new_chunk->used = 0;
        new_chunk->capacity = new_cap;
        new_chunk->next = region->current_chunk;
        
        region->current_chunk = new_chunk;
        chunk = new_chunk;
    }
    
    void* ptr = chunk->data + chunk->used;
    chunk->used += size;
    return ptr;
}

void safe_region_free(safe_region_t* region) {
    if (!region) return;
    
    safe_region_chunk_t* chunk = region->current_chunk;
    while (chunk) {
        safe_region_chunk_t* next = chunk->next;
        owner_free(chunk);
        chunk = next;
    }
    
    owner_free(region);
}
