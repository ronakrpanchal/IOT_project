#!/usr/bin/env python3
"""
udp_relay.py  — simple UDP relay with registration and buffering.
"""
import socket
import argparse
import time
from collections import defaultdict, deque

parser = argparse.ArgumentParser(description="UDP relay for A <-> B (registration + buffering)")
parser.add_argument("--host", default="0.0.0.0", help="server bind address (your laptop IP)")
parser.add_argument("--port", type=int, default=5683, help="server port to listen on")
parser.add_argument("--max-buffer", type=int, default=50, help="max buffered messages per destination")
args = parser.parse_args()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((args.host, args.port))
print(f"UDP relay listening on {args.host}:{args.port}")

# clients: mapping logical name -> (ip, port)
clients = {}
# reverse mapping "ip:port" -> name
addr_to_key = {}

# buffered_messages[name] = deque([bytes, bytes, ...])
buffered_messages = defaultdict(lambda: deque(maxlen=args.max_buffer))

def addr_key(addr):
    return f"{addr[0]}:{addr[1]}"

def forward_to(addr, data):
    try:
        sock.sendto(data, addr)
        return True
    except Exception as e:
        print(f"Failed to forward to {addr}: {e}")
        return False

def flush_buffer_for(name):
    addr = clients.get(name)
    if not addr:
        return
    q = buffered_messages.get(name)
    if not q:
        return
    print(f"Flushing {len(q)} buffered message(s) to {name} {addr}")
    while q:
        data = q.popleft()
        success = forward_to(addr, data)
        if success:
            print(f"  forwarded buffered -> {name}: '{data.decode(errors='replace').strip()}'")
        else:
            print(f"  failed to forward buffered -> {name}, re-buffering")
            q.appendleft(data)
            break  # stop trying if network error

try:
    while True:
        data, addr = sock.recvfrom(4096)
        # We don't decode for forwarding, just for logging
        text = data.decode(errors="replace").strip()
        now = time.strftime("%H:%M:%S")
        print(f"[{now}] Received from {addr}: '{text}'")

        src_key = addr_key(addr)

        # handle explicit registrations
        # The CoAP library will send a "PUT" packet. We'll look for the payload.
        if "REG-A" in text:
            clients["A"] = addr
            addr_to_key[src_key] = "A"
            print(f"Registered A => {addr}")
            flush_buffer_for("A")
            continue
        elif "REG-B" in text:
            clients["B"] = addr
            addr_to_key[src_key] = "B"
            print(f"Registered B => {addr}")
            flush_buffer_for("B")
            continue

        # If sender already known, get logical name
        mapped = addr_to_key.get(src_key)
        if not mapped:
            name = f"unknown_{src_key}"
            clients[name] = addr
            addr_to_key[src_key] = name
            mapped = name
            print(f"Auto-registered {mapped} => {addr}")

        # Decide destination(s) and forward or buffer
        forwarded = False
        if mapped == "A":
            dest = clients.get("B")
            if dest:
                if forward_to(dest, data):
                    print(f"Forwarded from A -> B: '{text}' -> {dest}")
                    forwarded = True
                else:
                    buffered_messages["B"].append(data)
                    print(f"Failed to send; buffered for B ({len(buffered_messages['B'])})")
            else:
                buffered_messages["B"].append(data)
                print(f"B unknown: buffered message for B (size={len(buffered_messages['B'])})")
        elif mapped == "B":
            dest = clients.get("A")
            if dest:
                if forward_to(dest, data):
                    print(f"Forwarded from B -> A: '{text}' -> {dest}")
                    forwarded = True
                else:
                    buffered_messages["A"].append(data)
                    print(f"Failed to send; buffered for A ({len(buffered_messages['A'])})")
            else:
                buffered_messages["A"].append(data)
                print(f"A unknown: buffered message for A (size={len(buffered_messages['A'])})")
        else:
            # sender is an 'unknown_...' auto-registered client
            others = [k for k in clients.keys() if k != mapped]
            if not others:
                buffered_messages["A"].append(data)
                buffered_messages["B"].append(data)
                print(f"No other clients: buffered message for A and B")
            else:
                for k in others:
                    addr_dest = clients[k]
                    if forward_to(addr_dest, data):
                        print(f"Forwarded from {mapped} -> {k}: '{text}' -> {addr_dest}")
                        forwarded = True
                    else:
                        buffered_messages[k].append(data)
                        print(f"Failed to send to {k}; buffered (size={len(buffered_messages[k])})")

except KeyboardInterrupt:
    print("\nServer exiting")
finally:
    sock.close()