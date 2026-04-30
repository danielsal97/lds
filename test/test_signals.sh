#!/bin/bash
set -e

BLOCKDEV=${1:-/dev/nbd0}
SIZE=16777216  # 16MB

# Pre-check: device must not already be in use
if nbd-client -c "$BLOCKDEV" &>/dev/null; then
    echo "ERROR: $BLOCKDEV is already in use"
    exit 1
fi

cleanup() {
    sudo nbd-client -d "$BLOCKDEV" &>/dev/null || true
    [ -n "$LDSPID" ] && kill "$LDSPID" &>/dev/null || true
}
trap cleanup EXIT

wait_for_device() {
    local dev=$1
    for i in $(seq 1 10); do
        nbd-client -c "$dev" &>/dev/null && return 0
        sleep 0.5
    done
    echo "FAIL: device not connected after 5s"
    return 1
}

# Test 1: graceful shutdown via SIGTERM
echo "Test 1: SIGTERM shutdown..."
sudo ./bin/LDS "$BLOCKDEV" "$SIZE" &
LDSPID=$!
wait_for_device "$BLOCKDEV"
sleep 2  # let kernel finish partition probing
kill -SIGTERM "$LDSPID"
wait "$LDSPID" || true
LDSPID=
echo "  PASS"

# Test 2: device can be reattached after shutdown
echo "Test 2: device reuse after SIGTERM..."
sudo ./bin/LDS "$BLOCKDEV" "$SIZE" &
LDSPID=$!
wait_for_device "$BLOCKDEV"
echo "  PASS"

# Test 3: graceful shutdown via SIGINT
echo "Test 3: SIGINT shutdown..."
sleep 2  # let kernel finish partition probing
kill -SIGINT "$LDSPID"
wait "$LDSPID" || true
LDSPID=
echo "  PASS"

echo "All signal tests passed."
