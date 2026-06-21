#include "sqlite3.h"
#include "ownedc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// custom SQLite memory methods redirecting to OwnedC
static void* ownedc_xMalloc(int size) {
    return owner_malloc((size_t)size);
}

static void ownedc_xFree(void* ptr) {
    owner_free(ptr);
}

static void* ownedc_xRealloc(void* ptr, int size) {
    return owner_realloc(ptr, (size_t)size);
}

static int ownedc_xSize(void* ptr) {
    return (int)owner_malloc_usable_size(ptr);
}

static int ownedc_xRoundup(int size) {
    return (size + 7) & ~7;
}

static int ownedc_xInit(void* pAppData) {
    (void)pAppData;
    return 0;
}

static void ownedc_xShutdown(void* pAppData) {
    (void)pAppData;
}

static sqlite3_mem_methods ownedc_mem_methods = {
    ownedc_xMalloc,
    ownedc_xFree,
    ownedc_xRealloc,
    ownedc_xSize,
    ownedc_xRoundup,
    ownedc_xInit,
    ownedc_xShutdown,
    NULL
};

// SQL execution helper
static int execute_sql(sqlite3* db, const char* sql) {
    char* err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("SQL Error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    printf("==================================================\n");
    printf("   OwnedC Showcase: Memory-Safe SQLite Database   \n");
    printf("==================================================\n\n");

    // 1. Configure SQLite to use OwnedC's memory tracker!
    // This MUST be called before any other sqlite3_* function.
    int rc = sqlite3_config(SQLITE_CONFIG_MALLOC, &ownedc_mem_methods);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to configure SQLite memory allocator (code %d)\n", rc);
        return 1;
    }
    printf("[1] Successfully configured SQLite to use OwnedC's dynamic registry!\n");

    // 2. Open an in-memory database
    sqlite3* db = NULL;
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    printf("[2] Opened SQLite in-memory database instance.\n");

    // Print active allocations after opening DB
    printf("  > Active allocations tracked by OwnedC right after DB open:\n");
    ownership_stats();

    // 3. Create a table
    printf("\n[3] Creating table 'users' and inserting records...\n");
    execute_sql(db, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT);");

    // 4. Insert records
    execute_sql(db, "INSERT INTO users VALUES (1, 'Alice', 30);");
    execute_sql(db, "INSERT INTO users VALUES (2, 'Bob', 25);");
    execute_sql(db, "INSERT INTO users VALUES (3, 'Charlie', 35);");

    // 5. Query records
    printf("\n[4] Querying records...\n");
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, "SELECT * FROM users WHERE age > 20;", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        int age = sqlite3_column_int(stmt, 2);
        printf("  > Row: ID=%d, Name=%s, Age=%d\n", id, name, age);
    }
    
    // Check if user requested to simulate a memory leak
    if (argc > 1 && strcmp(argv[1], "--simulate-leak") == 0) {
        printf("\n[DANGER] Simulating a memory leak (forgetting to finalize sqlite3_stmt)...\n");
        // We do NOT call sqlite3_finalize(stmt)
        // Closing the database connection while a statement is active will fail or leak memory
        sqlite3_close(db);
        printf("[DANGER] Program ending now. OwnedC's exit checker should intercept the leak!\n");
        return 0;
    }

    // Check if user requested to simulate a double free
    if (argc > 1 && strcmp(argv[1], "--simulate-double-free") == 0) {
        printf("\n[DANGER] Simulating a use-after-free / double-free by explicitly calling sqlite3_free twice on a pointer...\n");
        void* temp_ptr = sqlite3_malloc(128);
        sqlite3_free(temp_ptr);
        sqlite3_free(temp_ptr); // Double-free!
        return 0;
    }

    // Clean up normally
    sqlite3_finalize(stmt);
    printf("\n[5] Cleaned up statement handles.\n");

    sqlite3_close(db);
    printf("[6] Closed SQLite database instance.\n");

    printf("\n[7] Database execution finished successfully.\n");
    printf("  > OwnedC stats before normal exit:\n");
    ownership_stats();

    return 0;
}
