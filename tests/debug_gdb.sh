#!/bin/bash

# GDB script to trace InnoDB API calls step by step
echo "=== GDB InnoDB Tracer ==="
echo "This will trace the execution path of InnoDB API calls"
echo ""

# Create GDB command file for automated tracing
cat > gdb_commands.txt << 'EOF'
# Set breakpoints on key InnoDB API functions
break ib_init
break ib_startup  
break ib_trx_begin
break ib_cursor_open_table
break ib_cursor_lock
break ib_cursor_insert_row
break ib_cursor_first
break ib_cursor_next
break ib_cursor_close
break ib_trx_commit
break ib_shutdown

# Set up automatic command execution on breakpoints
commands 1
printf "\n=== ib_init() called ===\n"
bt 3
continue
end

commands 2  
printf "\n=== ib_startup() called ===\n"
bt 3
continue
end

commands 3
printf "\n=== ib_trx_begin() called ===\n"
bt 3  
continue
end

commands 4
printf "\n=== ib_cursor_open_table() called ===\n"
bt 3
continue
end

commands 5
printf "\n=== ib_cursor_lock() called ===\n"
bt 3
continue
end

commands 6
printf "\n=== ib_cursor_insert_row() called ===\n"
bt 3
continue
end

commands 7
printf "\n=== ib_cursor_first() called ===\n"
bt 3
continue
end

commands 8
printf "\n=== ib_cursor_next() called ===\n"
bt 3
continue
end

commands 9
printf "\n=== ib_cursor_close() called ===\n"
bt 3
continue
end

commands 10
printf "\n=== ib_trx_commit() called ===\n"
bt 3
continue
end

commands 11
printf "\n=== ib_shutdown() called ===\n"
bt 3
continue
end

# Start execution
run
EOF

echo "Running GDB with automatic API tracing..."
echo "This will show the call stack for each major InnoDB operation"
echo ""

gdb -batch -x gdb_commands.txt ./ib_cursor

# Cleanup
rm -f gdb_commands.txt