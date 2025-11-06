#include "DeviceLogic.h"

// Global pointer to the one instance of our class
static DeviceLogic *_instance = nullptr;

// Constructor: Just initializes the CoAP object and saves config
DeviceLogic::DeviceLogic(const char *id, int port, const char *ssid, const char *pass, IPAddress server)
    : _device_id(id), _local_port(port), _wifi_ssid(ssid), _wifi_pass(pass), _server_ip(server),
      _newPacketWaiting(false), _lastValueReceived(0), _coap(_udp)
{
    _instance = this; // Save the instance for the static callbacks
}

void DeviceLogic::begin()
{
    Serial.printf("--- Starting Device %s (CoAP) ---\n", _device_id);

    WiFi.begin(_wifi_ssid, _wifi_pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());

    // Set up CoAP to use our static callbacks
    _coap.server(DeviceLogicWrapper::data_callback_static, "data");
    _coap.response(DeviceLogicWrapper::response_callback_static);
    _coap.start(_local_port);

    Serial.println("Registering with relay...");
    String reg_msg = String("REG-") + String(_device_id);
    _coap.put(_server_ip, 5683, "reg", reg_msg.c_str());
}

void DeviceLogic::sendInitialPacket(const char *payload)
{
    Serial.printf("Sending initial value: %s\n", payload);
    _coap.put(_server_ip, 5683, "data", payload);
}

void DeviceLogic::loop()
{
    _coap.loop(); // This is critical

    if (_newPacketWaiting)
    {
        _newPacketWaiting = false; // Clear the flag

        int val_to_send = _lastValueReceived + 1;
        Serial.printf("Incrementing %d -> %d\n", _lastValueReceived, val_to_send);

        delay(500); // 0.5s delay

        String newPayload = String(val_to_send);
        Serial.printf("Sending new value: '%s'\n", newPayload.c_str());
        _coap.put(_server_ip, 5683, "data", newPayload.c_str());
    }
}

// --- Static Wrapper Functions ---
// These C-style functions are called by the CoAP library
// and they simply route the call to the class instance.

void DeviceLogicWrapper::data_callback_static(CoapPacket &packet, IPAddress ip, int port)
{
    if (_instance)
    {
        _instance->data_callback(packet, ip, port);
    }
}

void DeviceLogicWrapper::response_callback_static(CoapPacket &packet, IPAddress ip, int port)
{
    if (_instance)
    {
        _instance->response_callback(packet, ip, port);
    }
}

// --- Private Class Methods ---

void DeviceLogic::data_callback(CoapPacket &packet, IPAddress ip, int port)
{
    Serial.println("Received CoAP packet...");

    char p[packet.payloadlen + 1];
    memcpy(p, packet.payload, packet.payloadlen);
    p[packet.payloadlen] = '\0';
    String payloadStr = String(p);
    Serial.printf("Got value: '%s'\n", payloadStr.c_str());

    _coap.sendResponse(ip, port, packet.messageid);

    _lastValueReceived = payloadStr.toInt();
    _newPacketWaiting = true;
}

void DeviceLogic::response_callback(CoapPacket &packet, IPAddress ip, int port)
{
    Serial.println("Got CoAP response (ACK)");
}