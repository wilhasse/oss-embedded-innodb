# InnoDB API Reference

Complete reference for the Embedded InnoDB API functions and their implementations.

## 🔧 Core API Functions

### Initialization and Shutdown

#### `ib_init()`
**Location**: `api/api0api.c:726`  
**Purpose**: Initialize the InnoDB engine  
**Must be called**: Before any other InnoDB function

```c
ib_err_t ib_init(void);
```

**Internal Flow**:
1. `ut_mem_init()` - Initialize memory subsystem
2. `ib_cfg_init()` - Initialize configuration system
3. Set default logger to `fprintf`

#### `ib_startup()`
**Location**: `api/api0api.c:749`  
**Purpose**: Start the storage engine  

```c
ib_err_t ib_startup(const char* format);
```

**Parameters**:
- `format`: File format version (e.g., "barracuda")

**Internal Flow**:
1. Create log directory structure
2. Initialize buffer pool
3. Start background threads
4. Open/create log files
5. Perform crash recovery if needed

#### `ib_shutdown()`
**Location**: `api/api0api.c`  
**Purpose**: Shutdown the storage engine

```c
ib_err_t ib_shutdown(ib_shutdown_t shutdown_flag);
```

**Parameters**:
- `shutdown_flag`: `IB_SHUTDOWN_NORMAL` or `IB_SHUTDOWN_NO_BUFPOOL_FLUSH`

### Database and Table Operations

#### `ib_database_create()`
**Purpose**: Create a database (directory)

```c
ib_err_t ib_database_create(const char* dbname);
```

#### `ib_table_create()`
**Purpose**: Create a table with specified schema

```c
ib_err_t ib_table_create(ib_trx_t ib_trx, const ib_table_def_t* table_def, ib_id_t* table_id);
```

#### `ib_table_drop()`
**Purpose**: Drop a table

```c
ib_err_t ib_table_drop(ib_trx_t ib_trx, const char* name);
```

### Transaction Management

#### `ib_trx_begin()`
**Location**: `api/api0api.c`  
**Purpose**: Begin a new transaction

```c
ib_trx_t ib_trx_begin(ib_trx_level_t ib_trx_level);
```

**Parameters**:
- `ib_trx_level`: Isolation level
  - `IB_TRX_READ_UNCOMMITTED`
  - `IB_TRX_READ_COMMITTED` 
  - `IB_TRX_REPEATABLE_READ`
  - `IB_TRX_SERIALIZABLE`

**Returns**: Transaction handle

#### `ib_trx_commit()`
**Purpose**: Commit a transaction

```c
ib_err_t ib_trx_commit(ib_trx_t ib_trx);
```

**Internal Flow**:
1. Write transaction log entries
2. Release row locks
3. Release table locks
4. Update transaction state

#### `ib_trx_rollback()`
**Purpose**: Rollback a transaction

```c
ib_err_t ib_trx_rollback(ib_trx_t ib_trx);
```

### Cursor Operations

#### `ib_cursor_open_table()`
**Purpose**: Open a cursor on a table

```c
ib_err_t ib_cursor_open_table(const char* name, ib_trx_t ib_trx, ib_crsr_t* ib_crsr);
```

**Parameters**:
- `name`: Table name
- `ib_trx`: Transaction handle
- `ib_crsr`: Cursor handle (output)

#### `ib_cursor_lock()`
**Purpose**: Acquire table-level lock

```c
ib_err_t ib_cursor_lock(ib_crsr_t ib_crsr, ib_lck_mode_t ib_lck_mode);
```

**Lock Modes**:
- `IB_LOCK_IS` - Intention Shared
- `IB_LOCK_IX` - Intention Exclusive
- `IB_LOCK_S` - Shared
- `IB_LOCK_X` - Exclusive

#### `ib_cursor_close()`
**Purpose**: Close cursor and release resources

```c
ib_err_t ib_cursor_close(ib_crsr_t ib_crsr);
```

### Data Manipulation

#### `ib_cursor_insert_row()`
**Purpose**: Insert a row into the table

```c
ib_err_t ib_cursor_insert_row(ib_crsr_t ib_crsr, const ib_tpl_t ib_tpl);
```

**Parameters**:
- `ib_crsr`: Cursor handle
- `ib_tpl`: Tuple containing row data

#### `ib_cursor_update_row()`
**Purpose**: Update current row

```c
ib_err_t ib_cursor_update_row(ib_crsr_t ib_crsr, const ib_tpl_t ib_old_tpl, const ib_tpl_t ib_new_tpl);
```

#### `ib_cursor_delete_row()`
**Purpose**: Delete current row

```c
ib_err_t ib_cursor_delete_row(ib_crsr_t ib_crsr);
```

### Cursor Navigation

#### `ib_cursor_first()`
**Purpose**: Position cursor at first record

```c
ib_err_t ib_cursor_first(ib_crsr_t ib_crsr);
```

#### `ib_cursor_next()`
**Purpose**: Move cursor to next record

```c
ib_err_t ib_cursor_next(ib_crsr_t ib_crsr);
```

#### `ib_cursor_prev()`
**Purpose**: Move cursor to previous record

```c
ib_err_t ib_cursor_prev(ib_crsr_t ib_crsr);
```

#### `ib_cursor_last()`
**Purpose**: Position cursor at last record

```c
ib_err_t ib_cursor_last(ib_crsr_t ib_crsr);
```

### Search Operations

#### `ib_cursor_moveto()`
**Purpose**: Search for specific key

```c
ib_err_t ib_cursor_moveto(ib_crsr_t ib_crsr, ib_tpl_t ib_tpl, ib_srch_mode_t ib_srch_mode);
```

**Search Modes**:
- `IB_CUR_G` - Greater than
- `IB_CUR_GE` - Greater than or equal
- `IB_CUR_L` - Less than  
- `IB_CUR_LE` - Less than or equal

### Tuple Operations

#### `ib_clust_read_tuple_create()`
**Purpose**: Create tuple for reading

```c
ib_tpl_t ib_clust_read_tuple_create(ib_crsr_t ib_crsr);
```

#### `ib_clust_search_tuple_create()`
**Purpose**: Create tuple for searching

```c
ib_tpl_t ib_clust_search_tuple_create(ib_crsr_t ib_crsr);
```

#### `ib_tuple_delete()`
**Purpose**: Delete tuple and free memory

```c
void ib_tuple_delete(ib_tpl_t ib_tpl);
```

### Column Operations

#### `ib_col_set_value()`
**Purpose**: Set column value in tuple

```c
ib_err_t ib_col_set_value(ib_tpl_t ib_tpl, ib_ulint_t col_no, const void* src, ib_ulint_t len);
```

#### `ib_col_get_value()`
**Purpose**: Get column value from tuple

```c
ib_err_t ib_col_get_value(ib_tpl_t ib_tpl, ib_ulint_t col_no, void* dst, ib_ulint_t len);
```

#### `ib_col_copy_value()`
**Purpose**: Copy column value to buffer

```c
ib_ulint_t ib_col_copy_value(ib_tpl_t ib_tpl, ib_ulint_t col_no, void* dst, ib_ulint_t len);
```

#### `ib_col_get_meta()`
**Purpose**: Get column metadata

```c
ib_err_t ib_col_get_meta(ib_tpl_t ib_tpl, ib_ulint_t col_no, ib_col_meta_t* ib_col_meta);
```

### Configuration Management

#### `ib_cfg_set()`
**Purpose**: Set configuration parameter

```c
ib_err_t ib_cfg_set(const char* name, const void* value);
```

#### `ib_cfg_get()`
**Purpose**: Get configuration parameter

```c
ib_err_t ib_cfg_get(const char* name, void* value);
```

## 📊 Data Types

### Core Types

```c
typedef enum ib_err_enum ib_err_t;     // Error codes
typedef struct ib_crsr_struct* ib_crsr_t;  // Cursor handle
typedef struct ib_trx_struct* ib_trx_t;    // Transaction handle
typedef struct ib_tpl_struct* ib_tpl_t;    // Tuple handle
```

### Error Codes

```c
typedef enum {
    DB_SUCCESS = 10,
    DB_ERROR,
    DB_OUT_OF_MEMORY,
    DB_OUT_OF_FILE_SPACE,
    DB_LOCK_WAIT,
    DB_DEADLOCK,
    DB_ROLLBACK,
    DB_DUPLICATE_KEY,
    DB_QUE_THR_SUSPENDED,
    DB_MISSING_HISTORY,
    DB_CLUSTER_NOT_FOUND,
    DB_TABLE_NOT_FOUND,
    DB_MUST_GET_MORE_FILE_SPACE,
    DB_TABLE_IS_BEING_USED,
    DB_TOO_BIG_RECORD,
    DB_LOCK_WAIT_TIMEOUT,
    DB_NO_REFERENCED_ROW,
    DB_ROW_IS_REFERENCED,
    DB_CANNOT_ADD_CONSTRAINT,
    DB_CORRUPTION,
    DB_CANNOT_DROP_CONSTRAINT,
    DB_NO_SAVEPOINT,
    DB_TABLESPACE_ALREADY_EXISTS,
    DB_TABLESPACE_DELETED,
    DB_CANNOT_CREATE_TABLE,
    DB_OUT_OF_DISK_SPACE,
    DB_SCHEMA_NOT_LOCKED,
    DB_SCHEMA_ERROR,
    DB_INVALID_INPUT,
    DB_CHILD_NO_INDEX,
    DB_PARENT_NO_INDEX,
    DB_NOT_FOUND,
    DB_END_OF_INDEX
} ib_err_t;
```

### Column Types

```c
typedef enum {
    IB_VARCHAR = 1,
    IB_CHAR,
    IB_BINARY,
    IB_VARBINARY,
    IB_BLOB,
    IB_INT,
    IB_SYS,
    IB_FLOAT,
    IB_DOUBLE,
    IB_DECIMAL,
    IB_TIMESTAMP,
    IB_DATE,
    IB_TIME,
    IB_DATETIME
} ib_col_type_t;
```

## 🎯 Usage Patterns

### Basic Table Operations

```c
// Initialize
ib_init();
ib_startup("barracuda");

// Create transaction
ib_trx_t trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

// Open table
ib_crsr_t cursor;
ib_cursor_open_table("test.users", trx, &cursor);

// Insert data
ib_tpl_t tuple = ib_clust_search_tuple_create(cursor);
ib_col_set_value(tuple, 0, &id, sizeof(id));
ib_col_set_value(tuple, 1, name, strlen(name));
ib_cursor_insert_row(cursor, tuple);

// Commit and cleanup
ib_trx_commit(trx);
ib_cursor_close(cursor);
ib_tuple_delete(tuple);
ib_shutdown(IB_SHUTDOWN_NORMAL);
```

### Scanning Table

```c
// Position at first record
ib_cursor_first(cursor);

// Read all records
do {
    ib_tpl_t read_tuple = ib_clust_read_tuple_create(cursor);
    // Process tuple data
    ib_tuple_delete(read_tuple);
} while (ib_cursor_next(cursor) == DB_SUCCESS);
```

---

This API provides the foundation for all database operations in the early InnoDB storage engine, offering direct access to transaction management, B+ tree operations, and ACID compliance mechanisms.