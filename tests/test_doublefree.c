#include "ownedc.h"
#include <stdio.h>

int main(void) {
    printf("Running double-free test...\n");
    void* ptr = owner_malloc(100);
    if (!ptr) return 1;

    owner_free(ptr);
    // This should trigger a double-free detection and abort
    owner_free(ptr);

    return 0;
}
