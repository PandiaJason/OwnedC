#ifndef OWNEDC_H
#define OWNEDC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ownership states */
typedef enum {
    OWNEDC_STATE_INVALID = 0,
    OWNEDC_STATE_OWNED,
    OWNEDC_STATE_BORROWED,
    OWNEDC_STATE_SHARED,
    OWNEDC_STATE_RELEASED,
    OWNEDC_STATE_FREED
} ownership_state_t;

#if defined(__GNUC__) || defined(__clang__)
  #define OWNEDC_MALLOC_ATTR __attribute__((warn_unused_result, alloc_size(1)))
  #define OWNEDC_CALLOC_ATTR __attribute__((warn_unused_result, alloc_size(1, 2)))
  #define OWNEDC_REALLOC_ATTR __attribute__((warn_unused_result, alloc_size(2)))
#else
  #define OWNEDC_MALLOC_ATTR
  #define OWNEDC_CALLOC_ATTR
  #define OWNEDC_REALLOC_ATTR
#endif

/* Pluggable Allocator Types */
typedef void* (*ownedc_malloc_fn)(size_t);
typedef void  (*ownedc_free_fn)(void*);
typedef void* (*ownedc_realloc_fn)(void*, size_t);

/* Allows dropping in jemalloc, mimalloc, or custom allocators */
void ownedc_set_allocators(ownedc_malloc_fn m_fn, ownedc_free_fn f_fn, ownedc_realloc_fn r_fn);

/* Public allocation API */
void* owner_malloc(size_t size) OWNEDC_MALLOC_ATTR;
void* owner_calloc(size_t num, size_t size) OWNEDC_CALLOC_ATTR;
void* owner_realloc(void* ptr, size_t size) OWNEDC_REALLOC_ATTR;
void  owner_free(void* ptr);

/* RAII Cleanup helper for GCC/Clang */
static inline void owner_free_cleanup(void* ptr) {
    void* actual_ptr = *(void**)ptr;
    if (actual_ptr) {
        owner_free(actual_ptr);
    }
}

#if defined(__GNUC__) || defined(__clang__)
#define OWNED __attribute__((cleanup(owner_free_cleanup)))
#else
#define OWNED
#endif

/* Ownership Operations */
void ownership_transfer(void* from, void* to);
void ownership_claim(void* ptr);
void ownership_borrow(void* ptr);
void ownership_release(void* ptr);
void ownership_share(void* ptr);

/* Diagnostics API */
void ownership_inspect(void* ptr);
void ownership_dump(void);
void ownership_dump_borrows(void);
void ownership_stats(void);
void ownership_dump_json(const char* filepath);

#ifdef __cplusplus
}
#endif

#endif /* OWNEDC_H */
