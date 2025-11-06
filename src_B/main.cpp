#include <Arduino.h>
#include "lib/DeviceLogic/DeviceLogic.h"

// --- CONFIGURATION ---
const char *WIFI_SSID = "Airtel_jimi_2244";
const char *WIFI_PASSWORD = "air80447";
IPAddress serverIp(192, 168, 1, 7); // Your Laptop's ETHERNET IP
const char *DEVICE_ID = "B";
const int LOCAL_PORT = 4211; // Must be different from Device A
// ---

DeviceLogic device_B(DEVICE_ID, LOCAL_PORT, WIFI_SSID, WIFI_PASSWORD, serverIp);

void setup()
{
    Serial.begin(115200);
    delay(5000);
    device_B.begin();

    // Device B just waits
    Serial.println("Registration sent. Waiting for data...");
}

void loop()
{
    device_B.loop();
}