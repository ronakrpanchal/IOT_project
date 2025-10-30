#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi credentials
const char *WIFI_SSID = "Airtel_jimi_2244";
const char *WIFI_PASS = "air80447";

// CoAP server
const char *COAP_SERVER = "192.168.1.7";
const int COAP_PORT = 5683;

// UDP object
WiFiUDP udp;

// Helper: build and send minimal CoAP POST/PUT packet
void sendCoapMessage(const char *path, const char *payload)
{
    // CoAP header bytes
    uint8_t packet[128];
    int i = 0;
    uint16_t message_id = random(0, 65535);

    packet[i++] = 0x40; // Ver=1, Type=0 (Confirmable), Token len=0
    packet[i++] = 0x02; // Code = 0.02 (POST)
    packet[i++] = message_id >> 8;
    packet[i++] = message_id & 0xFF;

    // URI-Path option
    uint8_t option_delta = 11; // URI-Path
    uint8_t option_len = strlen(path);
    packet[i++] = (option_delta << 4) | option_len;
    memcpy(&packet[i], path, option_len);
    i += option_len;

    // Payload marker
    packet[i++] = 0xFF;

    // Payload
    int payload_len = strlen(payload);
    memcpy(&packet[i], payload, payload_len);
    i += payload_len;

    // Send packet
    udp.beginPacket(COAP_SERVER, COAP_PORT);
    udp.write(packet, i);
    udp.endPacket();

    Serial.printf("✅ Sent CoAP message: path='%s', payload='%s'\n", path, payload);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting...");

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    udp.begin(random(10000, 60000));
    Serial.println("UDP started. Sending CoAP messages every 5 seconds...");
}

void loop()
{
    sendCoapMessage("motion", "1"); // example payload
    delay(5000);
}
