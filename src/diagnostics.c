#include <stdio.h>
#include <stdlib.h>

void ownedc_diagnostics_fatal(const char* reason, void* ptr, const char* file, int line) {
    fprintf(stderr, "\n========================================\n");
    fprintf(stderr, "OwnedC: Ownership Violation Detected\n");
    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Reason: %s\n", reason);
    if (ptr) {
        fprintf(stderr, "Object Ptr: %p\n", ptr);
    }
    if (file) {
        fprintf(stderr, "Location: %s:%d\n", file, line);
    }
    fprintf(stderr, "Program terminated.\n");
    fprintf(stderr, "========================================\n\n");
    exit(1);
}

void ownedc_diagnostics_report_leak(void* ptr, const char* file, int line, size_t size) {
    fprintf(stderr, "\n========================================\n");
    fprintf(stderr, "OwnedC: Memory Leak Detected\n");
    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Object Ptr: %p\n", ptr);
    fprintf(stderr, "Size: %zu bytes\n", size);
    fprintf(stderr, "Allocated at: %s:%d\n", file ? file : "<unknown>", line);
    fprintf(stderr, "========================================\n\n");
}
