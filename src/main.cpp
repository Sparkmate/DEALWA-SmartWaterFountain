// necessities
#include <Arduino.h>

// configs
#include <configs/hardware_configs.h>

// bricks
#include <bricks/tap.h>
#include <bricks/button.h>

// libs
#include <StatusLogger.h>

enum STATE_OPTIONS
{
    WAITING,
    READY,
    FILL,
    COOLDOWN,
    END_OF_FILL,
    ERROR
};
STATE_OPTIONS state = WAITING;
unsigned long state_set_time = 0;

void setState(STATE_OPTIONS new_state)
{
    state_set_time = millis();
    state = new_state;
    StatusLogger::log(StatusLogger::LEVEL_WARNING, StatusLogger::NAME_ESP32, "Status set to " + String(new_state));
    delay(50);
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    // Define the hardware pins
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    for (int i = 0; i < 3; i++)
    {
        Serial.println(".");
    }
    setState(FILL);

    // TODO Setup the screen

    // TODO Setup the RFID

    // TODO Touchscreen
}

void loop()
{
    // Logic
    //-- Waiting state (icon spinning around)
    if (state == WAITING)
    {
        setState(FILL);
    }
    //-- Ready state (either post payment, or some sensor being activate, or some mass sensor/button)
    else if (state == READY)
    {
        setState(WAITING);
    }
    //-- Fill state (while button is pressed, dispense water (open relay))
    else if (state == FILL)
    {
        Serial.println("A");
        Tap::getTapReady();
        while (1)
        {
            if (Tap::timeoutOfTapIn10S())
            // If we've not done anything for 15seconds!
            {
                Serial.println("Timed out");
                setState(WAITING);
                break;
            }
            else if (Button::buttonIsPressed())
            {
                Serial.println("D");

                Tap::startFillingWater();
                int i = 0;
                while (Button::buttonIsPressed())
                {
                    delay(5);
                    if (i > 100)
                    {
                        Serial.print(".");
                        i = 0;
                    }
                    i++;
                }
                Tap::stopFillingWater();
                setState(COOLDOWN);
            }
            delay(10);
        }
    }
    //-- Cooldown state (up to 3 seconds after button has been pressed we should enable continuing)
    else if (state == COOLDOWN)
    {
        while (Button::buttonHasBeenPressedInLastThreeSeconds())
        {
            delay(50);
        }
        setState(END_OF_FILL);
    }
    //-- End of fill, report final metrics
    else if (state == END_OF_FILL)
    {
        setState(WAITING);
    }
    //-- ERROR STATES
    else
    {
        setState(WAITING);
    }
}
