from coapthon.server.coap import CoAP
from coapthon.resources.resource import Resource
import time

class SensorResource(Resource):
    def __init__(self, name="sensor"):
        super(SensorResource, self).__init__(name)
        self.payload = "Waiting for data"

    def render_PUT(self, request):
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        print(f"\n[{timestamp}] 📥 CoAP PUT Request Received")
        print(f"From: {request.source}")
        print(f"URI Path: {request.uri_path}")
        print(f"Payload Type: {type(request.payload)}")
        print(f"Payload Length: {len(request.payload) if request.payload else 0}")
        
        self.payload = request.payload
        print(f"✅ Received Data: {self.payload}")
        
        try:
            # Try to decode if it's bytes
            if isinstance(self.payload, bytes):
                decoded = self.payload.decode('utf-8')
                print(f"🔤 Decoded String: {decoded}")
        except Exception as e:
            print(f"⚠️  Could not decode payload: {e}")
        
        print("📤 Sending CoAP response back to client")
        return self

if __name__ == "__main__":
    print("=== CoAP Server Starting ===")
    server = CoAP(("0.0.0.0", 5684))
    server.add_resource("sensor/data/", SensorResource())
    print("🚀 CoAP Server running on 0.0.0.0:5684...")
    print("📍 Listening for data at: /sensor/data/")
    print("🔍 Debug mode enabled - detailed logging active")
    print("-" * 50)
    try:
        server.listen(10)
    except KeyboardInterrupt:
        print("\n🛑 Server Shutdown")
        server.close()