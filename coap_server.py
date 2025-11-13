import socket
import time

# --- CONFIGURATION ---
HOST = "0.0.0.0"  # Listen on all interfaces
PORT = 5683
# ---------------------

print(f"✅ Clean CoAP Receiver listening on {HOST}:{PORT}")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((HOST, PORT))

try:
    while True:
        data, addr = sock.recvfrom(1024)
        
        # --- This is the fix ---
        # A CoAP packet's payload comes after a 0xFF byte
        payload_marker = data.find(b'\xff')
        
        if payload_marker != -1:
            # Extract *only* the payload
            payload_bytes = data[payload_marker + 1:]
            
            try:
                # Decode the payload as text
                payload_text = payload_bytes.decode('utf-8')
                
                # Print the clean message
                print(f"[{time.strftime('%H:%M:%S')}] Received from {addr[0]}: {payload_text}")
                
                # Send an ACK (a simple "ACK" string)
                # The CoAP library on the ESP32 will see this
                sock.sendto(b"ACK", addr)
                
            except Exception as e:
                print(f"Error decoding packet: {e}")
        else:
            print(f"Received unknown packet from {addr}")

except KeyboardInterrupt:
    print("\nServer shutting down.")
finally:
    sock.close()