#include "ownedc_cheri.h"
#include <stdio.h>

int main(void) {
    printf("--- OwnedC CHERI / Hybrid Compilation Demo ---\n");

    /* 
     * If compiled on a CHERI-enabled system (like Morello), 
     * 'ptr' will be a hardware-enforced capability.
     * On standard systems, it gracefully falls back to OwnedC software tracking.
     */
    OWNED void* ptr = owner_malloc_cheri(100);

    if (ptr) {
        printf("Successfully allocated bounded memory capability at %p\n", ptr);
    }

    // Attempting to do this will trigger a compiler warning due to warn_unused_result
    // if we don't assign it:
    // owner_malloc_cheri(50); // Warning: ignoring return value of function declared with 'warn_unused_result'

    return 0;
}
