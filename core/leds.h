#pragma once

#include "daisy_versio.h"

#define LED_COUNT 4

/**
 * @brief Class for controlling LEDs on DaisyVersio.
 *
 * This class provides methods to handle LEDs easily on DaisyVersio.
 */
class LedsControl
{
private:
    int times[LED_COUNT];            /**< Array to store the number of cycles for each LED. */
    float flash_color[LED_COUNT][3]; /**< Array to store the flash color for each LED. */
    float base_color[LED_COUNT][3];  /**< Array to store the base color for each LED. */
    daisy::DaisyVersio versio;       /**< DaisyVersio object for LED control. */

public:
    /**
     * @brief Constructor for LedsControl class.
     *
     * @param versio A reference to the DaisyVersio object.
     */
    LedsControl(daisy::DaisyVersio &versio);

    /**
     * @brief Resets the LED control to default values.
     */
    void Reset();

    /**
     * @brief Sets the LED to flash for a specified number of cycles with a given color.
     *
     * @param idx The index of the LED.
     * @param _times The number of cycles to flash the LED.
     * @param r The red component of the flash color.
     * @param g The green component of the flash color.
     * @param b The blue component of the flash color.
     */
    void SetForXCycles(int idx, int _times, float r, float g, float b);

    /**
     * @brief Switches off all LEDs.
     */
    void SwitchAllOff();

    /**
     * @brief Switches off a specific LED.
     *
     * @param idx The index of the LED to switch off.
     */
    void SwitchOffLed(int idx);

    /**
     * @brief Sets all LEDs to a specified color.
     *
     * @param r The red component of the color.
     * @param g The green component of the color.
     * @param b The blue component of the color.
     */
    void SetAllLeds(float r, float g, float b);

    /**
     * @brief Sets the base color for a specific LED.
     *
     * @param idx The index of the LED.
     * @param r The red component of the base color.
     * @param g The green component of the base color.
     * @param b The blue component of the base color.
     */
    void SetBaseColor(int idx, float r, float g, float b);

    /**
     * @brief Updates the state of the LEDs.
     */
    void UpdateLeds();
};
