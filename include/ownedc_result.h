#ifndef OWNEDC_RESULT_H
#define OWNEDC_RESULT_H

#include <stdbool.h>

#include "ownedc_env.h"

#ifdef __cplusplus
extern "C" {
#endif

// Generic Result Type (Option 1: void* payload)
// In a real application, you might use macros to generate typed versions.
typedef struct {
    bool is_ok;
    void* value;
    int error_code;
} owned_result_t;

// Constructors
static inline owned_result_t Ok(void* value) {
    owned_result_t res;
    res.is_ok = true;
    res.value = value;
    res.error_code = 0;
    return res;
}

static inline owned_result_t Err(int error_code) {
    owned_result_t res;
    res.is_ok = false;
    res.value = NULL;
    res.error_code = error_code;
    return res;
}

// "Unwrapping" forces the developer to check. If they unwrap an Err, it panics (or returns).
static inline void* owned_unwrap(owned_result_t res, const char* file, int line) {
    if (!res.is_ok) {
        fprintf(stderr, "PANIC: Unwrapped an Err value (%d) at %s:%d\n", res.error_code, file, line);
        exit(1);
    }
    return res.value;
}

#define UNWRAP(res) owned_unwrap(res, __FILE__, __LINE__)

// Rust-like "Try" macro (the `?` operator)
// Evaluates `res` exactly once to prevent side effects or memory leaks.
#define TRY_UNWRAP(res, out_val) \
    do { \
        owned_result_t _tmp_res = (res); \
        if (!_tmp_res.is_ok) return Err(_tmp_res.error_code); \
        (out_val) = _tmp_res.value; \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_RESULT_H
