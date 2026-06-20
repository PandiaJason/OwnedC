#include "ownedc.h"
#include "ownedc_string.h"
#include <stdio.h>

void process_string(void) {
    printf("Starting string processing...\n");
    
    // 1. Create a new Safe String managed by RAII
    OWNED_STRING safe_string_t* str = safe_string_new("Hello");
    printf("Initial: %s (Length: %zu)\n", safe_string_cstr(str), str->length);
    
    // 2. Safely append to it
    safe_string_append(str, ", OwnedC ");
    printf("After append: %s\n", safe_string_cstr(str));
    
    // 3. Append another safe string
    OWNED_STRING safe_string_t* suffix = safe_string_new("World!");
    safe_string_append_safe(str, suffix);
    printf("After safe append: %s (Length: %zu)\n", safe_string_cstr(str), str->length);
    
    // 4. Character manipulation
    safe_string_push_char(str, ' ');
    safe_string_push_char(str, ':');
    safe_string_push_char(str, ')');
    printf("After chars: %s\n", safe_string_cstr(str));
    
    // 5. Bounds checking demonstration
    printf("Character at index 7: '%c'\n", safe_string_char_at(str, 7));
    
    // safe_string_char_at(str, 999); // This would safely abort the program
    
    printf("Returning from function. Both strings will be auto-freed cleanly!\n");
}

int main(void) {
    printf("--- OwnedC Safe String Demo ---\n");
    process_string();
    return 0;
}
