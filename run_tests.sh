#!/bin/bash

# Always run from project root
cd "$(dirname "$0")"

# Start server from build folder on port 5001
./build/server 5001 &
SERVER_PID=$!

# Small delay so server starts
sleep 0.5

# Send commands through our own client
printf "SET x 10\nGET x\nEXIT\n" | ./build/client 127.0.0.1 5001

# Kill the server after test
kill $SERVER_PID 2>/dev/null
