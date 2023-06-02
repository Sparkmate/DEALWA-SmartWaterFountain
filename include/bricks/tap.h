#pragma once

// configs
#include <configs/hardware_configs.h>

// libs
#include <StatusLogger.h>

namespace Tap
{
    unsigned long tap_has_been_requested = 0;

    void startFillingWater()
    {
        StatusLogger::log(StatusLogger::LEVEL_GOOD_NEWS, "Water", "Water filling!");
        digitalWrite(RELAY_PIN, HIGH);
    }

    void stopFillingWater()
    {
        StatusLogger::log(StatusLogger::LEVEL_ERROR, "Water", "Stop filling water.");
        digitalWrite(RELAY_PIN, LOW);
    }

    bool timeoutOfTapIn10S()
    {
        return (millis() - tap_has_been_requested > 10000);
    }

    void getTapReady()
    {
        tap_has_been_requested = millis();
    }
}