# InnoDB Execution Tracing Guide

This guide provides comprehensive methods for tracing and understanding the execution path of InnoDB operations.

## 🔍 Overview

Understanding how InnoDB executes operations internally is crucial for:
- Learning database engine fundamentals
- Debugging performance issues
- Understanding ACID transaction implementation
- Studying B+ tree and buffer pool mechanics

## 🛠️ Required Tools

Before starting, ensure you have these debugging tools installed:

```bash
sudo apt install -y gdb ltrace strace build-essential
```

## 📊 Execution Tracing Methods

### 1. ltrace - Library Function Call Tracing

**Best for**: Seeing the exact order of InnoDB API function calls

```bash
# Basic API tracing
env LD_LIBRARY_PATH=../.libs ltrace -e "ib_*" .libs/ib_cursor

# Full library call tracing
env LD_LIBRARY_PATH=../.libs ltrace .libs/ib_cursor

# Save trace to file
env LD_LIBRARY_PATH=../.libs ltrace -e "ib_*" -o trace.log .libs/ib_cursor
```

**Example Output**:
```
ib_cursor->ib_init(0x7f43deac1a10, 1, 1, 4096) = 10
ib_cursor->ib_startup(0x55b114b278aa, 1, 1, 3072) = 10
ib_cursor->ib_database_create(0x55b114b278ce, 1, 1, 3072) = 1
ib_cursor->ib_trx_begin(3, 0, 0) = 0x55b116792c70
```

### 2. strace - System Call Tracing

**Best for**: Understanding file operations, memory allocation, and OS interactions

```bash
# Trace file operations
env LD_LIBRARY_PATH=../.libs strace -e trace=file .libs/ib_cursor

# Trace file I/O and synchronization
env LD_LIBRARY_PATH=../.libs strace -e trace=file,write,fsync,fdatasync .libs/ib_cursor

# Trace memory operations
env LD_LIBRARY_PATH=../.libs strace -e trace=mmap,munmap,brk .libs/ib_cursor
```

**Example Output**:
```
openat(AT_FDCWD, "./ibdata1", O_RDWR|O_CREAT, 0660) = 3
pwrite64(3, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"..., 16384, 0) = 16384
openat(AT_FDCWD, "log/ib_logfile0", O_RDWR|O_CREAT, 0660) = 4
```

### 3. GDB - Interactive Debugging

**Best for**: Step-by-step code analysis and call stack inspection

```bash
# Start GDB session
env LD_LIBRARY_PATH=../.libs gdb .libs/ib_cursor

# GDB commands for tracing
(gdb) break ib_init
(gdb) break ib_startup
(gdb) break ib_trx_begin
(gdb) run
(gdb) bt        # Show call stack
(gdb) step      # Step through code
(gdb) continue  # Continue to next breakpoint
```

**Automated GDB Tracing Script**:
```bash
# Create GDB command file
cat > gdb_trace.txt << 'EOF'
break ib_init
break ib_startup
break ib_trx_begin
break ib_cursor_open_table
break ib_cursor_insert_row
run
EOF

# Run automated tracing
env LD_LIBRARY_PATH=../.libs gdb -batch -x gdb_trace.txt .libs/ib_cursor
```

## 📋 API Call Execution Order

Based on the `ib_cursor` test, here's the typical execution sequence:

| Order | Function | Location | Purpose |
|-------|----------|----------|---------|
| 1 | `ib_init()` | `api/api0api.c:726` | Initialize memory and configuration |
| 2 | `ib_startup()` | `api/api0api.c:749` | Start storage engine |
| 3 | `ib_database_create()` | `api/api0api.c` | Create database directory |
| 4 | `ib_table_create()` | `api/api0api.c` | Create table schema |
| 5 | `ib_trx_begin()` | `api/api0api.c` | Start transaction |
| 6 | `ib_cursor_open_table()` | `api/api0api.c` | Open table cursor |
| 7 | `ib_cursor_lock()` | `api/api0api.c` | Acquire table lock |
| 8 | `ib_cursor_insert_row()` | `api/api0api.c` | Insert data |
| 9 | `ib_cursor_first()` | `api/api0api.c` | Position cursor |
| 10 | `ib_cursor_next()` | `api/api0api.c` | Iterate records |
| 11 | `ib_cursor_close()` | `api/api0api.c` | Close cursor |
| 12 | `ib_trx_commit()` | `api/api0api.c` | Commit transaction |
| 13 | `ib_shutdown()` | `api/api0api.c` | Shutdown engine |

## 🔬 Advanced Tracing Techniques

### Source Code Instrumentation

Add debug prints directly to the source code:

```c
// Edit api/api0api.c
ib_err_t ib_init(void) {
    fprintf(stderr, "=== ib_init() START ===\n");
    
    ut_mem_init();
    fprintf(stderr, "Memory subsystem initialized\n");
    
    ib_err_t ib_err = ib_cfg_init();
    fprintf(stderr, "Configuration initialized: %d\n", ib_err);
    
    fprintf(stderr, "=== ib_init() END ===\n");
    return ib_err;
}
```

Then rebuild:
```bash
make clean && make
```

### Function Call Mapping

Use `nm` and `objdump` to analyze symbols:

```bash
# List all InnoDB API functions
nm -D ../.libs/libinnodb.so | grep " ib_"

# Disassemble specific function
objdump -d ../.libs/libinnodb.so | grep -A 20 "ib_init"

# Show function addresses
readelf -s ../.libs/libinnodb.so | grep ib_
```

### Call Graph Generation

Create a visual call graph:

```bash
# Install call graph tools
sudo apt install -y graphviz

# Generate call graph (requires source modification)
cflow api/api0api.c > callgraph.txt
```

## 🎯 Practical Debugging Scenarios

### Scenario 1: Trace Transaction Lifecycle

```bash
# Focus on transaction-related calls
env LD_LIBRARY_PATH=../.libs ltrace -e "ib_trx_*" .libs/ib_cursor
```

### Scenario 2: Analyze Buffer Pool Operations

```bash
# Trace buffer management
env LD_LIBRARY_PATH=../.libs strace -e trace=mmap,madvise .libs/ib_cursor
```

### Scenario 3: Debug Table Operations

```bash
# GDB session focused on table operations
(gdb) break ib_table_create
(gdb) break ib_cursor_open_table
(gdb) run
```

## 📈 Performance Analysis

### Timing Function Calls

```bash
# Time each operation
env LD_LIBRARY_PATH=../.libs strace -T .libs/ib_cursor
```

### Memory Usage Tracking

```bash
# Monitor memory allocation
env LD_LIBRARY_PATH=../.libs strace -e trace=brk,mmap,munmap .libs/ib_cursor
```

## ⚠️ Common Issues and Solutions

### Library Path Issues
```bash
# Always set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/home/cslog/oss-embedded-innodb/.libs:$LD_LIBRARY_PATH
```

### Debug Symbols Missing
```bash
# Rebuild with debug symbols
make clean
CFLAGS="-g -O0 -DDEBUG" make
```

### GDB Not Finding Functions
```bash
# Use pending breakpoints
(gdb) set breakpoint pending on
(gdb) break ib_init
```

## 🧰 Useful Scripts

### Quick Trace Script
```bash
#!/bin/bash
export LD_LIBRARY_PATH=../.libs:$LD_LIBRARY_PATH
echo "=== API Call Trace ==="
ltrace -e "ib_*" .libs/ib_cursor 2>&1 | head -20
echo -e "\n=== File Operations ==="
strace -e trace=file .libs/ib_cursor 2>&1 | grep -E "(open|create|write)" | head -10
```

### Function Finder Script
```bash
#!/bin/bash
FUNC="$1"
echo "Finding implementation of: $FUNC"
grep -rn "^$FUNC" ../api/ --include="*.c"
grep -rn "$FUNC(" ../include/ --include="*.h" | head -3
```

---

This tracing methodology gives you deep insights into how the early InnoDB storage engine operates at the most fundamental level.