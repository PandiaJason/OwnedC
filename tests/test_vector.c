#include "ownedc_collections.h"
#include <stdio.h>

int main(void) {
    printf("Running safe_vector bounds test...\n");
    
    safe_vector_t* vec = safe_vector_create(2);
    if (!vec) return 1;

    int a = 10, b = 20, c = 30;

    safe_vector_push(vec, &a);
    safe_vector_push(vec, &b);
    safe_vector_push(vec, &c); // Triggers resize

    // This should abort due to out of bounds access
    safe_vector_get(vec, 5);

    return 0;
}
