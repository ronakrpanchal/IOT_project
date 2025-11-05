import socket
import time
import sys
# Linux-specific modules for non-blocking keyboard input
import select
import tty
import termios
import atexit

# --- CONFIGURATION ---
# Find this with: ip addr show
HOST = "192.168.1.7" 
PORT = 5683
# ---------------------

# --- Global State ---
device_ips = {}
paused = False
last_value = -1

# Save old terminal settings to restore them on exit
old_termios_settings = termios.tcgetattr(sys.stdin)

def restore_terminal():
    """Restores the terminal to its normal (cooked) mode on exit."""
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_termios_settings)
    print("Terminal restored.")

# Register the restore_terminal function to run when the script exits
atexit.register(restore_terminal)

def build_coap_payload(value_str):
    """Builds a minimal CoAP POST packet with a simple text payload."""
    packet = bytearray()
    packet.append(0x40)  # CoAP header
    packet.append(0x02)  # POST code
    packet.append(0x00)  # Message ID
    packet.append(0x01)  # Message ID
    packet.append(0xFF)  # Payload marker
    packet.extend(value_str.encode('utf-8'))
    return packet

def check_keyboard():
    """Checks for 'q' or 's' keypresses without blocking."""
    global paused
    
    # Check if sys.stdin has data to read (timeout=0 means non-blocking)
    if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
        key = sys.stdin.read(1) # Read one character
        key = key.lower()
        
        if key == 'q':
            paused = True
            print("\n--- PAUSED --- (Press 's' to resume)\n")
        elif key == 's':
            paused = False
            print("\n--- RESUMED ---\n")
        elif key in ('c', '\x03'): # 'c' or Ctrl+C
            print("\nExiting...")
            sys.exit(0) # atexit will handle terminal restore

def start_server():
    global device_ips, paused, last_value

    # Set terminal to "cbreak" mode to read keys instantly
    tty.setcbreak(sys.stdin.fileno())

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((HOST, PORT))
    sock.setblocking(0)  # Non-blocking socket
    print(f"✅ Server listening on {HOST}:{PORT}")
    print("Press 'q' to pause, 's' to resume, 'c' to quit.")

    while True:
        try:
            # 1. Check for keyboard input
            check_keyboard()

            # 2. Try to receive a packet
            try:
                data, addr = sock.recvfrom(1024)
            except BlockingIOError:
                time.sleep(0.01) # Prevent CPU spinning
                continue

            # 3. Find payload
            payload_marker = data.find(b'\xff')
            if payload_marker == -1:
                print(f"Ignoring malformed packet from {addr}")
                continue
            
            raw_payload = data[payload_marker + 1:].decode('utf-8')
            
            # 4. Parse payload
            try:
                device_id, value_str = raw_payload.split(':')
                current_value = int(value_str)
            except Exception as e:
                print(f"Error parsing payload '{raw_payload}': {e}")
                continue

            # 5. Log and store device IP
            if device_id not in device_ips:
                print(f"Discovered device '{device_id}' at {addr}")
                device_ips[device_id] = addr
            
            print(f"Received: {current_value} from '{device_id}'")
            last_value = current_value

            # 6. Determine target and forward if not paused
            if paused:
                print("... PAUSED, not forwarding.")
                continue

            target_device = 'B' if device_id == 'A' else 'A'
            
            if target_device in device_ips:
                next_value = last_value + 1
                coap_packet = build_coap_payload(str(next_value))
                sock.sendto(coap_packet, device_ips[target_device])
                print(f"Sent:     {next_value} to '{target_device}'")
            else:
                print(f"Waiting for '{target_device}' to send its first message...")

        except KeyboardInterrupt:
            print("\nExiting...")
            break
        except Exception as e:
            print(f"An error occurred: {e}")

    sock.close()
    print("Server shut down.")

if __name__ == "__main__":
    if HOST == "YOUR_ARCH_LINUX_IP":
        print("ERROR: Please update YOUR_ARCH_LINUX_IP in the script.")
        print("Find it by running: ip addr show")
    else:
        start_server()