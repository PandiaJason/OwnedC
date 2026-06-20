#include "ownedc_string.h"
#include "ownedc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void safe_string_grow(safe_string_t* str, size_t min_capacity) {
    if (str->capacity >= min_capacity) return;
    
    size_t new_capacity = str->capacity == 0 ? 16 : str->capacity * 2;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    
    // We use standard realloc for the internal buffer. 
    // The safe_string_t struct itself is tracked by OwnedC.
    char* new_data = (char*)realloc(str->data, new_capacity);
    if (!new_data) {
        fprintf(stderr, "OWNEDC FATAL: Out of memory in safe_string_grow\n");
        abort();
    }
    
    str->data = new_data;
    str->capacity = new_capacity;
}

safe_string_t* safe_string_new(const char* initial_cstr) {
    // The struct itself is tracked by the Ownership framework.
    safe_string_t* str = (safe_string_t*)owner_malloc(sizeof(safe_string_t));
    if (!str) return NULL;
    
    str->data = NULL;
    str->length = 0;
    str->capacity = 0;
    
    if (initial_cstr) {
        size_t len = strlen(initial_cstr);
        safe_string_grow(str, len + 1);
        memcpy(str->data, initial_cstr, len);
        str->data[len] = '\0';
        str->length = len;
    } else {
        safe_string_grow(str, 16);
        str->data[0] = '\0';
    }
    
    return str;
}

const char* safe_string_cstr(safe_string_t* str) {
    if (!str || !str->data) return "";
    return str->data;
}

void safe_string_append(safe_string_t* str, const char* suffix) {
    if (!str || !suffix) return;
    
    size_t suffix_len = strlen(suffix);
    if (suffix_len == 0) return;
    
    safe_string_grow(str, str->length + suffix_len + 1);
    memcpy(str->data + str->length, suffix, suffix_len);
    str->length += suffix_len;
    str->data[str->length] = '\0';
}

void safe_string_append_safe(safe_string_t* str, safe_string_t* suffix) {
    if (!str || !suffix) return;
    safe_string_append(str, safe_string_cstr(suffix));
}

void safe_string_push_char(safe_string_t* str, char c) {
    if (!str) return;
    safe_string_grow(str, str->length + 2);
    str->data[str->length] = c;
    str->length++;
    str->data[str->length] = '\0';
}

char safe_string_char_at(safe_string_t* str, size_t index) {
    if (!str || !str->data) {
        fprintf(stderr, "OWNEDC FATAL: Null string access.\n");
        abort();
    }
    if (index >= str->length) {
        fprintf(stderr, "OWNEDC FATAL: String bounds violation! Index %zu, Length %zu\n", index, str->length);
        abort();
    }
    return str->data[index];
}

void safe_string_truncate(safe_string_t* str, size_t new_length) {
    if (!str) return;
    if (new_length > str->length) {
        fprintf(stderr, "OWNEDC FATAL: Cannot truncate to length greater than current.\n");
        abort();
    }
    str->length = new_length;
    if (str->data) {
        str->data[new_length] = '\0';
    }
}

void safe_string_free(safe_string_t* str) {
    if (!str) return;
    if (str->data) {
        free(str->data);
        str->data = NULL;
    }
    // Free the struct itself using the OwnedC allocator
    owner_free(str);
}
