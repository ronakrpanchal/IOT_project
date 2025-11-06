#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

// --- CONFIGURATION ---
const char *WIFI_SSID = "Airtel_jimi_2244";
const char *WIFI_PASSWORD = "air80447";
IPAddress serverIp(192, 168, 1, 7); // Your Laptop's ETHERNET IP
// ---

const int SERVER_PORT = 5683;
const int LOCAL_PORT = 4210; // Listen on this port

WiFiUDP udp;
Coap coap(udp);

// --- NEW GLOBAL VARIABLES ---
volatile bool newPacketWaiting = false; // 'volatile' is important
volatile int lastValueReceived = 0;
// ---

// This is our SERVER logic: handles packets from Device B (via relay)
void data_callback(CoapPacket &packet, IPAddress ip, int port)
{
    Serial.println("Received CoAP packet...");

    // 1. Get payload
    char p[packet.payloadlen + 1];
    memcpy(p, packet.payload, packet.payloadlen);
    p[packet.payloadlen] = '\0';
    String payloadStr = String(p);
    Serial.printf("Got value: '%s'\n", payloadStr.c_str());

    // 2. Send an ACK back to the sender (Device B)
    // This is fast and non-blocking, so it's OK here.
    coap.sendResponse(ip, port, packet.messageid);

    // 3. Set flags for the main loop() to handle
    lastValueReceived = payloadStr.toInt();
    newPacketWaiting = true;

    // DO NOT delay() or coap.put() here!
}

// This is our CLIENT logic: handles ACKs for packets *we* sent
void response_callback(CoapPacket &packet, IPAddress ip, int port)
{
    Serial.println("Got CoAP response (ACK)");
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("--- Starting Device A (CoAP) ---");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());

    coap.server(data_callback, "data"); // Our server logic
    coap.response(response_callback);   // Our client logic
    coap.start(LOCAL_PORT);

    Serial.println("Registering with relay...");
    coap.put(serverIp, SERVER_PORT, "reg", "REG-A");
    delay(500);

    Serial.println("Sending initial value: 0");
    coap.put(serverIp, SERVER_PORT, "data", "0");
}

void loop()
{
    // coap.loop() MUST be called to receive packets
    coap.loop();

    // --- This is our new "work" section ---
    if (newPacketWaiting)
    {
        newPacketWaiting = false; // Clear the flag

        // 3. Increment the value
        int val_to_send = lastValueReceived + 1;
        Serial.printf("Incrementing %d -> %d\n", lastValueReceived, val_to_send);

        // 4. Delay (This is SAFE in the main loop)
        delay(500);

        // 5. Send the new value (as a CLIENT)
        String newPayload = String(val_to_send);
        Serial.printf("Sending new value: '%s'\n", newPayload.c_str());
        coap.put(serverIp, SERVER_PORT, "data", newPayload.c_str());
    }
}