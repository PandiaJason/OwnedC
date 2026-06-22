#include "owned_json.h"
#include "ownedc.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static const char* skip_whitespace(const char* p) {
    while (p && *p && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

static json_node_t* parse_value(const char** p);

static json_node_t* create_node(json_type_t type) {
    json_node_t* node = (json_node_t*)owner_malloc(sizeof(json_node_t));
    if (node) {
        node->type = type;
        memset(&node->value, 0, sizeof(node->value));
    }
    return node;
}

static json_node_t* parse_null(const char** p) {
    if (strncmp(*p, "null", 4) == 0) {
        *p += 4;
        return create_node(JSON_NULL);
    }
    return NULL;
}

static json_node_t* parse_bool(const char** p) {
    if (strncmp(*p, "true", 4) == 0) {
        *p += 4;
        json_node_t* node = create_node(JSON_BOOL);
        if (node) node->value.bool_val = 1;
        return node;
    } else if (strncmp(*p, "false", 5) == 0) {
        *p += 5;
        json_node_t* node = create_node(JSON_BOOL);
        if (node) node->value.bool_val = 0;
        return node;
    }
    return NULL;
}

static json_node_t* parse_number(const char** p) {
    const char* start = *p;
    char* end = NULL;
    double val = strtod(start, &end);
    if (end == start) return NULL;
    *p = end;
    json_node_t* node = create_node(JSON_NUMBER);
    if (node) node->value.num_val = val;
    return node;
}

static char* parse_raw_string(const char** p) {
    if (**p != '"') return NULL;
    (*p)++; // skip opening quote
    const char* start = *p;
    while (**p && **p != '"') {
        if (**p == '\\' && *(*p + 1)) {
            (*p)++; // skip escape sequence
        }
        (*p)++;
    }
    if (**p != '"') return NULL; // missing closing quote
    size_t len = *p - start;
    char* str = (char*)owner_malloc(len + 1);
    if (str) {
        memcpy(str, start, len);
        str[len] = '\0';
    }
    (*p)++; // skip closing quote
    return str;
}

static json_node_t* parse_string(const char** p) {
    char* str = parse_raw_string(p);
    if (!str) return NULL;
    json_node_t* node = create_node(JSON_STRING);
    if (node) {
        node->value.str_val = str;
    } else {
        owner_free(str);
    }
    return node;
}

static json_node_t* parse_array(const char** p) {
    if (**p != '[') return NULL;
    (*p)++; // skip '['
    *p = skip_whitespace(*p);

    json_node_t* node = create_node(JSON_ARRAY);
    if (!node) return NULL;

    if (**p == ']') {
        (*p)++;
        return node;
    }

    while (1) {
        json_node_t* item = parse_value(p);
        if (!item) {
            json_free(node);
            return NULL;
        }

        // Add to array items
        if (node->value.arr.count >= node->value.arr.capacity) {
            size_t new_cap = node->value.arr.capacity == 0 ? 4 : node->value.arr.capacity * 2;
            json_node_t** new_items = (json_node_t**)owner_realloc(node->value.arr.items, new_cap * sizeof(json_node_t*));
            if (!new_items) {
                json_free(item);
                json_free(node);
                return NULL;
            }
            node->value.arr.items = new_items;
            node->value.arr.capacity = new_cap;
        }
        node->value.arr.items[node->value.arr.count++] = item;

        *p = skip_whitespace(*p);
        if (**p == ']') {
            (*p)++;
            break;
        } else if (**p == ',') {
            (*p)++;
            *p = skip_whitespace(*p);
        } else {
            // Unexpected character
            json_free(node);
            return NULL;
        }
    }
    return node;
}

static json_node_t* parse_object(const char** p) {
    if (**p != '{') return NULL;
    (*p)++; // skip '{'
    *p = skip_whitespace(*p);

    json_node_t* node = create_node(JSON_OBJECT);
    if (!node) return NULL;

    if (**p == '}') {
        (*p)++;
        return node;
    }

    while (1) {
        char* key = parse_raw_string(p);
        if (!key) {
            json_free(node);
            return NULL;
        }

        *p = skip_whitespace(*p);
        if (**p != ':') {
            owner_free(key);
            json_free(node);
            return NULL;
        }
        (*p)++; // skip ':'
        *p = skip_whitespace(*p);

        json_node_t* val = parse_value(p);
        if (!val) {
            owner_free(key);
            json_free(node);
            return NULL;
        }

        // Add key-value pair to object lists
        if (node->value.obj.count >= node->value.obj.capacity) {
            size_t new_cap = node->value.obj.capacity == 0 ? 4 : node->value.obj.capacity * 2;
            char** new_keys = (char**)owner_realloc(node->value.obj.keys, new_cap * sizeof(char*));
            json_node_t** new_values = (json_node_t**)owner_realloc(node->value.obj.values, new_cap * sizeof(json_node_t*));
            if (!new_keys || !new_values) {
                owner_free(key);
                json_free(val);
                if (new_keys) owner_free(new_keys);
                if (new_values) owner_free(new_values);
                json_free(node);
                return NULL;
            }
            node->value.obj.keys = new_keys;
            node->value.obj.values = new_values;
            node->value.obj.capacity = new_cap;
        }
        node->value.obj.keys[node->value.obj.count] = key;
        node->value.obj.values[node->value.obj.count] = val;
        node->value.obj.count++;

        *p = skip_whitespace(*p);
        if (**p == '}') {
            (*p)++;
            break;
        } else if (**p == ',') {
            (*p)++;
            *p = skip_whitespace(*p);
        } else {
            // Unexpected character
            json_free(node);
            return NULL;
        }
    }
    return node;
}

static json_node_t* parse_value(const char** p) {
    *p = skip_whitespace(*p);
    if (!**p) return NULL;

    switch (**p) {
        case 'n': return parse_null(p);
        case 't':
        case 'f': return parse_bool(p);
        case '"': return parse_string(p);
        case '[': return parse_array(p);
        case '{': return parse_object(p);
        default:  return parse_number(p);
    }
}

json_node_t* json_parse(const char* json_str, const char** error_pos) {
    const char* p = json_str;
    json_node_t* root = parse_value(&p);
    p = skip_whitespace(p);
    if (!root || *p) {
        if (error_pos) *error_pos = p;
        if (root) {
            json_free(root);
            root = NULL;
        }
    }
    return root;
}

void json_free(json_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case JSON_STRING:
            if (node->value.str_val) {
                owner_free(node->value.str_val);
            }
            break;
        case JSON_ARRAY:
            if (node->value.arr.items) {
                for (size_t i = 0; i < node->value.arr.count; i++) {
                    json_free(node->value.arr.items[i]);
                }
                owner_free(node->value.arr.items);
            }
            break;
        case JSON_OBJECT:
            if (node->value.obj.keys) {
                for (size_t i = 0; i < node->value.obj.count; i++) {
                    owner_free(node->value.obj.keys[i]);
                    json_free(node->value.obj.values[i]);
                }
                owner_free(node->value.obj.keys);
                owner_free(node->value.obj.values);
            }
            break;
        default:
            break;
    }
    owner_free(node);
}

json_node_t* json_get_object_item(json_node_t* node, const char* key) {
    if (!node || node->type != JSON_OBJECT) return NULL;
    for (size_t i = 0; i < node->value.obj.count; i++) {
        if (strcmp(node->value.obj.keys[i], key) == 0) {
            return node->value.obj.values[i];
        }
    }
    return NULL;
}

json_node_t* json_get_array_item(json_node_t* node, size_t index) {
    if (!node || node->type != JSON_ARRAY || index >= node->value.arr.count) return NULL;
    return node->value.arr.items[index];
}

const char* json_get_string(json_node_t* node) {
    if (!node || node->type != JSON_STRING) return NULL;
    return node->value.str_val;
}

double json_get_number(json_node_t* node) {
    if (!node || node->type != JSON_NUMBER) return 0.0;
    return node->value.num_val;
}

int json_get_bool(json_node_t* node) {
    if (!node || node->type != JSON_BOOL) return 0;
    return node->value.bool_val;
}
