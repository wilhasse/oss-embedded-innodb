# Development Tools for InnoDB Analysis

This guide covers the essential tools and techniques for analyzing, debugging, and understanding the InnoDB codebase.

## 🛠️ Essential Tools Setup

### Installing Debug Tools

```bash
# Core debugging tools
sudo apt update
sudo apt install -y gdb ltrace strace valgrind

# Analysis tools  
sudo apt install -y binutils cppcheck clang-tools

# Documentation tools
sudo apt install -y graphviz doxygen

# Performance tools
sudo apt install -y perf linux-tools-generic htop
```

### Build Configuration

```bash
# Debug build with symbols
make clean
CFLAGS="-g -O0 -DDEBUG -DUNIV_DEBUG" make

# Release build with optimizations
make clean  
CFLAGS="-O2 -DNDEBUG" make

# Memory debugging build
make clean
CFLAGS="-g -O0 -fsanitize=address" make
```

## 🔍 Code Analysis Tools

### 1. Static Analysis with Cppcheck

```bash
# Install cppcheck
sudo apt install -y cppcheck

# Analyze entire codebase
cppcheck --enable=all --suppress=missingIncludeSystem . 2> cppcheck_report.txt

# Focus on specific modules
cppcheck --enable=all api/ > api_analysis.txt
cppcheck --enable=all btr/ > btree_analysis.txt
cppcheck --enable=all trx/ > transaction_analysis.txt
```

### 2. Code Complexity Analysis

```bash
# Cyclomatic complexity
sudo apt install -y pmccabe
pmccabe api/*.c | sort -nr > complexity_report.txt

# Lines of code analysis
cloc . --exclude-dir=tests,aconf
```

### 3. Function Call Analysis

```bash
# Generate call graphs
sudo apt install -y cflow
cflow api/api0api.c > api_callgraph.txt
cflow btr/btr0cur.c > btree_callgraph.txt

# Create visual call graphs
cflow api/api0api.c | python3 -c "
import sys
import re
for line in sys.stdin:
    if '->' in line:
        indent = len(line) - len(line.lstrip())
        func = re.search(r'(\w+)\(\)', line)
        if func:
            print('  ' * (indent//4) + func.group(1))
" > visual_callgraph.txt
```

## 🐛 Debugging Tools

### 1. GDB Advanced Usage

#### Custom GDB Commands

Create `.gdbinit` in project root:
```bash
cat > .gdbinit << 'EOF'
# Custom InnoDB debugging commands

define print_trx
    if $argc == 1
        print *(trx_t*)$arg0
    else
        echo Usage: print_trx <transaction_pointer>\n
    end
end

define print_page_header
    if $argc == 1
        print *(page_t*)$arg0
        x/10x $arg0
    else
        echo Usage: print_page_header <page_pointer>\n  
    end
end

define trace_api_calls
    break ib_init
    break ib_startup
    break ib_trx_begin
    break ib_cursor_open_table
    break ib_cursor_insert_row
    break ib_trx_commit
    commands
        bt 1
        continue
    end
end

set print pretty on
set pagination off
EOF
```

#### GDB Automation Scripts

```bash
# Create automated debugging session
cat > debug_session.gdb << 'EOF'
set environment LD_LIBRARY_PATH=../.libs
file tests/.libs/ib_test1
trace_api_calls
run
EOF

gdb -x debug_session.gdb
```

### 2. Memory Debugging with Valgrind

```bash
# Memory leak detection
env LD_LIBRARY_PATH=../.libs valgrind --leak-check=full --show-leak-kinds=all ./ib_test1

# Buffer overflow detection  
env LD_LIBRARY_PATH=../.libs valgrind --tool=memcheck --track-origins=yes ./ib_test1

# Cache performance analysis
env LD_LIBRARY_PATH=../.libs valgrind --tool=cachegrind ./ib_test1
```

### 3. Address Sanitizer

```bash
# Build with AddressSanitizer
make clean
CFLAGS="-g -O0 -fsanitize=address -fno-omit-frame-pointer" make

# Run with memory error detection
env LD_LIBRARY_PATH=../.libs ./ib_test1
```

## 📊 Performance Analysis

### 1. System Call Tracing

```bash
# Comprehensive system call analysis
strace -c ./ib_test1  # Summary statistics
strace -T ./ib_test1  # Time each call
strace -f ./ib_test1  # Follow forks/threads

# Focus on I/O operations
strace -e trace=read,write,fsync,fdatasync ./ib_test1

# File operations only
strace -e trace=file ./ib_test1
```

### 2. Performance Profiling

```bash
# CPU profiling with perf
sudo perf record -g ./ib_test1
sudo perf report

# Function-level profiling with gprof
CFLAGS="-pg" make clean && make
./ib_test1
gprof ./ib_test1 gmon.out > profile_report.txt
```

### 3. I/O Analysis

```bash
# Monitor file I/O
sudo iotop -o -d 1

# Disk usage monitoring
iostat -x 1

# File system activity
sudo inotifywait -m -r . --include='.*\.(db|log)$'
```

## 🔬 Advanced Analysis Techniques

### 1. Binary Analysis

```bash
# Examine binary structure
readelf -h libinnodb.so
readelf -S libinnodb.so  # Section headers
readelf -s libinnodb.so  # Symbol table

# Disassemble functions
objdump -d libinnodb.so | grep -A 20 "ib_init"
objdump -t libinnodb.so | grep ib_  # Symbol table

# Analyze dependencies
ldd tests/.libs/ib_test1
nm -D libinnodb.so | grep "ib_"  # Dynamic symbols
```

### 2. Code Coverage Analysis

```bash
# Build with coverage instrumentation
make clean
CFLAGS="-g -O0 --coverage" make

# Run tests
./ib_test1

# Generate coverage report
gcov api/api0api.c
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### 3. Thread Analysis

```bash
# Thread debugging with GDB
(gdb) info threads
(gdb) thread 2
(gdb) where

# Monitor thread activity
ps -eLf | grep ib_test
top -H -p <process_id>
```

## 🧪 Testing and Validation

### 1. Unit Testing Framework

Create simple test framework:
```c
// test_framework.h
#define ASSERT(condition) \
    if (!(condition)) { \
        fprintf(stderr, "ASSERTION FAILED: %s:%d %s\n", \
                __FILE__, __LINE__, #condition); \
        exit(1); \
    }

#define TEST(name) \
    void test_##name() { \
        printf("Running test: " #name "\n");

#define END_TEST \
    printf("PASSED\n"); \
    }
```

### 2. Regression Testing

```bash
#!/bin/bash
# regression_test.sh

TESTS=(
    "ib_test1"
    "ib_cursor"  
    "ib_types"
    "ib_cfg"
)

for test in "${TESTS[@]}"; do
    echo "Running $test..."
    if ./$test > ${test}_output.txt 2>&1; then
        echo "✓ $test PASSED"
    else  
        echo "✗ $test FAILED"
        cat ${test}_output.txt
    fi
done
```

### 3. Stress Testing

```bash
# Memory stress test
while true; do
    valgrind --leak-check=full ./ib_test1
    if [ $? -ne 0 ]; then
        echo "Memory issue detected!"
        break
    fi
done

# Concurrency stress test  
for i in {1..10}; do
    ./ib_mt_stress &
done
wait
```

## 📝 Documentation Generation

### 1. Code Documentation with Doxygen

```bash
# Install doxygen
sudo apt install -y doxygen graphviz

# Generate configuration
doxygen -g doxygen.config

# Edit doxygen.config for InnoDB
sed -i 's/EXTRACT_ALL = NO/EXTRACT_ALL = YES/' doxygen.config
sed -i 's/HAVE_DOT = NO/HAVE_DOT = YES/' doxygen.config
sed -i 's/CALL_GRAPH = NO/CALL_GRAPH = YES/' doxygen.config

# Generate documentation
doxygen doxygen.config
```

### 2. API Documentation

```bash
# Extract API functions
grep -n "^ib_" api/api0api.c | \
    sed 's/.*\(ib_[^(]*\).*/\1/' | \
    sort -u > api_functions.txt

# Generate function signatures
ctags -x --c-kinds=f api/*.c | \
    grep "^ib_" > api_signatures.txt
```

## 🔧 Automation Scripts

### 1. Build Automation

```bash
#!/bin/bash
# build_and_test.sh

echo "=== Building InnoDB ==="
make clean
if ! make; then
    echo "Build failed!"
    exit 1
fi

echo "=== Running Tests ==="
cd tests
make clean && make

echo "=== Testing API Functions ==="
for test in ib_test*; do
    if [ -x "$test" ]; then
        echo "Running $test..."
        ./$test || echo "FAILED: $test"
    fi
done
```

### 2. Analysis Automation

```bash
#!/bin/bash
# analyze_code.sh

echo "=== Static Analysis ==="
cppcheck --enable=all --suppress=missingIncludeSystem . > static_analysis.txt 2>&1

echo "=== Memory Analysis ==="
env LD_LIBRARY_PATH=../.libs valgrind --leak-check=full ./ib_test1 > memory_analysis.txt 2>&1

echo "=== Performance Analysis ==="
env LD_LIBRARY_PATH=../.libs strace -c ./ib_test1 > performance_analysis.txt 2>&1

echo "Analysis complete. Check *_analysis.txt files."
```

### 3. Debug Information Extractor

```bash
#!/bin/bash
# extract_debug_info.sh

API_FUNCS=$(nm -D ../.libs/libinnodb.so | grep " ib_" | awk '{print $3}')

echo "# InnoDB API Functions Debug Info"
echo

for func in $API_FUNCS; do
    echo "## $func"
    
    # Find source location
    grep -rn "^$func" ../api/ --include="*.c" 2>/dev/null
    
    # Find header declaration  
    grep -rn "$func" ../include/ --include="*.h" 2>/dev/null | head -1
    
    echo
done
```

## 💡 Pro Tips

### 1. Efficient Debugging Workflow

1. **Start with ltrace** to see API call order
2. **Use strace** to understand file operations  
3. **Apply GDB** for detailed code analysis
4. **Add printf debugging** for specific issues
5. **Use Valgrind** for memory problems

### 2. Code Reading Strategy

1. **Begin with API layer** (`api/api0api.c`)
2. **Follow function calls** into implementation layers
3. **Understand data structures** in header files
4. **Trace execution paths** with debugging tools
5. **Study test cases** for usage patterns

### 3. Performance Investigation

1. **Profile first** - don't guess where bottlenecks are
2. **Focus on hot paths** - optimize the 80/20 rule
3. **Measure impact** - before and after comparisons
4. **Consider trade-offs** - memory vs. CPU vs. I/O

---

These tools and techniques provide a comprehensive foundation for understanding, debugging, and analyzing the early InnoDB storage engine implementation.