#!/bin/bash
set -e

BLOCKDEV=${1:-/dev/nbd0}
SIZE=134217728  # 128MB
TESTFILE=/tmp/lds_test_data

# Pre-check: device must not already be in use
if nbd-client -c "$BLOCKDEV" &>/dev/null; then
    echo "ERROR: $BLOCKDEV is already in use"
    exit 1
fi

cleanup() {
    sudo nbd-client -d "$BLOCKDEV" &>/dev/null || true
    rm -f "$TESTFILE"
    [ -n "$LDSPID" ] && kill "$LDSPID" &>/dev/null || true
}
trap cleanup EXIT

# Start LDS
sudo ./bin/LDS "$BLOCKDEV" "$SIZE" &
LDSPID=$!
sleep 1

# Verify device is connected
nbd-client -c "$BLOCKDEV" > /dev/null || { echo "FAIL: device not connected"; exit 1; }

# Test 1: initial content is zero
echo "Test 1: zero-initialized..."
cmp <(sudo dd if="$BLOCKDEV" bs=1k count=5 skip=54 status=none) \
    <(dd if=/dev/zero bs=1k count=5 status=none)
echo "  PASS"

# Test 2: write random data and read it back
echo "Test 2: write/read at boundary..."
dd if=/dev/urandom of="$TESTFILE" bs=1M count=2 status=none
sudo dd if="$TESTFILE" of="$BLOCKDEV" bs=1M seek=126 status=none
cmp <(sudo dd if="$BLOCKDEV" bs=1M count=2 skip=126 status=none) "$TESTFILE"
echo "  PASS"

# Test 3: overwrite and verify new data
echo "Test 3: overwrite and verify..."
dd if=/dev/urandom of="$TESTFILE" bs=4k count=10 status=none
sudo dd if="$TESTFILE" of="$BLOCKDEV" bs=4k seek=1 status=none
cmp <(sudo dd if="$BLOCKDEV" bs=4k count=10 skip=1 status=none) "$TESTFILE"
echo "  PASS"

echo "All tests passed."
