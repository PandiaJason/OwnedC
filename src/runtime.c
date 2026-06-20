#include "ownedc.h"
#include "ownedc_env.h"

#ifndef OWNEDC_NO_STDLIB
#include <pthread.h>
#define OWNEDC_LOCK() pthread_mutex_lock(&registry_mutex)
#define OWNEDC_UNLOCK() pthread_mutex_unlock(&registry_mutex)
#define OWNEDC_THREAD_T pthread_t
#define OWNEDC_THREAD_SELF() pthread_self()
#define OWNEDC_THREAD_EQUAL(t1, t2) pthread_equal((t1), (t2))
#else
// In NO_STDLIB, threading is disabled by default. If the user is on an RTOS, they must provide their own locks.
#define OWNEDC_LOCK() ((void)0)
#define OWNEDC_UNLOCK() ((void)0)
#define OWNEDC_THREAD_T size_t
#define OWNEDC_THREAD_SELF() 0
#define OWNEDC_THREAD_EQUAL(t1, t2) ((t1) == (t2))
#endif

extern void ownedc_diagnostics_fatal(const char* reason, void* ptr, const char* file, int line);
extern void ownedc_diagnostics_report_leak(void* ptr, const char* file, int line, size_t size);

#define HASH_TABLE_SIZE 1024

typedef struct alloc_meta {
    void* ptr;
    size_t size;
    ownership_state_t state;
    int borrow_count;
    OWNEDC_THREAD_T owner_thread;
    const char* alloc_file;
    int alloc_line;
    const char* free_file;
    int free_line;
    struct alloc_meta* next;
} alloc_meta_t;

static alloc_meta_t* registry[HASH_TABLE_SIZE] = {0};
static int atexit_registered = 0;
#ifndef OWNEDC_NO_STDLIB
static pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

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
    OWNEDC_LOCK();
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
    OWNEDC_UNLOCK();
    if (leaked_count > 0) {
        OWNEDC_PRINTF("OwnedC: Found %d memory leaks. Terminating.\n", leaked_count);
        OWNEDC_EXIT(1);
    }
}

/* Pluggable Allocator Defaults */
static ownedc_malloc_fn global_malloc = OWNEDC_DEFAULT_MALLOC;
static ownedc_free_fn global_free = OWNEDC_DEFAULT_FREE;
static ownedc_realloc_fn global_realloc = OWNEDC_DEFAULT_REALLOC;

void ownedc_set_allocators(ownedc_malloc_fn m_fn, ownedc_free_fn f_fn, ownedc_realloc_fn r_fn) {
    if (m_fn) global_malloc = m_fn;
    if (f_fn) global_free = f_fn;
    if (r_fn) global_realloc = r_fn;
}

void ownedc_init(void) {
    // Optional explicit initialization
}

/* We use macros in a real header to capture file/line, but for now we'll mock or wrap.
 * To make it compatible, we'll redefine malloc/free using macros later or expose specific versions. 
 * For this phase, we'll just track generic allocations. We'll use __builtin_return_address for simple tracking or just pass unknown.
 * Wait, without macros we can't get file/line easily. Let's add macros to ownedc.h later, or just accept NULL file/line for standard calls.
 */

void* owner_malloc_internal(size_t size, const char* file, int line) {
    OWNEDC_LOCK();
    if (!atexit_registered) {
#ifndef OWNEDC_NO_STDLIB
        atexit(check_leaks);
#endif
        atexit_registered = 1;
    }
    OWNEDC_UNLOCK();

    void* ptr = global_malloc(size);
    if (!ptr) return NULL;

    alloc_meta_t* meta = (alloc_meta_t*)global_malloc(sizeof(alloc_meta_t));
    if (!meta) {
        global_free(ptr);
        return NULL;
    }

    meta->ptr = ptr;
    meta->size = size;
    meta->state = OWNEDC_STATE_OWNED;
    meta->borrow_count = 0;
    meta->owner_thread = OWNEDC_THREAD_SELF();
    meta->alloc_file = file ? file : "<unknown>";
    meta->alloc_line = line;
    meta->free_file = NULL;
    meta->free_line = 0;

    OWNEDC_LOCK();
    insert_meta(meta);
    OWNEDC_UNLOCK();

    // Poison memory to help detect uninitialized usage
    OWNEDC_MEMSET(ptr, 0xAA, size);

    return ptr;
}

void* owner_malloc(size_t size) {
    return owner_malloc_internal(size, NULL, 0);
}

void* owner_calloc(size_t num, size_t size) {
    void* ptr = owner_malloc_internal(num * size, NULL, 0);
    if (ptr) OWNEDC_MEMSET(ptr, 0, num * size);
    return ptr;
}

void* owner_realloc(void* ptr, size_t size) {
    if (!ptr) return owner_malloc(size);
    if (size == 0) {
        owner_free(ptr);
        return NULL;
    }

    OWNEDC_LOCK();
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Invalid Realloc (Untracked pointer)", ptr, NULL, 0);
    }
    if (meta->state == OWNEDC_STATE_FREED) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Use-after-free (Realloc on freed pointer)", ptr, NULL, 0);
    }
    if (meta->state != OWNEDC_STATE_SHARED && !OWNEDC_THREAD_EQUAL(meta->owner_thread, OWNEDC_THREAD_SELF())) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Thread Ownership Violation (Realloc)", ptr, NULL, 0);
    }

    void* new_ptr = global_realloc(ptr, size);
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
    OWNEDC_UNLOCK();
    return new_ptr;
}

void owner_free_internal(void* ptr, const char* file, int line) {
    if (!ptr) return;

    OWNEDC_LOCK();
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Invalid free (Untracked pointer)", ptr, file, line);
    }

    if (meta->state == OWNEDC_STATE_FREED) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Double-free", ptr, file, line);
    }

    if (meta->borrow_count > 0) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Freeing borrowed object", ptr, file, line);
    }

    if (meta->state != OWNEDC_STATE_SHARED && !OWNEDC_THREAD_EQUAL(meta->owner_thread, OWNEDC_THREAD_SELF())) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Thread Ownership Violation (Free)", ptr, file, line);
    }

    meta->state = OWNEDC_STATE_FREED;
    meta->free_file = file ? file : "<unknown>";
    meta->free_line = line;
    OWNEDC_UNLOCK();

    // Poison memory to help detect use-after-free
    OWNEDC_MEMSET(ptr, 0xEF, meta->size);

    // We don't actually call free(ptr) yet to allow catching use-after-free via poison.
    // In a real implementation we might quarantine it. For phase 1, we'll free it but keep metadata.
    global_free(ptr);
}

void owner_free(void* ptr) {
    owner_free_internal(ptr, NULL, 0);
}

/* Ownership Operations */
void ownership_transfer(void* from, void* to) { (void)from; (void)to; }

void ownership_claim(void* ptr) {
    if (!ptr) return;
    OWNEDC_LOCK();
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Invalid claim (Untracked pointer)", ptr, NULL, 0);
    }
    // Update the owner to the current thread
    meta->owner_thread = OWNEDC_THREAD_SELF();
    meta->state = OWNEDC_STATE_OWNED; // Reset to purely owned just in case
    OWNEDC_UNLOCK();
}

void ownership_borrow(void* ptr) {
    if (!ptr) return;
    OWNEDC_LOCK();
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Invalid borrow (Untracked pointer)", ptr, NULL, 0);
    }
    if (meta->state == OWNEDC_STATE_FREED) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Use-after-free (Borrow on freed pointer)", ptr, NULL, 0);
    }
    if (meta->state != OWNEDC_STATE_SHARED && !OWNEDC_THREAD_EQUAL(meta->owner_thread, OWNEDC_THREAD_SELF())) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Thread Ownership Violation (Borrow)", ptr, NULL, 0);
    }
    meta->borrow_count++;
    if (meta->state != OWNEDC_STATE_SHARED) {
        meta->state = OWNEDC_STATE_BORROWED;
    }
    OWNEDC_UNLOCK();
}

void ownership_release(void* ptr) {
    if (!ptr) return;
    OWNEDC_LOCK();
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Invalid release (Untracked pointer)", ptr, NULL, 0);
    }
    if (meta->borrow_count > 0) {
        meta->borrow_count--;
        if (meta->borrow_count == 0 && meta->state == OWNEDC_STATE_BORROWED) {
            meta->state = OWNEDC_STATE_OWNED;
        }
    } else {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Release without borrow", ptr, NULL, 0);
    }
    OWNEDC_UNLOCK();
}

void ownership_share(void* ptr) {
    if (!ptr) return;
    OWNEDC_LOCK();
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Invalid share (Untracked pointer)", ptr, NULL, 0);
    }
    if (!OWNEDC_THREAD_EQUAL(meta->owner_thread, OWNEDC_THREAD_SELF())) {
        OWNEDC_UNLOCK();
        ownedc_diagnostics_fatal("Thread Ownership Violation (Share must be called by owner)", ptr, NULL, 0);
    }
    meta->state = OWNEDC_STATE_SHARED;
    OWNEDC_UNLOCK();
}

/* Diagnostics API Stubs */
void ownership_inspect(void* ptr) {
    OWNEDC_LOCK();
    alloc_meta_t* meta = find_meta(ptr);
    if (!meta) {
        OWNEDC_UNLOCK();
        OWNEDC_PRINTF("Object %p: UNTRACKED\n", ptr);
        return;
    }
    OWNEDC_PRINTF("Object %p:\n", ptr);
    OWNEDC_PRINTF("  Size: %zu\n", meta->size);
    OWNEDC_PRINTF("  State: %d\n", meta->state);
    OWNEDC_PRINTF("  Borrows: %d\n", meta->borrow_count);
    OWNEDC_PRINTF("  Owner Thread: %p\n", (void*)meta->owner_thread);
    OWNEDC_PRINTF("  Allocated: %s:%d\n", meta->alloc_file, meta->alloc_line);
    if (meta->state == OWNEDC_STATE_FREED) {
        OWNEDC_PRINTF("  Freed: %s:%d\n", meta->free_file, meta->free_line);
    }
    OWNEDC_UNLOCK();
}

void ownership_dump(void) {
    OWNEDC_LOCK();
    OWNEDC_PRINTF("--- Ownership Dump ---\n");
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        alloc_meta_t* curr = registry[i];
        while (curr) {
            OWNEDC_PRINTF("Ptr %p, Size %zu, State %d, Borrows %d\n", curr->ptr, curr->size, curr->state, curr->borrow_count);
            curr = curr->next;
        }
    }
    OWNEDC_UNLOCK();
}

void ownership_dump_borrows(void) {
    OWNEDC_LOCK();
    OWNEDC_PRINTF("--- Borrowed Objects Dump ---\n");
    int found = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        alloc_meta_t* curr = registry[i];
        while (curr) {
            if (curr->borrow_count > 0) {
                OWNEDC_PRINTF("Ptr %p, Size %zu, Borrows %d\n", curr->ptr, curr->size, curr->borrow_count);
                found++;
            }
            curr = curr->next;
        }
    }
    if (found == 0) {
        OWNEDC_PRINTF("No active borrows.\n");
    }
    OWNEDC_UNLOCK();
}

void ownership_stats(void) {
    OWNEDC_LOCK();
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
    OWNEDC_UNLOCK();
    OWNEDC_PRINTF("OwnedC Stats: %d active allocations, %d freed allocations.\n", active, freed);
    (void)active;
    (void)freed;
}

#ifndef OWNEDC_NO_STDLIB
#include <stdio.h>
void ownership_dump_json(const char* filepath) {
    FILE* f = fopen(filepath, "w");
    if (!f) return;
    
    fprintf(f, "{\n  \"allocations\": [\n");
    
    OWNEDC_LOCK();
    int first = 1;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        alloc_meta_t* curr = registry[i];
        while (curr) {
            if (curr->state != OWNEDC_STATE_FREED) {
                if (!first) fprintf(f, ",\n");
                fprintf(f, "    {\n");
                fprintf(f, "      \"ptr\": \"%p\",\n", curr->ptr);
                fprintf(f, "      \"size\": %zu,\n", curr->size);
                fprintf(f, "      \"state\": \"%d\",\n", curr->state);
                fprintf(f, "      \"owner_thread\": %lld,\n", (long long)curr->owner_thread);
                fprintf(f, "      \"borrow_count\": %d\n", curr->borrow_count);
                fprintf(f, "    }");
                first = 0;
            }
            curr = curr->next;
        }
    }
    OWNEDC_UNLOCK();
    
    fprintf(f, "\n  ]\n}\n");
    fclose(f);
}
#else
void ownership_dump_json(const char* filepath) {
    (void)filepath;
    OWNEDC_PRINTF("ownership_dump_json is not available in NO_STDLIB mode.\n");
}
#endif
