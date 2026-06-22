#ifndef OWNED_JSON_H
#define OWNED_JSON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// JSON Node Types
typedef enum {
    JSON_NULL = 0,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type_t;

typedef struct json_node json_node_t;

struct json_node {
    json_type_t type;
    union {
        int bool_val;
        double num_val;
        char* str_val;
        struct {
            json_node_t** items;
            size_t count;
            size_t capacity;
        } arr;
        struct {
            char** keys;
            json_node_t** values;
            size_t count;
            size_t capacity;
        } obj;
    } value;
};

// Parse a JSON string.
// If parsing fails, returns NULL and error_pos (if not NULL) is set to the character where parsing failed.
// All nodes are dynamically registered with OwnedC via owner_malloc.
json_node_t* json_parse(const char* json_str, const char** error_pos);

// Recursively free a JSON node and all of its children.
void json_free(json_node_t* node);

// Access functions
json_node_t* json_get_object_item(json_node_t* node, const char* key);
json_node_t* json_get_array_item(json_node_t* node, size_t index);
const char* json_get_string(json_node_t* node);
double json_get_number(json_node_t* node);
int json_get_bool(json_node_t* node);

// Clean up helper for standard stack scope RAII macro
static inline void json_free_cleanup(void* ptr) {
    json_node_t** node_ptr = (json_node_t**)ptr;
    if (node_ptr && *node_ptr) {
        json_free(*node_ptr);
    }
}

// RAII macro for json_node_t
#define OWNED_JSON __attribute__((cleanup(json_free_cleanup)))

#ifdef __cplusplus
}
#endif

#endif // OWNED_JSON_H
