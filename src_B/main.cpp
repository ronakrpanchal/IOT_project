#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// --- ⬇️ ⬇️ EDIT THESE ⬇️ ⬇️ ---
const char *WIFI_SSID = "Airtel_jimi_2244";
const char *WIFI_PASSWORD = "air80447";
const char *SERVER_IP = "192.168.1.7"; // Your Laptop's ETHERNET IP
// --- ⬆️ ⬆️ EDIT THESE ⬆️ ⬆️ ---

const int SERVER_PORT = 5683;
const int LOCAL_PORT = 4211; // Listen on a different port

WiFiUDP udp;
char packetBuffer[255];

void sendRaw(const char *txt)
{
    udp.beginPacket(SERVER_IP, SERVER_PORT);
    udp.write((const uint8_t *)txt, strlen(txt));
    udp.endPacket();
    Serial.printf("Sent to server: %s\n", txt);
}

void sendNumber(int num)
{
    char msg[32];
    int n = snprintf(msg, sizeof(msg), "%d", num);
    udp.beginPacket(SERVER_IP, SERVER_PORT);
    udp.write((uint8_t *)msg, n);
    udp.endPacket();
    Serial.printf("Sent number to server: %s\n", msg);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("--- Starting Device B (PING-B) ---");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
    udp.begin(LOCAL_PORT);
    Serial.printf("UDP listener started on port %d\n", LOCAL_PORT);

    // Send a registration message so server learns our IP:port immediately
    delay(200);
    sendRaw("REG-B");
}

void loop()
{
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
        packetBuffer[len] = 0;
        Serial.printf("Received from server: '%s'\n", packetBuffer);

        // parse number, increment and send back to server
        int received = atoi(packetBuffer);
        int toSend = received + 1;

        delay(100); // small pause
        sendNumber(toSend);
    }

    delay(50);
}
