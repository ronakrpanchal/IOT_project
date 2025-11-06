#!/usr/bin/env python3
"""
Modular UDP relay server with registration and buffering.
"""
import socket
import argparse
import time
from collections import defaultdict, deque

class RelayServer:
    def __init__(self, host, port, max_buffer):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        self.clients = {}
        self.addr_to_key = {}
        self.buffered_messages = defaultdict(lambda: deque(maxlen=max_buffer))
        print(f"UDP relay listening on {self.host}:{self.port}")

    def _addr_key(self, addr):
        return f"{addr[0]}:{addr[1]}"

    def _forward_to(self, addr, data):
        try:
            self.sock.sendto(data, addr)
            return True
        except Exception as e:
            print(f"Failed to forward to {addr}: {e}")
            return False

    def _flush_buffer_for(self, name):
        addr = self.clients.get(name)
        if not addr: return
        q = self.buffered_messages.get(name)
        if not q: return

        print(f"Flushing {len(q)} buffered message(s) to {name} {addr}")
        while q:
            data = q.popleft()
            if self._forward_to(addr, data):
                print(f"  forwarded buffered -> {name}: '{data.decode(errors='replace').strip()}'")
            else:
                print(f"  failed to forward buffered -> {name}, re-buffering")
                q.appendleft(data)
                break

    def _handle_packet(self, data, addr):
        text = data.decode(errors="replace").strip()
        now = time.strftime("%H:%M:%S")
        print(f"[{now}] Received from {addr}: '{text}'")

        src_key = self._addr_key(addr)

        if "REG-A" in text:
            self.clients["A"] = addr
            self.addr_to_key[src_key] = "A"
            print(f"Registered A => {addr}")
            self._flush_buffer_for("A")
            return
        elif "REG-B" in text:
            self.clients["B"] = addr
            self.addr_to_key[src_key] = "B"
            print(f"Registered B => {addr}")
            self._flush_buffer_for("B")
            return

        mapped = self.addr_to_key.get(src_key)
        if not mapped:
            name = f"unknown_{src_key}"
            self.clients[name] = addr
            self.addr_to_key[src_key] = name
            mapped = name
            print(f"Auto-registered {mapped} => {addr}")

        # Forwarding logic
        dest_key = None
        if mapped == "A": dest_key = "B"
        elif mapped == "B": dest_key = "A"
        
        if dest_key:
            dest_addr = self.clients.get(dest_key)
            if dest_addr:
                if self._forward_to(dest_addr, data):
                    print(f"Forwarded from {mapped} -> {dest_key}: '{text}'")
                else:
                    self.buffered_messages[dest_key].append(data)
                    print(f"Failed to send; buffered for {dest_key}")
            else:
                self.buffered_messages[dest_key].append(data)
                print(f"{dest_key} unknown: buffered message for {dest_key}")
        else:
            # Handle 'unknown' sender
            print(f"Packet from unknown client {mapped}, not forwarding.")


    def run(self):
        self.sock.bind((self.host, self.port))
        try:
            while True:
                data, addr = self.sock.recvfrom(4096)
                self._handle_packet(data, addr)
        except KeyboardInterrupt:
            print("\nServer exiting")
        finally:
            self.sock.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="UDP relay for A <-> B")
    parser.add_argument("--host", default="0.0.0.0", help="server bind address")
    parser.add_argument("--port", type=int, default=5683, help="server port to listen on")
    parser.add_argument("--max-buffer", type=int, default=50, help="max buffered messages")
    args = parser.parse_args()

    server = RelayServer(args.host, args.port, args.max_buffer)
    server.run()