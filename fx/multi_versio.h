#pragma once

#include <daisy_versio.h>
#include "../core/leds.h"
#include "daisysp.h"
#include "daisy_versio.h"
#include "string"
#include "arm_math.h"
#include "shy_fft.h"
#include <stddef.h>

#include "../core/mode.h"

#include "delay.h"
#include "filter.h"
#include "lofi.h"
#include "mlooper.h"
#include "resonator.h"
#include "reverb.h"
#include "spectrings.h"
#include "spectra.h"
#include "IMultiVersioCommon.h"
#include "IEffect.h"

class MultiVersio : public IMultiVersioCommon
{
public:
    daisy::DaisyVersio versio;
    LedsControl leds;

    /*
     * Singleton instance of multiversio.
     */
    static MultiVersio *instance;

    MultiVersio();

    void run();
    int getMode();
    void Controls();
    static void AudioCallback(daisy::AudioHandle::InputBuffer in,
                              daisy::AudioHandle::OutputBuffer out,
                              size_t size);

private:
    void initialize_common(float sample_rate, float current_delay);
    void initialize_fx();
    void UpdateKnobs();
};