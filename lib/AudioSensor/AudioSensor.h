#ifndef AUDIO_SENSOR_H
#define AUDIO_SENSOR_H

#include <Arduino.h>

class AudioSensor
{
public:
    // Constructor: Takes the digital pin number
    AudioSensor(int digitalPin);

    // Call this in setup()
    void begin();

    // Call this in loop() to check for sound
    // Returns true if a sound was detected
    bool isSoundDetected();

private:
    int _digitalPin;
    bool _lastState;
};

#endif