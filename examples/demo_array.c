#include "ownedc.h"
#include "ownedc_array.h"
#include "ownedc_string.h"
#include <stdio.h>

void build_and_process_array(void) {
    printf("Creating deep-freeing array of capacity 3...\n");
    
    // 1. Create a safe array of pointers
    OWNED_ARRAY owned_array_t* arr = owned_array_new(3);
    
    // 2. Create some owned strings
    safe_string_t* str1 = safe_string_new("Apple");
    safe_string_t* str2 = safe_string_new("Banana");
    safe_string_t* str3 = safe_string_new("Cherry");
    
    // 3. Store them in the array (Transferring ownership to the array!)
    owned_array_set(arr, 0, str1);
    owned_array_set(arr, 1, str2);
    owned_array_set(arr, 2, str3);
    
    // 4. Access elements safely
    printf("Item 1: %s\n", safe_string_cstr((safe_string_t*)owned_array_get(arr, 0)));
    printf("Item 2: %s\n", safe_string_cstr((safe_string_t*)owned_array_get(arr, 1)));
    printf("Item 3: %s\n", safe_string_cstr((safe_string_t*)owned_array_get(arr, 2)));
    
    // 5. Bounds-check test
    // owned_array_get(arr, 5); // This would safely abort
    
    printf("Returning from function. The array AND all strings will be instantly freed!\n");
}

int main(void) {
    printf("--- OwnedC Safe Array Demo ---\n");
    build_and_process_array();
    
    return 0;
}
