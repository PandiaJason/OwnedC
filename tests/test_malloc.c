#include "ownedc.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    printf("Running basic malloc test...\n");
    void* ptr = owner_malloc(100);
    if (!ptr) {
        printf("owner_malloc failed\n");
        return 1;
    }

    // Write to memory
    memset(ptr, 'A', 100);

    void* ptr2 = owner_calloc(10, sizeof(int));
    if (!ptr2) {
        printf("owner_calloc failed\n");
        return 1;
    }

    void* ptr3 = owner_realloc(ptr, 200);
    if (!ptr3) {
        printf("owner_realloc failed\n");
        return 1;
    }

    owner_free(ptr2);
    owner_free(ptr3);

    printf("Basic malloc test passed!\n");
    return 0; // Success
}
