#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN 4096
#define MAX_VARS 1024
#define MAX_VAR_NAME 128

typedef struct {
    char name[MAX_VAR_NAME];
    enum { STATE_OWNED, STATE_FREED, STATE_RETURNED } state;
    int line_allocated;
    int is_raii;
} TrackedVar;

TrackedVar tracked_vars[MAX_VARS];
int tracked_count = 0;
char current_func[256] = "";

int is_ident(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

void extract_identifier(const char* start, char* dest) {
    while (*start && isspace((unsigned char)*start)) start++;
    int i = 0;
    while (*start && is_ident(*start) && i < MAX_VAR_NAME - 1) {
        dest[i++] = *start++;
    }
    dest[i] = '\0';
}

void add_tracked_var(const char* name, int line, int is_raii) {
    if (tracked_count >= MAX_VARS) return;
    // Avoid duplicates
    for (int i = 0; i < tracked_count; i++) {
        if (strcmp(tracked_vars[i].name, name) == 0) {
            tracked_vars[i].state = STATE_OWNED;
            tracked_vars[i].line_allocated = line;
            tracked_vars[i].is_raii = is_raii;
            return;
        }
    }
    strncpy(tracked_vars[tracked_count].name, name, MAX_VAR_NAME - 1);
    tracked_vars[tracked_count].state = STATE_OWNED;
    tracked_vars[tracked_count].line_allocated = line;
    tracked_vars[tracked_count].is_raii = is_raii;
    tracked_count++;
}

void mark_freed(const char* name, const char* filepath, int line_no, int* errors) {
    for (int i = 0; i < tracked_count; i++) {
        if (strcmp(tracked_vars[i].name, name) == 0) {
            if (tracked_vars[i].state == STATE_FREED) {
                printf("[%s:%d] ERROR: Double-free detected on '%s' in %s\n", filepath, line_no, name, current_func);
                (*errors)++;
            } else {
                tracked_vars[i].state = STATE_FREED;
            }
            return;
        }
    }
}

void check_borrow(const char* name, const char* filepath, int line_no, int* errors) {
    for (int i = 0; i < tracked_count; i++) {
        if (strcmp(tracked_vars[i].name, name) == 0) {
            if (tracked_vars[i].state == STATE_FREED) {
                printf("[%s:%d] ERROR: Use-after-free (borrow) detected on '%s' in %s\n", filepath, line_no, name, current_func);
                (*errors)++;
            }
            return;
        }
    }
}

int is_word(const char* line, const char* p, const char* word) {
    size_t len = strlen(word);
    if (strncmp(p, word, len) != 0) return 0;
    if (p > line) {
        char prev = *(p - 1);
        if (is_ident(prev)) return 0;
    }
    char next = *(p + len);
    if (is_ident(next)) return 0;
    return 1;
}

int is_raw_alloc_free(const char* line, const char* p, const char* word) {
    if (!is_word(line, p, word)) return 0;
    if (p >= line + 6 && strncmp(p - 6, "owner_", 6) == 0) return 0;
    if (p >= line + 15 && strncmp(p - 15, "OWNEDC_DEFAULT_", 15) == 0) return 0;
    if (p >= line + 7 && strncmp(p - 7, "global_", 7) == 0) return 0;
    if (p >= line + 8 && strncmp(p - 8, "openssl_", 8) == 0) return 0;
    if (p >= line + 6 && strncmp(p - 6, "cJSON_", 6) == 0) return 0;
    return 1;
}

void preprocess_line(char* line, int* in_block) {
    char* src = line;
    char* dst = line;
    int in_string = 0;
    while (*src) {
        if (*in_block) {
            if (src[0] == '*' && src[1] == '/') {
                *in_block = 0;
                src += 2;
            } else {
                src++;
            }
        } else if (in_string) {
            if (src[0] == '\\' && src[1]) {
                src += 2;
            } else if (src[0] == '"') {
                in_string = 0;
                *dst++ = *src++;
            } else {
                src++;
            }
        } else {
            if (src[0] == '/' && src[1] == '*') {
                *in_block = 1;
                src += 2;
            } else if (src[0] == '/' && src[1] == '/') {
                break;
            } else if (src[0] == '"') {
                in_string = 1;
                *dst++ = *src++;
            } else {
                *dst++ = *src++;
            }
        }
    }
    *dst = '\0';
}

void lint_file(const char* filepath, int* total_errors, int* total_warnings) {
    FILE* f = fopen(filepath, "r");
    if (!f) {
        printf("Error: Could not open file %s\n", filepath);
        (*total_errors)++;
        return;
    }

    printf("Linting %s...\n", filepath);

    char raw_line[MAX_LINE_LEN];
    char clean_line[MAX_LINE_LEN];
    char prev_line[MAX_LINE_LEN] = "";
    int line_no = 0;
    int nesting_level = 0;
    int in_function = 0;
    int in_block_comment = 0;
    int local_errors = 0;
    int local_warnings = 0;

    while (fgets(raw_line, sizeof(raw_line), f)) {
        line_no++;
        strcpy(clean_line, raw_line);
        preprocess_line(clean_line, &in_block_comment);

        // Track raw malloc/free usage
        const char* p = clean_line;
        while (*p) {
            if (is_word(clean_line, p, "malloc")) {
                if (is_raw_alloc_free(clean_line, p, "malloc")) {
                    int col = p - clean_line + 1;
                    printf("%s:%d:%d: warning: use of raw 'malloc' detected. Use 'owner_malloc' instead for memory safety. [-Wownedc-raw-memory]\n", filepath, line_no, col);
                    local_warnings++;
                }
            } else if (is_word(clean_line, p, "calloc")) {
                if (is_raw_alloc_free(clean_line, p, "calloc")) {
                    int col = p - clean_line + 1;
                    printf("%s:%d:%d: warning: use of raw 'calloc' detected. Use 'owner_calloc' instead for memory safety. [-Wownedc-raw-memory]\n", filepath, line_no, col);
                    local_warnings++;
                }
            } else if (is_word(clean_line, p, "realloc")) {
                if (is_raw_alloc_free(clean_line, p, "realloc")) {
                    int col = p - clean_line + 1;
                    printf("%s:%d:%d: warning: use of raw 'realloc' detected. Use 'owner_realloc' instead for memory safety. [-Wownedc-raw-memory]\n", filepath, line_no, col);
                    local_warnings++;
                }
            } else if (is_word(clean_line, p, "free")) {
                if (is_raw_alloc_free(clean_line, p, "free")) {
                    int col = p - clean_line + 1;
                    printf("%s:%d:%d: error: use of raw 'free' detected. Use 'owner_free' instead to prevent double-free/use-after-free. [-Wownedc-raw-free]\n", filepath, line_no, col);
                    local_errors++;
                }
            }
            p++;
        }

        // Track function boundary entry/exit
        int braces_before = nesting_level;
        for (int i = 0; clean_line[i]; i++) {
            if (clean_line[i] == '{') nesting_level++;
            else if (clean_line[i] == '}') nesting_level--;
        }

        if (braces_before == 0 && nesting_level > 0 && !in_function) {
            in_function = 1;
            tracked_count = 0;
            const char* paren = strchr(clean_line, '(');
            if (!paren) paren = strchr(prev_line, '(');
            
            if (paren) {
                const char* start = paren - 1;
                const char* base_line = (strchr(clean_line, '(')) ? clean_line : prev_line;
                while (start > base_line && isspace((unsigned char)*start)) start--;
                const char* end = start;
                while (start > base_line && (isalnum((unsigned char)*start) || *start == '_')) start--;
                if (!isalnum((unsigned char)*start) && *start != '_') start++;
                int len = end - start + 1;
                if (len > 0 && len < 255) {
                    strncpy(current_func, start, len);
                    current_func[len] = '\0';
                } else {
                    strcpy(current_func, "function");
                }
            } else {
                strcpy(current_func, "function");
            }
        }

        if (in_function) {
            // Check for owner_malloc/calloc/realloc
            const char* alloc = strstr(clean_line, "owner_malloc");
            if (!alloc) alloc = strstr(clean_line, "owner_calloc");
            if (!alloc) alloc = strstr(clean_line, "owner_realloc");
            if (alloc) {
                int is_raii = 0;
                if (strstr(clean_line, "OWNED")) {
                    is_raii = 1;
                } else {
                    int col = alloc - clean_line + 1;
                    printf("%s:%d:%d: warning: use of owner_malloc/calloc/realloc without 'OWNED' macro. This memory must be manually freed. [-Wownedc-manual-free]\n", filepath, line_no, col);
                    local_warnings++;
                }

                // Extract variable name
                const char* eq = alloc;
                while (eq > clean_line && *eq != '=') eq--;
                if (eq > clean_line && *eq == '=') {
                    const char* end = eq - 1;
                    while (end > clean_line && isspace((unsigned char)*end)) end--;
                    const char* start = end;
                    while (start > clean_line && is_ident(*start)) start--;
                    if (!is_ident(*start)) start++;
                    int len = end - start + 1;
                    if (len > 0 && len < MAX_VAR_NAME) {
                        char var_name[MAX_VAR_NAME] = {0};
                        strncpy(var_name, start, len);
                        var_name[len] = '\0';
                        add_tracked_var(var_name, line_no, is_raii);
                    }
                }
            }

            // Check for owner_free calls
            const char* f = strstr(clean_line, "owner_free");
            while (f) {
                f += strlen("owner_free");
                while (*f && *f != '(') f++;
                if (*f == '(') {
                    f++;
                    char var_name[MAX_VAR_NAME] = {0};
                    extract_identifier(f, var_name);
                    if (strlen(var_name) > 0) {
                        mark_freed(var_name, filepath, line_no, &local_errors);
                    }
                }
                f = strstr(f, "owner_free");
            }

            // Check for ownership_borrow
            const char* b = strstr(clean_line, "ownership_borrow");
            while (b) {
                b += strlen("ownership_borrow");
                while (*b && *b != '(') b++;
                if (*b == '(') {
                    b++;
                    char var_name[MAX_VAR_NAME] = {0};
                    extract_identifier(b, var_name);
                    if (strlen(var_name) > 0) {
                        check_borrow(var_name, filepath, line_no, &local_errors);
                    }
                }
                b = strstr(b, "ownership_borrow");
            }

            // Check for returns
            const char* r = strstr(clean_line, "return");
            if (r && is_word(clean_line, r, "return")) {
                r += 6;
                while (*r && isspace((unsigned char)*r)) r++;
                char var_name[MAX_VAR_NAME] = {0};
                extract_identifier(r, var_name);
                if (strlen(var_name) > 0) {
                    for (int i = 0; i < tracked_count; i++) {
                        if (strcmp(tracked_vars[i].name, var_name) == 0) {
                            tracked_vars[i].state = STATE_RETURNED;
                        }
                    }
                }
            }
        }

        // Exiting function
        if (in_function && nesting_level == 0) {
            for (int i = 0; i < tracked_count; i++) {
                if (tracked_vars[i].state == STATE_OWNED && !tracked_vars[i].is_raii) {
                    printf("[%s:%d] ERROR: Memory leak detected! '%s' (allocated at line %d) is never freed or returned in %s\n",
                           filepath, line_no, tracked_vars[i].name, tracked_vars[i].line_allocated, current_func);
                    local_errors++;
                }
            }
            in_function = 0;
            tracked_count = 0;
            strcpy(current_func, "");
        }

        if (strlen(clean_line) > 0 && clean_line[strlen(clean_line)-1] != '\n') {
            strcpy(prev_line, clean_line);
        }
    }

    fclose(f);

    if (local_errors == 0 && local_warnings == 0) {
        printf("Success: No ownership issues found in %s!\n", filepath);
    } else {
        printf("Found %d errors and %d warnings in %s.\n", local_errors, local_warnings, filepath);
    }

    *total_errors += local_errors;
    *total_warnings += local_warnings;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file1.c> [file2.c ...]\n", argv[0]);
        return 1;
    }

    int total_errors = 0;
    int total_warnings = 0;

    for (int i = 1; i < argc; i++) {
        lint_file(argv[i], &total_errors, &total_warnings);
    }

    if (total_errors > 0) {
        return 1;
    }
    return 0;
}
