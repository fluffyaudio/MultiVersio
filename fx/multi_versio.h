#pragma once

#include "libDaisy/src/daisy_versio.h"
#include "core/leds.h"
#include "DaisySP/Source/daisysp.h"
#include "libDaisy/src/daisy_versio.h"
#include <string>
#include "libDaisy/Drivers/CMSIS/DSP/Include/arm_math.h"
#include "stmlib/fft/shy_fft.h"
#include <cstddef>
#include "core/mode.h"
#include "delay.h"
#include "filter.h"
#include "lofi.h"
#include "mlooper.h"
#include "resonator.h"
#include "reverb.h"
#include "spectrings.h"
#include "spectra.h"
#include "core/IMultiVersioCommon.h"
#include "IEffect.h"

/**
 * @brief Main class for the MultiVersio effects firmware.
 */
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
    void processControls();
    void processAudio(daisy::AudioHandle::InputBuffer in,
                      daisy::AudioHandle::OutputBuffer out,
                      size_t size);
    static void AudioCallback(daisy::AudioHandle::InputBuffer in,
                              daisy::AudioHandle::OutputBuffer out,
                              size_t size);

private:
    void initialize_common(float sample_rate, float current_delay);
    void initialize_fx();
    void updateActiveEffect();
};
