#!/usr/bin/env python3
"""
InnoDB Component Analysis Helper Script

This script helps you feed the extracted InnoDB components to an LLM for analysis.
You can use it to:
1. List all available components
2. Display a specific component
3. Get prompts for analyzing each component
"""

import os
import sys

COMPONENT_FILES = [
    ("00_COMPONENT_OVERVIEW.txt", "Overview and reading guide"),
    ("21_universal_header.txt", "Global definitions and constants"),
    ("01_page_management.txt", "16KB page structure - fundamental storage unit"),
    ("02_file_tablespace_management.txt", "File and tablespace organization"),
    ("03_buffer_pool.txt", "In-memory page caching"),
    ("05_record_data_formats.txt", "Record formats and data types"),
    ("04_btree_index.txt", "B+ tree implementation"),
    ("06_mini_transactions.txt", "Atomic operations (MTR)"),
    ("07_logging_recovery.txt", "Write-ahead logging and recovery"),
    ("08_transaction_system.txt", "ACID and MVCC implementation"),
    ("09_locking_system.txt", "Concurrency control"),
    ("10_row_operations.txt", "Insert, update, delete, select"),
    ("11_data_dictionary.txt", "Schema and metadata management"),
    ("12_query_processing.txt", "SQL-like query execution"),
    ("13_insert_buffer.txt", "Change buffering optimization"),
    ("14_adaptive_hash_index.txt", "Hash index optimization"),
    ("15_synchronization.txt", "Thread coordination primitives"),
    ("16_os_abstraction.txt", "Operating system abstraction"),
    ("17_memory_management.txt", "Custom memory allocators"),
    ("18_utilities.txt", "Common utility functions"),
    ("19_server_layer.txt", "Background threads and services"),
    ("20_api_layer.txt", "Public API for applications"),
]

ANALYSIS_PROMPTS = {
    "page_management": """Analyze this InnoDB page management code and explain:
1. The exact structure of a 16KB page (header, records, free space, directory, trailer)
2. How records are organized within a page
3. The page cursor mechanism for navigation
4. Different page types and their purposes
5. Key algorithms for space management within pages""",
    
    "buffer_pool": """Analyze this buffer pool implementation and explain:
1. How the buffer pool manages pages in memory
2. The LRU algorithm implementation and optimizations
3. The flush list and dirty page management
4. Page hash table for lookups
5. Read-ahead mechanisms
6. Performance considerations and tuning""",
    
    "btree": """Analyze this B+ tree implementation and explain:
1. The B+ tree structure and node types
2. Search, insert, and delete algorithms
3. Page split and merge operations
4. Pessimistic vs optimistic operations
5. Cursor types and their use cases
6. Performance characteristics""",
    
    "transaction": """Analyze this transaction system and explain:
1. How MVCC is implemented
2. Transaction lifecycle and states
3. Read view creation and visibility rules
4. Undo log structure and usage
5. Isolation level implementation
6. Commit and rollback processes""",
    
    "locking": """Analyze this locking system and explain:
1. Lock types and granularities
2. Lock compatibility matrix
3. Next-key locking algorithm
4. Deadlock detection mechanism
5. Lock wait handling
6. Performance implications""",
    
    "logging": """Analyze this logging system and explain:
1. Redo log structure and format
2. LSN (Log Sequence Number) mechanism
3. Checkpoint algorithm
4. Recovery process phases
5. Log buffer management
6. Durability guarantees""",
}

def list_components():
    """List all available components"""
    print("\n" + "="*60)
    print("INNODB COMPONENTS FOR ANALYSIS")
    print("="*60)
    for i, (filename, description) in enumerate(COMPONENT_FILES):
        size = os.path.getsize(filename) if os.path.exists(filename) else 0
        print(f"{i:2d}. {filename:35s} - {description}")
        if size > 0:
            print(f"    Size: {size:,} bytes ({size//1024}KB)")
    print("="*60)

def show_component(index):
    """Display a specific component file"""
    if 0 <= index < len(COMPONENT_FILES):
        filename = COMPONENT_FILES[index][0]
        if os.path.exists(filename):
            with open(filename, 'r') as f:
                content = f.read()
            print(f"\n{'='*60}")
            print(f"COMPONENT: {filename}")
            print(f"{'='*60}")
            print(content[:5000])  # Show first 5000 chars
            print(f"\n... (truncated, total size: {len(content):,} bytes)")
        else:
            print(f"File {filename} not found!")
    else:
        print("Invalid component index!")

def get_prompt(component_type):
    """Get analysis prompt for a component type"""
    return ANALYSIS_PROMPTS.get(component_type, 
           "Analyze this code and explain its key concepts, algorithms, and implementation details.")

def main():
    if len(sys.argv) < 2:
        print("\nUsage:")
        print("  python analyze_components.py list           - List all components")
        print("  python analyze_components.py show <index>   - Show component content")
        print("  python analyze_components.py prompt <type>  - Get analysis prompt")
        print("\nExample:")
        print("  python analyze_components.py list")
        print("  python analyze_components.py show 3")
        print("  python analyze_components.py prompt btree")
        return
    
    command = sys.argv[1]
    
    if command == "list":
        list_components()
    elif command == "show" and len(sys.argv) > 2:
        try:
            index = int(sys.argv[2])
            show_component(index)
        except ValueError:
            print("Please provide a valid component index!")
    elif command == "prompt" and len(sys.argv) > 2:
        prompt_type = sys.argv[2]
        print("\n" + "="*60)
        print(f"ANALYSIS PROMPT FOR: {prompt_type}")
        print("="*60)
        print(get_prompt(prompt_type))
    else:
        print("Invalid command!")

if __name__ == "__main__":
    main()