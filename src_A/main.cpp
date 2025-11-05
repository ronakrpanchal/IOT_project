#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// --- ⬇️ ⬇️ EDIT THESE ⬇️ ⬇️ ---
const char *WIFI_SSID = "Airtel_jimi_2244";
const char *WIFI_PASSWORD = "air80447";
const char *SERVER_IP = "192.168.1.7"; // Your PC's IP
const char *DEVICE_ID = "A";           // This is ESP32 "A"
// --- ⬆️ ⬆️ EDIT THESE ⬆️ ⬇️ ---

const int SERVER_PORT = 5683;
WiFiUDP udp;
int myValue = 0;

void sendCoapMessage(const char *path, const char *payload)
{
    char buffer[128];
    int offset = 0;

    buffer[offset++] = 0x40;           // Version=1, Type=CON, Token len=0
    buffer[offset++] = 0x02;           // Code=POST
    buffer[offset++] = random(0, 255); // Message ID MSB (random)
    buffer[offset++] = random(0, 255); // Message ID LSB (random)
    buffer[offset++] = 0xB4;           // Delta: Uri-Path, Len: 4
    strcpy(&buffer[offset], path);
    offset += strlen(path);
    buffer[offset++] = 0xFF; // Payload marker
    strcpy(&buffer[offset], payload);
    offset += strlen(payload);

    Serial.println("Sending packet...");
    udp.beginPacket(SERVER_IP, SERVER_PORT);
    udp.write((uint8_t *)buffer, offset);
    udp.endPacket();
    Serial.printf("✅ Sent to server: '%s'\n", payload);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("--- Starting Device A ---");

    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    udp.begin(SERVER_PORT);
    Serial.println("UDP listener started.");

    Serial.println("--- Sending initial packet ---");
    String initialPayload = String(DEVICE_ID) + ":" + String(myValue);
    sendCoapMessage("data", initialPayload.c_str());
}

void loop()
{
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        Serial.print("Received packet! Size: ");
        Serial.println(packetSize);

        char packetBuffer[255];
        int len = udp.read(packetBuffer, 254);
        packetBuffer[len] = 0;

        char *payload = strstr(packetBuffer, "\xff");
        if (payload)
        {
            payload++; // Move past the 0xFF marker
            Serial.println("Parsing payload...");

            myValue = atoi(payload);
            Serial.printf("Got new value: %d\n", myValue);

            Serial.println("Incrementing value...");
            myValue++;

            Serial.println("Waiting 1 second...");
            delay(1000);

            String payloadStr = String(DEVICE_ID) + ":" + String(myValue);
            sendCoapMessage("data", payloadStr.c_str());
        }
        else
        {
            Serial.println("Error: Received packet, but no payload marker (0xFF) found.");
        }
    }
    // Add this delay so your "listening" logs don't spam the console
    delay(100);
}