#include "ownedc_env.h"


void ownedc_diagnostics_fatal(const char* reason, void* ptr, const char* file, int line) {
    OWNEDC_PRINTF("\n========================================\n");
    OWNEDC_PRINTF("OwnedC: Ownership Violation Detected\n");
    OWNEDC_PRINTF("========================================\n");
    OWNEDC_PRINTF("Reason: %s\n", reason);
    if (ptr) {
        OWNEDC_PRINTF("Object Ptr: %p\n", ptr);
    }
    if (file) {
        OWNEDC_PRINTF("Location: %s:%d\n", file, line);
    }
    OWNEDC_PRINTF("Program terminated.\n");
    OWNEDC_PRINTF("========================================\n\n");
    OWNEDC_EXIT(1);
}



void ownedc_diagnostics_report_leak(void* ptr, const char* file, int line, size_t size) {
    OWNEDC_PRINTF("\n========================================\n");
    OWNEDC_PRINTF("OwnedC: Memory Leak Detected\n");
    OWNEDC_PRINTF("========================================\n");
    OWNEDC_PRINTF("Object Ptr: %p\n", ptr);
    OWNEDC_PRINTF("Size: %zu bytes\n", size);
    OWNEDC_PRINTF("Allocated at: %s:%d\n", file ? file : "<unknown>", line);
    OWNEDC_PRINTF("========================================\n\n");
}
