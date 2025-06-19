# OSS Embedded InnoDB Documentation

Welcome to the documentation for OSS Embedded InnoDB - the early version of InnoDB from the Innobase era, before Oracle's acquisition.

## 📚 Documentation Index

- [Execution Tracing Guide](./execution-tracing.md) - Complete guide for tracing InnoDB execution paths
- [API Reference](./api-reference.md) - InnoDB API functions and their source code locations
- [Architecture Overview](./architecture.md) - Core components and their responsibilities  
- [Development Tools](./development-tools.md) - Tools and techniques for debugging and analysis
- [Source Code Mapping](./source-mapping.md) - Mapping of API calls to internal implementations

## 🏛️ Historical Context

This is **Embedded InnoDB 1.0.6.6750** - one of the early versions of InnoDB when it belonged to **Innobase Oy**, before being acquired by Oracle. This represents the **pre-MySQL integration era** of InnoDB, giving you a pure view of the storage engine without most of the optimizations and complexity that came later.

## 🎯 Why This Matters

- **Pure Database Engine**: Clean implementation without MySQL-specific optimizations
- **Educational Value**: Easier to understand core database concepts
- **ACID Fundamentals**: See how transactions, durability, and concurrency work at the lowest level
- **B+ Tree Implementation**: Study the classic index structure in its original form
- **Historical Significance**: Experience database technology from the early 2000s

## 🚀 Quick Start

1. **Build the project**:
   ```bash
   ./configure
   make
   cd tests && make
   ```

2. **Run basic test**:
   ```bash
   ./ib_test1
   ```

3. **Trace execution**:
   ```bash
   env LD_LIBRARY_PATH=../.libs ltrace -e "ib_*" .libs/ib_cursor
   ```

## 📖 Key Concepts

This InnoDB implementation demonstrates:

- **MVCC (Multi-Version Concurrency Control)**
- **Write-Ahead Logging (WAL)**
- **Buffer Pool Management**
- **B+ Tree Index Structures**
- **Page-based Storage**
- **Transaction Isolation Levels**
- **Row-level Locking**

---

*Documentation generated for the OSS Embedded InnoDB project by Claude Code*