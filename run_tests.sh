#!/bin/bash

# Start server in background
./server &
SERVER_PID=$!

# Small delay to ensure it starts
sleep 0.5

# Send 3 commands and print output
echo -e "SET x 10\nGET x\nEXIT" | nc 127.0.0.1 5001

# Kill server after test
kill $SERVER_PID
