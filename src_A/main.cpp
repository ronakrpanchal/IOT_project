#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "ProximitySensor.h"
#include "AudioSensor.h"

// --- 1. CONFIGURATION ---
const char *WIFI_SSID_STR = WIFI_SSID;
const char *WIFI_PASS_STR = WIFI_PASS;
const char *SERVER_IP_STR = SERVER_IP;
IPAddress serverIp;

const int SERVER_PORT = 5683;
const int LOCAL_PORT = 4210;
const int ALERT_THRESHOLD_CM = 100;
const long ALERT_COOLDOWN_MS = 100;
unsigned long lastProxAlertTime = 0;
unsigned long lastSoundAlertTime = 0;

// Timers for non-blocking loop
unsigned long lastProxCheck = 0;
const long PROX_CHECK_INTERVAL = 250; // Check proximity 4 times/sec

// --- 2. HARDWARE SETUP ---
const int TRIG_PIN = 27;
const int ECHO_PIN = 26;
ProximitySensor proxSensor(TRIG_PIN, ECHO_PIN);

const int AUDIO_D0_PIN = 25;
AudioSensor soundSensor(AUDIO_D0_PIN);

// --- 3. COAP & WIFI OBJECTS ---
WiFiUDP udp;
Coap coap(udp);

// CoAP ACK callback
void response_callback(CoapPacket &packet, IPAddress ip, int port)
{
    Serial.println("Got CoAP response (ACK)");
}

// --- 4. SETUP ---
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("--- Starting Multi-Sensor Alerter ---");

    serverIp.fromString(SERVER_IP_STR);

    proxSensor.begin();
    soundSensor.begin();
    Serial.println("Sensors initialized.");

    Serial.printf("Connecting to %s...\n", WIFI_SSID_STR);
    WiFi.begin(WIFI_SSID_STR, WIFI_PASS_STR);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());

    coap.response(response_callback);
    coap.start(LOCAL_PORT);

    Serial.println("Registering with server as 'A'...");
    coap.put(serverIp, SERVER_PORT, "reg", "REG-A");
}

// --- 5. LOOP (Non-Blocking) ---
void loop()
{
    coap.loop();

    unsigned long currentTime = millis();

    // Check Audio Sensor (Fast - runs every loop)
    if (soundSensor.isSoundDetected())
    {
        if (currentTime - lastSoundAlertTime > ALERT_COOLDOWN_MS)
        {
            Serial.println("!!! SOUND ALERTE !! Envoi du signal.");
            coap.put(serverIp, SERVER_PORT, "alert", "SOUND_DETECTED");
            lastSoundAlertTime = currentTime;
        }
    }

    // Check Proximity Sensor (Slow - runs every 250ms)
    if (currentTime - lastProxCheck > PROX_CHECK_INTERVAL)
    {
        lastProxCheck = currentTime;

        int distance = proxSensor.getDistance();
        Serial.printf("Distance: %d cm\n", distance);

        if (distance > 0 && distance < ALERT_THRESHOLD_CM)
        {
            if (currentTime - lastProxAlertTime > ALERT_COOLDOWN_MS)
            {
                Serial.println("!!! PROXIMITY ALERTE !! Envoi du signal.");
                coap.put(serverIp, SERVER_PORT, "alert", "PROXIMITY_DETECTED");
                lastProxAlertTime = currentTime;
            }
        }
    }
}