#!/bin/bash

# InnoDB Component Extraction Script
# This script extracts and organizes InnoDB source code into logical components
# for deep analysis and understanding

SRC_DOCS="src-docs"
cd /home/cslog/oss-embedded-innodb

echo "Extracting InnoDB components for analysis..."

# 1. PAGE MANAGEMENT - The fundamental storage unit (16KB pages)
echo "Extracting Page Management component..."
cat > $SRC_DOCS/01_page_management.txt << 'EOF'
==================================================================
COMPONENT: PAGE MANAGEMENT
==================================================================
Pages are the fundamental unit of storage in InnoDB (16KB by default).
Understanding pages is crucial as everything else builds on this.

Key concepts:
- Page structure (header, records, free space, directory, trailer)
- Page types (index, undo, system, blob, etc.)
- Record formats within pages
- Page cursors for navigating records
- Page compression (optional)

Files included:
EOF
cat include/page0*.h include/page0*.ic >> $SRC_DOCS/01_page_management.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/01_page_management.txt
cat page/*.c >> $SRC_DOCS/01_page_management.txt

# 2. FILE AND TABLESPACE MANAGEMENT - How pages are organized into files
echo "Extracting File and Tablespace Management component..."
cat > $SRC_DOCS/02_file_tablespace_management.txt << 'EOF'
==================================================================
COMPONENT: FILE AND TABLESPACE MANAGEMENT
==================================================================
Tablespaces organize pages into files. This layer manages the physical
storage and provides abstraction over the operating system.

Key concepts:
- Tablespace structure (system tablespace, file-per-table)
- Space management (segments, extents, pages)
- File I/O operations
- Space allocation and deallocation

Files included:
EOF
cat include/fil0fil.h include/fsp0*.h include/fsp0*.ic include/fut0*.h include/fut0*.ic >> $SRC_DOCS/02_file_tablespace_management.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/02_file_tablespace_management.txt
cat fil/*.c fsp/*.c fut/*.c >> $SRC_DOCS/02_file_tablespace_management.txt

# 3. BUFFER POOL - In-memory page caching
echo "Extracting Buffer Pool component..."
cat > $SRC_DOCS/03_buffer_pool.txt << 'EOF'
==================================================================
COMPONENT: BUFFER POOL
==================================================================
The buffer pool is InnoDB's cache for database pages in memory.
It's critical for performance and manages the lifecycle of pages in RAM.

Key concepts:
- Buffer pool structure and initialization
- LRU list for page replacement
- Flush list for dirty pages
- Page hash for fast lookups
- Read-ahead mechanisms
- Buddy allocator for compressed pages

Files included:
EOF
cat include/buf0*.h include/buf0*.ic >> $SRC_DOCS/03_buffer_pool.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/03_buffer_pool.txt
cat buf/*.c >> $SRC_DOCS/03_buffer_pool.txt

# 4. B+ TREE IMPLEMENTATION - The core index structure
echo "Extracting B+ Tree component..."
cat > $SRC_DOCS/04_btree_index.txt << 'EOF'
==================================================================
COMPONENT: B+ TREE INDEXES
==================================================================
B+ trees are the fundamental index structure in InnoDB.
All data is stored in B+ tree indexes (clustered and secondary).

Key concepts:
- B+ tree structure (internal nodes, leaf nodes)
- Tree operations (search, insert, delete)
- Page splits and merges
- Pessimistic and optimistic operations
- Persistent and positioned cursors
- Adaptive hash index

Files included:
EOF
cat include/btr0*.h include/btr0*.ic >> $SRC_DOCS/04_btree_index.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/04_btree_index.txt
cat btr/*.c >> $SRC_DOCS/04_btree_index.txt

# 5. RECORD AND DATA FORMATS - How data is structured
echo "Extracting Record and Data Formats component..."
cat > $SRC_DOCS/05_record_data_formats.txt << 'EOF'
==================================================================
COMPONENT: RECORD AND DATA FORMATS
==================================================================
This component defines how records are formatted and compared,
and how different data types are handled.

Key concepts:
- Record formats (compact, redundant)
- Data type definitions
- Record comparison
- External/overflow storage for large fields

Files included:
EOF
cat include/rem0*.h include/rem0*.ic include/data0*.h include/data0*.ic include/row0ext.* >> $SRC_DOCS/05_record_data_formats.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/05_record_data_formats.txt
cat rem/*.c data/*.c row/row0ext.c >> $SRC_DOCS/05_record_data_formats.txt

# 6. MINI-TRANSACTIONS - Atomic operations
echo "Extracting Mini-transactions component..."
cat > $SRC_DOCS/06_mini_transactions.txt << 'EOF'
==================================================================
COMPONENT: MINI-TRANSACTIONS (MTR)
==================================================================
Mini-transactions ensure atomicity of operations that modify pages.
They are the foundation for maintaining consistency.

Key concepts:
- MTR lifecycle (start, operations, commit)
- Redo log generation
- Latching protocol
- Ensuring atomicity of page modifications

Files included:
EOF
cat include/mtr0*.h include/mtr0*.ic >> $SRC_DOCS/06_mini_transactions.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/06_mini_transactions.txt
cat mtr/*.c >> $SRC_DOCS/06_mini_transactions.txt

# 7. LOGGING AND RECOVERY - Write-ahead logging
echo "Extracting Logging and Recovery component..."
cat > $SRC_DOCS/07_logging_recovery.txt << 'EOF'
==================================================================
COMPONENT: LOGGING AND RECOVERY
==================================================================
The log system implements write-ahead logging (WAL) for durability
and crash recovery. This is essential for the 'D' in ACID.

Key concepts:
- Redo log structure and format
- Log sequence numbers (LSN)
- Checkpointing mechanism
- Recovery process (analysis, redo, undo phases)
- Log buffer and flushing

Files included:
EOF
cat include/log0*.h include/log0*.ic >> $SRC_DOCS/07_logging_recovery.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/07_logging_recovery.txt
cat log/*.c >> $SRC_DOCS/07_logging_recovery.txt

# 8. TRANSACTION SYSTEM - ACID and MVCC
echo "Extracting Transaction System component..."
cat > $SRC_DOCS/08_transaction_system.txt << 'EOF'
==================================================================
COMPONENT: TRANSACTION SYSTEM
==================================================================
The transaction system implements ACID properties and MVCC
(Multi-Version Concurrency Control) for isolation.

Key concepts:
- Transaction lifecycle
- Transaction IDs and visibility
- Read views for MVCC
- Undo logs and rollback segments
- Transaction isolation levels
- Commit and rollback operations
- Purge system for old versions

Files included:
EOF
cat include/trx0*.h include/trx0*.ic include/read0*.h include/read0*.ic >> $SRC_DOCS/08_transaction_system.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/08_transaction_system.txt
cat trx/*.c read/*.c >> $SRC_DOCS/08_transaction_system.txt

# 9. LOCKING SYSTEM - Concurrency control
echo "Extracting Locking System component..."
cat > $SRC_DOCS/09_locking_system.txt << 'EOF'
==================================================================
COMPONENT: LOCKING SYSTEM
==================================================================
The lock manager implements row-level and table-level locking
for concurrency control and isolation.

Key concepts:
- Lock types (shared, exclusive, intention)
- Lock granularity (table, page, record, gap)
- Lock compatibility matrix
- Deadlock detection
- Lock wait management
- Next-key locking for phantom prevention

Files included:
EOF
cat include/lock0*.h include/lock0*.ic >> $SRC_DOCS/09_locking_system.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/09_locking_system.txt
cat lock/*.c >> $SRC_DOCS/09_locking_system.txt

# 10. ROW OPERATIONS - High-level data manipulation
echo "Extracting Row Operations component..."
cat > $SRC_DOCS/10_row_operations.txt << 'EOF'
==================================================================
COMPONENT: ROW OPERATIONS
==================================================================
This layer implements high-level operations on rows: insert,
update, delete, and select. It coordinates lower layers.

Key concepts:
- Insert operations and duplicate handling
- Update operations and triggers
- Delete operations and marking
- Select operations and cursors
- Purge of old MVCC versions
- Undo operations for rollback

Files included:
EOF
cat include/row0*.h include/row0*.ic >> $SRC_DOCS/10_row_operations.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/10_row_operations.txt
cat row/*.c >> $SRC_DOCS/10_row_operations.txt

# 11. DATA DICTIONARY - Schema and metadata
echo "Extracting Data Dictionary component..."
cat > $SRC_DOCS/11_data_dictionary.txt << 'EOF'
==================================================================
COMPONENT: DATA DICTIONARY
==================================================================
The data dictionary stores and manages metadata about tables,
indexes, columns, and other database objects.

Key concepts:
- System tables for metadata
- Table and index definitions
- Column information
- Foreign key constraints
- Dictionary cache for performance
- Bootstrap process

Files included:
EOF
cat include/dict0*.h include/dict0*.ic >> $SRC_DOCS/11_data_dictionary.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/11_data_dictionary.txt
cat dict/*.c >> $SRC_DOCS/11_data_dictionary.txt

# 12. QUERY PROCESSING - SQL-like execution
echo "Extracting Query Processing component..."
cat > $SRC_DOCS/12_query_processing.txt << 'EOF'
==================================================================
COMPONENT: QUERY PROCESSING
==================================================================
The query processing layer includes parsing, optimization,
and execution of SQL-like commands.

Key concepts:
- SQL parser (Bison/Yacc based)
- Query graphs and execution plans
- Query optimization
- Stored procedures
- Query execution engine

Files included:
EOF
cat include/pars0*.h include/pars0*.ic include/que0*.h include/que0*.ic include/eval0*.h include/eval0*.ic >> $SRC_DOCS/12_query_processing.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/12_query_processing.txt
cat pars/*.c que/*.c eval/*.c >> $SRC_DOCS/12_query_processing.txt

# 13. INSERT BUFFER - Change buffering
echo "Extracting Insert Buffer component..."
cat > $SRC_DOCS/13_insert_buffer.txt << 'EOF'
==================================================================
COMPONENT: INSERT BUFFER
==================================================================
The insert buffer (change buffer) optimizes modifications to
secondary indexes when the affected pages are not in memory.

Key concepts:
- Buffering secondary index changes
- Merging buffered changes
- Insert buffer bitmap
- Performance optimization for I/O

Files included:
EOF
cat include/ibuf0*.h include/ibuf0*.ic >> $SRC_DOCS/13_insert_buffer.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/13_insert_buffer.txt
cat ibuf/*.c >> $SRC_DOCS/13_insert_buffer.txt

# 14. ADAPTIVE HASH INDEX - Performance optimization
echo "Extracting Adaptive Hash Index component..."
cat > $SRC_DOCS/14_adaptive_hash_index.txt << 'EOF'
==================================================================
COMPONENT: ADAPTIVE HASH INDEX
==================================================================
The adaptive hash index automatically creates hash indexes
for frequently accessed pages to speed up lookups.

Key concepts:
- Hash table structure
- Automatic index creation
- Hash index maintenance
- Performance monitoring

Files included:
EOF
cat include/ha0*.h include/ha0*.ic include/hash0*.h include/hash0*.ic include/btr0sea.* >> $SRC_DOCS/14_adaptive_hash_index.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/14_adaptive_hash_index.txt
cat ha/*.c btr/btr0sea.c >> $SRC_DOCS/14_adaptive_hash_index.txt

# 15. SYNCHRONIZATION PRIMITIVES - Thread coordination
echo "Extracting Synchronization Primitives component..."
cat > $SRC_DOCS/15_synchronization.txt << 'EOF'
==================================================================
COMPONENT: SYNCHRONIZATION PRIMITIVES
==================================================================
Low-level synchronization mechanisms for thread coordination
and protecting shared data structures.

Key concepts:
- Mutexes and latches
- Read-write locks
- Sync arrays for wait management
- Atomic operations
- Thread local storage

Files included:
EOF
cat include/sync0*.h include/sync0*.ic include/thr0*.h include/thr0*.ic >> $SRC_DOCS/15_synchronization.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/15_synchronization.txt
cat sync/*.c thr/*.c >> $SRC_DOCS/15_synchronization.txt

# 16. OS ABSTRACTION LAYER - Platform independence
echo "Extracting OS Abstraction Layer component..."
cat > $SRC_DOCS/16_os_abstraction.txt << 'EOF'
==================================================================
COMPONENT: OS ABSTRACTION LAYER
==================================================================
Abstracts operating system specific functionality to provide
platform independence.

Key concepts:
- File I/O operations
- Thread management
- Process management
- Memory allocation
- Synchronization primitives

Files included:
EOF
cat include/os0*.h include/os0*.ic >> $SRC_DOCS/16_os_abstraction.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/16_os_abstraction.txt
cat os/*.c >> $SRC_DOCS/16_os_abstraction.txt

# 17. MEMORY MANAGEMENT - Custom allocators
echo "Extracting Memory Management component..."
cat > $SRC_DOCS/17_memory_management.txt << 'EOF'
==================================================================
COMPONENT: MEMORY MANAGEMENT
==================================================================
Custom memory management with pools and heaps for efficient
allocation and debugging support.

Key concepts:
- Memory heaps
- Memory pools
- Debug support for leak detection
- Custom allocators

Files included:
EOF
cat include/mem0*.h include/mem0*.ic >> $SRC_DOCS/17_memory_management.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/17_memory_management.txt
cat mem/*.c >> $SRC_DOCS/17_memory_management.txt

# 18. UTILITIES - Common utilities and helpers
echo "Extracting Utilities component..."
cat > $SRC_DOCS/18_utilities.txt << 'EOF'
==================================================================
COMPONENT: UTILITIES
==================================================================
Common utility functions and data structures used throughout
the codebase.

Key concepts:
- Byte operations
- Random number generation
- Vector operations
- List structures
- Debug utilities
- Machine-dependent data

Files included:
EOF
cat include/ut0*.h include/ut0*.ic include/mach0*.h include/mach0*.ic include/dyn0*.h include/dyn0*.ic >> $SRC_DOCS/18_utilities.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/18_utilities.txt
cat ut/*.c mach/*.c dyn/*.c >> $SRC_DOCS/18_utilities.txt

# 19. SERVER LAYER - Service threads and management
echo "Extracting Server Layer component..."
cat > $SRC_DOCS/19_server_layer.txt << 'EOF'
==================================================================
COMPONENT: SERVER LAYER
==================================================================
Server infrastructure including background threads, startup,
and service management.

Key concepts:
- Server startup and shutdown
- Background threads (master, purge, etc.)
- Thread pool management
- Server configuration

Files included:
EOF
cat include/srv0*.h include/srv0*.ic include/usr0*.h include/usr0*.ic >> $SRC_DOCS/19_server_layer.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/19_server_layer.txt
cat srv/*.c usr/*.c >> $SRC_DOCS/19_server_layer.txt

# 20. API LAYER - External interface
echo "Extracting API Layer component..."
cat > $SRC_DOCS/20_api_layer.txt << 'EOF'
==================================================================
COMPONENT: API LAYER
==================================================================
The public API that applications use to interact with InnoDB.
This is the entry point for all operations.

Key concepts:
- Database operations (create, open, close)
- Transaction management
- Cursor operations
- Tuple manipulation
- Configuration management
- Error handling

Files included:
EOF
cat include/api0*.h include/db0err.h include/ddl0*.h >> $SRC_DOCS/20_api_layer.txt
echo -e "\n\n=== IMPLEMENTATION ===\n" >> $SRC_DOCS/20_api_layer.txt
cat api/*.c ddl/*.c >> $SRC_DOCS/20_api_layer.txt

# 21. UNIVERSAL HEADER - Global definitions
echo "Extracting Universal Header..."
cat > $SRC_DOCS/21_universal_header.txt << 'EOF'
==================================================================
COMPONENT: UNIVERSAL HEADER (univ.i)
==================================================================
The universal header contains global definitions, constants,
and macros used throughout InnoDB.

Key concepts:
- Global constants (page size, etc.)
- Debug macros
- Compiler directives
- Version information
- Basic type definitions

Files included:
EOF
cat include/univ.i include/ib0config.h >> $SRC_DOCS/21_universal_header.txt

# Create overview document
echo "Creating component overview..."
cat > $SRC_DOCS/00_COMPONENT_OVERVIEW.txt << 'EOF'
==================================================================
INNODB COMPONENT ANALYSIS GUIDE
==================================================================

This collection contains the complete source code of InnoDB organized
into logical components for deep analysis. Each file represents a major
conceptual component of the storage engine.

RECOMMENDED READING ORDER:
---------------------------

FOUNDATION CONCEPTS (Start Here):
1. 21_universal_header.txt - Global definitions and constants
2. 01_page_management.txt - The 16KB page structure (fundamental unit)
3. 02_file_tablespace_management.txt - How pages are organized in files
4. 03_buffer_pool.txt - In-memory page caching

CORE DATA STRUCTURES:
5. 05_record_data_formats.txt - How records are formatted
6. 04_btree_index.txt - B+ tree implementation (all data stored here)
7. 13_insert_buffer.txt - Change buffering optimization
8. 14_adaptive_hash_index.txt - Hash index optimization

TRANSACTION AND CONCURRENCY:
9. 06_mini_transactions.txt - Atomic operations
10. 07_logging_recovery.txt - Write-ahead logging (WAL)
11. 08_transaction_system.txt - ACID and MVCC implementation
12. 09_locking_system.txt - Concurrency control

HIGH-LEVEL OPERATIONS:
13. 10_row_operations.txt - Insert, update, delete, select
14. 11_data_dictionary.txt - Schema and metadata
15. 12_query_processing.txt - SQL-like query execution

INFRASTRUCTURE:
16. 15_synchronization.txt - Thread coordination
17. 16_os_abstraction.txt - Platform independence
18. 17_memory_management.txt - Custom memory allocators
19. 18_utilities.txt - Common utilities
20. 19_server_layer.txt - Background threads and services

EXTERNAL INTERFACE:
21. 20_api_layer.txt - Public API for applications

KEY CONCEPTS TO UNDERSTAND:
----------------------------

1. PAGE-BASED ARCHITECTURE:
   - Everything is stored in 16KB pages
   - Pages contain records in specific formats
   - Pages are organized into B+ trees

2. BUFFER POOL:
   - Acts as cache between disk and memory
   - Uses LRU for page replacement
   - Manages dirty page flushing

3. B+ TREE INDEXES:
   - All data stored in B+ tree indexes
   - Clustered index contains actual data
   - Secondary indexes point to clustered index

4. MVCC (Multi-Version Concurrency Control):
   - Each transaction sees consistent snapshot
   - Old versions kept for concurrent readers
   - Purge process cleans old versions

5. WRITE-AHEAD LOGGING:
   - Changes logged before written to data files
   - Enables crash recovery
   - Uses redo and undo logs

6. MINI-TRANSACTIONS:
   - Ensure atomicity of page modifications
   - Generate redo log records
   - Hold latches during operation

7. LOCKING:
   - Row-level locking for concurrency
   - Next-key locking prevents phantoms
   - Deadlock detection and resolution

8. TRANSACTION ISOLATION:
   - Four isolation levels supported
   - Read views for consistent reads
   - Undo logs for rollback

ANALYSIS TIPS:
--------------

1. Start with page structure to understand physical layout
2. Study B+ trees to understand logical organization
3. Examine buffer pool for memory management
4. Review transaction system for ACID properties
5. Analyze locking for concurrency understanding
6. Check logging for durability mechanisms
7. Look at API layer for usage patterns

Each component file contains both header files (interface) and
implementation files (actual code). Headers are included first
for understanding the design, followed by implementation details.

DEPENDENCIES:
-------------
Most components depend on:
- univ.i (universal definitions)
- Page management
- Memory management
- OS abstraction
- Synchronization primitives

Higher-level components build on lower-level ones:
- Row operations use B+ trees
- B+ trees use pages
- Pages use buffer pool
- Buffer pool uses OS file I/O

Understanding these dependencies helps in comprehending
the overall architecture.
EOF

echo "Component extraction complete!"
echo "Files created in $SRC_DOCS:"
ls -la $SRC_DOCS/*.txt | awk '{print "  - " $9 " (" $5 " bytes)"}'
echo ""
echo "Total size: $(du -sh $SRC_DOCS | cut -f1)"
echo ""
echo "Start with 00_COMPONENT_OVERVIEW.txt for guidance on analysis order."