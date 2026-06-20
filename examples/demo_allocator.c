#include "ownedc.h"
#include <stdio.h>
#include <stdlib.h>

// Mocking a custom game-engine or enterprise allocator (like jemalloc)
int custom_allocs = 0;
int custom_frees = 0;

void* my_custom_malloc(size_t size) {
    custom_allocs++;
    printf("  [Custom Allocator] Allocating %zu bytes...\n", size);
    return malloc(size);
}

void my_custom_free(void* ptr) {
    if (ptr) {
        custom_frees++;
        printf("  [Custom Allocator] Freeing pointer...\n");
        free(ptr);
    }
}

void* my_custom_realloc(void* ptr, size_t size) {
    printf("  [Custom Allocator] Reallocating to %zu bytes...\n", size);
    return realloc(ptr, size);
}

void test_custom_allocator(void) {
    // Inject our custom allocator into OwnedC
    ownedc_set_allocators(my_custom_malloc, my_custom_free, my_custom_realloc);
    
    printf("--- OwnedC Pluggable Allocator Demo ---\n");
    printf("Allocating an integer array...\n");
    
    // This will now internally call `my_custom_malloc`!
    OWNED int* numbers = (int*)owner_malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) numbers[i] = i * 10;
    
    printf("Reallocating...\n");
    numbers = (int*)owner_realloc(numbers, 10 * sizeof(int));
    
    printf("Returning early. The OWNED macro will trigger cleanup...\n");
    // When returning, `owner_free_cleanup` fires, which eventually calls `my_custom_free`!
}

int main(void) {
    test_custom_allocator();
    
    printf("\nValidating custom allocator stats:\n");
    printf("Custom Allocs: %d\n", custom_allocs);
    printf("Custom Frees: %d\n", custom_frees);
    
    // OwnedC allocates 1 data block + 1 persistent metadata block per owner_malloc
    if (custom_allocs > 0 && custom_allocs == custom_frees * 2) {
        printf("SUCCESS: OwnedC routed through the custom allocator perfectly!\n");
        printf("Note: The extra allocation is the persistent metadata registry block.\n");
    } else {
        printf("FAILED: Custom allocator count mismatch.\n");
    }
    
    return 0;
}
