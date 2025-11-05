#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// --- ⬇️ ⬇️ EDIT THESE ⬇️ ⬇️ ---
const char *WIFI_SSID = "Jimit Chavda";
const char *WIFI_PASSWORD = "heilschindler";
const char *SERVER_IP = "10.240.48.131"; // Your PC's IP
const char *DEVICE_ID = "A";
// --- ⬆️ ⬆️ EDIT THESE ⬆️ ⬇️ ---

const int SERVER_PORT = 5683;
const unsigned long RETRY_TIMEOUT = 2000; // 2 seconds
const unsigned long SEND_DELAY = 500;     // 0.5 seconds

WiFiUDP udp;
int myValue = 0;
bool waiting_for_ack = false;
unsigned long last_sent_time = 0;
String last_payload_sent = "";

// Builds and sends the raw CoAP packet
void sendCoapMessage(const char *payload)
{
    char buffer[128];
    int offset = 0;
    buffer[offset++] = 0x40;
    buffer[offset++] = 0x02;
    buffer[offset++] = random(0, 255);
    buffer[offset++] = random(0, 255);
    buffer[offset++] = 0xFF; // Payload marker
    strcpy(&buffer[offset], payload);
    offset += strlen(payload);

    udp.beginPacket(SERVER_IP, SERVER_PORT);
    udp.write((uint8_t *)buffer, offset);
    udp.endPacket();
}

void sendMyValue()
{
    last_payload_sent = String(DEVICE_ID) + ":" + String(myValue);
    Serial.printf("➡️  Sending: '%s'\n", last_payload_sent.c_str());
    sendCoapMessage(last_payload_sent.c_str());
    waiting_for_ack = true;
    last_sent_time = millis();
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("--- Starting Device A ---");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
    udp.begin(4210);
    Serial.println("UDP listener started.");

    // --- KICK OFF THE PROCESS ---
    Serial.println("--- Sending initial packet (0) ---");
    sendMyValue();
}

void loop()
{
    // 1. Check for incoming packets
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        char packetBuffer[255];
        int len = udp.read(packetBuffer, 254);
        packetBuffer[len] = 0;

        char *payload = strstr(packetBuffer, "\xff");
        if (payload)
        {
            payload++; // Move past the 0xFF marker
            String payloadStr = String(payload);

            // Check if it's an ACK for our last sent value
            String expected_ack = "ACK:" + String(myValue);
            if (waiting_for_ack && payloadStr.equals(expected_ack))
            {
                Serial.printf("✅ Got ACK for %d\n", myValue);
                waiting_for_ack = false;
                last_payload_sent = ""; // Clear buffer

                // Check if it's a new value from the server
            }
            else if (!payloadStr.startsWith("ACK:") && !payloadStr.startsWith(DEVICE_ID))
            {
                Serial.printf("⬇️  Got new value: '%s'\n", payloadStr.c_str());
                int new_val = payloadStr.toInt();
                myValue = new_val;

                // Send ACK for this new value
                String ackPayload = "ACK:" + String(myValue);
                Serial.printf("⬅️  Sending ACK: '%s'\n", ackPayload.c_str());
                sendCoapMessage(ackPayload.c_str());

                // Wait 0.5s, then send the *next* value
                delay(SEND_DELAY);
                myValue++;
                sendMyValue(); // This sends "A:(myValue)"
            }
        }
    }

    // 2. Check for retry
    if (waiting_for_ack && (millis() - last_sent_time > RETRY_TIMEOUT))
    {
        Serial.printf("🔁 RETRY: Sending '%s' again...\n", last_payload_sent.c_str());
        sendCoapMessage(last_payload_sent.c_str()); // Resend the last packet
        last_sent_time = millis();                  // Reset the retry timer
    }
}