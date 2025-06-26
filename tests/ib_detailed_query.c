/***********************************************************************
Detailed Query Tool for InnoDB - Shows all column information

This program displays complete column information including data types,
sizes, and full content for easier analysis.

Usage: ./ib_detailed_query [--limit n] [--offset n] [--id n]
************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "test0aux.h"

#ifdef UNIV_DEBUG_VALGRIND
#include <valgrind/memcheck.h>
#endif

#define DATABASE        "bulk_test"
#define TABLE          "massive_data"
#define DEFAULT_LIMIT   10

/*********************************************************************
Display detailed row information */
static void
print_detailed_row(ib_tpl_t ib_tpl, ib_ulint_t row_num)
{
    ib_err_t        err;
    ib_ulint_t      data_len;
    ib_col_meta_t   col_meta;
    const void*     ptr;
    ib_u64_t        id;
    ib_u32_t        user_id;
    char            name[1024] = {0};
    char            email[1024] = {0};
    double          score;
    ib_u32_t        created_at;
    const char*     blob_data;
    ib_ulint_t      blob_len;
    int             i;

    printf("\n================================================================================\n");
    printf("ROW #%lu DETAILS\n", row_num);
    printf("================================================================================\n");

    /* Column 0: ID (BIGINT UNSIGNED) */
    printf("Column 0 - ID (BIGINT UNSIGNED):\n");
    err = ib_tuple_read_u64(ib_tpl, 0, &id);
    if (err == DB_SUCCESS) {
        printf("  Value: %llu\n", (unsigned long long)id);
    } else {
        printf("  Error reading: %d\n", err);
    }
    ib_col_get_meta(ib_tpl, 0, &col_meta);
    printf("  Type: %d, Length: %lu\n", col_meta.type, ib_col_get_len(ib_tpl, 0));

    /* Column 1: user_id (INT UNSIGNED) */
    printf("\nColumn 1 - USER_ID (INT UNSIGNED):\n");
    err = ib_tuple_read_u32(ib_tpl, 1, &user_id);
    if (err == DB_SUCCESS) {
        printf("  Value: %u\n", user_id);
    } else {
        printf("  Error reading: %d\n", err);
    }
    ib_col_get_meta(ib_tpl, 1, &col_meta);
    printf("  Type: %d, Length: %lu\n", col_meta.type, ib_col_get_len(ib_tpl, 1));

    /* Column 2: name (VARCHAR(100)) */
    printf("\nColumn 2 - NAME (VARCHAR(100)):\n");
    ptr = ib_col_get_value(ib_tpl, 2);
    data_len = ib_col_get_len(ib_tpl, 2);
    ib_col_get_meta(ib_tpl, 2, &col_meta);
    
    if (ptr != NULL && data_len > 0) {
        memcpy(name, ptr, data_len < 1023 ? data_len : 1023);
        name[data_len < 1023 ? data_len : 1023] = '\0';
        printf("  Value: '%s'\n", name);
        printf("  Length: %lu characters\n", data_len);
    } else {
        printf("  Value: NULL or empty\n");
    }
    printf("  Type: %d, Declared Length: %lu\n", col_meta.type, col_meta.type_len);

    /* Column 3: email (VARCHAR(255)) */
    printf("\nColumn 3 - EMAIL (VARCHAR(255)):\n");
    ptr = ib_col_get_value(ib_tpl, 3);
    data_len = ib_col_get_len(ib_tpl, 3);
    ib_col_get_meta(ib_tpl, 3, &col_meta);
    
    if (ptr != NULL && data_len > 0) {
        memcpy(email, ptr, data_len < 1023 ? data_len : 1023);
        email[data_len < 1023 ? data_len : 1023] = '\0';
        printf("  Value: '%s'\n", email);
        printf("  Length: %lu characters\n", data_len);
    } else {
        printf("  Value: NULL or empty\n");
    }
    printf("  Type: %d, Declared Length: %lu\n", col_meta.type, col_meta.type_len);

    /* Column 4: score (DOUBLE) */
    printf("\nColumn 4 - SCORE (DOUBLE):\n");
    err = ib_tuple_read_double(ib_tpl, 4, &score);
    if (err == DB_SUCCESS) {
        printf("  Value: %.6f\n", score);
    } else {
        printf("  Error reading: %d\n", err);
    }
    ib_col_get_meta(ib_tpl, 4, &col_meta);
    printf("  Type: %d, Length: %lu\n", col_meta.type, ib_col_get_len(ib_tpl, 4));

    /* Column 5: created_at (INT UNSIGNED - Unix timestamp) */
    printf("\nColumn 5 - CREATED_AT (INT UNSIGNED - Unix Timestamp):\n");
    err = ib_tuple_read_u32(ib_tpl, 5, &created_at);
    if (err == DB_SUCCESS) {
        time_t timestamp = (time_t)created_at;
        struct tm* tm_info = localtime(&timestamp);
        printf("  Value: %u\n", created_at);
        printf("  Formatted: %04d-%02d-%02d %02d:%02d:%02d\n", 
               tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    } else {
        printf("  Error reading: %d\n", err);
    }
    ib_col_get_meta(ib_tpl, 5, &col_meta);
    printf("  Type: %d, Length: %lu\n", col_meta.type, ib_col_get_len(ib_tpl, 5));

    /* Column 6: data_blob (BLOB) */
    printf("\nColumn 6 - DATA_BLOB (BLOB):\n");
    ptr = ib_col_get_value(ib_tpl, 6);
    blob_len = ib_col_get_len(ib_tpl, 6);
    ib_col_get_meta(ib_tpl, 6, &col_meta);
    
    printf("  Size: %lu bytes\n", blob_len);
    printf("  Type: %d, Declared Length: %lu\n", col_meta.type, col_meta.type_len);
    
    if (ptr != NULL && blob_len > 0) {
        blob_data = (const char*)ptr;
        printf("  Content preview (first 100 bytes):\n  ");
        
        for (i = 0; i < (blob_len < 100 ? blob_len : 100); i++) {
            if (blob_data[i] >= 32 && blob_data[i] <= 126) {
                printf("%c", blob_data[i]);  /* Printable character */
            } else {
                printf(".");  /* Non-printable character */
            }
        }
        printf("\n");
        
        if (blob_len > 100) {
            printf("  ... (showing first 100 of %lu bytes)\n", blob_len);
        }
        
        /* Show hex dump of first 32 bytes */
        printf("  Hex dump (first 32 bytes):\n  ");
        for (i = 0; i < (blob_len < 32 ? blob_len : 32); i++) {
            printf("%02x ", (unsigned char)blob_data[i]);
            if ((i + 1) % 16 == 0) printf("\n  ");
        }
        printf("\n");
    } else {
        printf("  Content: NULL or empty\n");
    }
}

/*********************************************************************
Query and display detailed data */
static ib_err_t
query_detailed_data(ib_ulint_t limit, ib_ulint_t offset, ib_u64_t specific_id)
{
    ib_err_t        err;
    ib_crsr_t       ib_crsr;
    ib_trx_t        ib_trx;
    ib_tpl_t        ib_tpl;
    char            table_name[IB_MAX_TABLE_NAME_LEN];
    ib_ulint_t      found_rows = 0;
    ib_ulint_t      processed_rows = 0;
    ib_ulint_t      skipped_rows = 0;
    ib_u64_t        current_id;

    snprintf(table_name, sizeof(table_name), "%s/%s", DATABASE, TABLE);

    printf("InnoDB Detailed Column Information\n");
    printf("==================================\n");
    printf("Table: %s\n", table_name);
    if (specific_id > 0) {
        printf("Showing: ID = %llu\n", (unsigned long long)specific_id);
    } else {
        printf("Showing: %lu rows", limit);
        if (offset > 0) {
            printf(" (offset: %lu)", offset);
        }
        printf("\n");
    }

    /* Begin transaction */
    ib_trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);
    assert(ib_trx != NULL);

    /* Open cursor on the table */
    err = ib_cursor_open_table(table_name, ib_trx, &ib_crsr);
    if (err != DB_SUCCESS) {
        printf("Error: Cannot open table '%s'. Error code: %d\n", table_name, err);
        ib_trx_rollback(ib_trx);
        return err;
    }

    /* Create tuple template for reading */
    ib_tpl = ib_clust_read_tuple_create(ib_crsr);
    assert(ib_tpl != NULL);

    /* Position cursor at first record */
    err = ib_cursor_first(ib_crsr);
    if (err == DB_END_OF_INDEX) {
        printf("Table is empty.\n");
        goto cleanup;
    }
    
    if (err != DB_SUCCESS) {
        printf("Error positioning cursor: %d\n", err);
        goto cleanup;
    }

    /* Scan through records */
    while (err == DB_SUCCESS) {
        /* Read current row */
        err = ib_cursor_read_row(ib_crsr, ib_tpl);
        if (err != DB_SUCCESS) {
            break;
        }

        processed_rows++;

        /* Check if specific ID requested */
        if (specific_id > 0) {
            ib_tuple_read_u64(ib_tpl, 0, &current_id);
            if (current_id != specific_id) {
                err = ib_cursor_next(ib_crsr);
                continue;
            }
        }

        /* Handle offset */
        if (skipped_rows < offset) {
            skipped_rows++;
        } else {
            found_rows++;
            print_detailed_row(ib_tpl, found_rows);

            /* Check limit */
            if (found_rows >= limit) {
                break;
            }
        }

        /* Move to next row */
        err = ib_cursor_next(ib_crsr);
    }

    if (err == DB_END_OF_INDEX) {
        err = DB_SUCCESS;
    }

    printf("\n================================================================================\n");
    printf("QUERY SUMMARY\n");
    printf("================================================================================\n");
    printf("Processed rows: %lu\n", processed_rows);
    printf("Displayed rows: %lu\n", found_rows);

cleanup:
    if (ib_tpl != NULL) {
        ib_tuple_delete(ib_tpl);
    }

    if (ib_crsr != NULL) {
        ib_cursor_close(ib_crsr);
    }

    err = ib_trx_commit(ib_trx);
    
    return err;
}

/*********************************************************************
Main function */
int main(int argc, char* argv[])
{
    ib_err_t        err;
    ib_ulint_t      limit = DEFAULT_LIMIT;
    ib_ulint_t      offset = 0;
    ib_u64_t        specific_id = 0;
    int             i;

    /* Parse simple command line arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            limit = strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--offset") == 0 && i + 1 < argc) {
            offset = strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--id") == 0 && i + 1 < argc) {
            specific_id = strtoull(argv[++i], NULL, 10);
            limit = 1; /* Only show one row for specific ID */
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--limit n] [--offset n] [--id n]\n", argv[0]);
            printf("  --limit n   : Show n rows (default: %d)\n", DEFAULT_LIMIT);
            printf("  --offset n  : Skip first n rows\n");
            printf("  --id n      : Show specific ID only\n");
            printf("  --help      : Show this help\n");
            return EXIT_SUCCESS;
        }
    }

    err = ib_init();
    assert(err == DB_SUCCESS);

    test_configure();

    err = ib_startup("barracuda");
    assert(err == DB_SUCCESS);

    /* Execute the detailed query */
    err = query_detailed_data(limit, offset, specific_id);
    if (err != DB_SUCCESS) {
        printf("Error executing query: %d\n", err);
    }

    err = ib_shutdown(IB_SHUTDOWN_NORMAL);
    assert(err == DB_SUCCESS);

    return(EXIT_SUCCESS);
}