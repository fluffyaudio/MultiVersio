/**
 * @file multi_versio.cpp
 * @brief Implementation file for the MultiVersio class.
 *
 * This file contains the implementation of the MultiVersio class, which is responsible for initializing and running the audio effects on the DaisyVersio hardware.
 */
#include "libDaisy/src/daisy_versio.h"
#include "core/leds.h"
#include "DaisySP/Source/daisysp.h"
#include <string>
#include "libDaisy/Drivers/CMSIS/DSP/Include/arm_math.h"
#include "stmlib/fft/shy_fft.h"
#include <cstddef>

#include "core/mode.h"

#include "multi_versio.h"

#include "delay.h"
#include "filter.h"
#include "lofi.h"
#include "mlooper.h"
#include "resonator.h"
#include "reverb.h"
#include "spectrings.h"
#include "spectra.h"
#include "core/IMultiVersioCommon.h"

daisysp::DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS IMultiVersioCommon::dell;
daisysp::DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS IMultiVersioCommon::delr;
float DSY_SDRAM_BSS IMultiVersioCommon::mlooper_buf_1l[LOOPER_MAX_SIZE];
float DSY_SDRAM_BSS IMultiVersioCommon::mlooper_buf_1r[LOOPER_MAX_SIZE];
float DSY_SDRAM_BSS IMultiVersioCommon::mlooper_frozen_buf_1l[LOOPER_MAX_SIZE];
float DSY_SDRAM_BSS IMultiVersioCommon::mlooper_frozen_buf_1r[LOOPER_MAX_SIZE];

MultiVersio *MultiVersio::instance = nullptr;

MultiVersio::MultiVersio() : IMultiVersioCommon(versio), leds(versio)
{
    instance = this;

    // initialize DaisyVersio
    this->versio.Init(true);
    this->versio.StartAdc();
    this->versio.StartAudio(AudioCallback);

    // initialize common buffers etc
    this->initialize_common(this->versio.AudioSampleRate(), this->versio.AudioSampleRate() * 0.75f);

    // initialize effects
    this->initialize_fx();
}

/**
 * @brief Processes audio.
 *
 * This function processes audio. It is called by the audio callback.
 *
 * @param in The input buffer.
 * @param out The output buffer.
 * @param size The size of the buffer.
 *
 * @return void
 */
void MultiVersio::processAudio(daisy::AudioHandle::InputBuffer in,
                               daisy::AudioHandle::OutputBuffer out,
                               size_t size)
{
    float out1, out2, in1, in2;

    IEffect *effect = effects[mode];

    // run pre-processing on the current effect
    // NOTE: the default preProcess implementation does nothing
    effect->preProcess(in[0], in[1], size);

    // process samples
    for (size_t i = 0; i < size; i += 1)
    {
        in1 = in[0][i];
        in2 = in[1][i];

        out1 = 0.f;
        out2 = 0.f;

        // run effect
        effect->processSample(out1, out2, in1, in2);

        // apply reverb for those effects that use it
        if (effect->usesReverb())
        {
            effects[REV]->processSample(out1, out2, out1, out2);
        }

        out[0][i] = out1;
        out[1][i] = out2;
    }

    // runs postprocessing. This is a noop for all effects apart from the filter.
    effect->postProcess(out[0], out[1], in[0], in[1], size);
}

/**
 * @brief Audio callback.
 *
 * This is the audio callback function. It is called by the DaisyVersio
 * hardware to process audio.
 *
 * This function is responsible for calling the audio processing functions
 * of the effects.
 *
 * Calls the following functions:
 * - MultiVersio::processControls()
 * - MultiVersio::updateActiveEffect()
 * - MultiVersio::processAudio()
 * - LedsControl::UpdateLeds()
 *
 * @param in The input buffer.
 * @param out The output buffer.
 * @param size The size of the buffer.
 *
 * @return void
 */
void MultiVersio::AudioCallback(daisy::AudioHandle::InputBuffer in,
                                daisy::AudioHandle::OutputBuffer out,
                                size_t size)
{
    instance->processControls();
    instance->updateActiveEffect();
    instance->processAudio(in, out, size);
    instance->leds.UpdateLeds();
};

/**
 * @brief Initializes the common buffers used by all effects.
 *
 * This function initializes the common buffers used by all effects.
 *
 * @param sample_rate The sample rate of the audio.
 * @param current_delay The current delay.
 *
 * @return void
 */
void MultiVersio::initialize_common(float sample_rate, float current_delay)
{
    rev.Init(sample_rate);
    IMultiVersioCommon::dell.Init();
    IMultiVersioCommon::delr.Init();
    tonel.Init(sample_rate);
    toner.Init(sample_rate);
    svfl.Init(sample_rate);
    svfl.SetFreq(0.0);
    svfl.SetRes(0.5);
    svfr.Init(sample_rate);
    svfr.SetFreq(0.0);
    svfr.SetRes(0.5);
    svf2l.Init();
    svf2r.Init();
    biquad.Init(sample_rate);
    biquad.SetCutoff(0.0);
    biquad.SetRes(0.5);

    dcblock_l.Init(sample_rate);
    dcblock_r.Init(sample_rate);
    dcblock_2l.Init(sample_rate);
    dcblock_2r.Init(sample_rate);

    // Delay parameters
    IMultiVersioCommon::dell.SetDelay(current_delay);
    IMultiVersioCommon::delr.SetDelay(current_delay);

    global_sample_rate = sample_rate;

    for (size_t i = 0; i < 300; i++)
    {
        if (i < 48)
        {
            attack_lut[i] = map(i, 0, 48, 1.0f, 0.f);
        }
        else
        {
            attack_lut[i] = map(i, 48, 300, 0.0f, 1.f);
        }
    }
}

/**
 * @brief Initializes the effects.
 *
 * This function initializes the effects used in the MultiVersio class.
 *
 * @return void
 */
void MultiVersio::initialize_fx()
{
    effects[REV] = new Reverb(*this);
    effects[RESONATOR] = new Resonator(*this);
    effects[FILTER] = new Filter(*this);
    effects[LOFI] = new Lofi(*this);
    effects[MLOOPER] = new MLooper(*this);
    effects[DELAY] = new DelayEffect(*this);
    effects[SPECTRA] = new Spectra(*this);
    effects[SPECTRINGS] = new Spectrings(*this, *static_cast<Spectra *>(effects[SPECTRA]), sample_rate);
}

/**
 * Run method.
 *
 * This method does not do anything.  It probably should do something.
 */
void MultiVersio::run()
{
    while (1)
    {
        // UpdateOled();
    }
}

/**
 * @brief Gets the device mode.
 *
 * Returns the current mode that the MultiVersio is in, according
 * to the position of the switches.
 *
 * If the mode has changed, the LEDs are reset.
 *
 * The mode is calculated as follows:
 *
 * - 0: SW1 = 0, SW2 = 0
 * - 1: SW1 = 1, SW2 = 0
 * - 2: SW1 = 2, SW2 = 0
 * - 3: SW1 = 0, SW2 = 1
 * - 4: SW1 = 1, SW2 = 1
 * - 5: SW1 = 2, SW2 = 1
 * - 6: SW1 = 0, SW2 = 2
 * - 7: SW1 = 1, SW2 = 2
 * - 8: SW1 = 2, SW2 = 2
 *
 * @see DaisyVersio::SW_0
 * @see DaisyVersio::SW_1
 *
 * @return int The current mode.
 */
int MultiVersio::getMode()
{
    int sw1 = this->versio.sw[daisy::DaisyVersio::SW_0].Read();
    int sw2 = this->versio.sw[daisy::DaisyVersio::SW_1].Read();

    this->mode = sw1 + (3 * sw2);

    if (mode != previous_mode)
    {
        previous_mode = mode;
        this->leds.Reset();
    }

    return mode;
}

/**
 * @brief Runs the active effect.
 *
 * This function gets the value of all knobs, gets the current mode,
 * and runs the active effect.
 *
 * Naming of the knobs is derived from the
 * [Desmodus Versio](https://noiseengineering.us/products/desmodus-versio)
 * module, as follows:
 *
 * - Knob 0: Blend
 * - Knob 1: Speed
 * - Knob 2: Tone
 * - Knob 3: Index
 * - Knob 4: Regen
 * - Knob 5: Size
 * - Knob 6: Dense
 *
 * In addition, the value of the gate FSU input is required by some effects.
 *
 * @see MultiVersio::getMode()
 * @see MultiVersio::effects
 *
 * @return void
 */
void MultiVersio::updateActiveEffect()
{
    float blend = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_0);
    float speed = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_1);
    float tone = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_2);
    float index = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_3);
    float regen = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_4);
    float size = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_5);
    float dense = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_6);
    bool gate = this->versio.Gate();

    mode = this->getMode();

    effects[mode]->run(blend, regen, tone, speed, size, index, dense, gate);
}

void MultiVersio::processControls()
{
    // TODO why are these variables set here?
    Resonator *resonator = (Resonator *)effects[RESONATOR];
    Reverb *reverb = (Reverb *)effects[REV];
    resonator->resonator_feedback = 0;
    reverb->reverb_drywet = 0;

    this->versio.ProcessAnalogControls();

    // this->versio.ProcessDigitalControls();

    this->versio.tap.Debounce();

    // UpdateEncoder();
}

// void UpdateEncoder()
//{
//     mode = mode + patch.encoder.Increment();
//     mode = (mode % NUM_MODES + NUM_MODES) % NUM_MODES;
// }
