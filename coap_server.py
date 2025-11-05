import socket
import time
import sys
import select
import tty
import termios
import atexit

# --- CONFIGURATION ---
HOST = "10.240.48.131"  # Your Arch Linux IP
PORT = 5683
RETRY_TIMEOUT = 2.0  # (seconds) Resend packets after this long
# ---------------------

# --- Global State ---
device_state = {
    'A': {'addr': None, 'waiting_for_ack': False, 'last_sent_val': None, 'last_sent_time': 0, 'last_received_val': None},
    'B': {'addr': None, 'waiting_for_ack': False, 'last_sent_val': None, 'last_sent_time': 0, 'last_received_val': None}
}
paused = False
global_value = 0 # The master number in the sequence

# --- Terminal Setup (for Linux) ---
old_termios_settings = termios.tcgetattr(sys.stdin)
def restore_terminal():
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_termios_settings)
atexit.register(restore_terminal)
# ---------------------------------

def build_coap_payload(payload_str):
    """Builds a minimal CoAP POST packet."""
    packet = bytearray()
    packet.append(0x40)
    packet.append(0x02)
    packet.append(0x00)
    packet.append(0x01)
    packet.append(0xFF)
    packet.extend(payload_str.encode('utf-8'))
    return packet

def check_keyboard():
    """Checks for 'q' or 's' keypresses."""
    global paused
    if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
        key = sys.stdin.read(1).lower()
        if key == 'q':
            paused = True
            print("\n--- PAUSED --- (Press 's' to resume)\n")
        elif key == 's':
            paused = False
            print("\n--- RESUMED ---\n")
            # If we resume, we might need to re-send the last value
            # This logic is handled in the main loop's retry check
        elif key in ('c', '\x03'):
            print("\nExiting...")
            sys.exit(0)

def handle_packet(data, addr, sock):
    """Main logic for processing a new packet."""
    global global_value
    
    payload_marker = data.find(b'\xff')
    if payload_marker == -1: return

    raw_payload = data[payload_marker + 1:].decode('utf-8')
    
    # 1. Is this an ACK? (e.g., "ACK:1")
    if raw_payload.startswith("ACK:"):
        try:
            ack_val = int(raw_payload.split(':')[1])
            device_id = 'A' if addr == device_state['A']['addr'] else 'B'
            if device_state[device_id]['waiting_for_ack'] and device_state[device_id]['last_sent_val'] == ack_val:
                print(f"✅ Got ACK for {ack_val} from '{device_id}'")
                device_state[device_id]['waiting_for_ack'] = False
            else:
                 print(f"⚠️ Got STALE ACK for {ack_val} from '{device_id}' (ignoring)")
        except Exception as e:
            print(f"Error parsing ACK: {e}")
        return # ACKs don't trigger a new value

    # 2. Is this a new value packet? (e.g., "A:0" or "B:2")
    if ':' in raw_payload:
        try:
            device_id, value_str = raw_payload.split(':')
            received_val = int(value_str)
            
            # Update device address if it's new
            if device_state[device_id]['addr'] != addr:
                print(f"Discovered device '{device_id}' at {addr}")
                device_state[device_id]['addr'] = addr
            
            # Check for duplicate packets
            if device_state[device_id]['last_received_val'] == received_val:
                print(f"♻️ Got duplicate {received_val} from '{device_id}'. Sending ACK again.")
                ack_payload = f"ACK:{received_val}"
                sock.sendto(build_coap_payload(ack_payload), addr)
                return # Don't process this duplicate further
            
            # This is a new, valid packet
            print(f"➡️  Got {received_val} from '{device_id}'")
            device_state[device_id]['last_received_val'] = received_val
            global_value = received_val
            
            # Send an ACK back immediately
            ack_payload = f"ACK:{received_val}"
            sock.sendto(build_coap_payload(ack_payload), addr)
            print(f"⬅️  Sent ACK:{received_val} to '{device_id}'")

            # --- This is the main "ping-pong" logic ---
            if paused:
                print("... PAUSED, not forwarding.")
                return

            # Determine the *next* device
            target_device = 'B' if device_id == 'A' else 'A'
            if device_state[target_device]['addr'] is None:
                print(f"Waiting for '{target_device}' to connect...")
                return

            # Prepare the new value to send
            next_value = global_value + 1
            global_value = next_value # Increment the global value
            
            print(f"⬆️  Sending {next_value} to '{target_device}'...")
            
            # Set state for the target device
            state = device_state[target_device]
            state['waiting_for_ack'] = True
            state['last_sent_val'] = next_value
            state['last_sent_time'] = time.time()
            
            # Send the packet
            payload_to_send = str(next_value)
            sock.sendto(build_coap_payload(payload_to_send), state['addr'])

        except Exception as e:
            print(f"Error parsing data packet: {e}")

def check_retries(sock):
    """Checks if any packets need to be re-sent."""
    if paused: return # Don't retry if paused
    
    current_time = time.time()
    for device_id in ['A', 'B']:
        state = device_state[device_id]
        
        if state['waiting_for_ack'] and (current_time - state['last_sent_time'] > RETRY_TIMEOUT):
            if state['addr'] is None: continue # Can't retry if we don't know the IP
            
            print(f"🔁 RETRY: Sending {state['last_sent_val']} to '{device_id}' again...")
            
            # Resend the *same* value
            payload_to_send = str(state['last_sent_val'])
            sock.sendto(build_coap_payload(payload_to_send), state['addr'])
            
            # Update the timestamp
            state['last_sent_time'] = current_time

def start_server():
    tty.setcbreak(sys.stdin.fileno())
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((HOST, PORT))
    sock.setblocking(0)
    print(f"✅ Server listening on {HOST}:{PORT}")
    print("Press 'q' to pause, 's' to resume, 'c' to quit.")

    while True:
        try:
            check_keyboard()
            check_retries(sock)
            
            try:
                data, addr = sock.recvfrom(1024)
                handle_packet(data, addr, sock)
            except BlockingIOError:
                pass # No data, loop
            
            # Main loop delay
            time.sleep(0.01) # Small sleep to be responsive

        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"CRITICAL ERROR: {e}")
            break
    
    sock.close()
    print("Server shut down.")

if __name__ == "__main__":
    start_server()