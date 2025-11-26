#!/bin/bash

SERVER_PORT=5001
SERVER_CMD="./server $SERVER_PORT"
CLIENT_CMD="nc 127.0.0.1 $SERVER_PORT"
TEST_FILE="test_results.txt"

echo "==== Running TinyKV Test Suite ====="
echo "Results:" > "$TEST_FILE"

# ===========================
#  START SERVER IN BACKGROUND
# ===========================
$SERVER_CMD > /dev/null 2>&1 &
SERVER_PID=$!
sleep 1

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "❌ ERROR: Server did not start."
    exit 1
fi

echo "🔌 Server started (PID $SERVER_PID)"

# ============= TEST FUNCTION =============
run_test() {
    description="$1"
    input="$2"
    expected="$3"

    # Send input to client and capture output
    output=$(echo -e "$input" | $CLIENT_CMD 2>/dev/null)

    if echo "$output" | grep -q "$expected"; then
        echo "✔ PASSED: $description"
        echo "PASS: $description" >> "$TEST_FILE"
    else
        echo "❌ FAILED: $description"
        echo "Expected: $expected"
        echo "Got:      $output"
        echo "FAIL: $description" >> "$TEST_FILE"
    fi
}

# =======================
#  ACTUAL TEST CASES
# =======================
run_test "SET stores value"               "SET name tomas\nGET name\nEXIT" "tomas"
run_test "OVERWRITE value"                "SET key x\nSET key y\nGET key\nEXIT" "y"
run_test "DELETE entry"                   "SET a b\nDEL a\nGET a\nEXIT" "NOT FOUND"
run_test "LIST KEYS"                      "SET x 1\nSET y 2\nKEYS\nEXIT" "x"
run_test "MALFORMED GET rejected"         "GET\nEXIT" "Usage GET"
run_test "MALFORMED SET rejected"         "SET x\nEXIT" "Usage SET"
run_test "EXTRA SET arguments rejected"   "SET a b c\nEXIT" "Too many arguments"
run_test "DELETE missing key error"       "DEL x\nEXIT" "NOT FOUND"
run_test "SAVE command works"             "SAVE\nEXIT" "SAVED"

# =======================
#  CLEANUP
# =======================
kill $SERVER_PID 2>/dev/null
echo "🛑 Server stopped"
echo "📄 Results saved to $TEST_FILE"
echo "=================================="
