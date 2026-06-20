#include "ownedc_region.h"
#include <stdio.h>
#include <string.h>

int main() {
    printf("--- OwnedC High-Performance Region (Arena) Demo ---\n");
    
    // Create a region with 4KB chunks
    safe_region_t* arena = safe_region_create(4096);
    
    printf("Allocating multiple small objects from the arena...\n");
    
    // Fast pointer-bump allocations, bypassing the global hash map
    for (int i = 0; i < 1000; i++) {
        void* entity = safe_region_alloc(arena, 32);
        // Do something with entity
        memset(entity, 0, 32);
    }
    
    printf("Successfully allocated 1000 objects efficiently.\n");
    
    // Free the entire arena at once
    safe_region_free(arena);
    printf("Arena completely freed.\n");
    
    return 0;
}
