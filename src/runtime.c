#include "ownedc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

extern void ownedc_diagnostics_fatal(const char* reason, void* ptr, const char* file, int line);
extern void ownedc_diagnostics_report_leak(void* ptr, const char* file, int line, size_t size);

#define HASH_TABLE_SIZE 1024

typedef struct alloc_meta {
    void* ptr;
    size_t size;
    ownership_state_t state;
    int borrow_count;
    pthread_t owner_thread;
    const char* alloc_file;
    int alloc_line;
    const char* free_file;
    int free_line;
    struct alloc_meta* next;
} alloc_meta_t;

static alloc_meta_t* registry[HASH_TABLE_SIZE] = {0};
static int atexit_registered = 0;
static pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned int hash_ptr(void* ptr) {
    return ((uintptr_t)ptr >> 4) % HASH_TABLE_SIZE;
}

static alloc_meta_t* find_meta(void* ptr) {
    unsigned int idx = hash_ptr(ptr);
    alloc_meta_t* curr = registry[idx];
    while (curr) {
        if (curr->ptr == ptr) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

static void insert_meta(alloc_meta_t* meta) {
    unsigned int idx = hash_ptr(meta->ptr);
    meta->next = registry[idx];
    registry[idx] = meta;
}

static void check_leaks(void) {
    pthread_mutex_lock(&registry_mutex);
    int leaked_count = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        alloc_meta_t* curr = registry[i];
        while (curr) {
            if (curr->state == OWNEDC_STATE_OWNED || curr->state == OWNEDC_STATE_SHARED) {
                ownedc_diagnostics_report_leak(curr->ptr, curr->alloc_file, curr->alloc_line, curr->size);
                leaked_count++;
            }
            curr = curr->next;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
    if (leaked_count > 0) {
        fprintf(stderr, "OwnedC: Found %d memory leaks. Terminating.\n", leaked_count);
        exit(1);
    }
}

/* We use macros in a real header to capture file/line, but for now we'll mock or wrap.
 * To make it compatible, we'll redefine malloc/free using macros later or expose specific versions. 
 * For this phase, we'll just track generic allocations. We'll use __builtin_return_address for simple tracking or just pass unknown.
 * Wait, without macros we can't get file/line easily. Let's add macros to ownedc.h later, or just accept NULL file/line for standard calls.
 */

void* owner_malloc_internal(size_t size, const char* file, int line) {
    pthread_mutex_lock(&registry_mutex);
    if (!atexit_registered) {
        atexit(check_leaks);
        atexit_registered = 1;
    }
    pthread_mutex_unlock(&registry_mutex);

    void* ptr = malloc(size);
    if (!ptr) return NULL;

    alloc_meta_t* meta = (alloc_meta_t*)malloc(sizeof(alloc_meta_t));
    if (!meta) {
        free(ptr);
        return NULL;
    }

    meta->ptr = ptr;
    meta->size = size;
    meta->state = OWNEDC_STATE_OWNED;
    meta->borrow_count = 0;
    meta->owner_thread = pthread_self();
    meta->alloc_file = file ? file : "<unknown>";
    meta->alloc_line = line;
    meta->free_file = NULL;
    meta->free_line = 0;

    pthread_mutex_lock(&registry_mutex);
    insert_meta(meta);
    pthread_mutex_unlock(&registry_mutex);

    // Poison memory to help detect uninitialized usage
    memset(ptr, 0xAA, size);

    return ptr;
}

void* owner_malloc(size_t size) {
    return owner_malloc_internal(size, NULL, 0);
}

void* owner_calloc(size_t num, size_t size) {
    void* ptr = owner_malloc_internal(num * size, NULL, 0);
    if (ptr) memset(ptr, 0, num * size);
    return ptr;
}

void* owner_realloc(void* ptr, size_t size) {
    if (!ptr) return owner_malloc(size);
    if (size == 0) {
        owner_free(ptr);
        return NULL;
    }

    pthread_mutex_lock(&registry_mutex);
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Invalid Realloc (Untracked pointer)", ptr, NULL, 0);
    }
    if (meta->state == OWNEDC_STATE_FREED) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Use-after-free (Realloc on freed pointer)", ptr, NULL, 0);
    }
    if (meta->state != OWNEDC_STATE_SHARED && !pthread_equal(meta->owner_thread, pthread_self())) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Thread Ownership Violation (Realloc)", ptr, NULL, 0);
    }

    void* new_ptr = realloc(ptr, size);
    if (new_ptr && new_ptr != ptr) {
        // Pointer changed, update registry
        unsigned int old_idx = hash_ptr(ptr);
        alloc_meta_t** curr = &registry[old_idx];
        while (*curr) {
            if (*curr == meta) {
                *curr = meta->next;
                break;
            }
            curr = &(*curr)->next;
        }
        meta->ptr = new_ptr;
        meta->size = size;
        insert_meta(meta);
    } else if (new_ptr) {
        meta->size = size;
    }
    pthread_mutex_unlock(&registry_mutex);
    return new_ptr;
}

void owner_free_internal(void* ptr, const char* file, int line) {
    if (!ptr) return;

    pthread_mutex_lock(&registry_mutex);
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Invalid free (Untracked pointer)", ptr, file, line);
    }

    if (meta->state == OWNEDC_STATE_FREED) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Double-free", ptr, file, line);
    }

    if (meta->borrow_count > 0) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Freeing borrowed object", ptr, file, line);
    }

    if (meta->state != OWNEDC_STATE_SHARED && !pthread_equal(meta->owner_thread, pthread_self())) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Thread Ownership Violation (Free)", ptr, file, line);
    }

    meta->state = OWNEDC_STATE_FREED;
    meta->free_file = file ? file : "<unknown>";
    meta->free_line = line;
    pthread_mutex_unlock(&registry_mutex);

    // Poison memory to help detect use-after-free
    memset(ptr, 0xEF, meta->size);

    // We don't actually call free(ptr) yet to allow catching use-after-free via poison.
    // In a real implementation we might quarantine it. For phase 1, we'll free it but keep metadata.
    free(ptr);
}

void owner_free(void* ptr) {
    owner_free_internal(ptr, NULL, 0);
}

/* Ownership Operations Stubs for Phase 1/2 */
void ownership_transfer(void* from, void* to) { (void)from; (void)to; }

void ownership_borrow(void* ptr) {
    if (!ptr) return;
    pthread_mutex_lock(&registry_mutex);
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Invalid borrow (Untracked pointer)", ptr, NULL, 0);
    }
    if (meta->state == OWNEDC_STATE_FREED) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Use-after-free (Borrow on freed pointer)", ptr, NULL, 0);
    }
    if (meta->state != OWNEDC_STATE_SHARED && !pthread_equal(meta->owner_thread, pthread_self())) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Thread Ownership Violation (Borrow)", ptr, NULL, 0);
    }
    meta->borrow_count++;
    if (meta->state != OWNEDC_STATE_SHARED) {
        meta->state = OWNEDC_STATE_BORROWED;
    }
    pthread_mutex_unlock(&registry_mutex);
}

void ownership_release(void* ptr) {
    if (!ptr) return;
    pthread_mutex_lock(&registry_mutex);
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Invalid release (Untracked pointer)", ptr, NULL, 0);
    }
    if (meta->borrow_count > 0) {
        meta->borrow_count--;
        if (meta->borrow_count == 0 && meta->state == OWNEDC_STATE_BORROWED) {
            meta->state = OWNEDC_STATE_OWNED;
        }
    } else {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Release without borrow", ptr, NULL, 0);
    }
    pthread_mutex_unlock(&registry_mutex);
}

void ownership_share(void* ptr) {
    if (!ptr) return;
    pthread_mutex_lock(&registry_mutex);
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Invalid share (Untracked pointer)", ptr, NULL, 0);
    }
    if (!pthread_equal(meta->owner_thread, pthread_self())) {
        pthread_mutex_unlock(&registry_mutex);
        ownedc_diagnostics_fatal("Thread Ownership Violation (Share must be called by owner)", ptr, NULL, 0);
    }
    meta->state = OWNEDC_STATE_SHARED;
    pthread_mutex_unlock(&registry_mutex);
}

/* Diagnostics API Stubs */
void ownership_inspect(void* ptr) {
    pthread_mutex_lock(&registry_mutex);
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        pthread_mutex_unlock(&registry_mutex);
        printf("Object %p: UNTRACKED\n", ptr);
        return;
    }
    printf("Object %p:\n", ptr);
    printf("  Size: %zu\n", meta->size);
    printf("  State: %d\n", meta->state);
    printf("  Borrows: %d\n", meta->borrow_count);
    printf("  Owner Thread: %p\n", (void*)meta->owner_thread);
    printf("  Allocated: %s:%d\n", meta->alloc_file, meta->alloc_line);
    if (meta->state == OWNEDC_STATE_FREED) {
        printf("  Freed: %s:%d\n", meta->free_file, meta->free_line);
    }
    pthread_mutex_unlock(&registry_mutex);
}

void ownership_dump(void) {
    pthread_mutex_lock(&registry_mutex);
    printf("--- Ownership Dump ---\n");
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        alloc_meta_t* curr = registry[i];
        while (curr) {
            printf("Ptr %p, Size %zu, State %d, Borrows %d\n", curr->ptr, curr->size, curr->state, curr->borrow_count);
            curr = curr->next;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
}

void ownership_dump_borrows(void) {
    pthread_mutex_lock(&registry_mutex);
    printf("--- Borrowed Objects Dump ---\n");
    int found = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        alloc_meta_t* curr = registry[i];
        while (curr) {
            if (curr->borrow_count > 0) {
                printf("Ptr %p, Size %zu, Borrows %d\n", curr->ptr, curr->size, curr->borrow_count);
                found++;
            }
            curr = curr->next;
        }
    }
    if (found == 0) {
        printf("No active borrows.\n");
    }
    pthread_mutex_unlock(&registry_mutex);
}

void ownership_stats(void) {
    pthread_mutex_lock(&registry_mutex);
    int active = 0;
    int freed = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        alloc_meta_t* curr = registry[i];
        while (curr) {
            if (curr->state == OWNEDC_STATE_OWNED || curr->state == OWNEDC_STATE_SHARED || curr->state == OWNEDC_STATE_BORROWED) active++;
            else if (curr->state == OWNEDC_STATE_FREED) freed++;
            curr = curr->next;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
    printf("OwnedC Stats: %d active allocations, %d freed allocations.\n", active, freed);
}
