#!/bin/bash

# InnoDB Massive Bulk Insert Test Runner
# This script provides an easy interface to run bulk insert performance tests

echo "🚀 InnoDB Massive Bulk Insert Performance Test"
echo "============================================="

# Default values
DEFAULT_ROWS=1000000
DEFAULT_BATCH=10000
DEFAULT_THREADS=4

# Parse command line arguments
ROWS=${1:-$DEFAULT_ROWS}
BATCH_SIZE=${2:-$DEFAULT_BATCH}
THREADS=${3:-$DEFAULT_THREADS}

# Validate arguments
if ! [[ "$ROWS" =~ ^[0-9]+$ ]] || [ "$ROWS" -lt 1 ]; then
    echo "❌ Error: Invalid number of rows: $ROWS"
    echo "Usage: $0 [rows] [batch_size] [threads]"
    echo "Example: $0 1000000 10000 4"
    exit 1
fi

if ! [[ "$BATCH_SIZE" =~ ^[0-9]+$ ]] || [ "$BATCH_SIZE" -lt 1 ]; then
    echo "❌ Error: Invalid batch size: $BATCH_SIZE"
    exit 1
fi

if ! [[ "$THREADS" =~ ^[0-9]+$ ]] || [ "$THREADS" -lt 1 ] || [ "$THREADS" -gt 16 ]; then
    echo "❌ Error: Invalid thread count: $THREADS (must be 1-16)"
    exit 1
fi

# Check if executable exists
if [ ! -f ".libs/ib_bulk_insert" ]; then
    echo "⚙️  Building bulk insert test program..."
    make ib_bulk_insert
    if [ $? -ne 0 ]; then
        echo "❌ Build failed!"
        exit 1
    fi
fi

# Clean up any existing test data
echo "🧹 Cleaning up previous test data..."
rm -rf bulk_test/ ibdata1 log/ ib_logfile* 2>/dev/null

# Show test configuration
echo ""
echo "📊 Test Configuration:"
echo "   Rows to insert: $(printf "%'d" $ROWS)"
echo "   Batch size: $(printf "%'d" $BATCH_SIZE)"
echo "   Threads: $THREADS"
echo "   Estimated data size: ~$(( ROWS * 500 / 1024 / 1024 )) MB"

# Calculate estimated time
if [ "$ROWS" -gt 100000 ]; then
    echo "   ⏱️  Estimated time: $(( ROWS / 50000 )) - $(( ROWS / 20000 )) minutes"
fi

echo ""
read -p "🔥 Ready to start bulk insert test? (y/N): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Test cancelled."
    exit 0
fi

# Run the test
echo ""
echo "🏃 Starting bulk insert test..."
echo "================================================"

# Set library path and run
export LD_LIBRARY_PATH=../.libs:$LD_LIBRARY_PATH

# Run with time measurement
START_TIME=$(date +%s)
.libs/ib_bulk_insert "$ROWS" "$BATCH_SIZE" "$THREADS"
EXIT_CODE=$?
END_TIME=$(date +%s)

TOTAL_TIME=$((END_TIME - START_TIME))

echo ""
echo "================================================"

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ Bulk insert test completed successfully!"
    echo "⏱️  Total execution time: ${TOTAL_TIME} seconds"
    
    # Show database files created
    if [ -f "ibdata1" ]; then
        DATA_SIZE=$(du -h ibdata1 | cut -f1)
        echo "💾 Database file size: $DATA_SIZE"
    fi
    
    if [ -d "log" ]; then
        LOG_SIZE=$(du -sh log | cut -f1)
        echo "📝 Log files size: $LOG_SIZE"
    fi
    
    echo ""
    echo "🔍 To inspect the data:"
    echo "   - Database files are in current directory"
    echo "   - Use other test programs to query the data"
    echo "   - Run: LD_LIBRARY_PATH=../.libs .libs/ib_test1 to see basic operations"
    
else
    echo "❌ Bulk insert test failed with exit code: $EXIT_CODE"
    exit $EXIT_CODE
fi

echo ""
echo "🧹 Clean up test data? (y/N): "
read -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -rf bulk_test/ ibdata1 log/ ib_logfile* 2>/dev/null
    echo "✅ Test data cleaned up"
else
    echo "💾 Test data preserved for inspection"
fi