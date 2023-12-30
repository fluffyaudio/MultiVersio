#include "leds.h"
#include "daisy_versio.h"

/**
 * @brief Construct a new LedsControl::LedsControl object
 *
 * @param versio A reference to the DaisyVersio object
 */
LedsControl::LedsControl(daisy::DaisyVersio &versio) : versio(versio)
{
    Reset();
};

/**
 * @brief Sets the color of the specified led.
 *
 * @param idx The index of the led to set.
 * @param r The red component of the color.
 * @param g The green component of the color.
 * @param b The blue component of the color.
 */
void LedsControl::Reset()
{
    SwitchAllOff();
    // trick to initialize everything to black
    for (int i = 0; i < LED_COUNT; i++)
    {
        SetForXCycles(i, -1, 0, 0, 0);
    }
}

/**
 * @brief Sets the LED to flash for a specified number of cycles with a given color.
 *
 * @param idx The index of the LED.
 * @param _times The number of cycles to flash the LED.
 * @param r The red component of the flash color.
 * @param g The green component of the flash color.
 * @param b The blue component of the flash color.
 */
void LedsControl::SetForXCycles(int idx, int _times, float r, float g, float b)
{
    flash_color[idx][0] = r;
    flash_color[idx][1] = g;
    flash_color[idx][2] = b;
    times[idx] = _times;
};

/**
 * @brief Switches off all LEDs.
 */
void LedsControl::SwitchAllOff()
{
    SetAllLeds(0, 0, 0);
};

/**
 * @brief Switches off a specific LED.
 *
 * @param idx The index of the LED to switch off.
 */
void LedsControl::SwitchOffLed(int idx)
{
    this->versio.SetLed(idx, 0, 0, 0);
};

/**
 * @brief Sets all LEDs to a specified color.
 *
 * @param r The red component of the color.
 * @param g The green component of the color.
 * @param b The blue component of the color.
 */
void LedsControl::SetAllLeds(float r, float g, float b)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        SetBaseColor(i, r, g, b);
    }
}

/**
 * @brief Sets the base color for a specific LED.
 *
 * @param idx The index of the LED.
 * @param r The red component of the base color.
 * @param g The green component of the base color.
 * @param b The blue component of the base color.
 */
void LedsControl::SetBaseColor(int idx, float r, float g, float b)
{
    base_color[idx][0] = r;
    base_color[idx][1] = g;
    base_color[idx][2] = b;
}

/**
 * @brief Updates the state of the LEDs.
 */
void LedsControl::UpdateLeds()
{
    // handle flashing leds
    for (int i = 0; i < LED_COUNT; i++)
    {
        this->versio.SetLed(i, base_color[i][0], base_color[i][1], base_color[i][2]);
        if (times[i] > 0)
        {
            times[i]--;

            this->versio.SetLed(i, flash_color[i][0], flash_color[i][1], flash_color[i][2]);
        }
    }
    this->versio.UpdateLeds();
};