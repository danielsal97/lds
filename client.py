#!/usr/bin/env python3
"""
TCP Client with AUTO-RECONNECT for testing TCPDriverComm
Automatically reconnects if server crashes
"""

import socket
import struct
import sys
import time

READ = 0
WRITE = 1
DISCONNECT = 2
FLUSH = 3
TRIM = 4
GET_SIZE = 5
LIST_OFFSETS = 6

class TCPBlockDeviceClient:
    def __init__(self, host='192.168.68.62', port=9999, max_retries=10, retry_delay=2):
        self.host = host
        self.port = port
        self.socket = None
        self.written_offsets = []
        self.max_retries = max_retries
        self.retry_delay = retry_delay

    def connect_with_retry(self):
        """Connect with automatic retry on failure"""
        for attempt in range(self.max_retries):
            try:
                self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.socket.connect((self.host, self.port))
                print(f"   ✓ Connected to {self.host}:{self.port}")
                self.discover_offsets()
                return True
            except (ConnectionRefusedError, ConnectionResetError, OSError) as e:
                if attempt < self.max_retries - 1:
                    print(f"   ✗ Attempt {attempt + 1} failed, retrying in {self.retry_delay}s...")
                if self.socket:
                    try:
                        self.socket.close()
                    except:
                        pass
                    self.socket = None

                if attempt < self.max_retries - 1:
                    time.sleep(self.retry_delay)

        print(f"   ✗ Failed after {self.max_retries} attempts")
        return False

    def connect(self):
        """Connect to server (wrapper for compatibility)"""
        return self.connect_with_retry()

    def discover_offsets(self):
        """Query server for existing offsets"""
        try:
            self.socket.settimeout(2.0)
            handle = int(time.time() * 1000) % 1000000
            header = struct.pack('>I Q Q I', LIST_OFFSETS, handle, 0, 0)
            self.socket.sendall(header)

            reply_header = self.socket.recv(16)
            if reply_header and len(reply_header) == 16:
                error, ret_handle, data_len = struct.unpack('>I Q I', reply_header)
                if error == 0 and data_len > 0:
                    data = self.socket.recv(data_len)
                    for i in range(0, len(data), 16):
                        if i + 16 <= len(data):
                            offset = int.from_bytes(data[i:i+8], 'big')
                            if offset not in self.written_offsets:
                                self.written_offsets.append(offset)
                    self.written_offsets.sort()
                    if self.written_offsets:
                        print(f"✓ Discovered {len(self.written_offsets)} previous offsets: {self.written_offsets}")

            self.socket.settimeout(None)
        except:
            if self.socket:
                self.socket.settimeout(None)

    def disconnect(self):
        """Close connection"""
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            print("✓ Disconnected")

    def send_request_with_reconnect(self, req_type, handle, offset, length, data=None):
        """Send request, auto-reconnect if broken"""
        try:
            self.send_request(req_type, handle, offset, length, data)
            return True
        except (BrokenPipeError, ConnectionResetError, OSError) as e:
            print("\n" + "="*60)
            print("🚨 SERVER CRASHED - CONNECTION LOST!")
            print("="*60)
            print(f"Error: {e}")
            print("\n⏳ Attempting automatic reconnection...")
            print(f"   Retrying up to {self.max_retries} times with {self.retry_delay}s delay...")
            self.disconnect()

            if self.connect_with_retry():
                print("\n" + "="*60)
                print("✅ SERVER RECOVERED - RECONNECTED!")
                print("="*60)
                print("Retrying operation...")
                self.send_request(req_type, handle, offset, length, data)
                return True
            else:
                print("\n" + "="*60)
                print("❌ FATAL: Could not reconnect to server after all retries")
                print("="*60)
                return False

    def send_request(self, req_type, handle, offset, length, data=None):
        """Send a request to the server"""
        header = struct.pack('>I Q Q I', req_type, handle, offset, length)
        type_name = {READ: 'READ', WRITE: 'WRITE', FLUSH: 'FLUSH', TRIM: 'TRIM',
                     DISCONNECT: 'DISCONNECT', GET_SIZE: 'GET_SIZE'}
        print(f"\n[→ Send] {type_name.get(req_type, 'UNKNOWN')} handle={handle} offset={offset} len={length}")

        self.socket.sendall(header)
        print(f"  Header sent: {len(header)} bytes")

        if req_type == WRITE and data:
            if isinstance(data, str):
                data = data.encode()
            self.socket.sendall(data)
            print(f"  Payload sent: {data}")

    def recv_reply(self):
        """Receive reply from server"""
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
        """Receive payload data"""
        if length == 0:
            return b""
        payload = self.socket.recv(length)
        print(f"  Payload received: {len(payload)} bytes")
        return payload

    def test_write(self, offset, data):
        """Test WRITE operation"""
        if isinstance(data, str):
            data_bytes = data.encode()
        else:
            data_bytes = data

        handle = int(time.time() * 1000) % 1000000

        if not self.send_request_with_reconnect(WRITE, handle, offset, len(data_bytes), data_bytes):
            return

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
            if offset not in self.written_offsets:
                self.written_offsets.append(offset)
                self.written_offsets.sort()

    def test_read(self, offset, length):
        """Test READ operation"""
        handle = int(time.time() * 1000) % 1000000

        if not self.send_request_with_reconnect(READ, handle, offset, length):
            return None

        try:
            self.socket.settimeout(5.0)
            error, ret_handle, status, _ = self.recv_reply()
            self.socket.settimeout(None)
        except socket.timeout:
            self.socket.settimeout(None)
            print("✗ READ timeout")
            return None

        if error == 0:
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
        handle = int(time.time() * 1000) % 1000000
        if self.send_request_with_reconnect(FLUSH, handle, 0, 0):
            try:
                self.socket.settimeout(5.0)
                error, ret_handle, status, _ = self.recv_reply()
                self.socket.settimeout(None)
            except socket.timeout:
                self.socket.settimeout(None)

    def test_get_size(self, offset):
        """Test GET_SIZE operation"""
        handle = int(time.time() * 1000) % 1000000

        if not self.send_request_with_reconnect(GET_SIZE, handle, offset, 0):
            return 0

        try:
            self.socket.settimeout(5.0)
            error, ret_handle, status, size = self.recv_reply()
            self.socket.settimeout(None)
        except socket.timeout:
            self.socket.settimeout(None)
            return 0

        if error == 0:
            print(f"  Data size at offset {offset}: {size} bytes")
            return size
        return 0

    def test_cycle(self, offset, data):
        """Test WRITE then READ"""
        if isinstance(data, str):
            data_bytes = data.encode()
        else:
            data_bytes = data

        write_handle = 100
        print(f"\n[Step 1] Write at offset {offset}")
        if not self.send_request_with_reconnect(WRITE, write_handle, offset, len(data_bytes), data_bytes):
            return

        error, _, _, _ = self.recv_reply()
        time.sleep(0.1)

        read_handle = 101
        print(f"\n[Step 2] Read from offset {offset}")
        if not self.send_request_with_reconnect(READ, read_handle, offset, len(data_bytes)):
            return

        error, _, status, reply_len = self.recv_reply()
        if error == 0:
            payload = self.recv_payload(reply_len)
            match = payload == data_bytes
            print(f"\n[Result] Data match: {'✓ YES' if match else '✗ NO'}")

def interactive_menu(client):
    """Interactive menu with auto-reconnect"""
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
                offset = int(input("Enter offset: "))
                data = input("Enter data to write: ")
                client.test_write(offset, data)

            elif choice == '2':
                if client.written_offsets:
                    print("\nAvailable offsets:")
                    for i, off in enumerate(client.written_offsets):
                        print(f"  {i+1}. Offset {off}")
                    print("  M. Manual offset")
                    selection = input("Select option: ").strip()

                    if selection.lower() == 'm':
                        offset = int(input("Enter offset: "))
                    else:
                        offset = client.written_offsets[int(selection) - 1]
                else:
                    offset = int(input("Enter offset manually: "))

                size = client.test_get_size(offset)
                if size > 0:
                    data = client.test_read(offset, size)
                    if data:
                        print("\n📄 Data read:")
                        print(f"  Text: {data.decode('utf-8', errors='ignore')}")
                        print(f"  Hex:  {data.hex()}")

            elif choice == '3':
                client.test_flush()

            elif choice == '4':
                offset = int(input("Enter offset: "))
                length = int(input("Enter bytes to trim: "))
                client.test_trim(offset, length)

            elif choice == '5':
                offset = int(input("Enter offset: "))
                data = input("Enter data: ")
                client.test_cycle(offset, data)

            elif choice == '6':
                offset = int(input("Enter offset: "))
                client.test_get_size(offset)

            elif choice == '7':
                break

        except Exception as e:
            print(f"❌ Error: {e}")

def main():
    print("\n" + "="*60)
    print("TCP Block Device Client - AUTO-RECONNECT")
    print("="*60)

    client = TCPBlockDeviceClient(max_retries=10, retry_delay=2)
    print("⏳ Connecting with auto-retry (10 attempts, 2s delay)...")

    if client.connect_with_retry():
        try:
            interactive_menu(client)
        finally:
            client.disconnect()
    else:
        print("✗ Could not connect to server")

if __name__ == '__main__':
    main()
