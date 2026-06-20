#include "ownedc_collections.h"
#include <stdio.h>

int main() {
    printf("--- OwnedC Safe Vector Demo ---\n");
    
    // Create a vector for integers with initial capacity of 2
    OWNED safe_vector_t* vec = safe_vector_create(2);
    
    int a = 10, b = 20, c = 30;
    
    printf("Pushing %d\n", a);
    safe_vector_push(vec, &a);
    
    printf("Pushing %d\n", b);
    safe_vector_push(vec, &b);
    
    // Vector will safely resize here
    printf("Pushing %d (Resizing)\n", c);
    safe_vector_push(vec, &c);
    
    printf("Elements:\n");
    for (size_t i = 0; i < vec->length; i++) {
        int* val = (int*)safe_vector_get(vec, i);
        printf("  [%zu] = %d\n", i, *val);
    }
    
    // The vector and its internal buffer are automatically freed when vec goes out of scope
    return 0;
}
