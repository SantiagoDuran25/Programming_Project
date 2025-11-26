#!/bin/bash

# ===========================
#  CONFIGURATION
# ===========================
SERVER_PORT=5001
SERVER_CMD="./build/server $SERVER_PORT"
CLIENT_CMD="nc 127.0.0.1 $SERVER_PORT"
TEST_FILE="test_results.txt"

GREEN="\033[1;32m"
RED="\033[1;31m"
YELLOW="\033[1;33m"
BLUE="\033[1;34m"
NC="\033[0m" # No Color

PASS_COUNT=0
FAIL_COUNT=0

echo -e "${YELLOW}==== Running TinyKV Test Suite =====${NC}"
echo "Results:" > "$TEST_FILE"

# ===========================
#  START SERVER IN BACKGROUND
# ===========================
$SERVER_CMD > /dev/null 2>&1 &
SERVER_PID=$!
sleep 0.5

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "❌ ${RED}ERROR: Server did not start.${NC}"
    exit 1
fi
echo -e "🔌 Server started (PID $SERVER_PID)"

# ===========================
#  HELPER FUNCTION
# ===========================
run_test() {
    NAME="$1"
    INPUT="$2"
    EXPECT="$3"

    RESULT=$(echo -e "$INPUT" | $CLIENT_CMD | tail -n 1)

    if [[ "$RESULT" == "$EXPECT" ]]; then
        PASS_COUNT=$((PASS_COUNT+1))
        echo -e "✔ ${GREEN}$NAME${NC}"
        echo "[PASS] $NAME" >> "$TEST_FILE"
    else
        FAIL_COUNT=$((FAIL_COUNT+1))
        echo -e "✖ ${RED}$NAME${NC} (expected '$EXPECT' got '$RESULT')"
        echo "[FAIL] $NAME (expected '$EXPECT', got '$RESULT')" >> "$TEST_FILE"
    fi
}

# ===========================
#  TEST CASES
# ===========================
run_test "SET stores value"              "SET name tomas\nGET name\nEXIT"         "tomas"
run_test "OVERWRITE value"               "SET key x\nSET key y\nGET key\nEXIT"     "y"
run_test "DELETE entry"                  "SET a b\nDEL a\nGET a\nEXIT"             "NOT FOUND"
run_test "LIST KEYS"                     "SET x 1\nSET y 2\nKEYS\nEXIT"             "x"

run_test "MALFORMED GET"                 "GET\nEXIT"                               "Usage GET"
run_test "MALFORMED SET rejected"        "SET x\nEXIT"                             "Usage SET"
run_test "EXTRA SET arguments rejected"  "SET x a b\nEXIT"                         "Too many arguments"
run_test "DELETE missing key error"      "DEL x\nEXIT"                             "NOT FOUND"
run_test "SAVE command works"            "SAVE\nEXIT"                              "SAVED"

# ===========================
#  CLEANUP + SUMMARY
# ===========================
kill $SERVER_PID 2>/dev/null
echo -e "🛑 Server stopped"

TOTAL=$((PASS_COUNT + FAIL_COUNT))
PERCENT=$(( (PASS_COUNT * 100) / TOTAL ))

echo "" >> "$TEST_FILE"
echo "Total: $TOTAL" >> "$TEST_FILE"
echo "Passed: $PASS_COUNT" >> "$TEST_FILE"
echo "Failed: $FAIL_COUNT" >> "$TEST_FILE"
echo "Score: $PERCENT%" >> "$TEST_FILE"

# ===========================
#  DISPLAY SUMMARY
# ===========================
echo -e "\n${BLUE}====== TEST SUMMARY ======${NC}"
echo -e "✔ Passed:  ${GREEN}$PASS_COUNT${NC}"
echo -e "✖ Failed:  ${RED}$FAIL_COUNT${NC}"
echo -e "📊 Score:   ${YELLOW}$PERCENT %${NC}"
echo -e "📄 Results saved to ${GREEN}$TEST_FILE${NC}"
echo -e "===========================\n"
