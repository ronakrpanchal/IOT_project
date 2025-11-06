#include "AudioSensor.h"

AudioSensor::AudioSensor(int digitalPin)
{
    _digitalPin = digitalPin;
    _lastState = LOW;
}

void AudioSensor::begin()
{
    pinMode(_digitalPin, INPUT);
    _lastState = digitalRead(_digitalPin);
}

// This function checks for a "rising edge"
// It only returns true the moment a new sound is detected
bool AudioSensor::isSoundDetected()
{
    bool currentState = digitalRead(_digitalPin);
    bool detected = false;

    if (currentState == HIGH && _lastState == LOW)
    {
        detected = true;
    }

    _lastState = currentState;
    return detected;
}