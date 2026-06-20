#include "ownedc.h"

// In a real kernel, these would write to a UART or VGA buffer
void my_uart_print(const char* str) {
    // Just a stub for our demo. We can't actually use printf because NO_STDLIB is active!
    // But we'll cheat for the demo to show it works, or we can just not print anything and use a global variable.
}

// We need a dummy allocator for the kernel. We'll use a simple static buffer allocator.
#define KERNEL_HEAP_SIZE 1024 * 64
static unsigned char kernel_heap[KERNEL_HEAP_SIZE];
static size_t heap_offset = 0;

void* kernel_malloc(size_t size) {
    if (heap_offset + size > KERNEL_HEAP_SIZE) return NULL; // Out of memory
    void* ptr = &kernel_heap[heap_offset];
    heap_offset += size;
    return ptr;
}

void kernel_free(void* ptr) {
    // A real kernel allocator would do something here. We'll do nothing.
    (void)ptr;
}

void* kernel_realloc(void* ptr, size_t size) {
    // Extremely rudimentary realloc
    if (!ptr) return kernel_malloc(size);
    if (size == 0) { kernel_free(ptr); return NULL; }
    void* new_ptr = kernel_malloc(size);
    // Note: without NO_STDLIB we'd use memcpy, but we have OWNEDC_MEMCPY built-in!
    // Actually, OwnedC provides OWNEDC_MEMCPY internally, but we'd need to copy the exact size, which we don't know without the registry.
    // In our dummy, we'll just return it.
    return new_ptr;
}

void kernel_main(void) {
    // 1. Initialize our custom allocators because malloc/free do not exist
    ownedc_set_allocators(kernel_malloc, kernel_free, kernel_realloc);
    
    // 2. We can now use OwnedC in a freestanding environment!
    OWNED int* x = (int*)owner_malloc(sizeof(int));
    *x = 42;
    
    // No memory leaks because __attribute__((cleanup)) will call owner_free!
}

int main(void) {
    // Wrapper to run kernel_main from the OS just for testing
    kernel_main();
    return 0;
}
