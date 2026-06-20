#include "ownedc.h"
#include <stdio.h>

int main(void) {
    printf("Running leak test...\n");
    void* ptr = owner_malloc(100);
    if (!ptr) return 1;

    // We intentionally do not free ptr
    // owner_free(ptr);
    
    // atexit handler should catch this and abort
    return 0;
}
