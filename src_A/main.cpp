#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "lib/ProximitySensor/ProximitySensor.h" // <-- Include our new sensor library

// --- 1. CONFIGURATION ---
// These values are injected from platformio.ini (which read .env)
const char *WIFI_SSID_STR = WIFI_SSID;
const char *WIFI_PASS_STR = WIFI_PASS;
const char *SERVER_IP_STR = SERVER_IP;
IPAddress serverIp; // IPAddress object for CoAP

const int SERVER_PORT = 5683;
const int LOCAL_PORT = 4210;
const int ALERT_THRESHOLD_CM = 100;
const long ALERT_COOLDOWN_MS = 100; // 5 seconds

// --- 2. HARDWARE SETUP ---
const int TRIG_PIN = 27;
const int ECHO_PIN = 26;
ProximitySensor sensor(TRIG_PIN, ECHO_PIN); // Create our sensor

// --- 3. COAP & WIFI OBJECTS ---
WiFiUDP udp;
Coap coap(udp);
unsigned long lastAlertTime = 0; // For cooldown

// This callback just handles the ACK from the server
void response_callback(CoapPacket &packet, IPAddress ip, int port)
{
    Serial.println("Got CoAP response (ACK)");
}

// --- 4. SETUP ---
void setup()
{
    Serial.begin(115200);
    delay(100);
    Serial.println("--- Starting Proximity Alerter ---");

    // Convert server IP string to IPAddress object
    serverIp.fromString(SERVER_IP_STR);

    // Start the sensor
    sensor.begin();

    // Connect to Wi-Fi
    Serial.printf("Connecting to %s...\n", WIFI_SSID_STR);
    WiFi.begin(WIFI_SSID_STR, WIFI_PASS_STR);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());

    // Start CoAP
    coap.response(response_callback);
    coap.start(LOCAL_PORT);

    // --- THIS IS THE FIX ---
    // Register this device with the server as "A"
    Serial.println("Registering with server as 'A'...");
    coap.put(serverIp, SERVER_PORT, "reg", "REG-A");
    // -----------------------
}

// --- 5. LOOP ---
void loop()
{
    coap.loop(); // This is required to process CoAP ACKs

    int distance = sensor.getDistance();
    Serial.printf("Distance: %d cm\n", distance);

    // Check if the threshold is breached
    if (distance > 0 && distance < ALERT_THRESHOLD_CM)
    {

        // Check if the cooldown has passed
        if (millis() - lastAlertTime > ALERT_COOLDOWN_MS)
        {
            Serial.printf("!!! ALERTE !! Objet à %d cm. Envoi du signal.\n", distance);

            String payload = "PROXIMITY_DETECTED";

            // Send the alert to the server
            coap.put(serverIp, SERVER_PORT, "alert", payload.c_str());

            // Reset the cooldown timer
            lastAlertTime = millis();
        }
    }

    delay(250); // Check 4 times per second
}