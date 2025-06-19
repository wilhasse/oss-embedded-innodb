# InnoDB Architecture Overview

This document describes the core architecture of the early InnoDB storage engine from the Innobase era.

## 🏗️ High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        CLIENT APPLICATION                   │
├─────────────────────────────────────────────────────────────┤
│                         API LAYER                          │
│                      (api/api0api.c)                       │
├─────────────────────────────────────────────────────────────┤
│  TRANSACTION MGMT  │  LOCK MANAGER  │   DATA DICTIONARY    │
│     (trx/*)        │    (lock/*)    │      (dict/*)        │
├─────────────────────────────────────────────────────────────┤
│     BUFFER POOL    │   B+ TREE     │    ROW OPERATIONS    │
│      (buf/*)       │    (btr/*)     │      (row/*)         │
├─────────────────────────────────────────────────────────────┤
│  PAGE MANAGEMENT   │  MINI-TRANS    │   LOG MANAGEMENT     │
│     (page/*)       │    (mtr/*)     │      (log/*)         │
├─────────────────────────────────────────────────────────────┤
│              FILE SYSTEM ABSTRACTION LAYER                 │
│                  (fil/* + os/*)                            │
├─────────────────────────────────────────────────────────────┤
│                      OPERATING SYSTEM                      │
│                   (Files, Memory, Threads)                 │
└─────────────────────────────────────────────────────────────┘
```

## 📂 Core Components

### 1. API Layer (`api/`)
**Purpose**: External interface for applications  
**Key Files**: `api0api.c`

- **Entry Point**: All `ib_*` functions start here
- **Function Mapping**: Maps public API to internal functions
- **Error Handling**: Converts internal errors to API error codes
- **Resource Management**: Manages cursors, transactions, and tuples

### 2. Transaction Management (`trx/`)
**Purpose**: ACID compliance and concurrency control  
**Key Files**: `trx0trx.c`, `trx0roll.c`, `trx0sys.c`

#### Components:
- **Transaction Control Block (TCB)**: Tracks transaction state
- **Undo Log Management**: Rollback functionality  
- **Read Views**: MVCC implementation
- **Transaction System**: Global transaction coordination

#### Key Concepts:
```c
typedef struct trx_struct {
    ulint           state;          // TRX_ACTIVE, TRX_COMMITTED, etc.
    ulint           isolation_level; // REPEATABLE_READ, etc.
    trx_id_t        id;             // Unique transaction ID
    read_view_t*    read_view;      // MVCC read view
    UT_LIST_BASE_NODE_T(lock_t) trx_locks; // Locks held
    // ... more fields
} trx_t;
```

### 3. Buffer Pool Management (`buf/`)
**Purpose**: Memory management and caching  
**Key Files**: `buf0buf.c`, `buf0lru.c`, `buf0flu.c`

#### Components:
- **Buffer Pool**: Main memory cache for database pages
- **LRU Manager**: Least Recently Used page replacement
- **Flusher**: Writes dirty pages to disk
- **Read-ahead**: Predictive page loading

#### Architecture:
```
Buffer Pool (Default: 32MB)
├── Hash Table (for fast page lookup)
├── LRU List (page replacement policy)
├── Free List (available buffer frames)
├── Flush List (dirty pages to be written)
└── Buffer Frames (16KB each)
```

### 4. B+ Tree Implementation (`btr/`)
**Purpose**: Index and data storage structure  
**Key Files**: `btr0btr.c`, `btr0cur.c`, `btr0pcur.c`

#### Tree Structure:
```
        Root Page (Level 2)
       /              \
   Internal Page    Internal Page (Level 1)
   /     |     \     /      |      \
 Leaf  Leaf   Leaf Leaf   Leaf    Leaf (Level 0)
 Page  Page   Page Page   Page    Page
```

#### Key Operations:
- **Search**: Efficient key lookup via tree traversal
- **Insert**: Maintains tree balance with page splits
- **Delete**: Handles page merges and tree rebalancing
- **Range Scans**: Sequential leaf page traversal

### 5. Row Operations (`row/`)
**Purpose**: Record-level operations  
**Key Files**: `row0ins.c`, `row0sel.c`, `row0upd.c`, `row0purge.c`

#### Components:
- **Insert Engine**: Adds new records to tables
- **Select Engine**: Retrieves records with WHERE conditions  
- **Update Engine**: Modifies existing records
- **Purge System**: Removes old MVCC versions

### 6. Lock Manager (`lock/`)
**Purpose**: Concurrency control via locking  
**Key Files**: `lock0lock.c`, `lock0iter.c`

#### Lock Types:
```c
// Table-level locks
IB_LOCK_IS    // Intention Shared
IB_LOCK_IX    // Intention Exclusive  
IB_LOCK_S     // Shared
IB_LOCK_X     // Exclusive

// Row-level locks  
LOCK_REC_S    // Shared record lock
LOCK_REC_X    // Exclusive record lock
LOCK_GAP      // Gap lock (between records)
LOCK_REC_NOT_GAP // Record lock without gap
```

#### Deadlock Detection:
- **Wait-for Graph**: Tracks lock dependencies
- **Cycle Detection**: Identifies deadlock situations
- **Victim Selection**: Chooses transaction to rollback

### 7. Write-Ahead Logging (`log/`)
**Purpose**: Durability and crash recovery  
**Key Files**: `log0log.c`, `log0recv.c`

#### Log Structure:
```
Log Files (ib_logfile0, ib_logfile1, ...)
├── Log Records (LSN-ordered)
├── Checkpoints (recovery points)
├── MLOG Records (mini-transaction logs)
└── Recovery Information
```

#### Recovery Process:
1. **Redo Phase**: Re-apply committed transactions
2. **Undo Phase**: Rollback uncommitted transactions  
3. **Purge Phase**: Clean up old record versions

### 8. Page Management (`page/`)
**Purpose**: 16KB page operations  
**Key Files**: `page0page.c`, `page0cur.c`, `page0zip.c`

#### Page Structure:
```
┌────────────────────────────────────────────────────────────┐
│                    Page Header (38 bytes)                  │
├────────────────────────────────────────────────────────────┤
│                   System Records                           │
├────────────────────────────────────────────────────────────┤
│                    User Records                            │
│                       (grows →)                           │
├────────────────────────────────────────────────────────────┤
│                    Free Space                              │
├────────────────────────────────────────────────────────────┤
│                  Page Directory                            │
│                     (← grows)                             │
├────────────────────────────────────────────────────────────┤
│                   Page Trailer (8 bytes)                  │
└────────────────────────────────────────────────────────────┘
```

### 9. Data Dictionary (`dict/`)
**Purpose**: Schema and metadata management  
**Key Files**: `dict0dict.c`, `dict0load.c`, `dict0mem.c`

#### Metadata Stored:
- **Table Definitions**: Column types, constraints
- **Index Definitions**: Key structure, statistics
- **Foreign Keys**: Referential integrity rules
- **Tablespace Information**: File mappings

### 10. Mini-Transactions (`mtr/`)
**Purpose**: Atomic operation units  
**Key Files**: `mtr0mtr.c`, `mtr0log.c`

#### MTR Lifecycle:
```c
mtr_t mtr;
mtr_start(&mtr);
// Perform atomic operations
mtr_commit(&mtr);  // Write log and release latches
```

### 11. File System Layer (`fil/` + `os/`)
**Purpose**: OS abstraction and file management  
**Key Files**: `fil0fil.c`, `os0file.c`

#### Responsibilities:
- **Tablespace Management**: Multiple file handling
- **File I/O**: Asynchronous and synchronous operations
- **Space Allocation**: Free space tracking
- **File Compression**: Optional data compression

## 🔄 Data Flow Example

### Insert Operation Flow:

```
1. API Call: ib_cursor_insert_row()
   ↓
2. Transaction: Check transaction state  
   ↓
3. Lock Manager: Acquire necessary locks
   ↓
4. Row Operations: Format record
   ↓  
5. B+ Tree: Find insertion point
   ↓
6. Buffer Pool: Load required pages
   ↓
7. Mini-Transaction: Start atomic operation
   ↓
8. Page Operations: Insert record into page
   ↓
9. Log Manager: Write WAL record
   ↓
10. Mini-Transaction: Commit changes
```

### Query Operation Flow:

```
1. API Call: ib_cursor_first() / ib_cursor_next()
   ↓
2. Transaction: Create read view (MVCC)
   ↓
3. B+ Tree: Navigate to record
   ↓
4. Buffer Pool: Fetch page (if not cached)
   ↓
5. Row Operations: Read and format record
   ↓
6. MVCC: Check record visibility
   ↓
7. Return data to application
```

## 🎯 Key Design Principles

### 1. **ACID Compliance**
- **Atomicity**: Mini-transactions ensure atomic operations
- **Consistency**: Constraints enforced by data dictionary
- **Isolation**: MVCC + locking provides transaction isolation
- **Durability**: Write-ahead logging ensures persistence

### 2. **Multi-Version Concurrency Control (MVCC)**
- Each transaction sees consistent snapshot
- No read locks needed for SELECT operations
- Old versions maintained for concurrent readers
- Purge process cleans up old versions

### 3. **Write-Ahead Logging (WAL)**
- Changes logged before data pages written
- Enables fast recovery after crashes
- Log files provide redo information
- Undo information stored in rollback segments

### 4. **Buffer Pool Optimization**
- Hot data kept in memory
- LRU algorithm for page replacement  
- Read-ahead for sequential access
- Dirty page flushing strategies

### 5. **B+ Tree Benefits**
- Logarithmic search time: O(log n)
- Sequential access via leaf links
- Excellent for range queries
- Self-balancing maintains performance

## 📊 Memory Layout

```
InnoDB Memory Usage:
├── Buffer Pool (32MB default)
│   ├── Data Pages (16KB each) 
│   ├── Index Pages (16KB each)
│   └── Hash Tables (page lookup)
├── Log Buffer (1MB default)
├── Data Dictionary Cache
├── Lock System Memory
└── Various Pools (query, transaction, etc.)
```

## 🔧 Configuration Parameters

Key parameters that affect architecture behavior:

```c
// Buffer pool size
innodb_buffer_pool_size = 32M

// Log file size  
innodb_log_file_size = 5M

// Log files in group
innodb_log_files_in_group = 2

// Page size (fixed at 16KB in this version)
UNIV_PAGE_SIZE = 16384

// Lock wait timeout
innodb_lock_wait_timeout = 50
```

---

This architecture represents the elegant simplicity of early InnoDB - a pure storage engine focused on ACID compliance, performance, and reliability without the complexity of later MySQL integration.