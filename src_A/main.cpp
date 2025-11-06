#include <Arduino.h>
#include "lib/DeviceLogic/DeviceLogic.h"

// --- CONFIGURATION ---
const char *WIFI_SSID = "Airtel_jimi_2244";
const char *WIFI_PASSWORD = "air80447";
IPAddress serverIp(192, 168, 1, 7); // Your Laptop's ETHERNET IP
const char *DEVICE_ID = "A";
const int LOCAL_PORT = 4210;
// ---

DeviceLogic device_A(DEVICE_ID, LOCAL_PORT, WIFI_SSID, WIFI_PASSWORD, serverIp);

void setup()
{
    Serial.begin(115200);
    delay(5000);
    device_A.begin();

    // Device A kicks off the loop
    delay(500);
    device_A.sendInitialPacket("0");
}

void loop()
{
    device_A.loop();
}