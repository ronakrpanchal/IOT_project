#include <WiFi.h>
#include <coap-simple.h>
#include <DHT.h>

#define WIFI_SSID "Wokwi-GUEST"   // Wokwi's built-in WiFi
#define WIFI_PASSWORD ""          // No password

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WiFiUDP udp;
Coap coap(udp);

void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("Response received from CoAP Server");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32 CoAP Client Starting ===");
  
  dht.begin();
  Serial.println("DHT sensor initialized");

  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());

  Serial.println("Starting CoAP client...");
  coap.start();
  coap.response(callback_response);
  Serial.println("✅ CoAP client ready");
}

void loop() {
  Serial.println("\n--- Reading Sensors ---");
  
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  Serial.print("Raw Temperature: ");
  Serial.println(temperature);
  Serial.print("Raw Humidity: ");
  Serial.println(humidity);

  if (!isnan(temperature) && !isnan(humidity)) {
    String payload = "{\"temperature\":" + String(temperature) + 
                     ", \"humidity\":" + String(humidity) + "}";
    
    Serial.println("📦 Payload created: " + payload);
    Serial.print("📡 Sending to CoAP server at ");
    Serial.print("192.168.1.1:5684/sensor/data");
    
    // Send to gateway IP (Wokwi's gateway)
    int result = coap.put(IPAddress(192, 168, 1, 1), 5684, "sensor/data", payload.c_str());
    Serial.print(" | Result: ");
    Serial.println(result);
    
    if (result == 1) {
      Serial.println("✅ CoAP PUT request sent successfully");
    } else {
      Serial.println("❌ CoAP PUT request failed");
    }
  } else {
    Serial.println("❌ Failed to read from DHT sensor");
    Serial.println("Temperature isnan: " + String(isnan(temperature)));
    Serial.println("Humidity isnan: " + String(isnan(humidity)));
  }

  Serial.println("🔄 Processing CoAP responses...");
  coap.loop();
  
  Serial.println("⏳ Waiting 5 seconds...");
  delay(5000);
}