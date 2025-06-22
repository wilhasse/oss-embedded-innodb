# InnoDB Tablespace Storage Models

This document explains the two storage models available in OSS Embedded InnoDB 1.0.6.6750 and provides detailed source code locations for understanding the implementation.

## Table of Contents

- [Overview](#overview)
- [Default Storage Model: System Tablespace](#default-storage-model-system-tablespace)
- [Alternative Storage Model: Per-Table Tablespaces](#alternative-storage-model-per-table-tablespaces)
- [Configuration and Control](#configuration-and-control)
- [Source Code Locations](#source-code-locations)
- [Key Functions and Implementation](#key-functions-and-implementation)
- [File Formats and Structure](#file-formats-and-structure)
- [Comparison with Modern MySQL](#comparison-with-modern-mysql)

## Overview

Early InnoDB supported two distinct storage models, though only one was enabled by default:

1. **System Tablespace Model** (Default): All data in single `ibdata1` file
2. **Per-Table Tablespace Model** (Optional): Each table in individual `.ibd` files

The choice between these models is controlled by the `srv_file_per_table` configuration variable.

## Default Storage Model: System Tablespace

### Characteristics

- **Single File**: All user data stored in `ibdata1`
- **Default Setting**: `srv_file_per_table = FALSE`
- **Space ID**: All user tables use space ID 0
- **File Growth**: `ibdata1` grows as needed, never shrinks
- **System Tables**: Data dictionary always in system tablespace

### Advantages

- **Simplicity**: Single file to manage and backup
- **Performance**: No file descriptor limits
- **Atomicity**: All operations within one file
- **Cross-table Operations**: Efficient for operations spanning multiple tables

### File Structure

```
ibdata1 (System Tablespace - Space ID 0)
├── System Pages (0-7)
│   ├── File Space Header (Page 0)
│   ├── Insert Buffer Bitmap (Page 1)
│   ├── Segment Inode (Page 2)
│   ├── Insert Buffer Header (Page 3)
│   ├── Insert Buffer Root (Page 4)
│   ├── Transaction System (Page 5)
│   ├── Rollback Segment (Page 6)
│   └── Data Dictionary Header (Page 7)
├── Data Dictionary Tables
│   ├── SYS_TABLES
│   ├── SYS_COLUMNS
│   ├── SYS_INDEXES
│   └── SYS_FIELDS
├── User Table Data
│   ├── Table 1 (Clustered Index + Secondary Indexes)
│   ├── Table 2 (Clustered Index + Secondary Indexes)
│   └── ...
├── Undo Log Pages
├── Insert Buffer Pages
└── Free/Allocated Extents
```

## Alternative Storage Model: Per-Table Tablespaces

### Characteristics

- **Multiple Files**: Each table in separate `.ibd` file
- **Configuration**: `srv_file_per_table = TRUE`
- **Space ID**: Each table gets unique space ID > 0
- **File Format**: `{datadir}/{dbname}/{tablename}.ibd`
- **System Tables**: Still remain in `ibdata1`

### Advantages

- **Isolation**: Table data isolated in separate files
- **Portability**: Individual tables can be copied/moved
- **Space Reclamation**: Dropping table reclaims disk space
- **Backup Granularity**: Backup individual tables
- **Corruption Isolation**: Corruption limited to single table

### File Structure

```
Database Directory Structure:
├── ibdata1 (System Tablespace - Space ID 0)
│   ├── System Pages (0-7)
│   ├── Data Dictionary Tables
│   ├── Undo Log Pages
│   ├── Insert Buffer Pages
│   └── Transaction System Data
├── database1/
│   ├── table1.ibd (Space ID 1)
│   ├── table2.ibd (Space ID 2)
│   └── table3.ibd (Space ID 3)
└── database2/
    ├── table4.ibd (Space ID 4)
    └── table5.ibd (Space ID 5)

Each .ibd file (4 initial pages):
├── Page 0: File Space Header + Extent Descriptor
├── Page 1: Insert Buffer Bitmap
├── Page 2: Segment Inode Page
└── Page 3: Clustered Index Root Page
```

## Configuration and Control

### Runtime Configuration

```c
// Enable per-table tablespaces
ib_cfg_set_bool_on("file_per_table");

// Disable per-table tablespaces (default)
ib_cfg_set_bool_off("file_per_table");

// Check current setting
ib_bool_t file_per_table = ib_cfg_get_bool("file_per_table");
```

### Configuration Variable

- **Variable Name**: `srv_file_per_table`
- **Type**: `ib_bool_t` (boolean)
- **Default Value**: `FALSE`
- **Scope**: Global, affects all new table creation
- **API Name**: `"file_per_table"`

## Source Code Locations

### Core Configuration

| File | Location | Purpose |
|------|----------|---------|
| `include/srv0srv.h` | Line 82 | `srv_file_per_table` declaration |
| `srv/srv0srv.c` | Line 677 | Default value assignment (`FALSE`) |
| `api/api0cfg.c` | Multiple locations | Configuration API implementation |

### Table Creation Logic

| File | Function | Line | Purpose |
|------|----------|------|---------|
| `dict/dict0crea.c` | `dict_create_table_low()` | 253 | Main table creation with space assignment |
| `dict/dict0crea.c` | `dict_create_tablespace()` | 162 | Individual tablespace creation |
| `dict/dict0crea.c` | `dict_build_table_def()` | 421 | Table definition building |

### File Space Management

| File | Function | Purpose |
|------|----------|---------|
| `fil/fil0fil.c` | `fil_create_new_single_table_tablespace()` | Create individual `.ibd` files |
| `fil/fil0fil.c` | `fil_load_single_table_tablespaces()` | Load existing `.ibd` files at startup |
| `fil/fil0fil.c` | `fil_open_single_table_tablespace()` | Open individual tablespace files |
| `fil/fil0fil.c` | `fil_delete_tablespace()` | Delete `.ibd` files |
| `fil/fil0fil.c` | `fil_rename_tablespace()` | Rename `.ibd` files |

### Space ID Management

| File | Function | Purpose |
|------|----------|---------|
| `dict/dict0load.c` | `dict_check_tablespaces_and_store_max_id()` | Validate space IDs and find maximum |
| `dict/dict0boot.c` | `dict_boot()` | Initialize data dictionary |
| `fsp/fsp0fsp.c` | `fsp_header_init()` | Initialize tablespace headers |

### File System Operations

| File | Function | Purpose |
|------|----------|---------|
| `os/os0file.c` | `os_file_create_simple()` | Low-level file creation |
| `os/os0file.c` | `os_file_delete()` | Low-level file deletion |
| `os/os0file.c` | `os_file_rename()` | Low-level file renaming |

## Key Functions and Implementation

### Table Creation Decision Logic

```c
// From dict/dict0crea.c:253
if (srv_file_per_table) {
    /* We create a new single-table tablespace for the table.
       We initially let it be 4 pages:
       - page 0 is the fsp header and an extent descriptor page,
       - page 1 is an ibuf bitmap page,
       - page 2 is the first inode page,
       - page 3 will contain the root of the clustered index */
    
    space = dict_create_tablespace(table, id);
    
    if (space == ULINT_UNDEFINED) {
        return(DB_ERROR);
    }
    
    table->space = space;
} else {
    /* We use the system tablespace */
    table->space = 0;
}
```

### Tablespace Creation Function

```c
// From dict/dict0crea.c - dict_create_tablespace()
ulint dict_create_tablespace(
    dict_table_t* table,    /* in: table */
    ulint         id)       /* in: table id */
{
    char* path;
    ulint space;
    
    path = fil_make_ibd_name(table->name, FALSE);
    
    space = fil_create_new_single_table_tablespace(
        id, path, FIL_IBD_FILE_INITIAL_SIZE);
    
    mem_free(path);
    
    return(space);
}
```

### File Path Construction

```c
// From fil/fil0fil.c - fil_make_ibd_name()
char* fil_make_ibd_name(
    const char* name,       /* in: table name */
    ibool       is_temp)    /* in: TRUE if temp table */
{
    char* filename;
    ulint len;
    
    len = strlen(srv_data_home) + strlen(name) + 10;
    filename = mem_alloc(len);
    
    ut_snprintf(filename, len, "%s%s.ibd", 
                fil_normalize_path(srv_data_home), name);
    
    return(filename);
}
```

### Startup Tablespace Loading

```c
// From fil/fil0fil.c - fil_load_single_table_tablespaces()
void fil_load_single_table_tablespaces(void)
{
    char* dbpath;
    os_file_dir_t dir;
    os_file_stat_t fileinfo;
    
    /* Scan all database directories */
    dir = os_file_opendir(srv_data_home, FALSE);
    
    while (!os_file_readdir_next_file(srv_data_home, dir, 
                                      &fileinfo)) {
        if (fileinfo.type == OS_FILE_TYPE_DIR) {
            /* Found database directory */
            fil_load_single_table_tablespace_from_dir(
                fileinfo.name);
        }
    }
    
    os_file_closedir(dir);
}
```

## File Formats and Structure

### .ibd File Initial Layout

```c
// Constants from include/fil0fil.h
#define FIL_IBD_FILE_INITIAL_SIZE  4  /* 4 pages = 64KB initial size */

Initial .ibd file structure:
Page 0: File Space Header + Extent Descriptor
  - Space ID (unique per tablespace)
  - Space size and flags
  - Free extent lists
  - Segment inode lists

Page 1: Insert Buffer Bitmap
  - Tracks free space for insert buffer
  - 2 bits per page in the tablespace

Page 2: File Segment Inode Page
  - Segment descriptors for the table
  - Leaf segment (for data pages)
  - Non-leaf segment (for index internal pages)

Page 3: Clustered Index Root Page
  - Root page of the table's primary key index
  - Contains index header and initial records
```

### Space ID Assignment

```c
// From dict/dict0crea.c
ulint dict_hdr_get_new_id(
    ulint   type)  /* in: DICT_HDR_ROW_ID, DICT_HDR_TABLE_ID, ... */
{
    dulint  id;
    mtr_t   mtr;
    
    mtr_start(&mtr);
    
    id = mtr_read_dulint(dict_hdr + type, &mtr);
    id = ut_dulint_add(id, 1);
    mlog_write_dulint(dict_hdr + type, id, &mtr);
    
    mtr_commit(&mtr);
    
    return(ut_dulint_get_low(id));
}
```

## Comparison with Modern MySQL

| Aspect | Early InnoDB (1.0.6) | Modern MySQL InnoDB |
|--------|----------------------|---------------------|
| **Default Storage** | All in `ibdata1` | Each table in `.ibd` |
| **Configuration Variable** | `srv_file_per_table` | `innodb_file_per_table` |
| **Default Setting** | `FALSE` (system tablespace) | `ON` (per-table files) |
| **System Tables** | Always in `ibdata1` | Still in `ibdata1` |
| **Undo Logs** | In `ibdata1` only | Separate undo tablespaces available |
| **Temporary Tables** | In `ibdata1` | Separate temp tablespace |
| **Doublewrite Buffer** | In `ibdata1` | Can be separate file |
| **File Extension** | `.ibd` | `.ibd` (same) |
| **Initial File Size** | 4 pages (64KB) | Configurable, default 96KB |
| **Space ID Range** | 0 (system), >0 (per-table) | Same concept |
| **API Configuration** | `ib_cfg_set_bool_on()` | `SET GLOBAL innodb_file_per_table=ON` |

### Evolution Timeline

1. **Early InnoDB**: System tablespace only
2. **InnoDB 1.0.6** (this version): Per-table available but not default
3. **MySQL 5.6+**: Per-table becomes default
4. **MySQL 8.0+**: Enhanced with undo tablespaces, temp tablespaces

This early version represents the **transitional period** where per-table infrastructure existed but wasn't the default behavior, requiring explicit configuration to enable the file-per-table storage model that would later become standard.