# InnoDB Bulk Insert Performance Tests

This directory contains comprehensive bulk insert performance tests for the embedded InnoDB database engine, including comparison tools with MySQL 8.

## Overview

The bulk insert test suite demonstrates massive data insertion capabilities (millions of rows) and provides performance comparison between embedded InnoDB and traditional MySQL server deployments.

## Test Programs

### 1. Embedded InnoDB Tests

#### `ib_bulk_insert` - Multi-threaded Bulk Insert
High-performance multi-threaded bulk insert test for embedded InnoDB.

**Features:**
- Multi-threaded parallel insertion
- Configurable batch sizes for optimal performance
- Real-time performance monitoring
- Comprehensive statistics reporting
- Transaction batching with proper ACID compliance

**Usage:**
```bash
# Basic usage (1M rows, 10K batch size, 4 threads)
LD_LIBRARY_PATH=../.libs .libs/ib_bulk_insert 1000000 10000 4

# Quick test (10K rows)
LD_LIBRARY_PATH=../.libs .libs/ib_bulk_insert 10000 1000 2

# Massive test (10M rows, optimized batching)
LD_LIBRARY_PATH=../.libs .libs/ib_bulk_insert 10000000 50000 8
```

**Table Schema:**
```sql
CREATE TABLE massive_data (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(255) NOT NULL,
    score DOUBLE NOT NULL,
    created_at INT UNSIGNED NOT NULL,
    data_blob TEXT NOT NULL,
    PRIMARY KEY (id),
    INDEX idx_user_id (user_id)
)
```

#### `ib_simple_bulk` - Single-threaded Test
Simplified single-threaded version for debugging and baseline testing.

**Usage:**
```bash
LD_LIBRARY_PATH=../.libs .libs/ib_simple_bulk 100000 5000
```

### 2. MySQL 8 Comparison Tests

#### `mysql_bulk_insert` - MySQL Protocol Test
Equivalent functionality using MySQL 8 server via TCP/IP protocol.

**Setup:**
```bash
# Run the setup script (installs MySQL 8, creates database, compiles program)
./setup_mysql_comparison.sh

# Or manual setup:
sudo apt install mysql-server mysql-client libmysqlclient-dev
gcc -o mysql_bulk_insert mysql_bulk_insert.c -lperconaserverclient -lpthread -lm
```

**Usage:**
```bash
# Basic usage
./mysql_bulk_insert 100000 10000 4 localhost root [password]

# Using test database
./mysql_bulk_insert 1000000 25000 8 localhost root
```

## Wrapper Scripts

### `run_bulk_test.sh` - User-friendly Test Runner
Automated test execution with progress monitoring and cleanup.

**Features:**
- Automatic environment setup
- Progress tracking with ETA
- Performance statistics
- Automatic cleanup of test data
- Error handling and recovery

**Usage:**
```bash
# Run with default parameters
./run_bulk_test.sh

# Custom test (500K rows, 20K batch, 6 threads)
./run_bulk_test.sh 500000 20000 6
```

### `setup_mysql_comparison.sh` - MySQL Setup Automation
Complete MySQL 8 installation and configuration for performance testing.

**What it does:**
- Installs MySQL 8 server and client libraries
- Creates optimized database configuration
- Compiles MySQL comparison program
- Sets up test database and user accounts
- Creates performance comparison scripts

### `test_mysql_ready.sh` - MySQL Connection Validator
Validates MySQL setup and tests connectivity before running benchmarks.

**Usage:**
```bash
./test_mysql_ready.sh
```

### `run_comparison_test.sh` - Head-to-Head Performance Test
Direct performance comparison between embedded InnoDB and MySQL 8.

**Usage:**
```bash
# Quick comparison (100K rows)
./run_comparison_test.sh 100000 10000 4

# Large scale comparison (5M rows)
./run_comparison_test.sh 5000000 50000 8
```

## Performance Characteristics

### Typical Results

**Embedded InnoDB:**
- Throughput: 40,000-80,000 rows/sec
- Latency: Direct memory access, no network overhead
- CPU Usage: Lower due to no protocol parsing
- Memory: Direct buffer pool access

**MySQL 8 Server:**
- Throughput: 8,000-15,000 rows/sec
- Latency: TCP/IP + MySQL protocol overhead
- CPU Usage: Higher due to connection handling
- Memory: Additional protocol buffers

### Optimization Settings

**Embedded InnoDB:**
- Batch sizes: 10,000-50,000 rows per transaction
- Buffer pool: Direct access, no fragmentation
- Log files: Optimized for sequential writes

**MySQL 8:**
- `autocommit = 0` - Disable autocommit for batching
- `unique_checks = 0` - Skip unique constraint checks during bulk insert
- `foreign_key_checks = 0` - Skip foreign key validation
- `sql_log_bin = 0` - Disable binary logging for performance

## Test Data Generation

All tests generate realistic random data:

- **Names:** Random strings (10-50 characters)
- **Emails:** Realistic email addresses with various domains
- **Scores:** Random floating point values (0-100.00)
- **Timestamps:** Current Unix timestamp
- **Blob Data:** Variable length text data (100-500 characters)

## Compilation Requirements

**Embedded InnoDB:**
```bash
# Already included in main Makefile.am
make ib_bulk_insert ib_simple_bulk
```

**MySQL Comparison:**
```bash
# Requires MySQL client development libraries
sudo apt install libmysqlclient-dev
# or for Percona:
sudo apt install libperconaserverclient21-dev

# Compile
gcc -o mysql_bulk_insert mysql_bulk_insert.c -lperconaserverclient -lpthread -lm
```

## File Descriptions

| File | Purpose | Type |
|------|---------|------|
| `ib_bulk_insert.c` | Multi-threaded embedded InnoDB bulk insert | C Source |
| `ib_simple_bulk.c` | Single-threaded embedded InnoDB test | C Source |
| `mysql_bulk_insert.c` | MySQL 8 protocol bulk insert | C Source |
| `run_bulk_test.sh` | User-friendly test runner | Shell Script |
| `setup_mysql_comparison.sh` | MySQL 8 setup automation | Shell Script |
| `test_mysql_ready.sh` | MySQL connectivity validator | Shell Script |
| `run_comparison_test.sh` | Performance comparison runner | Shell Script |

## Performance Monitoring

All test programs provide detailed performance metrics:

- **Throughput:** Rows per second and MB per second
- **Wall Clock Time:** Total execution time
- **Thread Performance:** Per-thread statistics
- **Batch Statistics:** Average batch sizes and commit rates
- **Memory Usage:** Data size processed

## Troubleshooting

### Common Issues

1. **"Schema not locked" error:**
   - Fixed in current version with proper `ib_schema_lock_exclusive()`

2. **MySQL connection failures:**
   - Run `sudo mysql_secure_installation` to set up root password
   - Check MySQL service: `sudo systemctl status mysql`

3. **Compilation errors:**
   - Install development libraries: `sudo apt install libmysqlclient-dev`
   - For Percona: Use `-lperconaserverclient` instead of `-lmysqlclient`

4. **Performance issues:**
   - Adjust batch sizes (larger batches = better throughput)
   - Tune thread count based on CPU cores
   - Ensure sufficient disk space for data files

### Debug Mode

Enable verbose output:
```bash
# Embedded InnoDB with debug info
LD_LIBRARY_PATH=../.libs gdb .libs/ib_bulk_insert

# MySQL with connection debugging
./mysql_bulk_insert 1000 100 1 localhost root [password]
```

## Integration with Main Build System

The bulk insert tests are integrated into the main autotools build system via `Makefile.am`:

```makefile
noinst_PROGRAMS += ib_bulk_insert ib_simple_bulk
ib_bulk_insert_SOURCES = test0aux.c ib_bulk_insert.c  
ib_simple_bulk_SOURCES = test0aux.c ib_simple_bulk.c
```

Build with:
```bash
make ib_bulk_insert ib_simple_bulk
```

---

**Author:** Willian Hasse  
**Created:** 2024  
**Purpose:** Performance testing and benchmarking for embedded InnoDB database engine