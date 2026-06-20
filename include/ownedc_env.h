#ifndef OWNEDC_ENV_H
#define OWNEDC_ENV_H

// This header encapsulates all standard library dependencies.
// By defining OWNEDC_NO_STDLIB, OwnedC can be compiled for bare-metal, kernel, or embedded systems.

#ifndef OWNEDC_NO_STDLIB
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    
    #define OWNEDC_PRINTF printf
    #define OWNEDC_ABORT abort
    #define OWNEDC_EXIT exit
#else
    // In NO_STDLIB mode, we don't include standard libraries.
    // The user must define their own OWNEDC_PRINTF or we default to a no-op.
    #ifndef OWNEDC_PRINTF
        #define OWNEDC_PRINTF(...) ((void)0)
    #endif

    #ifndef OWNEDC_ABORT
        #define OWNEDC_ABORT() while(1) {} // Infinite loop for embedded panic
    #endif

    #ifndef OWNEDC_EXIT
        #define OWNEDC_EXIT(code) while(1) {}
    #endif

    // We still need size_t, which is provided by stddef.h (a freestanding header safe for bare-metal)
    #include <stddef.h>
    
    // We also need some basic memset/memcpy substitutes if not provided
    #ifndef OWNEDC_MEMSET
        static inline void* ownedc_memset(void* s, int c, size_t n) {
            unsigned char* p = (unsigned char*)s;
            while (n--) *p++ = (unsigned char)c;
            return s;
        }
        #define OWNEDC_MEMSET ownedc_memset
    #else
        #define OWNEDC_MEMSET OWNEDC_MEMSET
    #endif

    #ifndef OWNEDC_MEMCPY
        static inline void* ownedc_memcpy(void* dest, const void* src, size_t n) {
            unsigned char* d = (unsigned char*)dest;
            const unsigned char* s = (const unsigned char*)src;
            while (n--) *d++ = *s++;
            return dest;
        }
        #define OWNEDC_MEMCPY ownedc_memcpy
    #endif

    #ifndef OWNEDC_STRLEN
        static inline size_t ownedc_strlen(const char* s) {
            size_t len = 0;
            while (s[len]) len++;
            return len;
        }
        #define OWNEDC_STRLEN ownedc_strlen
    #endif

    // In NO_STDLIB mode, malloc/free defaults to NULL since there is no OS.
    // The user MUST call ownedc_set_allocators() during boot!
    #define OWNEDC_DEFAULT_MALLOC NULL
    #define OWNEDC_DEFAULT_FREE NULL
    #define OWNEDC_DEFAULT_REALLOC NULL
#endif

// Default definitions for hosted mode (with stdlib)
#ifndef OWNEDC_NO_STDLIB
    #define OWNEDC_MEMSET memset
    #define OWNEDC_MEMCPY memcpy
    #define OWNEDC_STRLEN strlen
    #define OWNEDC_DEFAULT_MALLOC malloc
    #define OWNEDC_DEFAULT_FREE free
    #define OWNEDC_DEFAULT_REALLOC realloc
#endif

#endif // OWNEDC_ENV_H
