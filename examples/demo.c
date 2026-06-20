#include "ownedc.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("--- OwnedC Demo ---\n");

    // 1. Safe Allocation
    printf("\n[1] Allocating secure buffer...\n");
    char* buffer = (char*)owner_malloc(256);
    if (!buffer) return 1;

    // Inspect the pointer metadata
    ownership_inspect(buffer);

    // 2. Coexistence with standard malloc
    printf("\n[2] Coexistence with standard C...\n");
    char* legacy = (char*)malloc(100);
    printf("Legacy buffer allocated at %p\n", (void*)legacy);

    // Inspect the legacy pointer (should report UNTRACKED)
    ownership_inspect(legacy);
    
    // Free the legacy pointer
    free(legacy);

    // 3. Stats dump
    printf("\n[3] Current Runtime Stats:\n");
    ownership_stats();

    // 4. Safe Free
    printf("\n[4] Freeing secure buffer...\n");
    owner_free(buffer);

    // Stats after free
    ownership_stats();

    // The program exits cleanly. If we had not called owner_free(buffer),
    // OwnedC would detect the leak and terminate the program with an error.

    printf("\nDemo completed successfully!\n");
    return 0;
}
