#include "ownedc.h"
#include "ownedc_env.h"

#ifndef OWNEDC_NO_STDLIB
#include <pthread.h>
#define OWNEDC_LOCK() pthread_mutex_lock(&global_lock)
#define OWNEDC_UNLOCK() pthread_mutex_unlock(&global_lock)
#define OWNEDC_THREAD_T pthread_t
#define OWNEDC_THREAD_SELF() pthread_self()
#define OWNEDC_THREAD_EQUAL(t1, t2) pthread_equal((t1), (t2))
#else
#define OWNEDC_LOCK() ((void)0)
#define OWNEDC_UNLOCK() ((void)0)
#define OWNEDC_THREAD_T size_t
#define OWNEDC_THREAD_SELF() 0
#define OWNEDC_THREAD_EQUAL(t1, t2) ((t1) == (t2))
#endif

extern void ownedc_diagnostics_fatal(const char* reason, void* ptr, const char* file, int line);
extern void ownedc_diagnostics_report_leak(void* ptr, const char* file, int line, size_t size);

#define NUM_SHARDS 64
#define OWNEDC_MAGIC 0x05E5CE1005E5CE10ULL
#define QUARANTINE_SIZE 1024

typedef struct alloc_meta {
    uint64_t magic;
    size_t size;
    ownership_state_t state;
    int borrow_count;
    OWNEDC_THREAD_T owner_thread;
    const char* alloc_file;
    int alloc_line;
    const char* free_file;
    int free_line;
    struct alloc_meta* prev;
    struct alloc_meta* next;
} alloc_meta_t;

static alloc_meta_t* registry_heads[NUM_SHARDS] = {0};
static int atexit_registered = 0;

#ifndef OWNEDC_NO_STDLIB
static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t registry_mutexes[NUM_SHARDS] = {
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER
};
#define SHARD_LOCK(idx) pthread_mutex_lock(&registry_mutexes[idx])
#define SHARD_UNLOCK(idx) pthread_mutex_unlock(&registry_mutexes[idx])

static pthread_mutex_t quarantine_mutex = PTHREAD_MUTEX_INITIALIZER;
#else
#define SHARD_LOCK(idx) ((void)(idx))
#define SHARD_UNLOCK(idx) ((void)(idx))
#endif

static alloc_meta_t* quarantine[QUARANTINE_SIZE] = {0};
static int quarantine_tail = 0;

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

static inline unsigned int get_shard_idx(void* ptr) {
    return ((uintptr_t)ptr >> 4) % NUM_SHARDS;
}

static inline alloc_meta_t* get_meta(void* ptr) {
    if (!ptr) return NULL;
    alloc_meta_t* meta = (alloc_meta_t*)((char*)ptr - sizeof(alloc_meta_t));
    if (meta->magic == OWNEDC_MAGIC) {
        return meta;
    }
    return NULL;
}

static void insert_meta(alloc_meta_t* meta) {
    void* user_ptr = (void*)((char*)meta + sizeof(alloc_meta_t));
    unsigned int idx = get_shard_idx(user_ptr);
    SHARD_LOCK(idx);
    meta->prev = NULL;
    meta->next = registry_heads[idx];
    if (registry_heads[idx]) {
        registry_heads[idx]->prev = meta;
    }
    registry_heads[idx] = meta;
    SHARD_UNLOCK(idx);
}

static void remove_meta(alloc_meta_t* meta) {
    void* user_ptr = (void*)((char*)meta + sizeof(alloc_meta_t));
    unsigned int idx = get_shard_idx(user_ptr);
    SHARD_LOCK(idx);
    if (meta->prev) {
        meta->prev->next = meta->next;
    } else if (registry_heads[idx] == meta) {
        registry_heads[idx] = meta->next;
    }
    if (meta->next) {
        meta->next->prev = meta->prev;
    }
    meta->prev = NULL;
    meta->next = NULL;
    SHARD_UNLOCK(idx);
}

#ifndef OWNEDC_NO_STDLIB
static void check_leaks(void) {
    int leaked_count = 0;
    for (int i = 0; i < NUM_SHARDS; i++) {
        SHARD_LOCK(i);
        alloc_meta_t* curr = registry_heads[i];
        while (curr) {
            if (curr->state == OWNEDC_STATE_OWNED || curr->state == OWNEDC_STATE_SHARED) {
                void* user_ptr = (void*)((char*)curr + sizeof(alloc_meta_t));
                ownedc_diagnostics_report_leak(user_ptr, curr->alloc_file, curr->alloc_line, curr->size);
                leaked_count++;
            }
            curr = curr->next;
        }
        SHARD_UNLOCK(i);
    }

    // Clean up quarantine memory at exit
    pthread_mutex_lock(&quarantine_mutex);
    for (int i = 0; i < QUARANTINE_SIZE; i++) {
        if (quarantine[i]) {
            global_free(quarantine[i]);
            quarantine[i] = NULL;
        }
    }
    pthread_mutex_unlock(&quarantine_mutex);

    if (leaked_count > 0) {
        OWNEDC_PRINTF("OwnedC: Found %d memory leaks. Terminating.\n", leaked_count);
        OWNEDC_EXIT(1);
    }
}
#endif

void* owner_malloc_internal(size_t size, const char* file, int line) {
    OWNEDC_LOCK();
    if (!atexit_registered) {
#ifndef OWNEDC_NO_STDLIB
        atexit(check_leaks);
#endif
        atexit_registered = 1;
    }
    OWNEDC_UNLOCK();

    alloc_meta_t* meta = (alloc_meta_t*)global_malloc(sizeof(alloc_meta_t) + size);
    if (!meta) return NULL;

    void* user_ptr = (void*)((char*)meta + sizeof(alloc_meta_t));

    meta->magic = OWNEDC_MAGIC;
    meta->size = size;
    meta->state = OWNEDC_STATE_OWNED;
    meta->borrow_count = 0;
    meta->owner_thread = OWNEDC_THREAD_SELF();
    meta->alloc_file = file ? file : "<unknown>";
    meta->alloc_line = line;
    meta->free_file = NULL;
    meta->free_line = 0;

    insert_meta(meta);

    // Poison memory to help detect uninitialized usage
    OWNEDC_MEMSET(user_ptr, 0xAA, size);

    return user_ptr;
}

void* owner_malloc(size_t size) {
    return owner_malloc_internal(size, NULL, 0);
}

void* owner_calloc(size_t num, size_t size) {
    void* ptr = owner_malloc_internal(num * size, NULL, 0);
    if (ptr) OWNEDC_MEMSET(ptr, 0, num * size);
    return ptr;
}

void* owner_realloc_internal(void* ptr, size_t size, const char* file, int line) {
    if (!ptr) return owner_malloc_internal(size, file, line);
    if (size == 0) {
        owner_free_internal(ptr, file, line);
        return NULL;
    }

    alloc_meta_t* meta = get_meta(ptr);
    if (!meta) {
        ownedc_diagnostics_fatal("Invalid Realloc (Untracked pointer)", ptr, file, line);
    }
    if (meta->state == OWNEDC_STATE_FREED) {
        ownedc_diagnostics_fatal("Use-after-free (Realloc on freed pointer)", ptr, file, line);
    }
    if (meta->state != OWNEDC_STATE_SHARED && !OWNEDC_THREAD_EQUAL(meta->owner_thread, OWNEDC_THREAD_SELF())) {
        ownedc_diagnostics_fatal("Thread Ownership Violation (Realloc)", ptr, file, line);
    }

    remove_meta(meta);

    alloc_meta_t* new_meta = (alloc_meta_t*)global_realloc(meta, sizeof(alloc_meta_t) + size);
    if (!new_meta) {
        insert_meta(meta);
        return NULL;
    }

    new_meta->size = size;
    insert_meta(new_meta);

    return (void*)((char*)new_meta + sizeof(alloc_meta_t));
}

void* owner_realloc(void* ptr, size_t size) {
    return owner_realloc_internal(ptr, size, NULL, 0);
}

void owner_free_internal(void* ptr, const char* file, int line) {
    if (!ptr) return;

    alloc_meta_t* meta = get_meta(ptr);
    if (!meta) {
        ownedc_diagnostics_fatal("Invalid free (Untracked pointer)", ptr, file, line);
    }

    unsigned int idx = get_shard_idx(ptr);
    SHARD_LOCK(idx);
    if (meta->state == OWNEDC_STATE_FREED) {
        SHARD_UNLOCK(idx);
        ownedc_diagnostics_fatal("Double-free", ptr, file, line);
    }

    if (meta->borrow_count > 0) {
        SHARD_UNLOCK(idx);
        ownedc_diagnostics_fatal("Freeing borrowed object", ptr, file, line);
    }

    if (meta->state != OWNEDC_STATE_SHARED && !OWNEDC_THREAD_EQUAL(meta->owner_thread, OWNEDC_THREAD_SELF())) {
        SHARD_UNLOCK(idx);
        ownedc_diagnostics_fatal("Thread Ownership Violation (Free)", ptr, file, line);
    }

    meta->state = OWNEDC_STATE_FREED;
    meta->free_file = file ? file : "<unknown>";
    meta->free_line = line;
    SHARD_UNLOCK(idx);

    // Poison memory to help detect use-after-free
    OWNEDC_MEMSET(ptr, 0xEF, meta->size);

    remove_meta(meta);

    // Quarantine the block to safely detect UAF and double-frees
#ifndef OWNEDC_NO_STDLIB
    pthread_mutex_lock(&quarantine_mutex);
#endif
    if (quarantine[quarantine_tail]) {
        global_free(quarantine[quarantine_tail]);
    }
    quarantine[quarantine_tail] = meta;
    quarantine_tail = (quarantine_tail + 1) % QUARANTINE_SIZE;
#ifndef OWNEDC_NO_STDLIB
    pthread_mutex_unlock(&quarantine_mutex);
#endif
}

void owner_free(void* ptr) {
    owner_free_internal(ptr, NULL, 0);
}

/* Ownership Operations */
void ownership_transfer(void* from, void* to) { (void)from; (void)to; }

void ownership_claim(void* ptr) {
    if (!ptr) return;
    alloc_meta_t* meta = get_meta(ptr);
    if (!meta) {
        ownedc_diagnostics_fatal("Invalid claim (Untracked pointer)", ptr, NULL, 0);
    }
    unsigned int idx = get_shard_idx(ptr);
    SHARD_LOCK(idx);
    meta->owner_thread = OWNEDC_THREAD_SELF();
    meta->state = OWNEDC_STATE_OWNED;
    SHARD_UNLOCK(idx);
}

void ownership_borrow(void* ptr) {
    if (!ptr) return;
    alloc_meta_t* meta = get_meta(ptr);
    if (!meta) {
        ownedc_diagnostics_fatal("Invalid borrow (Untracked pointer)", ptr, NULL, 0);
    }
    unsigned int idx = get_shard_idx(ptr);
    SHARD_LOCK(idx);
    if (meta->state == OWNEDC_STATE_FREED) {
        SHARD_UNLOCK(idx);
        ownedc_diagnostics_fatal("Use-after-free (Borrow on freed pointer)", ptr, NULL, 0);
    }
    if (meta->state != OWNEDC_STATE_SHARED && !OWNEDC_THREAD_EQUAL(meta->owner_thread, OWNEDC_THREAD_SELF())) {
        SHARD_UNLOCK(idx);
        ownedc_diagnostics_fatal("Thread Ownership Violation (Borrow)", ptr, NULL, 0);
    }
    meta->borrow_count++;
    if (meta->state != OWNEDC_STATE_SHARED) {
        meta->state = OWNEDC_STATE_BORROWED;
    }
    SHARD_UNLOCK(idx);
}

void ownership_release(void* ptr) {
    if (!ptr) return;
    alloc_meta_t* meta = get_meta(ptr);
    if (!meta) {
        ownedc_diagnostics_fatal("Invalid release (Untracked pointer)", ptr, NULL, 0);
    }
    unsigned int idx = get_shard_idx(ptr);
    SHARD_LOCK(idx);
    if (meta->borrow_count > 0) {
        meta->borrow_count--;
        if (meta->borrow_count == 0 && meta->state == OWNEDC_STATE_BORROWED) {
            meta->state = OWNEDC_STATE_OWNED;
        }
    } else {
        SHARD_UNLOCK(idx);
        ownedc_diagnostics_fatal("Release without borrow", ptr, NULL, 0);
    }
    SHARD_UNLOCK(idx);
}

void ownership_share(void* ptr) {
    if (!ptr) return;
    alloc_meta_t* meta = get_meta(ptr);
    if (!meta) {
        ownedc_diagnostics_fatal("Invalid share (Untracked pointer)", ptr, NULL, 0);
    }
    unsigned int idx = get_shard_idx(ptr);
    SHARD_LOCK(idx);
    if (!OWNEDC_THREAD_EQUAL(meta->owner_thread, OWNEDC_THREAD_SELF())) {
        SHARD_UNLOCK(idx);
        ownedc_diagnostics_fatal("Thread Ownership Violation (Share must be called by owner)", ptr, NULL, 0);
    }
    meta->state = OWNEDC_STATE_SHARED;
    SHARD_UNLOCK(idx);
}

/* Diagnostics API */
void ownership_inspect(void* ptr) {
    alloc_meta_t* meta = get_meta(ptr);
    if (!meta) {
        OWNEDC_PRINTF("Object %p: UNTRACKED\n", ptr);
        return;
    }
    unsigned int idx = get_shard_idx(ptr);
    SHARD_LOCK(idx);
    OWNEDC_PRINTF("Object %p:\n", ptr);
    OWNEDC_PRINTF("  Size: %zu\n", meta->size);
    OWNEDC_PRINTF("  State: %d\n", meta->state);
    OWNEDC_PRINTF("  Borrows: %d\n", meta->borrow_count);
    OWNEDC_PRINTF("  Owner Thread: %p\n", (void*)meta->owner_thread);
    OWNEDC_PRINTF("  Allocated: %s:%d\n", meta->alloc_file, meta->alloc_line);
    if (meta->state == OWNEDC_STATE_FREED) {
        OWNEDC_PRINTF("  Freed: %s:%d\n", meta->free_file, meta->free_line);
    }
    SHARD_UNLOCK(idx);
}

void ownership_dump(void) {
    OWNEDC_PRINTF("--- Ownership Dump ---\n");
    for (int i = 0; i < NUM_SHARDS; i++) {
        SHARD_LOCK(i);
        alloc_meta_t* curr = registry_heads[i];
        while (curr) {
            void* user_ptr = (void*)((char*)curr + sizeof(alloc_meta_t));
            OWNEDC_PRINTF("Ptr %p, Size %zu, State %d, Borrows %d\n", user_ptr, curr->size, curr->state, curr->borrow_count);
            curr = curr->next;
        }
        SHARD_UNLOCK(i);
    }
}

void ownership_dump_borrows(void) {
    OWNEDC_PRINTF("--- Borrowed Objects Dump ---\n");
    int found = 0;
    for (int i = 0; i < NUM_SHARDS; i++) {
        SHARD_LOCK(i);
        alloc_meta_t* curr = registry_heads[i];
        while (curr) {
            if (curr->borrow_count > 0) {
                void* user_ptr = (void*)((char*)curr + sizeof(alloc_meta_t));
                OWNEDC_PRINTF("Ptr %p, Size %zu, Borrows %d\n", user_ptr, curr->size, curr->borrow_count);
                found++;
            }
            curr = curr->next;
        }
        SHARD_UNLOCK(i);
    }
    if (found == 0) {
        OWNEDC_PRINTF("No active borrows.\n");
    }
}

void ownership_stats(void) {
    int active = 0;
    int freed = 0;
    for (int i = 0; i < NUM_SHARDS; i++) {
        SHARD_LOCK(i);
        alloc_meta_t* curr = registry_heads[i];
        while (curr) {
            if (curr->state == OWNEDC_STATE_OWNED || curr->state == OWNEDC_STATE_SHARED || curr->state == OWNEDC_STATE_BORROWED) {
                active++;
            } else if (curr->state == OWNEDC_STATE_FREED) {
                freed++;
            }
            curr = curr->next;
        }
        SHARD_UNLOCK(i);
    }

#ifndef OWNEDC_NO_STDLIB
    pthread_mutex_lock(&quarantine_mutex);
#endif
    for (int i = 0; i < QUARANTINE_SIZE; i++) {
        if (quarantine[i]) {
            freed++;
        }
    }
#ifndef OWNEDC_NO_STDLIB
    pthread_mutex_unlock(&quarantine_mutex);
#endif

    OWNEDC_PRINTF("OwnedC Stats: %d active allocations, %d freed allocations.\n", active, freed);
}

#ifndef OWNEDC_NO_STDLIB
#include <stdio.h>
void ownership_dump_json(const char* filepath) {
    FILE* f = fopen(filepath, "w");
    if (!f) return;
    
    fprintf(f, "{\n  \"allocations\": [\n");
    
    int first = 1;
    for (int i = 0; i < NUM_SHARDS; i++) {
        SHARD_LOCK(i);
        alloc_meta_t* curr = registry_heads[i];
        while (curr) {
            if (curr->state != OWNEDC_STATE_FREED) {
                if (!first) fprintf(f, ",\n");
                void* user_ptr = (void*)((char*)curr + sizeof(alloc_meta_t));
                fprintf(f, "    {\n");
                fprintf(f, "      \"ptr\": \"%p\",\n", user_ptr);
                fprintf(f, "      \"size\": %zu,\n", curr->size);
                fprintf(f, "      \"state\": \"%d\",\n", curr->state);
                fprintf(f, "      \"owner_thread\": %lld,\n", (long long)curr->owner_thread);
                fprintf(f, "      \"borrow_count\": %d\n", curr->borrow_count);
                fprintf(f, "    }");
                first = 0;
            }
            curr = curr->next;
        }
        SHARD_UNLOCK(i);
    }
    
    fprintf(f, "\n  ]\n}\n");
    fclose(f);
}
#else
void ownership_dump_json(const char* filepath) {
    (void)filepath;
    OWNEDC_PRINTF("ownership_dump_json is not available in NO_STDLIB mode.\n");
}
#endif

size_t owner_malloc_usable_size(void* ptr) {
    if (!ptr) return 0;
    alloc_meta_t* meta = get_meta(ptr);
    return meta ? meta->size : 0;
}
