/***********************************************************************
Simple InnoDB Bulk Insert Test - Single Threaded

This is a simplified version for testing bulk inserts without the
complexity of multi-threading, making it easier to debug issues.

Usage: ./ib_simple_bulk [rows] [batch_size]
************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "test0aux.h"

#define DATABASE        "simple_test"
#define TABLE          "data"
#define DEFAULT_ROWS   10000
#define DEFAULT_BATCH  1000

/*********************************************************************
Create database if it doesn't exist */
static ib_err_t
create_database(const char* dbname)
{
    ib_bool_t   err;

    err = ib_database_create(dbname);
    if (err == IB_TRUE) {
        printf("Created database '%s'\n", dbname);
        return DB_SUCCESS;
    } else {
        printf("Database '%s' already exists\n", dbname);
        return DB_SUCCESS;
    }
}

/*********************************************************************
Create simple table for bulk inserts */
static ib_err_t
create_simple_table(const char* dbname, const char* name)
{
    ib_trx_t    ib_trx;
    ib_id_t     table_id = 0;
    ib_err_t    err = DB_SUCCESS;
    ib_tbl_sch_t ib_tbl_sch = NULL;
    ib_idx_sch_t ib_idx_sch = NULL;
    char        table_name[IB_MAX_TABLE_NAME_LEN];

    snprintf(table_name, sizeof(table_name), "%s/%s", dbname, name);

    /* Begin transaction for DDL */
    ib_trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);
    assert(ib_trx != NULL);

    /* Lock schema for DDL operations */
    err = ib_schema_lock_exclusive(ib_trx);
    assert(err == DB_SUCCESS);

    /* Create table schema */
    err = ib_table_schema_create(table_name, &ib_tbl_sch, IB_TBL_COMPACT, 0);
    assert(err == DB_SUCCESS);

    /* Add simple columns */
    
    /* Primary key: auto-increment ID */
    err = ib_table_schema_add_col(ib_tbl_sch, "id", IB_INT, IB_COL_UNSIGNED, 0, 4);
    assert(err == DB_SUCCESS);

    /* Name - variable length string */
    err = ib_table_schema_add_col(ib_tbl_sch, "name", IB_VARCHAR, IB_COL_NONE, 0, 50);
    assert(err == DB_SUCCESS);

    /* Value - integer */
    err = ib_table_schema_add_col(ib_tbl_sch, "value", IB_INT, IB_COL_NONE, 0, 4);
    assert(err == DB_SUCCESS);

    /* Create primary key index */
    err = ib_table_schema_add_index(ib_tbl_sch, "PRIMARY_KEY", &ib_idx_sch);
    assert(err == DB_SUCCESS);

    err = ib_index_schema_add_col(ib_idx_sch, "id", 0);
    assert(err == DB_SUCCESS);

    err = ib_index_schema_set_clustered(ib_idx_sch);
    assert(err == DB_SUCCESS);

    /* Create the table */
    err = ib_table_create(ib_trx, ib_tbl_sch, &table_id);
    if (err != DB_SUCCESS) {
        if (err == DB_TABLE_IS_BEING_USED) {
            printf("Table '%s' already exists\n", table_name);
            err = DB_SUCCESS;
        } else {
            fprintf(stderr, "Table creation failed: %s\n", ib_strerror(err));
        }
    } else {
        printf("Created table '%s'\n", table_name);
    }

    if (ib_tbl_sch != NULL) {
        ib_table_schema_delete(ib_tbl_sch);
    }

    err = ib_trx_commit(ib_trx);
    assert(err == DB_SUCCESS);

    return err;
}

/*********************************************************************
Perform simple bulk insert */
static ib_err_t
simple_bulk_insert(unsigned long total_rows, unsigned long batch_size)
{
    ib_err_t err;
    char table_name[IB_MAX_TABLE_NAME_LEN];
    ib_trx_t ib_trx = NULL;
    ib_crsr_t cursor = NULL;
    ib_tpl_t tpl = NULL;
    unsigned long current_row;
    unsigned long batch_count = 0;
    char name_buffer[51];
    struct timeval start_time, end_time;
    
    snprintf(table_name, sizeof(table_name), "%s/%s", DATABASE, TABLE);

    printf("Inserting %lu rows in batches of %lu...\n", total_rows, batch_size);
    gettimeofday(&start_time, NULL);

    /* Begin first transaction */
    ib_trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);
    assert(ib_trx != NULL);

    /* Open cursor */
    err = ib_cursor_open_table(table_name, ib_trx, &cursor);
    assert(err == DB_SUCCESS);

    /* Lock cursor for inserts */
    err = ib_cursor_lock(cursor, IB_LOCK_IX);
    assert(err == DB_SUCCESS);

    /* Create tuple template */
    tpl = ib_clust_read_tuple_create(cursor);
    assert(tpl != NULL);

    /* Insert rows in batches */
    for (current_row = 1; current_row <= total_rows; current_row++) {
        
        /* Generate simple data */
        snprintf(name_buffer, sizeof(name_buffer), "User_%lu", current_row);
        ib_u32_t id = (ib_u32_t)current_row;
        ib_i32_t value = (ib_i32_t)(current_row % 1000);

        /* Set column values */
        err = ib_col_set_value(tpl, 0, &id, sizeof(id));
        assert(err == DB_SUCCESS);

        err = ib_col_set_value(tpl, 1, name_buffer, strlen(name_buffer));
        assert(err == DB_SUCCESS);

        err = ib_col_set_value(tpl, 2, &value, sizeof(value));
        assert(err == DB_SUCCESS);

        /* Insert row */
        err = ib_cursor_insert_row(cursor, tpl);
        if (err != DB_SUCCESS) {
            fprintf(stderr, "Insert failed at row %lu: %s\n", current_row, ib_strerror(err));
            break;
        }

        /* Commit batch when batch size is reached */
        if (current_row % batch_size == 0) {
            /* Close cursor before committing */
            err = ib_cursor_close(cursor);
            assert(err == DB_SUCCESS);
            
            err = ib_trx_commit(ib_trx);
            assert(err == DB_SUCCESS);
            
            batch_count++;
            printf("Committed batch %lu (%lu rows)\n", batch_count, current_row);

            /* Start new transaction for next batch if not done */
            if (current_row < total_rows) {
                ib_trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);
                assert(ib_trx != NULL);

                err = ib_cursor_open_table(table_name, ib_trx, &cursor);
                assert(err == DB_SUCCESS);

                err = ib_cursor_lock(cursor, IB_LOCK_IX);
                assert(err == DB_SUCCESS);
            }
        }

        /* Reset tuple for next iteration */
        ib_tuple_clear(tpl);
    }

    /* Commit final batch if needed */
    if (total_rows % batch_size != 0) {
        err = ib_cursor_close(cursor);
        assert(err == DB_SUCCESS);
        
        err = ib_trx_commit(ib_trx);
        assert(err == DB_SUCCESS);
        
        batch_count++;
        printf("Committed final batch %lu (%lu rows)\n", batch_count, total_rows);
    }

    gettimeofday(&end_time, NULL);

    /* Cleanup */
    if (tpl != NULL) {
        ib_tuple_delete(tpl);
    }

    /* Calculate performance */
    double elapsed = (end_time.tv_sec - start_time.tv_sec) + 
                    (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    double throughput = (double)total_rows / elapsed;

    printf("\n=== PERFORMANCE RESULTS ===\n");
    printf("Total rows: %lu\n", total_rows);
    printf("Batches: %lu\n", batch_count);
    printf("Elapsed time: %.3f seconds\n", elapsed);
    printf("Throughput: %.0f rows/sec\n", throughput);

    return DB_SUCCESS;
}

/*********************************************************************
Main function */
int
main(int argc, char* argv[])
{
    ib_err_t err;
    unsigned long total_rows = DEFAULT_ROWS;
    unsigned long batch_size = DEFAULT_BATCH;

    /* Parse command line arguments */
    if (argc > 1) {
        total_rows = strtoul(argv[1], NULL, 10);
    }
    if (argc > 2) {
        batch_size = strtoul(argv[2], NULL, 10);
    }

    printf("=== Simple InnoDB Bulk Insert Test ===\n");
    printf("Target rows: %lu\n", total_rows);
    printf("Batch size: %lu\n", batch_size);

    /* Initialize InnoDB */
    err = ib_init();
    assert(err == DB_SUCCESS);

    test_configure();

    err = ib_startup("barracuda");
    assert(err == DB_SUCCESS);

    /* Create database and table */
    err = create_database(DATABASE);
    assert(err == DB_SUCCESS);

    err = create_simple_table(DATABASE, TABLE);
    assert(err == DB_SUCCESS);

    /* Perform bulk insert */
    err = simple_bulk_insert(total_rows, batch_size);
    assert(err == DB_SUCCESS);

    /* Cleanup */
    err = ib_shutdown(IB_SHUTDOWN_NORMAL);
    assert(err == DB_SUCCESS);

    return EXIT_SUCCESS;
}