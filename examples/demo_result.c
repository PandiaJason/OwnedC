#include "ownedc.h"
#include "ownedc_result.h"
#include <stdio.h>
#include <string.h>

// A function that might fail
owned_result_t parse_integer(const char* input) {
    if (!input || strlen(input) == 0) {
        return Err(1); // Error code 1: Empty input
    }
    
    if (input[0] == 'X') {
        return Err(2); // Error code 2: Invalid format
    }
    
    // Success! We dynamically allocate an int and return it wrapped in Ok()
    // We do NOT use OWNED here because we are returning ownership to the caller!
    int* val = (int*)owner_malloc(sizeof(int));
    *val = 42; 
    
    return Ok(val);
}

// A function that uses the TRY macro to propagate errors
owned_result_t process_data(const char* input) {
    void* raw_val = NULL;
    
    // If parse_integer fails, this macro immediately returns Err(code) to the caller!
    TRY_UNWRAP(parse_integer(input), raw_val);
    
    // We do NOT use OWNED here because we are returning ownership to main!
    int* val = (int*)raw_val;
    printf("Processing data... value is %d\n", *val);
    
    return Ok(val);
}

int main(void) {
    printf("--- OwnedC Result Types Demo ---\n");
    
    printf("\nTest 1: Valid Input\n");
    owned_result_t res1 = process_data("ValidString");
    if (res1.is_ok) {
        // Main assumes ownership of the unwrapped value and guarantees cleanup
        int* final_val = (int*)res1.value;
        printf("Success! Unwrapped: %d\n", *final_val);
        owner_free(final_val);
    }
    
    printf("\nTest 2: Invalid Input (Propagated Error)\n");
    owned_result_t res2 = process_data("X-Invalid");
    if (!res2.is_ok) {
        printf("Gracefully caught error code: %d\n", res2.error_code);
    }
    
    printf("\nTest 3: Forcing a Panic via UNWRAP (Uncomment to test)\n");
    // int* crashed_val = (int*)UNWRAP(res2);
    
    return 0;
}
