#ifndef PROXIMITY_SENSOR_H
#define PROXIMITY_SENSOR_H

#include <Arduino.h>

class ProximitySensor
{
public:
    // Constructor: Takes the pin numbers
    ProximitySensor(int trigPin, int echoPin);

    // Call this in setup()
    void begin();

    // Call this in loop() to get the distance
    int getDistance();

private:
    int _trigPin;
    int _echoPin;
};

#endif