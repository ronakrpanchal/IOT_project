#!/usr/bin/env python3
"""
udp_relay.py  — simple UDP relay with registration and buffering.

Behavior:
- Clients send "REG-A" or "REG-B" to register themselves.
- When a packet arrives and its intended recipient isn't registered yet,
  the server buffers the packet and forwards it once the recipient registers.
- If a sender is unknown, we auto-register them as "unknown_<ip:port>" and
  forward to all other known clients (or buffer if none known).

Run:
    python udp_relay.py --host 0.0.0.0 --port 5683
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
        text = data.decode(errors="replace").strip()
        now = time.strftime("%H:%M:%S")
        print(f"[{now}] Received from {addr}: '{text}'")

        src_key = addr_key(addr)

        # handle explicit registrations
        if text == "REG-A":
            clients["A"] = addr
            addr_to_key[src_key] = "A"
            print(f"Registered A => {addr}")
            # flush any buffered messages for A
            flush_buffer_for("A")
            continue
        elif text == "REG-B":
            clients["B"] = addr
            addr_to_key[src_key] = "B"
            print(f"Registered B => {addr}")
            # flush any buffered messages for B
            flush_buffer_for("B")
            continue

        # If sender already known, get logical name
        mapped = addr_to_key.get(src_key)
        if not mapped:
            # auto-register unknown sender as unknown_<ip:port>
            name = f"unknown_{src_key}"
            clients[name] = addr
            addr_to_key[src_key] = name
            mapped = name
            print(f"Auto-registered {mapped} => {addr}")

        # Decide destination(s) and forward or buffer
        forwarded = False
        if mapped == "A":
            # packet from A -> deliver to B (if known) else buffer for B
            dest = clients.get("B")
            if dest:
                if forward_to(dest, data):
                    print(f"Forwarded from A -> B: '{text}' -> {dest}")
                    forwarded = True
                else:
                    # network send failed -> buffer for B
                    buffered_messages["B"].append(data)
                    print(f"Failed to send; buffered for B ({len(buffered_messages['B'])})")
            else:
                # B unknown -> buffer
                buffered_messages["B"].append(data)
                print(f"B unknown: buffered message for B (size={len(buffered_messages['B'])})")
        elif mapped == "B":
            # packet from B -> deliver to A (if known) else buffer for A
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
            # forward to all other clients (A and B if they exist). If none exist, buffer to both.
            others = [k for k in clients.keys() if k != mapped]
            if not others:
                # nobody else known yet: buffer to 'A' and 'B' queues so they'll be delivered when those register
                buffered_messages["A"].append(data)
                buffered_messages["B"].append(data)
                print(f"No other clients: buffered message for A and B (A={len(buffered_messages['A'])}, B={len(buffered_messages['B'])})")
            else:
                for k in others:
                    addr_dest = clients[k]
                    if forward_to(addr_dest, data):
                        print(f"Forwarded from {mapped} -> {k}: '{text}' -> {addr_dest}")
                        forwarded = True
                    else:
                        buffered_messages[k].append(data)
                        print(f"Failed to send to {k}; buffered (size={len(buffered_messages[k])})")

        if not forwarded:
            # just a friendly log (messages may be buffered)
            print("No immediate forward performed for this packet (it may be buffered).")

except KeyboardInterrupt:
    print("\nServer exiting")
finally:
    sock.close()
