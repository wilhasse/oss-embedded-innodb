# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is **OSS Embedded InnoDB 1.0.6.6750** - an early version of the InnoDB storage engine from the Innobase era, before Oracle's acquisition. This is a pure database engine implementation without MySQL-specific optimizations, making it excellent for educational purposes and understanding core database concepts.

## Build Commands

### Unix/Linux Build
```bash
# Configure and build the library
./configure
make

# Build with compression disabled
./configure --disable-compression
make

# Build tests
cd tests && make

# Clean build
make clean
```

### Running Tests
```bash
# Run correctness tests (from root directory)
make test

# Run stress tests (takes longer)
make test-stress

# Run individual test (from tests directory)
cd tests
LD_LIBRARY_PATH=../.libs ./ib_test1

# Clean test artifacts
make test-clean
```

### Tracing and Debugging
```bash
# Trace InnoDB API calls
cd tests
env LD_LIBRARY_PATH=../.libs ltrace -e "ib_*" .libs/ib_cursor

# Debug with GDB
cd tests
./debug_gdb.sh ib_test1
```

## Architecture Overview

The codebase follows a layered architecture:

1. **API Layer** (`api/`) - External interface with `ib_*` functions
2. **Transaction Management** (`trx/`) - ACID compliance and MVCC
3. **Buffer Pool** (`buf/`) - Memory management and page caching
4. **B+ Tree** (`btr/`) - Index structures and tree operations
5. **Row Operations** (`row/`) - Record-level operations
6. **Lock Manager** (`lock/`) - Concurrency control
7. **Log Management** (`log/`) - Write-ahead logging for durability
8. **Page Management** (`page/`) - 16KB page operations
9. **File System** (`fil/`, `os/`) - OS abstraction layer

### Key Data Flows

**Insert Operation**: API → Transaction → Lock → Row Format → B+ Tree → Buffer Pool → Mini-Transaction → Page → Log → Disk

**Query Operation**: API → Transaction (MVCC) → B+ Tree → Buffer Pool → Page → Row Format → Return

## Core Components

### Buffer Pool (`buf/`)
- Default size: 32MB
- 16KB pages
- LRU replacement policy
- Dirty page flushing

### B+ Tree (`btr/`)
- All indexes use B+ trees
- Leaf pages linked for range scans
- Self-balancing with splits/merges

### Transaction System (`trx/`)
- MVCC for read consistency
- Four isolation levels supported
- Undo logs for rollback
- Read views for snapshot isolation

### Mini-Transactions (`mtr/`)
- Atomic operation units
- Ensure consistency during page modifications
- Generate redo log records

### Log System (`log/`)
- Write-ahead logging (WAL)
- Checkpoint mechanism
- Recovery on startup

## Important Files and Patterns

### API Entry Points
All public API functions are in `api/api0api.c` and start with `ib_`. They map to internal implementations in various subsystems.

### Error Handling
- Internal errors use `ulint` type
- API errors use `ib_err_t` enum
- Error codes defined in `include/db0err.h`

### Memory Management
- Uses custom memory pools
- Debug builds include memory tracking
- Memory allocation through `mem0mem.c`

### Page Structure
- Fixed 16KB pages (UNIV_PAGE_SIZE)
- Page headers, user records, free space, directory
- Compressed pages optional (`page0zip.c`)

## Development Notes

### Adding New Features
1. Start with API function in `api/api0api.c`
2. Implement core logic in appropriate subsystem
3. Use mini-transactions for atomic operations
4. Ensure proper locking and MVCC handling
5. Add test case in `tests/` directory

### Testing Approach
- Unit tests are C programs in `tests/` directory
- Each test creates its own database files
- Clean test data between runs with `make test-clean`
- Use `ltrace` for API call tracing
- Use `gdb` with `debug_gdb.sh` for debugging

### Common Patterns
- Most operations wrapped in mini-transactions
- Buffer pool access through `buf_page_get()`
- Tree operations through `btr_cur_*` functions
- Row operations through `row_ins_*`, `row_sel_*`, etc.