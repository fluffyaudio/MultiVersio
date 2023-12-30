#include "leds.h"
#include "daisy_versio.h"

LedsControl::LedsControl(daisy::DaisyVersio &versio) : versio(versio)
{
    Reset();
};

void LedsControl::Reset()
{
    SwitchAllOff();
    // trick to initialize everything to black
    for (int i = 0; i < LED_COUNT; i++)
    {
        SetForXCycles(i, -1, 0, 0, 0);
    }
}
void LedsControl::SetForXCycles(int idx, int _times, float r, float g, float b)
{
    flash_color[idx][0] = r;
    flash_color[idx][1] = g;
    flash_color[idx][2] = b;
    times[idx] = _times;
};

void LedsControl::SwitchAllOff()
{
    SetAllLeds(0, 0, 0);
};

void LedsControl::SwitchOffLed(int idx)
{
    this->versio.SetLed(idx, 0, 0, 0);
};

void LedsControl::SetAllLeds(float r, float g, float b)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        SetBaseColor(i, r, g, b);
    }
}

void LedsControl::SetBaseColor(int idx, float r, float g, float b)
{
    base_color[idx][0] = r;
    base_color[idx][1] = g;
    base_color[idx][2] = b;
}

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