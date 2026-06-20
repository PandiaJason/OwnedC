#ifndef OWNEDC_GENERICS_H
#define OWNEDC_GENERICS_H

#include "ownedc.h"
#include "ownedc_collections.h"

/**
 * @brief Macro to define a type-safe vector wrapper over owned_vector_t.
 *
 * Usage:
 *   OWNEDC_DEFINE_VECTOR(int, int_vector_t);
 *
 *   int_vector_t v = int_vector_t_new();
 *   int* my_int = owner_malloc(sizeof(int));
 *   int_vector_t_push(&v, my_int);
 *   int* retrieved = int_vector_t_get(&v, 0);
 *   int_vector_t_free(&v);
 */
#define OWNEDC_DEFINE_VECTOR(T, Name) \
    typedef struct { \
        safe_vector_t* _inner; \
    } Name; \
    \
    static inline Name Name##_new(void) { \
        Name v; \
        v._inner = safe_vector_create(16); \
        return v; \
    } \
    \
    static inline void Name##_free(Name* v) { \
        if (v && v->_inner) { \
            safe_vector_free(v->_inner); \
            v->_inner = NULL; \
        } \
    } \
    \
    static inline void Name##_push(Name* v, T* item) { \
        safe_vector_push(v->_inner, (void*)item); \
    } \
    \
    static inline T* Name##_get(Name* v, size_t index) { \
        return (T*)safe_vector_get(v->_inner, index); \
    } \
    \
    static inline size_t Name##_length(Name* v) { \
        return safe_vector_length(v->_inner); \
    } \
    \
    static inline void Name##_remove(Name* v, size_t index) { \
        safe_vector_remove(v->_inner, index); \
    }

#endif // OWNEDC_GENERICS_H
