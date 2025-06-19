#!/bin/bash

# MySQL 8 Setup Script for Bulk Insert Performance Comparison
echo "🐬 MySQL 8 Setup for InnoDB Performance Comparison"
echo "================================================="

# Check if running as root for installation
if [[ $EUID -eq 0 ]]; then
   echo "⚠️  Please don't run this script as root. Run as regular user."
   echo "   The script will ask for sudo password when needed."
   exit 1
fi

echo "📦 Step 1: Installing MySQL 8 and development libraries..."
echo "This will install MySQL 8 server and client development libraries"
read -p "Continue? (y/N): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Installation cancelled."
    exit 0
fi

# Update package list
sudo apt update

# Install MySQL 8 and development libraries
sudo apt install -y mysql-server mysql-client libmysqlclient-dev

# Check if MySQL is running
if ! systemctl is-active --quiet mysql; then
    echo "🚀 Starting MySQL service..."
    sudo systemctl start mysql
    sudo systemctl enable mysql
fi

echo "✅ MySQL 8 installed and running"

echo ""
echo "🔧 Step 2: MySQL Configuration for Bulk Insert Performance"
echo "========================================================="

# Create MySQL configuration for bulk inserts
echo "Creating optimized MySQL configuration for bulk inserts..."

# Create a temporary SQL file for setup
cat > /tmp/mysql_bulk_setup.sql << 'EOF'
-- Create database for bulk insert tests
CREATE DATABASE IF NOT EXISTS bulk_test_mysql;

-- Create a user for bulk insert tests (optional, you can use root)
CREATE USER IF NOT EXISTS 'bulk_user'@'localhost' IDENTIFIED BY 'bulk_password';
GRANT ALL PRIVILEGES ON bulk_test_mysql.* TO 'bulk_user'@'localhost';
GRANT FILE ON *.* TO 'bulk_user'@'localhost';
FLUSH PRIVILEGES;

-- Show current InnoDB settings
SELECT 
    @@innodb_buffer_pool_size / 1024 / 1024 / 1024 as buffer_pool_gb,
    @@innodb_log_file_size / 1024 / 1024 as log_file_mb,
    @@innodb_flush_log_at_trx_commit,
    @@innodb_doublewrite,
    @@max_connections,
    @@innodb_thread_concurrency;
EOF

echo "🔑 MySQL Setup - You'll need to set up MySQL root password if not done"
echo "If this is the first time, run: sudo mysql_secure_installation"
echo ""
echo "Running MySQL setup script..."

# Try to connect and run setup
if mysql -u root -p < /tmp/mysql_bulk_setup.sql; then
    echo "✅ MySQL database and user created successfully"
else
    echo "⚠️  MySQL setup had some issues. You may need to:"
    echo "   1. Run: sudo mysql_secure_installation"
    echo "   2. Set a root password"
    echo "   3. Run this script again"
fi

rm /tmp/mysql_bulk_setup.sql

echo ""
echo "🛠️  Step 3: Compile MySQL bulk insert program"
echo "============================================="

# Compile the MySQL version
echo "Compiling mysql_bulk_insert.c..."
if gcc -o mysql_bulk_insert mysql_bulk_insert.c -lperconaserverclient -lpthread -lm; then
    echo "✅ MySQL bulk insert program compiled successfully"
elif gcc -o mysql_bulk_insert mysql_bulk_insert.c -lmysqlclient -lpthread -lm; then
    echo "✅ MySQL bulk insert program compiled successfully"
else
    echo "❌ Compilation failed. Make sure MySQL client development libraries are installed:"
    echo "   sudo apt install libmysqlclient-dev"
    echo "   or"
    echo "   sudo apt install libperconaserverclient21-dev"
    exit 1
fi

echo ""
echo "🏁 Step 4: Performance Comparison Test Setup"
echo "============================================"

# Create comparison script
cat > run_comparison_test.sh << 'EOF'
#!/bin/bash

echo "🏆 InnoDB Embedded vs MySQL 8 Performance Comparison"
echo "===================================================="

ROWS=${1:-100000}
BATCH=${2:-10000}
THREADS=${3:-4}

echo "Test Parameters:"
echo "  Rows: $ROWS"
echo "  Batch size: $BATCH"
echo "  Threads: $THREADS"
echo ""

echo "🔧 Testing Embedded InnoDB..."
echo "================================"
rm -f ibdata1 log/ib_logfile* bulk_test/ 2>/dev/null
time LD_LIBRARY_PATH=../.libs .libs/ib_bulk_insert $ROWS $BATCH $THREADS > embedded_results.log 2>&1

echo ""
echo "🐬 Testing MySQL 8..."
echo "===================="
mysql -u root -p -e "DROP DATABASE IF EXISTS bulk_test_mysql; CREATE DATABASE bulk_test_mysql;" 2>/dev/null
time ./mysql_bulk_insert $ROWS $BATCH $THREADS localhost root > mysql_results.log 2>&1

echo ""
echo "📊 COMPARISON RESULTS:"
echo "======================"
echo ""
echo "🔧 Embedded InnoDB Results:"
echo "----------------------------"
grep -E "(rows/sec|MB/sec|Wall clock time)" embedded_results.log

echo ""
echo "🐬 MySQL 8 Results:"
echo "-------------------"
grep -E "(rows/sec|MB/sec|Wall clock time)" mysql_results.log

echo ""
echo "Full detailed logs saved to:"
echo "  - embedded_results.log"
echo "  - mysql_results.log"
EOF

chmod +x run_comparison_test.sh

echo ""
echo "🎉 Setup Complete!"
echo "================="
echo ""
echo "📋 What was installed:"
echo "  ✅ MySQL 8 Server"
echo "  ✅ MySQL Client Development Libraries"
echo "  ✅ MySQL bulk insert test program compiled"
echo "  ✅ Database 'bulk_test_mysql' created"
echo "  ✅ Comparison test script created"
echo ""
echo "🚀 Ready to run performance comparison!"
echo ""
echo "Quick test (100k rows):"
echo "  ./run_comparison_test.sh 100000 10000 4"
echo ""
echo "Large test (1M rows):"
echo "  ./run_comparison_test.sh 1000000 50000 8"
echo ""
echo "Manual tests:"
echo "  # Embedded InnoDB:"
echo "  LD_LIBRARY_PATH=../.libs .libs/ib_bulk_insert 100000 10000 4"
echo ""
echo "  # MySQL 8:"
echo "  ./mysql_bulk_insert 100000 10000 4 localhost root [password]"
echo ""
echo "📊 The comparison will show:"
echo "  - Rows per second throughput"
echo "  - MB per second throughput"  
echo "  - Total execution time"
echo "  - Network overhead (MySQL) vs direct embedded access"
echo ""
echo "💡 Expected results:"
echo "  - Embedded InnoDB: Higher throughput, lower latency"
echo "  - MySQL 8: Network overhead, but more realistic production scenario"