#ifndef DEVICE_LOGIC_H
#define DEVICE_LOGIC_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

// This static class wrapper is needed so the C-style
// callbacks from the CoAP library can call our C++ class methods.
class DeviceLogicWrapper
{
public:
    static void data_callback_static(CoapPacket &packet, IPAddress ip, int port);
    static void response_callback_static(CoapPacket &packet, IPAddress ip, int port);
};

class DeviceLogic
{
public:
    DeviceLogic(const char *id, int port, const char *ssid, const char *pass, IPAddress server);

    void begin();
    void loop();
    void sendInitialPacket(const char *payload);

private:
    // Callbacks
    void data_callback(CoapPacket &packet, IPAddress ip, int port);
    void response_callback(CoapPacket &packet, IPAddress ip, int port);
    friend class DeviceLogicWrapper; // Allows static class to call private methods

    // State
    const char *_device_id;
    int _local_port;
    const char *_wifi_ssid;
    const char *_wifi_pass;
    IPAddress _server_ip;

    volatile bool _newPacketWaiting;
    volatile int _lastValueReceived;

    WiFiUDP _udp;
    Coap _coap;
};

#endif // DEVICE_LOGIC_H