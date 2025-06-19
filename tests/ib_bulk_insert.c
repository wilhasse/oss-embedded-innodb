/***********************************************************************
InnoDB Massive Bulk Insert Performance Test

This program demonstrates high-performance bulk inserts into InnoDB by:
- Creating tables with multiple data types
- Generating random data for millions of rows
- Using optimized batch inserts with large transactions
- Monitoring performance metrics and throughput

Usage: ./ib_bulk_insert [rows] [batch_size] [threads]
************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#include "test0aux.h"

#ifdef UNIV_DEBUG_VALGRIND
#include <valgrind/memcheck.h>
#endif

#define DATABASE        "bulk_test"
#define TABLE          "massive_data"
#define DEFAULT_ROWS   1000000
#define DEFAULT_BATCH  10000
#define MAX_THREADS    16

/* Performance monitoring structure */
typedef struct {
    struct timeval start_time;
    struct timeval end_time;
    unsigned long rows_inserted;
    unsigned long batches_completed;
    unsigned long total_bytes;
} perf_stats_t;

/* Thread worker structure */
typedef struct {
    int thread_id;
    unsigned long start_row;
    unsigned long end_row;
    unsigned long batch_size;
    perf_stats_t stats;
    ib_trx_t trx;
    ib_crsr_t cursor;
} worker_t;

/* Global configuration */
static unsigned long g_total_rows = DEFAULT_ROWS;
static unsigned long g_batch_size = DEFAULT_BATCH;
static int g_num_threads = 1;
static int g_verbose = 1;

/*********************************************************************
Get current time in microseconds */
static unsigned long
get_time_usec(void) 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

/*********************************************************************
Generate random string data */
static void
generate_random_string(char* buffer, int min_len, int max_len)
{
    int len = min_len + (rand() % (max_len - min_len + 1));
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
    
    for (int i = 0; i < len; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[len] = '\0';
}

/*********************************************************************
Generate random email address */
static void
generate_random_email(char* buffer, int max_len)
{
    const char* domains[] = {"gmail.com", "yahoo.com", "hotmail.com", "company.com", "test.org"};
    char username[64];
    
    generate_random_string(username, 5, 15);
    snprintf(buffer, max_len, "%s@%s", username, domains[rand() % 5]);
}

/*********************************************************************
Create database if it doesn't exist */
static ib_err_t
create_database(const char* dbname)
{
    ib_bool_t   err;

    err = ib_database_create(dbname);
    if (err == IB_TRUE) {
        if (g_verbose) {
            printf("Created database '%s'\n", dbname);
        }
        return DB_SUCCESS;
    } else {
        if (g_verbose) {
            printf("Database '%s' already exists\n", dbname);
        }
        return DB_SUCCESS;
    }
}

/*********************************************************************
Create optimized table for bulk inserts */
static ib_err_t
create_bulk_table(const char* dbname, const char* name)
{
    ib_trx_t    ib_trx;
    ib_id_t     table_id = 0;
    ib_err_t    err = DB_SUCCESS;
    ib_tbl_sch_t ib_tbl_sch = NULL;
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

    /* Add columns optimized for bulk inserts */
    
    /* Primary key: auto-increment ID */
    err = ib_table_schema_add_col(ib_tbl_sch, "id", IB_INT, IB_COL_UNSIGNED, 0, 8);
    assert(err == DB_SUCCESS);

    /* User ID - simulates foreign key */
    err = ib_table_schema_add_col(ib_tbl_sch, "user_id", IB_INT, IB_COL_UNSIGNED, 0, 4);
    assert(err == DB_SUCCESS);

    /* Name - variable length string */
    err = ib_table_schema_add_col(ib_tbl_sch, "name", IB_VARCHAR, IB_COL_NONE, 0, 100);
    assert(err == DB_SUCCESS);

    /* Email - variable length string */
    err = ib_table_schema_add_col(ib_tbl_sch, "email", IB_VARCHAR, IB_COL_NONE, 0, 255);
    assert(err == DB_SUCCESS);

    /* Score - floating point */
    err = ib_table_schema_add_col(ib_tbl_sch, "score", IB_DOUBLE, IB_COL_NONE, 0, 8);
    assert(err == DB_SUCCESS);

    /* Created timestamp */
    err = ib_table_schema_add_col(ib_tbl_sch, "created_at", IB_INT, IB_COL_UNSIGNED, 0, 4);
    assert(err == DB_SUCCESS);

    /* Data blob - simulates larger payloads */
    err = ib_table_schema_add_col(ib_tbl_sch, "data_blob", IB_BLOB, IB_COL_NONE, 0, 0);
    assert(err == DB_SUCCESS);

    /* Create primary key index */
    ib_idx_sch_t ib_idx_sch = NULL;
    err = ib_table_schema_add_index(ib_tbl_sch, "PRIMARY_KEY", &ib_idx_sch);
    assert(err == DB_SUCCESS);

    err = ib_index_schema_add_col(ib_idx_sch, "id", 0);
    assert(err == DB_SUCCESS);

    err = ib_index_schema_set_clustered(ib_idx_sch);
    assert(err == DB_SUCCESS);

    /* Add secondary index on user_id for realistic workload */
    ib_idx_sch_t ib_idx_sch2 = NULL;
    err = ib_table_schema_add_index(ib_tbl_sch, "IDX_USER_ID", &ib_idx_sch2);
    assert(err == DB_SUCCESS);

    err = ib_index_schema_add_col(ib_idx_sch2, "user_id", 0);
    assert(err == DB_SUCCESS);

    /* Create the table */
    err = ib_table_create(ib_trx, ib_tbl_sch, &table_id);
    if (err != DB_SUCCESS) {
        if (err == DB_TABLE_IS_BEING_USED) {
            if (g_verbose) {
                printf("Table '%s' already exists\n", table_name);
            }
            err = DB_SUCCESS;
        } else {
            fprintf(stderr, "Table creation failed: %s\n", ib_strerror(err));
        }
    } else {
        if (g_verbose) {
            printf("Created table '%s' with optimized schema\n", table_name);
        }
    }

    if (ib_tbl_sch != NULL) {
        ib_table_schema_delete(ib_tbl_sch);
    }

    err = ib_trx_commit(ib_trx);
    assert(err == DB_SUCCESS);

    return err;
}

/*********************************************************************
Perform bulk insert for a range of rows */
static void*
bulk_insert_worker(void* arg)
{
    worker_t* worker = (worker_t*)arg;
    ib_err_t err;
    char table_name[IB_MAX_TABLE_NAME_LEN];
    ib_tpl_t tpl = NULL;
    unsigned long current_row;
    unsigned long batch_count = 0;
    char name_buffer[101];
    char email_buffer[256];
    char blob_buffer[1024];
    time_t current_time = time(NULL);

    snprintf(table_name, sizeof(table_name), "%s/%s", DATABASE, TABLE);

    /* Begin transaction */
    worker->trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);
    assert(worker->trx != NULL);

    /* Open cursor */
    err = ib_cursor_open_table(table_name, worker->trx, &worker->cursor);
    assert(err == DB_SUCCESS);

    /* Lock cursor for inserts */
    err = ib_cursor_lock(worker->cursor, IB_LOCK_IX);
    assert(err == DB_SUCCESS);

    /* Create tuple template */
    tpl = ib_clust_read_tuple_create(worker->cursor);
    assert(tpl != NULL);

    gettimeofday(&worker->stats.start_time, NULL);

    /* Insert rows in batches */
    for (current_row = worker->start_row; current_row < worker->end_row; current_row++) {
        
        /* Generate random data */
        generate_random_string(name_buffer, 10, 50);
        generate_random_email(email_buffer, sizeof(email_buffer));
        generate_random_string(blob_buffer, 100, 500);

        /* Set column values */
        err = ib_col_set_value(tpl, 0, &current_row, sizeof(current_row));
        assert(err == DB_SUCCESS);

        ib_u32_t user_id = (current_row % 100000) + 1;  /* Simulate user references */
        err = ib_col_set_value(tpl, 1, &user_id, sizeof(user_id));
        assert(err == DB_SUCCESS);

        err = ib_col_set_value(tpl, 2, name_buffer, strlen(name_buffer));
        assert(err == DB_SUCCESS);

        err = ib_col_set_value(tpl, 3, email_buffer, strlen(email_buffer));
        assert(err == DB_SUCCESS);

        double score = (double)(rand() % 10000) / 100.0;  /* Random score 0-100 */
        err = ib_col_set_value(tpl, 4, &score, sizeof(score));
        assert(err == DB_SUCCESS);

        ib_u32_t timestamp = (ib_u32_t)current_time;
        err = ib_col_set_value(tpl, 5, &timestamp, sizeof(timestamp));
        assert(err == DB_SUCCESS);

        err = ib_col_set_value(tpl, 6, blob_buffer, strlen(blob_buffer));
        assert(err == DB_SUCCESS);

        /* Insert row */
        err = ib_cursor_insert_row(worker->cursor, tpl);
        assert(err == DB_SUCCESS || err == DB_DUPLICATE_KEY);

        worker->stats.rows_inserted++;
        worker->stats.total_bytes += strlen(name_buffer) + strlen(email_buffer) + strlen(blob_buffer) + 24;

        /* Commit batch when batch size is reached */
        if ((current_row - worker->start_row + 1) % worker->batch_size == 0) {
            err = ib_trx_commit(worker->trx);
            assert(err == DB_SUCCESS);
            
            batch_count++;
            worker->stats.batches_completed = batch_count;

            if (g_verbose && worker->thread_id == 0 && batch_count % 10 == 0) {
                printf("Thread %d: Completed %lu batches (%lu rows)\n", 
                       worker->thread_id, batch_count, current_row - worker->start_row + 1);
            }

            /* Start new transaction for next batch */
            worker->trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);
            assert(worker->trx != NULL);

            err = ib_cursor_close(worker->cursor);
            assert(err == DB_SUCCESS);

            err = ib_cursor_open_table(table_name, worker->trx, &worker->cursor);
            assert(err == DB_SUCCESS);

            err = ib_cursor_lock(worker->cursor, IB_LOCK_IX);
            assert(err == DB_SUCCESS);
        }

        /* Reset tuple for next iteration */
        ib_tuple_clear(tpl);
    }

    /* Commit final batch */
    if (worker->stats.rows_inserted % worker->batch_size != 0) {
        err = ib_trx_commit(worker->trx);
        assert(err == DB_SUCCESS);
        worker->stats.batches_completed++;
    } else {
        /* If we ended on a batch boundary, we still need to close the current transaction */
        err = ib_trx_commit(worker->trx);
        assert(err == DB_SUCCESS);
    }

    gettimeofday(&worker->stats.end_time, NULL);

    /* Cleanup */
    if (tpl != NULL) {
        ib_tuple_delete(tpl);
    }

    err = ib_cursor_close(worker->cursor);
    assert(err == DB_SUCCESS);

    return NULL;
}

/*********************************************************************
Print performance statistics */
static void
print_performance_stats(worker_t* workers, int num_workers)
{
    unsigned long total_rows = 0;
    unsigned long total_batches = 0;
    unsigned long total_bytes = 0;
    unsigned long min_time_usec = ULONG_MAX;
    unsigned long max_time_usec = 0;
    unsigned long total_time_usec = 0;

    printf("\n=== BULK INSERT PERFORMANCE RESULTS ===\n");

    for (int i = 0; i < num_workers; i++) {
        unsigned long worker_time = 
            (workers[i].stats.end_time.tv_sec - workers[i].stats.start_time.tv_sec) * 1000000 +
            (workers[i].stats.end_time.tv_usec - workers[i].stats.start_time.tv_usec);

        total_rows += workers[i].stats.rows_inserted;
        total_batches += workers[i].stats.batches_completed;
        total_bytes += workers[i].stats.total_bytes;

        if (worker_time < min_time_usec) min_time_usec = worker_time;
        if (worker_time > max_time_usec) max_time_usec = worker_time;
        total_time_usec += worker_time;

        printf("Thread %d: %lu rows, %lu batches, %.2f MB, %.3f sec\n",
               i, workers[i].stats.rows_inserted, workers[i].stats.batches_completed,
               (double)workers[i].stats.total_bytes / (1024*1024),
               (double)worker_time / 1000000.0);
    }

    double avg_time_sec = (double)max_time_usec / 1000000.0;  /* Use max time as wall clock */
    double throughput_rows_per_sec = (double)total_rows / avg_time_sec;
    double throughput_mb_per_sec = ((double)total_bytes / (1024*1024)) / avg_time_sec;

    printf("\n--- SUMMARY ---\n");
    printf("Total rows inserted: %lu\n", total_rows);
    printf("Total batches: %lu\n", total_batches);
    printf("Total data size: %.2f MB\n", (double)total_bytes / (1024*1024));
    printf("Wall clock time: %.3f seconds\n", avg_time_sec);
    printf("Throughput: %.0f rows/sec\n", throughput_rows_per_sec);
    printf("Throughput: %.2f MB/sec\n", throughput_mb_per_sec);
    printf("Average batch size: %.0f rows\n", (double)total_rows / total_batches);
    printf("Threads used: %d\n", num_workers);
}

/*********************************************************************
Main bulk insert test function */
int
main(int argc, char* argv[])
{
    ib_err_t err;
    worker_t workers[MAX_THREADS];
    pthread_t threads[MAX_THREADS];

    /* Parse command line arguments */
    if (argc > 1) {
        g_total_rows = strtoul(argv[1], NULL, 10);
    }
    if (argc > 2) {
        g_batch_size = strtoul(argv[2], NULL, 10);
    }
    if (argc > 3) {
        g_num_threads = atoi(argv[3]);
        if (g_num_threads > MAX_THREADS) {
            g_num_threads = MAX_THREADS;
        }
    }

    printf("=== InnoDB Bulk Insert Performance Test ===\n");
    printf("Target rows: %lu\n", g_total_rows);
    printf("Batch size: %lu\n", g_batch_size);
    printf("Threads: %d\n", g_num_threads);

    srand(time(NULL));

    /* Initialize InnoDB */
    err = ib_init();
    assert(err == DB_SUCCESS);

    test_configure();

    err = ib_startup("barracuda");
    assert(err == DB_SUCCESS);

    /* Create database and table */
    err = create_database(DATABASE);
    assert(err == DB_SUCCESS);

    err = create_bulk_table(DATABASE, TABLE);
    assert(err == DB_SUCCESS);

    printf("\n--- Starting bulk insert ---\n");

    /* Calculate rows per thread */
    unsigned long rows_per_thread = g_total_rows / g_num_threads;
    unsigned long remaining_rows = g_total_rows % g_num_threads;

    /* Create and start worker threads */
    for (int i = 0; i < g_num_threads; i++) {
        workers[i].thread_id = i;
        workers[i].start_row = i * rows_per_thread + 1;
        workers[i].end_row = (i + 1) * rows_per_thread;
        
        /* Give remaining rows to last thread */
        if (i == g_num_threads - 1) {
            workers[i].end_row += remaining_rows;
        }
        
        workers[i].batch_size = g_batch_size;
        memset(&workers[i].stats, 0, sizeof(perf_stats_t));

        pthread_create(&threads[i], NULL, bulk_insert_worker, &workers[i]);
    }

    /* Wait for all threads to complete */
    for (int i = 0; i < g_num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Print performance results */
    print_performance_stats(workers, g_num_threads);

    /* Cleanup */
    err = ib_shutdown(IB_SHUTDOWN_NORMAL);
    assert(err == DB_SUCCESS);

    return EXIT_SUCCESS;
}