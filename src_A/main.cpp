#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// --- ⬇️ ⬇️ EDIT THESE ⬇️ ⬇️ ---
const char *WIFI_SSID = "Airtel_jimi_2244";
const char *WIFI_PASSWORD = "air80447";
const char *SERVER_IP = "192.168.1.7"; // Your Laptop's ETHERNET IP
// --- ⬆️ ⬆️ EDIT THESE ⬆️ ⬆️ ---

const int SERVER_PORT = 5683;
const int LOCAL_PORT = 4210; // Listen on this port

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

    Serial.println("--- Starting Device A (PING-A) ---");
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
    sendRaw("REG-A");

    // Small delay then send initial number 0 to kick off the loop
    delay(500);
    sendNumber(0);
}

void loop()
{
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
        packetBuffer[len] = 0;
        Serial.printf("Received from server: '%s'\n", packetBuffer);

        // try parse as number
        int received = atoi(packetBuffer);

        // resend the same number back to server (so B will receive it next)
        delay(100); // small pause to avoid tight loop
        sendNumber(received);
    }

    delay(50);
}
