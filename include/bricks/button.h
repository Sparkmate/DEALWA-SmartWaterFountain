#pragma once

// configs
#include <configs/hardware_configs.h>

namespace Button
{
    unsigned long last_button_pressed_time = 0;

    bool buttonIsPressed()
    {
        if (!digitalRead(BUTTON_PIN))
        {
            last_button_pressed_time = millis();
            return true;
        }
        return false;
    }

    bool buttonHasBeenPressedInLastThreeSeconds()
    {
        if (buttonIsPressed())
        {
            last_button_pressed_time = millis();
            return true;
        }
        else if (millis() - last_button_pressed_time < 3000)
        {
            return true;
        }
        return false;
    }

}
