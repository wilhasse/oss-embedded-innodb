/***********************************************************************
MySQL 8 Bulk Insert Performance Test

This program performs the same bulk insert operations as the InnoDB
embedded version but connects to a real MySQL 8 server via the MySQL
protocol for performance comparison.

Requirements:
- MySQL 8 server running
- libmysqlclient-dev installed
- Database created: CREATE DATABASE bulk_test_mysql;

Usage: ./mysql_bulk_insert [rows] [batch_size] [threads] [host] [user] [password]

Compile: gcc -o mysql_bulk_insert mysql_bulk_insert.c -lmysqlclient -lpthread -lm
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <stdbool.h>
#include <mysql/mysql.h>

#define DATABASE        "bulk_test_mysql"
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
    MYSQL *conn;
    char *host;
    char *user;
    char *password;
} worker_t;

/* Global configuration */
static unsigned long g_total_rows = DEFAULT_ROWS;
static unsigned long g_batch_size = DEFAULT_BATCH;
static int g_num_threads = 1;
static int g_verbose = 1;
static char *g_host = "localhost";
static char *g_user = "root";
static char *g_password = "";

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
Escape string for MySQL */
static void
mysql_escape_string_safe(MYSQL *mysql, char *to, const char *from, unsigned long length)
{
    mysql_real_escape_string(mysql, to, from, length);
}

/*********************************************************************
Create MySQL connection */
static MYSQL*
create_mysql_connection(const char* host, const char* user, const char* password)
{
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return NULL;
    }

    /* Set connection options for better performance */
    bool reconnect = true;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
    
    unsigned int timeout = 60;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

    if (mysql_real_connect(conn, host, user, password, DATABASE, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    return conn;
}

/*********************************************************************
Create database and table */
static int
create_mysql_table(const char* host, const char* user, const char* password)
{
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return -1;
    }

    /* Connect without selecting database first */
    if (mysql_real_connect(conn, host, user, password, NULL, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }

    /* Create database if not exists */
    char query[512];
    snprintf(query, sizeof(query), "CREATE DATABASE IF NOT EXISTS %s", DATABASE);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "CREATE DATABASE failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }

    /* Select database */
    if (mysql_select_db(conn, DATABASE)) {
        fprintf(stderr, "mysql_select_db() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }

    /* Create table with same schema as InnoDB version */
    const char* create_table_sql = 
        "CREATE TABLE IF NOT EXISTS massive_data ("
        "  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "  user_id INT UNSIGNED NOT NULL,"
        "  name VARCHAR(100) NOT NULL,"
        "  email VARCHAR(255) NOT NULL,"
        "  score DOUBLE NOT NULL,"
        "  created_at INT UNSIGNED NOT NULL,"
        "  data_blob TEXT NOT NULL,"
        "  PRIMARY KEY (id),"
        "  INDEX idx_user_id (user_id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

    if (mysql_query(conn, create_table_sql)) {
        fprintf(stderr, "CREATE TABLE failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }

    /* Optimize for bulk inserts */
    mysql_query(conn, "SET autocommit = 0");
    mysql_query(conn, "SET unique_checks = 0");
    mysql_query(conn, "SET foreign_key_checks = 0");
    mysql_query(conn, "SET sql_log_bin = 0");

    if (g_verbose) {
        printf("Created MySQL database '%s' and table '%s'\n", DATABASE, TABLE);
    }

    mysql_close(conn);
    return 0;
}

/*********************************************************************
Perform bulk insert for a range of rows */
static void*
mysql_bulk_insert_worker(void* arg)
{
    worker_t* worker = (worker_t*)arg;
    MYSQL *conn;
    unsigned long current_row;
    unsigned long batch_count = 0;
    char name_buffer[101];
    char email_buffer[256];
    char blob_buffer[1024];
    char escaped_name[202];
    char escaped_email[512];
    char escaped_blob[2048];
    char query[4096];
    time_t current_time = time(NULL);

    /* Create MySQL connection */
    conn = create_mysql_connection(worker->host, worker->user, worker->password);
    if (conn == NULL) {
        fprintf(stderr, "Thread %d: Failed to connect to MySQL\n", worker->thread_id);
        return NULL;
    }

    /* Optimize for bulk inserts */
    mysql_query(conn, "SET autocommit = 0");
    mysql_query(conn, "SET unique_checks = 0");
    mysql_query(conn, "SET foreign_key_checks = 0");

    gettimeofday(&worker->stats.start_time, NULL);

    /* Start transaction */
    mysql_query(conn, "START TRANSACTION");

    /* Insert rows in batches */
    for (current_row = worker->start_row; current_row < worker->end_row; current_row++) {
        
        /* Generate random data */
        generate_random_string(name_buffer, 10, 50);
        generate_random_email(email_buffer, sizeof(email_buffer));
        generate_random_string(blob_buffer, 100, 500);

        /* Escape strings for MySQL */
        mysql_escape_string_safe(conn, escaped_name, name_buffer, strlen(name_buffer));
        mysql_escape_string_safe(conn, escaped_email, email_buffer, strlen(email_buffer));
        mysql_escape_string_safe(conn, escaped_blob, blob_buffer, strlen(blob_buffer));

        /* Build INSERT query */
        unsigned int user_id = (current_row % 100000) + 1;
        double score = (double)(rand() % 10000) / 100.0;
        unsigned int timestamp = (unsigned int)current_time;

        snprintf(query, sizeof(query),
            "INSERT INTO massive_data (id, user_id, name, email, score, created_at, data_blob) "
            "VALUES (%lu, %u, '%s', '%s', %.2f, %u, '%s')",
            current_row, user_id, escaped_name, escaped_email, score, timestamp, escaped_blob);

        /* Execute insert */
        if (mysql_query(conn, query)) {
            fprintf(stderr, "Thread %d: INSERT failed at row %lu: %s\n", 
                    worker->thread_id, current_row, mysql_error(conn));
            break;
        }

        worker->stats.rows_inserted++;
        worker->stats.total_bytes += strlen(name_buffer) + strlen(email_buffer) + strlen(blob_buffer) + 24;

        /* Commit batch when batch size is reached */
        if ((current_row - worker->start_row + 1) % worker->batch_size == 0) {
            if (mysql_query(conn, "COMMIT")) {
                fprintf(stderr, "Thread %d: COMMIT failed: %s\n", 
                        worker->thread_id, mysql_error(conn));
                break;
            }
            
            batch_count++;
            worker->stats.batches_completed = batch_count;

            if (g_verbose && worker->thread_id == 0 && batch_count % 10 == 0) {
                printf("Thread %d: Completed %lu batches (%lu rows)\n", 
                       worker->thread_id, batch_count, current_row - worker->start_row + 1);
            }

            /* Start new transaction for next batch */
            mysql_query(conn, "START TRANSACTION");
        }
    }

    /* Commit final batch */
    if (worker->stats.rows_inserted % worker->batch_size != 0) {
        mysql_query(conn, "COMMIT");
        worker->stats.batches_completed++;
    } else {
        mysql_query(conn, "COMMIT");
    }

    gettimeofday(&worker->stats.end_time, NULL);

    /* Cleanup */
    mysql_close(conn);

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

    printf("\n=== MYSQL BULK INSERT PERFORMANCE RESULTS ===\n");

    for (int i = 0; i < num_workers; i++) {
        unsigned long worker_time = 
            (workers[i].stats.end_time.tv_sec - workers[i].stats.start_time.tv_sec) * 1000000 +
            (workers[i].stats.end_time.tv_usec - workers[i].stats.start_time.tv_usec);

        total_rows += workers[i].stats.rows_inserted;
        total_batches += workers[i].stats.batches_completed;
        total_bytes += workers[i].stats.total_bytes;

        if (worker_time < min_time_usec) min_time_usec = worker_time;
        if (worker_time > max_time_usec) max_time_usec = worker_time;

        printf("Thread %d: %lu rows, %lu batches, %.2f MB, %.3f sec\n",
               i, workers[i].stats.rows_inserted, workers[i].stats.batches_completed,
               (double)workers[i].stats.total_bytes / (1024*1024),
               (double)worker_time / 1000000.0);
    }

    double avg_time_sec = (double)max_time_usec / 1000000.0;
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
    printf("MySQL Protocol: TCP/IP to %s\n", g_host);
}

/*********************************************************************
Main MySQL bulk insert test function */
int
main(int argc, char* argv[])
{
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
    if (argc > 4) {
        g_host = argv[4];
    }
    if (argc > 5) {
        g_user = argv[5];
    }
    if (argc > 6) {
        g_password = argv[6];
    }

    printf("=== MySQL 8 Bulk Insert Performance Test ===\n");
    printf("Target rows: %lu\n", g_total_rows);
    printf("Batch size: %lu\n", g_batch_size);
    printf("Threads: %d\n", g_num_threads);
    printf("MySQL Host: %s\n", g_host);
    printf("MySQL User: %s\n", g_user);

    srand(time(NULL));

    /* Initialize MySQL library */
    if (mysql_library_init(0, NULL, NULL)) {
        fprintf(stderr, "could not initialize MySQL client library\n");
        exit(1);
    }

    /* Create database and table */
    if (create_mysql_table(g_host, g_user, g_password) != 0) {
        mysql_library_end();
        exit(1);
    }

    printf("\n--- Starting MySQL bulk insert ---\n");

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
        workers[i].host = g_host;
        workers[i].user = g_user;
        workers[i].password = g_password;
        memset(&workers[i].stats, 0, sizeof(perf_stats_t));

        pthread_create(&threads[i], NULL, mysql_bulk_insert_worker, &workers[i]);
    }

    /* Wait for all threads to complete */
    for (int i = 0; i < g_num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Print performance results */
    print_performance_stats(workers, g_num_threads);

    /* Cleanup */
    mysql_library_end();

    return EXIT_SUCCESS;
}