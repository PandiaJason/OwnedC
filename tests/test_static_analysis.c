#include <stdlib.h>
#include "ownedc.h"

int main(void) {
    // This should trigger a warning: use of raw malloc
    void* p1 = malloc(10);
    
    // This should trigger an error: use of raw free
    free(p1);

    // This should trigger a warning: owner_malloc used without OWNED
    void* p2 = owner_malloc(20);
    
    // This is fine
    OWNED void* p3 = owner_malloc(30);

    return 0;
}
