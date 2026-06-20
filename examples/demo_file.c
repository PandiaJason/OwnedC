#include "ownedc.h"
#include "ownedc_file.h"
#include <stdio.h>

void read_file_safely(void) {
    // 1. Open the file safely. It will auto-close when the function returns.
    OWNED_FILE safe_file_t* file = safe_fopen("test_owned_file.txt", "w");
    if (!file) {
        printf("Failed to open file!\n");
        return;
    }
    
    // 2. Write something to it using the raw FILE*
    FILE* raw_file = safe_file_get(file);
    if (raw_file) {
        fprintf(raw_file, "Hello, Safe Resource Management in C!\n");
        printf("Wrote to test_owned_file.txt successfully.\n");
    }
    
    printf("Returning early! The file descriptor will be closed automatically.\n");
    // We intentionally return early WITHOUT calling fclose(). 
    // The OWNED_FILE macro guarantees it is closed!
}

int main(void) {
    printf("--- OwnedC Safe File Demo ---\n");
    read_file_safely();
    return 0;
}
