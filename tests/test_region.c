#include "ownedc_region.h"
#include <stdio.h>

int main(void) {
    printf("Running region test...\n");

    safe_region_t* region = safe_region_create(1024);
    if (!region) return 1;

    // Fast bump allocation, no global tracking
    void* ptr1 = safe_region_alloc(region, 100);
    void* ptr2 = safe_region_alloc(region, 2000); // Forces new chunk creation

    if (!ptr1 || !ptr2) return 1;

    // Check stats (should only track chunks and the region itself, not ptr1/ptr2)
    ownership_stats();

    // Free the entire region
    safe_region_free(region);

    return 0;
}
