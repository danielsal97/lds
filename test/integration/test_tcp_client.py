#!/usr/bin/env python3
"""
TCP Client for testing TCPDriverComm
Sends block device commands (READ/WRITE/FLUSH/TRIM) over TCP
Wire protocol: big-endian binary format
"""

import socket
import struct
import sys
import time

# Request types (must match DriverData::ActionType)
READ = 0
WRITE = 1
DISCONNECT = 2
FLUSH = 3
TRIM = 4
GET_SIZE = 5

class TCPBlockDeviceClient:
    def __init__(self, host='127.0.0.1', port=9999):
        self.host = host
        self.port = port
        self.socket = None
        self.written_offsets = []  # Track which offsets have been written to

    def connect(self):
        """Connect to TCPDriverComm server"""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.socket.connect((self.host, self.port))
            print(f"✓ Connected to {self.host}:{self.port}")
        except Exception as e:
            print(f"✗ Failed to connect: {e}")
            sys.exit(1)

    def disconnect(self):
        """Close connection"""
        if self.socket:
            self.socket.close()
            print("✓ Disconnected")

    def send_request(self, req_type, handle, offset, length, data=None):
        """
        Send a request to the server
        Request format: [type(4) | handle(8) | offset(8) | len(4)] + optional payload
        """
        # Pack header: type(uint32) handle(uint64) offset(uint64) len(uint32) - big-endian
        header = struct.pack('>I Q Q I', req_type, handle, offset, length)

        type_name = {READ: 'READ', WRITE: 'WRITE', FLUSH: 'FLUSH', TRIM: 'TRIM', DISCONNECT: 'DISCONNECT', GET_SIZE: 'GET_SIZE'}
        print(f"\n[→ Send] {type_name.get(req_type, 'UNKNOWN')} handle={handle} offset={offset} len={length}")

        # Send header
        self.socket.sendall(header)
        print(f"  Header sent: {len(header)} bytes")

        # Send payload for WRITE
        if req_type == WRITE and data:
            if isinstance(data, str):
                data = data.encode()
            self.socket.sendall(data)
            print(f"  Payload sent: {data}")

    def recv_reply(self):
        """
        Receive reply from server
        Reply format: [error(4) | handle(8) | len(4)] + optional payload for READ
        """
        # Receive header: error(uint32) handle(uint64) len(uint32) - big-endian
        header = self.socket.recv(16)
        if not header or len(header) < 16:
            print("✗ Failed to receive reply header")
            return None, None, None, 0

        error, handle, reply_len = struct.unpack('>I Q I', header)
        status = "SUCCESS" if error == 0 else f"ERROR({error})"

        print(f"[← Recv] Status={status} handle={handle} len={reply_len}")
        print(f"  Header received: {len(header)} bytes")

        return error, handle, status, reply_len

    def recv_payload(self, length):
        """Receive payload data (for READ operations)"""
        if length == 0:
            return b""

        payload = self.socket.recv(length)
        print(f"  Payload received: {len(payload)} bytes")
        return payload

    def test_write(self, offset, data):
        """Test WRITE operation"""
        print("\n" + "="*60)
        print("TEST: WRITE")
        print("="*60)

        if isinstance(data, str):
            data_bytes = data.encode()
        else:
            data_bytes = data

        handle = int(time.time() * 1000) % 1000000  # Use timestamp as handle

        self.send_request(WRITE, handle, offset, len(data_bytes), data_bytes)

        try:
            self.socket.settimeout(0.5)
            error, ret_handle, status, _ = self.recv_reply()
            self.socket.settimeout(None)
            print(f"Result: {status}")
            if offset not in self.written_offsets:
                self.written_offsets.append(offset)
                self.written_offsets.sort()
        except socket.timeout:
            self.socket.settimeout(None)
            print("(No reply expected for WRITE)")
            if offset not in self.written_offsets:
                self.written_offsets.append(offset)
                self.written_offsets.sort()

    def test_read(self, offset, length):
        """Test READ operation"""
        print("\n" + "="*60)
        print("TEST: READ")
        print("="*60)

        handle = int(time.time() * 1000) % 1000000

        self.send_request(READ, handle, offset, length)

        error, ret_handle, status, _ = self.recv_reply()

        if error == 0 and ret_handle == handle:
            payload = self.recv_payload(length)
            try:
                print(f"  Data: {payload.decode('utf-8', errors='ignore')}")
            except:
                print(f"  Data (raw): {payload}")
            return payload
        else:
            print(f"✗ Read failed: {status}")
            return None

    def test_flush(self):
        """Test FLUSH operation"""
        print("\n" + "="*60)
        print("TEST: FLUSH")
        print("="*60)

        handle = int(time.time() * 1000) % 1000000
        self.send_request(FLUSH, handle, 0, 0)

        error, ret_handle, status, _ = self.recv_reply()
        print(f"Result: {status}")

    def test_trim(self, offset, length):
        """Test TRIM operation"""
        print("\n" + "="*60)
        print("TEST: TRIM")
        print("="*60)

        handle = int(time.time() * 1000) % 1000000
        self.send_request(TRIM, handle, offset, length)

        error, ret_handle, status, _ = self.recv_reply()
        print(f"Result: {status}")

    def test_get_size(self, offset):
        """Test GET_SIZE operation"""
        print("\n" + "="*60)
        print("TEST: GET_SIZE")
        print("="*60)

        handle = int(time.time() * 1000) % 1000000

        self.send_request(GET_SIZE, handle, offset, 0)

        error, ret_handle, status, size = self.recv_reply()

        print(f"Result: {status}")
        if error == 0:
            print(f"  Data size at offset {offset}: {size} bytes")
            return size
        else:
            print(f"✗ GET_SIZE failed: {status}")
            return 0

    def test_cycle(self, offset, data):
        """Test WRITE then READ"""
        print("\n" + "="*60)
        print("TEST: WRITE/READ CYCLE")
        print("="*60)

        if isinstance(data, str):
            data_bytes = data.encode()
        else:
            data_bytes = data

        # WRITE
        write_handle = 100
        print(f"\n[Step 1] Write '{data.decode() if isinstance(data, bytes) else data}' at offset {offset}")
        self.send_request(WRITE, write_handle, offset, len(data_bytes), data_bytes)

        time.sleep(0.1)

        # READ
        read_handle = 101
        print(f"\n[Step 2] Read {len(data_bytes)} bytes from offset {offset}")
        self.send_request(READ, read_handle, offset, len(data_bytes))

        error, ret_handle, status, _ = self.recv_reply()

        if error == 0:
            payload = self.recv_payload(len(data_bytes))
            match = payload == data_bytes
            print(f"\n[Result] Data match: {'✓ YES' if match else '✗ NO'}")
            if not match:
                print(f"  Expected: {data_bytes}")
                print(f"  Got:      {payload}")
        else:
            print(f"[Result] Read failed: {status}")


def interactive_menu(client):
    """Interactive menu for manual testing"""
    while True:
        print("\n" + "="*60)
        print("Menu:")
        print("="*60)
        print("1. WRITE - Write data to offset")
        print("2. READ  - Read data from offset (auto-detect size)")
        print("3. FLUSH - Flush data")
        print("4. TRIM  - Trim data")
        print("5. CYCLE - Write then Read (verify)")
        print("6. GET_SIZE - Check data size at offset")
        print("7. EXIT  - Disconnect and exit")
        print("="*60)

        choice = input("Enter choice (1-7): ").strip()

        try:
            if choice == '1':
                # WRITE
                offset = int(input("Enter offset: "))
                data = input("Enter data to write: ")
                client.test_write(offset, data)

            elif choice == '2':
                # READ - show list of available offsets
                if not client.written_offsets:
                    print("❌ No data written yet. Use WRITE first.")
                else:
                    print("\nAvailable offsets:")
                    for i, offset in enumerate(client.written_offsets):
                        print(f"  {i+1}. Offset {offset}")

                    idx = int(input("Select offset (number): ")) - 1
                    if 0 <= idx < len(client.written_offsets):
                        offset = client.written_offsets[idx]
                        size = client.test_get_size(offset)
                        if size > 0:
                            print(f"\n[AUTO] Detected {size} bytes at offset {offset}")
                            data = client.test_read(offset, size)
                            if data:
                                print(f"\n📄 Data read:")
                                print(f"  Text: {data.decode('utf-8', errors='ignore')}")
                                print(f"  Hex:  {data.hex()}")
                        else:
                            print("✗ No data found at offset")
                    else:
                        print("❌ Invalid selection")

            elif choice == '3':
                # FLUSH
                client.test_flush()

            elif choice == '4':
                # TRIM
                offset = int(input("Enter offset: "))
                length = int(input("Enter bytes to trim: "))
                client.test_trim(offset, length)

            elif choice == '5':
                # CYCLE
                offset = int(input("Enter offset: "))
                data = input("Enter data to write/read: ")
                client.test_cycle(offset, data)

            elif choice == '6':
                # GET_SIZE
                offset = int(input("Enter offset: "))
                client.test_get_size(offset)

            elif choice == '7':
                # EXIT
                print("Exiting...")
                break

            else:
                print("❌ Invalid choice. Try again.")

        except ValueError as e:
            print(f"❌ Invalid input: {e}")
        except Exception as e:
            print(f"❌ Error: {e}")
            import traceback
            traceback.print_exc()


def main():
    import argparse

    parser = argparse.ArgumentParser(description='TCP Block Device Client')
    parser.add_argument('--host', default='127.0.0.1', help='Server host (default: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=9999, help='Server port (default: 9999)')
    parser.add_argument('--test', choices=['write', 'read', 'flush', 'trim', 'cycle', 'all', 'interactive'],
                       default='interactive', help='Which test to run')

    args = parser.parse_args()

    print("\n" + "="*60)
    print("TCP Block Device Client - LDS Testing")
    print("="*60)
    print(f"Connecting to {args.host}:{args.port}...")

    client = TCPBlockDeviceClient(args.host, args.port)
    client.connect()

    try:
        if args.test == 'interactive':
            # Interactive mode
            interactive_menu(client)

        else:
            # Automated tests
            if args.test == 'all' or args.test == 'write':
                client.test_write(0, "hello")

            if args.test == 'all' or args.test == 'read':
                client.test_read(0, 10)

            if args.test == 'all' or args.test == 'flush':
                client.test_flush()

            if args.test == 'all' or args.test == 'trim':
                client.test_trim(0, 100)

            if args.test == 'all' or args.test == 'cycle':
                client.test_cycle(100, "test data")

            print("\n" + "="*60)
            print("✅ All tests completed")
            print("="*60)

    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

    finally:
        client.disconnect()


if __name__ == '__main__':
    main()
