#!/bin/bash

echo "🐬 Testing MySQL Connection and Setup"
echo "====================================="

# Test if we can connect to MySQL
if mysql -u root -e "SELECT VERSION();" 2>/dev/null; then
    echo "✅ MySQL connection successful (no password)"
    MYSQL_PASSWORD=""
elif mysql -u root -p -e "SELECT VERSION();" 2>/dev/null; then
    echo "✅ MySQL connection successful (with password)"
    echo "⚠️  Note: You'll need to provide password for bulk insert tests"
else
    echo "❌ Cannot connect to MySQL. Run setup first:"
    echo "   sudo mysql_secure_installation"
    exit 1
fi

# Test our compiled program
echo ""
echo "🧪 Testing MySQL bulk insert program..."
if [ -f "./mysql_bulk_insert" ]; then
    echo "✅ mysql_bulk_insert program exists"
    
    # Quick test with 10 rows
    echo "Running quick test (10 rows)..."
    if ./mysql_bulk_insert 10 5 1 localhost root 2>&1 | grep -q "Total rows"; then
        echo "✅ MySQL bulk insert test successful!"
    else
        echo "⚠️  Test needs authentication. Try:"
        echo "   ./mysql_bulk_insert 10 5 1 localhost root [your_mysql_password]"
    fi
else
    echo "❌ mysql_bulk_insert not found. Compile it first:"
    echo "   gcc -o mysql_bulk_insert mysql_bulk_insert.c -lperconaserverclient -lpthread -lm"
fi

echo ""
echo "🏆 Ready for Performance Comparison!"
echo "===================================="
echo ""
echo "Test commands:"
echo "  # Small test (1K rows):"
echo "  ./mysql_bulk_insert 1000 100 1 localhost root"
echo ""
echo "  # Medium test (100K rows):"  
echo "  ./mysql_bulk_insert 100000 10000 4 localhost root"
echo ""
echo "  # Large test (1M rows):"
echo "  ./mysql_bulk_insert 1000000 50000 8 localhost root"
echo ""
echo "Compare with embedded InnoDB:"
echo "  LD_LIBRARY_PATH=../.libs .libs/ib_bulk_insert 100000 10000 4"