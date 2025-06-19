# Source Code Mapping Guide

This document provides a comprehensive mapping from InnoDB API calls to their internal implementations, helping you trace execution paths through the codebase.

## 🗺️ API to Implementation Mapping

### Core API Functions

| API Function | Location | Internal Functions Called | Key Files Involved |
|--------------|----------|---------------------------|-------------------|
| `ib_init()` | `api/api0api.c:726` | `ut_mem_init()`, `ib_cfg_init()` | `ut/ut0mem.c`, `api/api0cfg.c` |
| `ib_startup()` | `api/api0api.c:749` | `innobase_start_or_create()` | `srv/srv0start.c` |
| `ib_shutdown()` | `api/api0api.c` | `logs_empty_and_mark_files_at_shutdown()` | `srv/srv0start.c` |

### Transaction Management

| API Function | Location | Internal Flow | Key Components |
|--------------|----------|---------------|----------------|
| `ib_trx_begin()` | `api/api0api.c` | `trx_allocate_for_mysql()` → `trx_start_low()` | `trx/trx0trx.c` |
| `ib_trx_commit()` | `api/api0api.c` | `trx_commit_for_mysql()` → `trx_commit_low()` | `trx/trx0trx.c` |
| `ib_trx_rollback()` | `api/api0api.c` | `trx_rollback_for_mysql()` → `trx_rollback_low()` | `trx/trx0roll.c` |

### Cursor Operations

| API Function | Location | Internal Implementation | Core Files |
|--------------|----------|------------------------|------------|
| `ib_cursor_open_table()` | `api/api0api.c` | `dict_table_get()` → `btr_pcur_open()` | `dict/dict0dict.c`, `btr/btr0pcur.c` |
| `ib_cursor_close()` | `api/api0api.c` | `btr_pcur_close()` → cleanup cursors | `btr/btr0pcur.c` |
| `ib_cursor_lock()` | `api/api0api.c` | `lock_table()` | `lock/lock0lock.c` |

### Data Manipulation

| API Function | Location | Execution Path | Files Involved |
|--------------|----------|----------------|----------------|
| `ib_cursor_insert_row()` | `api/api0api.c` | `row_ins_for_mysql()` → `btr_cur_optimistic_insert()` | `row/row0ins.c`, `btr/btr0cur.c` |
| `ib_cursor_update_row()` | `api/api0api.c` | `row_upd_for_mysql()` → `btr_cur_optimistic_update()` | `row/row0upd.c`, `btr/btr0cur.c` |
| `ib_cursor_delete_row()` | `api/api0api.c` | `row_del_for_mysql()` → `btr_cur_optimistic_delete()` | `row/row0purge.c`, `btr/btr0cur.c` |

### Navigation Functions

| API Function | Location | B+ Tree Operations | Implementation |
|--------------|----------|-------------------|----------------|
| `ib_cursor_first()` | `api/api0api.c` | `btr_pcur_open_at_index_side()` with `TRUE` | `btr/btr0pcur.c` |
| `ib_cursor_last()` | `api/api0api.c` | `btr_pcur_open_at_index_side()` with `FALSE` | `btr/btr0pcur.c` |
| `ib_cursor_next()` | `api/api0api.c` | `btr_pcur_move_to_next()` | `btr/btr0pcur.c` |
| `ib_cursor_prev()` | `api/api0api.c` | `btr_pcur_move_to_prev()` | `btr/btr0pcur.c` |

## 🔍 Deep Dive: Function Call Chains

### Insert Operation Call Chain

```
ib_cursor_insert_row()                    # api/api0api.c
  └── row_ins_for_mysql()                 # row/row0mysql.c
      └── row_ins()                       # row/row0ins.c
          └── row_ins_index_entry()       # row/row0ins.c
              └── btr_cur_optimistic_insert() # btr/btr0cur.c
                  └── page_cur_tuple_insert()  # page/page0cur.c
                      └── page_mem_alloc()     # page/page0page.c
                          └── buf_page_get()   # buf/buf0buf.c
```

### Transaction Commit Call Chain

```
ib_trx_commit()                          # api/api0api.c
  └── trx_commit_for_mysql()             # trx/trx0trx.c
      └── trx_commit()                   # trx/trx0trx.c
          ├── trx_write_serialisation_history() # trx/trx0trx.c
          ├── lock_release_off_kernel()  # lock/lock0lock.c
          └── log_write_up_to()          # log/log0log.c
              └── log_group_write_buf()  # log/log0log.c
```

### Table Scan Call Chain

```
ib_cursor_first()                        # api/api0api.c
  └── btr_pcur_open_at_index_side()      # btr/btr0pcur.c
      └── btr_cur_open_at_index_side()   # btr/btr0cur.c
          └── btr_root_get()             # btr/btr0btr.c
              └── btr_page_get()         # btr/btr0btr.c
                  └── buf_page_get()     # buf/buf0buf.c
                      └── buf_read_page() # buf/buf0rea.c
```

## 📂 File Organization by Functionality

### Buffer Pool Management
```
buf/buf0buf.c    - Main buffer pool operations
buf/buf0lru.c    - LRU list management  
buf/buf0flu.c    - Dirty page flushing
buf/buf0rea.c    - Read-ahead logic
buf/buf0buddy.c  - Buddy allocator for compressed pages
```

### B+ Tree Operations
```
btr/btr0btr.c    - B+ tree structure operations
btr/btr0cur.c    - B+ tree cursor operations  
btr/btr0pcur.c   - Persistent cursor operations
btr/btr0sea.c    - Adaptive hash index (search acceleration)
```

### Transaction System
```
trx/trx0trx.c    - Transaction control blocks
trx/trx0roll.c   - Rollback operations
trx/trx0sys.c    - Transaction system global state
trx/trx0rseg.c   - Rollback segments  
trx/trx0undo.c   - Undo log management
trx/trx0purge.c  - Purge system (MVCC cleanup)
```

### Row Operations
```
row/row0ins.c    - Row insertion
row/row0sel.c    - Row selection/reading
row/row0upd.c    - Row updates
row/row0purge.c  - Row deletion/purging
row/row0undo.c   - Row-level undo operations
row/row0vers.c   - MVCC version control
```

### Locking System
```
lock/lock0lock.c - Main locking implementation
lock/lock0iter.c - Lock iteration utilities
```

### Logging System
```
log/log0log.c    - Write-ahead logging
log/log0recv.c   - Recovery (redo/undo)
```

## 🎯 Key Data Structures and Their Files

### Transaction Structure (`trx_t`)
**Defined in**: `include/trx0trx.h`  
**Implemented in**: `trx/trx0trx.c`

```c
struct trx_struct {
    ulint           state;          // TRX_ACTIVE, TRX_COMMITTED, etc.
    trx_id_t        id;            // Transaction ID
    trx_id_t        no;            // Transaction number  
    read_view_t*    read_view;     // MVCC read view
    ulint           isolation_level; // Isolation level
    // ... more fields
};
```

### Buffer Pool Block (`buf_block_t`)
**Defined in**: `include/buf0buf.h`  
**Implemented in**: `buf/buf0buf.c`

```c
struct buf_block_struct {
    buf_page_t      page;          // Page information
    byte*           frame;         // Pointer to buffer frame
    BUF_BLOCK_*     state;         // Block state
    rw_lock_t       lock;          // Page latch
    // ... more fields  
};
```

### B+ Tree Cursor (`btr_cur_t`)
**Defined in**: `include/btr0cur.h`  
**Implemented in**: `btr/btr0cur.c`

```c
struct btr_cur_struct {
    dict_index_t*   index;         // Index being accessed
    page_cur_t      page_cur;      // Page cursor
    purge_node_t*   purge_node;    // For purge operations
    // ... more fields
};
```

## 🔄 Control Flow Patterns

### Error Handling Pattern
```c
// Common pattern throughout codebase
ib_err_t some_function() {
    ib_err_t err = DB_SUCCESS;
    
    // Operation
    err = some_internal_function();
    if (err != DB_SUCCESS) {
        goto error_exit;
    }
    
    // More operations...
    
error_exit:
    // Cleanup code
    return err;
}
```

### Resource Management Pattern
```c
// RAII-style resource management
ib_err_t cursor_operation() {
    mtr_t mtr;
    
    mtr_start(&mtr);
    
    // Critical section with latches held
    // ...
    
    mtr_commit(&mtr);  // Releases all latches
    return DB_SUCCESS;
}
```

### Iterator Pattern
```c
// Common iteration pattern
ib_err_t scan_table() {
    btr_pcur_t pcur;
    
    btr_pcur_open_at_index_side(&pcur, ...);
    
    do {
        // Process current record
        rec = btr_pcur_get_rec(&pcur);
        // ...
        
    } while (btr_pcur_move_to_next(&pcur, &mtr) == TRUE);
    
    btr_pcur_close(&pcur);
}
```

## 🧩 Module Dependencies

```
API Layer (api/)
    ↓
Transaction Management (trx/) ←→ Lock Manager (lock/)
    ↓                              ↓
Row Operations (row/) ←→ B+ Tree (btr/)
    ↓                              ↓  
Page Management (page/) ←→ Buffer Pool (buf/)
    ↓                              ↓
Mini-Transactions (mtr/) ←→ Logging (log/)
    ↓                              ↓
File System (fil/) ←→ OS Layer (os/)
```

## 🔍 Finding Implementation Details

### Using grep to trace function calls:
```bash
# Find function definition
grep -rn "^function_name" . --include="*.c"

# Find function usage
grep -rn "function_name(" . --include="*.c"

# Find structure definition  
grep -rn "struct.*struct_name" . --include="*.h"

# Find all API functions
grep -rn "^ib_" api/ --include="*.c"
```

### Using ctags for navigation:
```bash
# Generate tags file
ctags -R .

# In vim, jump to function definition
:tag function_name

# Show function signature
:ts function_name
```

### Understanding call relationships:
```bash
# Create simple call graph
cflow api/api0api.c | grep -A 5 "ib_cursor_insert_row"

# Find what calls a function
grep -rn "function_name(" . --include="*.c" | head -10

# Find what a function calls
sed -n '/^function_name/,/^}/p' source_file.c | grep "("
```

---

This mapping provides the foundation for understanding how high-level API calls translate into low-level storage engine operations in the early InnoDB implementation.