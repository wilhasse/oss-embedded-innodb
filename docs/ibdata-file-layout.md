# InnoDB ibdata File Layout and Structure

This document provides a detailed technical analysis of the InnoDB ibdata file format based on the OSS Embedded InnoDB source code (version 1.0.6.6750).

## Table of Contents

- [Overview](#overview)
- [Page Fundamentals](#page-fundamentals)
- [Common Page Header](#common-page-header)
- [System Pages](#system-pages)
- [B+ Tree Index Pages](#b-tree-index-pages)
- [Record Storage](#record-storage)
- [Page Directory](#page-directory)
- [Extent Management](#extent-management)
- [Space Management Hierarchy](#space-management-hierarchy)
- [Key Constants](#key-constants)
- [Page Types Reference](#page-types-reference)
- [Source Code References](#source-code-references)

## Overview

The InnoDB ibdata file is the main tablespace file that contains all table data, indexes, and system metadata. It's organized as a collection of fixed-size pages with sophisticated space management through extents and segments.

## Page Fundamentals

- **Page Size**: 16KB (16,384 bytes) - `UNIV_PAGE_SIZE`
- **Extent Size**: 64 pages = 1MB - `FSP_EXTENT_SIZE`
- **Address Format**: 6 bytes (4-byte page number + 2-byte offset within page)
- **File Growth**: Grows in 1MB increments (extent by extent)

## Common Page Header

Every page in the ibdata file starts with a 38-byte header defined in `fil0fil.h`:

```
┌─────────────────────────────────────────────────────────────────┐
│                    Common Page Header (38 bytes)                │
├────────┬────────┬─────────────────────────────────────────────────┤
│ Offset │  Size  │ Field                                           │
├────────┼────────┼─────────────────────────────────────────────────┤
│   0    │   4    │ Space ID or Checksum (FIL_PAGE_SPACE_OR_CHKSUM)│
│   4    │   4    │ Page Number (FIL_PAGE_OFFSET)                   │
│   8    │   4    │ Previous Page Number (FIL_PAGE_PREV)            │
│  12    │   4    │ Next Page Number (FIL_PAGE_NEXT)                │
│  16    │   8    │ Log Sequence Number - LSN (FIL_PAGE_LSN)        │
│  24    │   2    │ Page Type (FIL_PAGE_TYPE)                       │
│  26    │   8    │ File Flush LSN (FIL_PAGE_FILE_FLUSH_LSN)        │
│  34    │   4    │ Space ID (FIL_PAGE_ARCH_LOG_NO_OR_SPACE_ID)     │
├────────┴────────┴─────────────────────────────────────────────────┤
│  38    │   -    │ Page Data Begins Here (FIL_PAGE_DATA)           │
└────────┴────────┴─────────────────────────────────────────────────┘
```

### Header Field Descriptions

- **Space ID**: Tablespace identifier (0 for system tablespace)
- **Page Number**: Unique page number within the tablespace
- **Prev/Next**: Form doubly-linked lists for extent and segment management
- **LSN**: Log Sequence Number for crash recovery and consistency
- **Page Type**: Identifies the page content type (see [Page Types](#page-types-reference))
- **File Flush LSN**: Used on page 0 for recovery coordination

## System Pages

The first 8 pages of the ibdata file contain critical system information:

### Page 0: File Space Header (`FIL_PAGE_TYPE_FSP_HDR`)

Contains tablespace metadata starting at offset 38:

```
┌─────────────────────────────────────────────────────────────────┐
│                    File Space Header                            │
├────────┬────────┬─────────────────────────────────────────────────┤
│ Offset │  Size  │ Field                                           │
├────────┼────────┼─────────────────────────────────────────────────┤
│   0    │   4    │ Space ID (FSP_SPACE_ID)                         │
│   4    │   4    │ Not Used (FSP_NOT_USED)                         │
│   8    │   4    │ Current Size in Pages (FSP_SIZE)                │
│  12    │   4    │ Free Limit (FSP_FREE_LIMIT)                     │
│  16    │   4    │ Space Flags (FSP_SPACE_FLAGS)                   │
│  20    │   4    │ Fragment Pages Used (FSP_FRAG_N_USED)           │
│  24    │  16    │ Free Extent List Base (FSP_FREE)                │
│  40    │  16    │ Free Fragment List Base (FSP_FREE_FRAG)         │
│  56    │  16    │ Full Fragment List Base (FSP_FULL_FRAG)         │
│  72    │   8    │ Next Segment ID (FSP_SEG_ID)                    │
│  80    │  16    │ Full Inode Pages List (FSP_SEG_INODES_FULL)     │
│  96    │  16    │ Free Inode Pages List (FSP_SEG_INODES_FREE)     │
└────────┴────────┴─────────────────────────────────────────────────┘
```

### Page 1: Insert Buffer Bitmap (`FIL_PAGE_IBUF_BITMAP`)

Tracks free space availability in pages for insert buffer operations. Each page in the tablespace has 2 bits in this bitmap indicating available space levels.

### Page 2: Segment Inode Page (`FIL_PAGE_INODE`)

Contains file segment descriptors that manage how extents and individual pages are allocated to different segments (leaf vs non-leaf pages).

### Page 3: Insert Buffer Header

Root of the insert buffer B+ tree (system tablespace only). The insert buffer optimizes random secondary index inserts.

### Page 4: Insert Buffer Root

Root page of the insert buffer data structure.

### Page 5: Transaction System Header (`FIL_PAGE_TYPE_TRX_SYS`)

Contains transaction system metadata:
- Active transaction slots
- Rollback segment pointers  
- Binary log coordination information

### Page 6: First Rollback Segment

Rollback segment header and undo log management information.

### Page 7: Data Dictionary Header (`FIL_PAGE_TYPE_SYS`)

Contains root page numbers for system tables:
- `SYS_TABLES` (table metadata)
- `SYS_COLUMNS` (column definitions)
- `SYS_INDEXES` (index metadata) 
- `SYS_FIELDS` (index field definitions)

## B+ Tree Index Pages

Index pages (`FIL_PAGE_INDEX`) contain the actual table data and index entries. After the 38-byte common header, they have an additional index-specific header:

```
┌─────────────────────────────────────────────────────────────────┐
│                    Index Page Header (56 bytes)                 │
├────────┬────────┬─────────────────────────────────────────────────┤
│ Offset │  Size  │ Field                                           │
├────────┼────────┼─────────────────────────────────────────────────┤
│  38    │   2    │ Number of Directory Slots (PAGE_N_DIR_SLOTS)    │
│  40    │   2    │ Heap Top Pointer (PAGE_HEAP_TOP)                │
│  42    │   2    │ Number of Records in Heap (PAGE_N_HEAP)         │
│  44    │   2    │ Free Record List Pointer (PAGE_FREE)            │
│  46    │   2    │ Garbage Bytes (PAGE_GARBAGE)                    │
│  48    │   2    │ Last Insert Position (PAGE_LAST_INSERT)         │
│  50    │   2    │ Insert Direction (PAGE_DIRECTION)               │
│  52    │   2    │ Consecutive Inserts in Direction (PAGE_N_DIRECTION) │
│  54    │   2    │ Number of User Records (PAGE_N_RECS)            │
│  56    │   8    │ Maximum Transaction ID (PAGE_MAX_TRX_ID)        │
│  64    │   2    │ B+ Tree Level (PAGE_LEVEL) - 0=leaf, >0=internal│
│  66    │   8    │ Index ID (PAGE_INDEX_ID)                        │
│  74    │  10    │ Leaf Segment Header (PAGE_BTR_SEG_LEAF)         │
│  84    │  10    │ Non-leaf Segment Header (PAGE_BTR_SEG_TOP)      │
├────────┴────────┴─────────────────────────────────────────────────┤
│  94    │   -    │ Record Data Area                                │
│        │        │ ┌─────────────────────────────────────────────┐ │
│        │        │ │ Infimum Record (min boundary)              │ │
│        │        │ │ User Records (in heap order)               │ │
│        │        │ │ Supremum Record (max boundary)             │ │
│        │        │ │ Free Space                                  │ │
│        │        │ └─────────────────────────────────────────────┘ │
│        │        │                                                 │
│   End-2n │  2n    │ Page Directory (n slots, grows downward)       │
└─────────┴────────┴─────────────────────────────────────────────────┘
```

### Page Header Fields

- **Directory Slots**: Number of page directory entries for binary search
- **Heap Top**: Offset where new records will be inserted
- **N Heap**: Count includes system records (infimum/supremum)
- **Free List**: Linked list of deleted record spaces for reuse
- **Garbage**: Total bytes in deleted records
- **Insert Position**: Optimization for sequential inserts
- **Direction**: Tracks insert patterns for optimization
- **Max Trx ID**: Highest transaction ID that modified this page
- **Level**: 0 for leaf pages (data), >0 for internal pages (keys only)
- **Index ID**: Links page to specific index definition

## Record Storage

### Record Format

Records in the data area use one of two formats:

**Compact Format** (default, 5 extra bytes):
```
┌────────────────────────────────────────────────────────────┐
│ Record Header (5 bytes) │ Null Bitmap │ Var Lengths │ Data │
├─────────────────────────┼─────────────┼─────────────┼──────┤
│ - Prev record offset    │ 1 bit per   │ 1-2 bytes   │ Raw  │
│ - Record info bits      │ nullable    │ per VARCHAR │ col  │
│ - Next record offset    │ column      │ column      │ data │
└─────────────────────────┴─────────────┴─────────────┴──────┘
```

**Old Format** (6 extra bytes):
```
┌─────────────────────────────────────────────────────────────┐
│ Record Header (6 bytes) │ Field Lengths │ Data              │
├─────────────────────────┼───────────────┼───────────────────┤
│ - Prev record offset    │ 1 byte per    │ Raw column data   │
│ - Next record offset    │ variable      │                   │
│ - Record info bits      │ length field  │                   │
│ - N fields + data size  │               │                   │
└─────────────────────────┴───────────────┴───────────────────┘
```

### Record Organization

- **Storage Order**: Records stored in heap order (insertion order), not sorted order
- **Logical Order**: Maintained through next-record pointers forming a linked list
- **System Records**: 
  - Infimum: Virtual minimum record (all searches start here)
  - Supremum: Virtual maximum record (marks end of page)
- **Free List**: Deleted records form a linked list for space reuse

## Page Directory

The page directory enables efficient binary search within pages:

```
┌─────────────────────────────────────────────────────────────┐
│                     Page Directory                          │
│                  (Located at page end)                      │
├─────────────────────────────────────────────────────────────┤
│ Slot N   │ Slot N-1 │ ... │ Slot 1   │ Slot 0   │          │
│ (2 bytes)│ (2 bytes) │     │ (2 bytes)│ (2 bytes)│          │
├─────────────────────────────────────────────────────────────┤
│ Points to│ Points to │     │ Points to│ Points to│          │
│ Record   │ Record    │     │ Record   │ Supremum │          │
└─────────────────────────────────────────────────────────────┘
```

**Directory Properties**:
- Each slot is 2 bytes pointing to record offset within page
- Slot 0 always points to supremum record
- Slot 1 always points to infimum record  
- Each slot "owns" 4-8 records (except boundary slots)
- Enables O(log n) search within page

## Extent Management

Extents are 1MB chunks (64 pages) managed through extent descriptors:

### Extent Descriptor Structure (32 bytes)

```
┌─────────────────────────────────────────────────────────────┐
│                   Extent Descriptor (XDES)                  │
├────────┬────────┬─────────────────────────────────────────────┤
│ Offset │  Size  │ Field                                       │
├────────┼────────┼─────────────────────────────────────────────┤
│   0    │   8    │ Segment ID (XDES_ID) - 0 if unassigned     │
│   8    │  12    │ List Node (XDES_FLST_NODE) - prev/next     │
│  20    │   4    │ State (XDES_STATE)                          │
│  24    │  16    │ Page Bitmap (XDES_BITMAP) - 2 bits per page│
└────────┴────────┴─────────────────────────────────────────────┘
```

### Extent States

- **`XDES_FREE` (1)**: Available for allocation to any segment
- **`XDES_FREE_FRAG` (2)**: Partially used for individual page allocation
- **`XDES_FULL_FRAG` (3)**: Fully used for individual page allocation  
- **`XDES_FSEG` (4)**: Assigned to a specific file segment

### Page Bitmap

Each extent has a 16-byte bitmap with 2 bits per page (128 bits total for 64 pages):
- **Bit 0**: Page is free/used
- **Bit 1**: Page is clean/dirty (for fragment pages)

## Space Management Hierarchy

```
Tablespace (ibdata1)
├── File Space Header (Page 0)
│   ├── Free Extent Lists
│   ├── Fragment Page Lists  
│   └── Segment Inode Page Lists
├── Extents (1MB each)
│   ├── 64 Pages (16KB each)
│   └── Extent Descriptor (every 16,384th page)
├── Segments (logical grouping)
│   ├── Leaf Page Segment (table data)
│   ├── Non-leaf Page Segment (index internal nodes)
│   ├── Individual Fragment Pages
│   ├── FREE Extent List
│   ├── NOT_FULL Extent List
│   └── FULL Extent List
└── File Segment Inodes
    ├── Segment Metadata
    ├── Extent List Bases
    └── Fragment Page Array (32 pages max)
```

### Segment Management Strategy

1. **Small Tables**: Use individual pages from fragment extents
2. **Growing Tables**: Allocate partial extents (NOT_FULL list)
3. **Large Tables**: Allocate full extents (FULL list)
4. **Page Types**: Separate segments for leaf and non-leaf pages

## Key Constants

```c
// Page and Extent Sizes
#define UNIV_PAGE_SIZE          16384   // 16KB page size
#define UNIV_PAGE_SIZE_SHIFT    14      // 2^14 = 16384
#define FSP_EXTENT_SIZE         64      // Pages per extent
#define UNIV_EXTENT_SIZE        (FSP_EXTENT_SIZE * UNIV_PAGE_SIZE) // 1MB

// File Addressing  
#define FIL_ADDR_SIZE           6       // File address: 4-byte page + 2-byte offset
#define FIL_NULL                0xFFFFFFFFUL // Null page number

// List Structures
#define FLST_BASE_NODE_SIZE     16      // List base node
#define FLST_NODE_SIZE          12      // List node
#define FLST_NULL               {0xFFFFFFFF, 0xFFFF} // Null file address

// Segment Headers
#define FSEG_HEADER_SIZE        10      // File segment header
#define FSEG_INODE_SIZE         192     // Segment inode size

// Page Structure
#define FIL_PAGE_DATA           38      // Data starts after header
#define PAGE_DIR_SLOT_SIZE      2       // Directory slot size
#define PAGE_NEW_SUPREMUM       112     // Supremum record offset
#define PAGE_NEW_INFIMUM        99      // Infimum record offset

// Record Format
#define REC_N_NEW_EXTRA_BYTES   5       // Compact format extra bytes
#define REC_N_OLD_EXTRA_BYTES   6       // Old format extra bytes
```

## Page Types Reference

| Constant | Value | Description |
|----------|-------|-------------|
| `FIL_PAGE_INDEX` | 17855 | B+ tree node (data/index page) |
| `FIL_PAGE_UNDO_LOG` | 2 | Undo log page |
| `FIL_PAGE_INODE` | 3 | File segment inode page |
| `FIL_PAGE_IBUF_FREE_LIST` | 4 | Insert buffer free list |
| `FIL_PAGE_TYPE_ALLOCATED` | 0 | Freshly allocated page |
| `FIL_PAGE_IBUF_BITMAP` | 5 | Insert buffer bitmap |
| `FIL_PAGE_TYPE_SYS` | 6 | System page |
| `FIL_PAGE_TYPE_TRX_SYS` | 7 | Transaction system data |
| `FIL_PAGE_TYPE_FSP_HDR` | 8 | File space header |
| `FIL_PAGE_TYPE_XDES` | 9 | Extent descriptor page |
| `FIL_PAGE_TYPE_BLOB` | 10 | Uncompressed BLOB page |
| `FIL_PAGE_TYPE_ZBLOB` | 11 | First compressed BLOB page |
| `FIL_PAGE_TYPE_ZBLOB2` | 12 | Subsequent compressed BLOB page |

## Source Code References

### Core Header Definitions

| File | Purpose | Key Structures |
|------|---------|----------------|
| **`include/fil0fil.h`** | File page header definitions and page types | `FIL_PAGE_*` constants, page type definitions |
| **`include/fsp0fsp.h`** | File space management structures | `FSP_*` constants, extent descriptors |
| **`include/page0page.h`** | Index page header and directory structures | `PAGE_*` constants, page directory layout |
| **`include/rem0rec.h`** | Record format definitions | Record header formats, field layouts |
| **`include/univ.i`** | Core constants and configuration | `UNIV_PAGE_SIZE`, fundamental constants |

### Implementation Files

| File | Purpose | Key Functions |
|------|---------|---------------|
| **`fsp/fsp0fsp.c`** | File space management implementation | `fsp_header_init()`, extent allocation |
| **`page/page0page.c`** | Page management implementation | Page creation, directory management |
| **`fil/fil0fil.c`** | File system interface implementation | File operations, space management |
| **`buf/buf0buf.c`** | Buffer pool management | Page caching, I/O operations |
| **`btr/btr0btr.c`** | B+ tree operations | Tree node management, page splits |

### Detailed Code Locations for ibdata Layout

#### File Page Header (`FIL_PAGE_*`)
```c
// include/fil0fil.h - Lines 174-190
#define FIL_PAGE_SPACE_OR_CHKSUM 0    // Space ID or checksum
#define FIL_PAGE_OFFSET         4     // Page number
#define FIL_PAGE_PREV           8     // Previous page
#define FIL_PAGE_NEXT           12    // Next page
#define FIL_PAGE_LSN            16    // Log sequence number
#define FIL_PAGE_TYPE           24    // Page type
#define FIL_PAGE_FILE_FLUSH_LSN 26    // File flush LSN
#define FIL_PAGE_ARCH_LOG_NO_OR_SPACE_ID 34  // Space ID
#define FIL_PAGE_DATA           38    // Start of page data
```

#### Page Types
```c
// include/fil0fil.h - Lines 215-235
#define FIL_PAGE_INDEX          17855  // B-tree node
#define FIL_PAGE_UNDO_LOG       2      // Undo log page
#define FIL_PAGE_INODE          3      // Index node
#define FIL_PAGE_TYPE_FSP_HDR   8      // File space header
#define FIL_PAGE_TYPE_XDES      9      // Extent descriptor
// ... (see Page Types Reference table above)
```

#### File Space Header Structure
```c
// fsp/fsp0fsp.c - Lines 89-120
#define FSP_HEADER_OFFSET   FIL_PAGE_DATA
#define FSP_SPACE_ID        0          // Space ID
#define FSP_SIZE            8          // Current size in pages
#define FSP_FREE_LIMIT      12         // Free limit
#define FSP_SPACE_FLAGS     16         // Space flags
#define FSP_FRAG_N_USED     20         // Used fragments
#define FSP_FREE            24         // Free extent list
// ... (complete structure in System Pages section)
```

#### B+ Tree Page Header
```c
// include/page0page.h - Lines 123-150
#define PAGE_HEADER         FSEG_PAGE_DATA
#define PAGE_N_DIR_SLOTS    0          // Number of directory slots
#define PAGE_HEAP_TOP       2          // Heap top pointer
#define PAGE_N_HEAP         4          // Number of records in heap
#define PAGE_FREE           6          // Free record list
#define PAGE_LEVEL          26         // B-tree level
#define PAGE_INDEX_ID       28         // Index ID
// ... (complete structure in B+ Tree section)
```

#### Extent Descriptor Structure
```c
// include/fsp0fsp.h - Lines 245-265
#define XDES_ID         0              // Segment ID
#define XDES_FLST_NODE  8              // List node
#define XDES_STATE      (FLST_NODE_SIZE + 8)    // Extent state
#define XDES_BITMAP     (FLST_NODE_SIZE + 12)   // Page bitmap
#define XDES_SIZE       (XDES_BITMAP + (FSP_EXTENT_SIZE / 8))
```

#### Record Format Definitions
```c
// include/rem0rec.h - Lines 89-120
#define REC_N_NEW_EXTRA_BYTES   5      // Compact format extra bytes
#define REC_N_OLD_EXTRA_BYTES   6      // Old format extra bytes
#define REC_INFO_BITS_SHIFT     0      // Info bits position
#define REC_STATUS_SHIFT        4      // Status bits position
```

### System Pages Implementation

| Page | Implementation File | Key Function |
|------|-------------------|--------------|
| **Page 0 (FSP Header)** | `fsp/fsp0fsp.c` | `fsp_header_init()` (line 412) |
| **Page 1 (Ibuf Bitmap)** | `ibuf/ibuf0ibuf.c` | `ibuf_bitmap_page_init()` |
| **Page 2 (Inode)** | `fsp/fsp0fsp.c` | `fsp_seg_inode_page_create()` |
| **Page 5 (Trx System)** | `trx/trx0sys.c` | `trx_sys_create()` (line 402) |
| **Page 7 (Dict Header)** | `dict/dict0boot.c` | `dict_hdr_create()` (line 275) |

### Debugging and Analysis Tools

#### Page Analysis Functions
```c
// page/page0page.c
page_print()           // Print page contents for debugging
page_validate()        // Validate page structure
page_check_dir()       // Check page directory consistency
```

#### Space Analysis Functions
```c
// fsp/fsp0fsp.c
fsp_print()           // Print file space information
fsp_validate()        // Validate space structures
```

### Related Documentation

For information about tablespace storage models (system vs per-table), see:
- **`docs/tablespace-storage-models.md`** - Detailed comparison of storage approaches

This documentation is based on InnoDB version 1.0.6.6750 from the pre-Oracle Innobase Oy era.