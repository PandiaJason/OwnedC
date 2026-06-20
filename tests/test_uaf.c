#include "ownedc.h"
#include <stdio.h>

int main(void) {
    printf("Running use-after-free realloc test...\n");
    void* ptr = owner_malloc(100);
    if (!ptr) return 1;

    owner_free(ptr);

    // This should trigger a UAF detection because we try to realloc a freed pointer
    owner_realloc(ptr, 200);

    return 0;
}
