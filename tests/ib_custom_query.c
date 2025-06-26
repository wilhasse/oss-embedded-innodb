/***********************************************************************
Custom Query Tool for InnoDB bulk_test database

This program allows flexible querying of the massive_data table with
various conditions and options.

Usage: ./ib_custom_query [options]
  --id <id>           Query specific ID
  --user-id <uid>     Query specific user ID  
  --range <start-end> Query ID range (e.g., 1000-2000)
  --limit <n>         Limit results (default: 20)
  --offset <n>        Skip first n rows
  --score-min <f>     Minimum score filter
  --score-max <f>     Maximum score filter
  --name-like <str>   Name contains string
  --email-domain <d>  Email domain filter
  --count-only        Just count matching rows
  --sample            Random sample of rows
************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include "test0aux.h"

#ifdef UNIV_DEBUG_VALGRIND
#include <valgrind/memcheck.h>
#endif

#define DATABASE        "bulk_test"
#define TABLE          "massive_data"
#define DEFAULT_LIMIT   20

/* Query parameters structure */
typedef struct {
    ib_u64_t    specific_id;
    ib_u32_t    specific_user_id;
    ib_u64_t    range_start;
    ib_u64_t    range_end;
    ib_ulint_t  limit;
    ib_ulint_t  offset;
    double      score_min;
    double      score_max;
    char        name_like[101];
    char        email_domain[256];
    int         count_only;
    int         sample_mode;
    int         use_specific_id;
    int         use_specific_user_id;
    int         use_range;
    int         use_score_filter;
    int         use_name_filter;
    int         use_email_filter;
} query_params_t;

/*********************************************************************
Display row data */
static void
print_row_data(ib_tpl_t ib_tpl, ib_ulint_t row_num)
{
    ib_err_t        err;
    ib_ulint_t      data_len;
    const void*     ptr;
    ib_u64_t        id;
    ib_u32_t        user_id;
    char            name[101] = {0};
    char            email[256] = {0};
    double          score;
    ib_u32_t        created_at;
    ib_ulint_t      blob_len;

    printf("Row %lu: ", row_num);

    /* Read ID (column 0) */
    err = ib_tuple_read_u64(ib_tpl, 0, &id);
    if (err == DB_SUCCESS) {
        printf("ID=%llu ", (unsigned long long)id);
    }

    /* Read user_id (column 1) */
    err = ib_tuple_read_u32(ib_tpl, 1, &user_id);
    if (err == DB_SUCCESS) {
        printf("UserID=%u ", user_id);
    }

    /* Read name (column 2) */
    ptr = ib_col_get_value(ib_tpl, 2);
    data_len = ib_col_get_len(ib_tpl, 2);
    
    if (ptr != NULL && data_len > 0) {
        memcpy(name, ptr, data_len < 100 ? data_len : 100);
        name[data_len < 100 ? data_len : 100] = '\0';
        printf("Name='%s' ", name);
    }

    /* Read email (column 3) */
    ptr = ib_col_get_value(ib_tpl, 3);
    data_len = ib_col_get_len(ib_tpl, 3);
    
    if (ptr != NULL && data_len > 0) {
        memcpy(email, ptr, data_len < 255 ? data_len : 255);
        email[data_len < 255 ? data_len : 255] = '\0';
        printf("Email='%s' ", email);
    }

    /* Read score (column 4) */
    err = ib_tuple_read_double(ib_tpl, 4, &score);
    if (err == DB_SUCCESS) {
        printf("Score=%.2f ", score);
    }

    /* Read created_at (column 5) */
    err = ib_tuple_read_u32(ib_tpl, 5, &created_at);
    if (err == DB_SUCCESS) {
        time_t timestamp = (time_t)created_at;
        struct tm* tm_info = localtime(&timestamp);
        printf("Created=%04d-%02d-%02d ", 
               tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    }

    /* Read blob data (column 6) - just show length */
    blob_len = ib_col_get_len(ib_tpl, 6);
    if (blob_len > 0) {
        printf("BlobSize=%lu", blob_len);
    }

    printf("\n");
}

/*********************************************************************
Check if row matches filters */
static int
row_matches_filters(ib_tpl_t ib_tpl, const query_params_t* params)
{
    ib_err_t        err;
    ib_ulint_t      data_len;
    const void*     ptr;
    ib_u64_t        id;
    ib_u32_t        user_id;
    char            name[101] = {0};
    char            email[256] = {0};
    double          score;

    /* Check ID filters */
    err = ib_tuple_read_u64(ib_tpl, 0, &id);
    if (err != DB_SUCCESS) return 0;

    if (params->use_specific_id && id != params->specific_id) {
        return 0;
    }

    if (params->use_range && (id < params->range_start || id > params->range_end)) {
        return 0;
    }

    /* Check user_id filter */
    if (params->use_specific_user_id) {
        err = ib_tuple_read_u32(ib_tpl, 1, &user_id);
        if (err != DB_SUCCESS || user_id != params->specific_user_id) {
            return 0;
        }
    }

    /* Check score filter */
    if (params->use_score_filter) {
        err = ib_tuple_read_double(ib_tpl, 4, &score);
        if (err != DB_SUCCESS || score < params->score_min || score > params->score_max) {
            return 0;
        }
    }

    /* Check name filter */
    if (params->use_name_filter) {
        ptr = ib_col_get_value(ib_tpl, 2);
        data_len = ib_col_get_len(ib_tpl, 2);
        
        if (ptr != NULL && data_len > 0) {
            memcpy(name, ptr, data_len < 100 ? data_len : 100);
            name[data_len < 100 ? data_len : 100] = '\0';
            if (strstr(name, params->name_like) == NULL) {
                return 0;
            }
        } else {
            return 0;
        }
    }

    /* Check email domain filter */
    if (params->use_email_filter) {
        ptr = ib_col_get_value(ib_tpl, 3);
        data_len = ib_col_get_len(ib_tpl, 3);
        
        if (ptr != NULL && data_len > 0) {
            memcpy(email, ptr, data_len < 255 ? data_len : 255);
            email[data_len < 255 ? data_len : 255] = '\0';
            if (strstr(email, params->email_domain) == NULL) {
                return 0;
            }
        } else {
            return 0;
        }
    }

    return 1;
}

/*********************************************************************
Execute custom query */
static ib_err_t
execute_custom_query(const query_params_t* params)
{
    ib_err_t        err;
    ib_crsr_t       ib_crsr;
    ib_trx_t        ib_trx;
    ib_tpl_t        ib_tpl;
    char            table_name[IB_MAX_TABLE_NAME_LEN];
    ib_ulint_t      found_rows = 0;
    ib_ulint_t      processed_rows = 0;
    ib_ulint_t      skipped_rows = 0;

    snprintf(table_name, sizeof(table_name), "%s/%s", DATABASE, TABLE);

    printf("=== Custom Query Results ===\n");
    printf("Table: %s\n", table_name);

    /* Print query parameters */
    if (params->use_specific_id) {
        printf("Filter: ID = %llu\n", (unsigned long long)params->specific_id);
    }
    if (params->use_specific_user_id) {
        printf("Filter: UserID = %u\n", params->specific_user_id);
    }
    if (params->use_range) {
        printf("Filter: ID BETWEEN %llu AND %llu\n", 
               (unsigned long long)params->range_start, 
               (unsigned long long)params->range_end);
    }
    if (params->use_score_filter) {
        printf("Filter: Score BETWEEN %.2f AND %.2f\n", params->score_min, params->score_max);
    }
    if (params->use_name_filter) {
        printf("Filter: Name LIKE '%%%s%%'\n", params->name_like);
    }
    if (params->use_email_filter) {
        printf("Filter: Email LIKE '%%%s%%'\n", params->email_domain);
    }
    printf("Limit: %lu", params->limit);
    if (params->offset > 0) {
        printf(", Offset: %lu", params->offset);
    }
    printf("\n\n");

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

        /* Check if row matches filters */
        if (row_matches_filters(ib_tpl, params)) {
            /* Handle offset */
            if (skipped_rows < params->offset) {
                skipped_rows++;
            } else {
                found_rows++;
                
                if (!params->count_only) {
                    print_row_data(ib_tpl, found_rows);
                }

                /* Check limit */
                if (found_rows >= params->limit) {
                    break;
                }
            }
        }

        /* Move to next row */
        err = ib_cursor_next(ib_crsr);
    }

    if (err == DB_END_OF_INDEX) {
        err = DB_SUCCESS;
    }

    printf("\nQuery Results:\n");
    printf("- Processed rows: %lu\n", processed_rows);
    printf("- Matching rows: %lu\n", found_rows + skipped_rows);
    printf("- Displayed rows: %lu\n", found_rows);

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
Print usage information */
static void
print_query_usage(const char* program_name)
{
    printf("Usage: %s [options]\n\n", program_name);
    printf("Query Options:\n");
    printf("  --id <id>           Query specific ID\n");
    printf("  --user-id <uid>     Query specific user ID\n");
    printf("  --range <start-end> Query ID range (e.g., 1000-2000)\n");
    printf("  --limit <n>         Limit results (default: %d)\n", DEFAULT_LIMIT);
    printf("  --offset <n>        Skip first n rows\n");
    printf("  --score-min <f>     Minimum score filter\n");
    printf("  --score-max <f>     Maximum score filter\n");
    printf("  --name-like <str>   Name contains string\n");
    printf("  --email-domain <d>  Email domain filter (e.g., gmail.com)\n");
    printf("  --count-only        Just count matching rows\n");
    printf("  --help              Show this help\n\n");
    
    printf("Examples:\n");
    printf("  %s --id 12345\n", program_name);
    printf("  %s --range 1000-2000 --limit 10\n", program_name);
    printf("  %s --user-id 500 --limit 5\n", program_name);
    printf("  %s --score-min 80.0 --score-max 100.0\n", program_name);
    printf("  %s --email-domain gmail.com --limit 10\n", program_name);
    printf("  %s --name-like \"John\" --count-only\n", program_name);
}

/*********************************************************************
Main function */
int main(int argc, char* argv[])
{
    ib_err_t        err;
    query_params_t  params = {0};
    int             option_index = 0;
    int             c;

    /* Initialize default parameters */
    params.limit = DEFAULT_LIMIT;
    params.score_min = 0.0;
    params.score_max = 100.0;

    /* Define long options */
    static struct option long_options[] = {
        {"id",           required_argument, 0, 'i'},
        {"user-id",      required_argument, 0, 'u'},
        {"range",        required_argument, 0, 'r'},
        {"limit",        required_argument, 0, 'l'},
        {"offset",       required_argument, 0, 'o'},
        {"score-min",    required_argument, 0, 's'},
        {"score-max",    required_argument, 0, 'S'},
        {"name-like",    required_argument, 0, 'n'},
        {"email-domain", required_argument, 0, 'e'},
        {"count-only",   no_argument,       0, 'c'},
        {"help",         no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    /* Parse command line arguments */
    while ((c = getopt_long(argc, argv, "i:u:r:l:o:s:S:n:e:ch", 
                           long_options, &option_index)) != -1) {
        switch (c) {
            case 'i':
                params.specific_id = strtoull(optarg, NULL, 10);
                params.use_specific_id = 1;
                break;
            case 'u':
                params.specific_user_id = strtoul(optarg, NULL, 10);
                params.use_specific_user_id = 1;
                break;
            case 'r':
                if (sscanf(optarg, "%llu-%llu", 
                          (unsigned long long*)&params.range_start,
                          (unsigned long long*)&params.range_end) == 2) {
                    params.use_range = 1;
                } else {
                    printf("Error: Invalid range format. Use start-end (e.g., 1000-2000)\n");
                    return EXIT_FAILURE;
                }
                break;
            case 'l':
                params.limit = strtoul(optarg, NULL, 10);
                break;
            case 'o':
                params.offset = strtoul(optarg, NULL, 10);
                break;
            case 's':
                params.score_min = strtod(optarg, NULL);
                params.use_score_filter = 1;
                break;
            case 'S':
                params.score_max = strtod(optarg, NULL);
                params.use_score_filter = 1;
                break;
            case 'n':
                strncpy(params.name_like, optarg, sizeof(params.name_like) - 1);
                params.use_name_filter = 1;
                break;
            case 'e':
                strncpy(params.email_domain, optarg, sizeof(params.email_domain) - 1);
                params.use_email_filter = 1;
                break;
            case 'c':
                params.count_only = 1;
                break;
            case 'h':
                print_query_usage(argv[0]);
                return EXIT_SUCCESS;
            case '?':
                print_query_usage(argv[0]);
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    printf("InnoDB Custom Query Tool\n");
    printf("========================\n\n");

    err = ib_init();
    assert(err == DB_SUCCESS);

    test_configure();

    err = ib_startup("barracuda");
    assert(err == DB_SUCCESS);

    /* Execute the query */
    err = execute_custom_query(&params);
    if (err != DB_SUCCESS) {
        printf("Error executing query: %d\n", err);
    }

    err = ib_shutdown(IB_SHUTDOWN_NORMAL);
    assert(err == DB_SUCCESS);

    return(EXIT_SUCCESS);
}