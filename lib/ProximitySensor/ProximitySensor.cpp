#include "ProximitySensor.h"

ProximitySensor::ProximitySensor(int trigPin, int echoPin)
{
    _trigPin = trigPin;
    _echoPin = echoPin;
}

void ProximitySensor::begin()
{
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
}

int ProximitySensor::getDistance()
{
    // Send the trigger pulse
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    // Read the echo time
    long duration = pulseIn(_echoPin, HIGH, 500000);

    // Calculate distance in cm
    int distance = duration * 0.0343 / 2;

    return distance;
}