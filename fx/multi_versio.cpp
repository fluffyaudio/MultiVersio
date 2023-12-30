/**
 * @file multi_versio.cpp
 * @brief Implementation file for the MultiVersio class.
 *
 * This file contains the implementation of the MultiVersio class, which is responsible for initializing and running the audio effects on the DaisyVersio hardware.
 */
#include <daisy_versio.h>
#include "leds.h"
#include "daisysp.h"
#include "string"
#include "arm_math.h"
#include "shy_fft.h"
#include <stddef.h>

#include "mode.h"

#include "multi_versio.h"

#include "delay.h"
#include "filter.h"
#include "lofi.h"
#include "mlooper.h"
#include "resonator.h"
#include "reverb.h"
#include "spectrings.h"
#include "spectra.h"
#include "IMultiVersioCommon.h"

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
 * @brief Audio callback.
 *
 * This is the audio callback function. It is called by the DaisyVersio
 * hardware to process audio.
 *
 * This function is responsible for calling the audio processing functions
 * of the effects.
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
    float out1, out2, in1, in2;

    instance->Controls();
    instance->leds.UpdateLeds();

    Filter *filter = (Filter *)instance->effects[FILTER];
    Spectra *spectra = (Spectra *)instance->effects[SPECTRA];
    Spectrings *spectrings = (Spectrings *)instance->effects[SPECTRINGS];

    if ((instance->mode == SPECTRA) or (instance->mode == SPECTRINGS))
    {
        spectra->spectra_oscbank.FillInputBuffer(in[0], in[1], size);
        if (spectra->spectra_do_analysis)
        {
            spectra->spectra_do_analysis = false;
            spectra->spectra_oscbank.CalculateSpectralanalysis();
        }

        if (instance->mode == SPECTRINGS)
        {
            spectrings->string_voice[spectrings->spectrings_current_voice].SetFreq(spectra->spectra_oscbank.getFrequency(spectrings->spectrings_current_voice));
        }
    };

    // audio
    for (size_t i = 0; i < size; i += 1)
    {
        in1 = in[0][i];
        in2 = in[1][i];

        out1 = 0.f;
        out2 = 0.f;

        switch (instance->mode)
        {
        case REV:
            instance->effects[REV]->getSample(out1, out2, in1, in2);
            break;
        case RESONATOR:
            instance->effects[RESONATOR]->getSample(out1, out2, in1, in2);
            break;
        case LOFI:
            instance->effects[REV]->getSample(out1, out2, in1, in2);
            instance->effects[LOFI]->getSample(out1, out2, out1, out2);
            break;
        case MLOOPER:
            instance->effects[MLOOPER]->getSample(out1, out2, in1, in2);
            break;
        case SPECTRA:
            instance->effects[SPECTRA]->getSample(out1, out2, in1, in2);
            instance->effects[REV]->getSample(out1, out2, out1, out2);
            break;
        case DELAY:
            instance->effects[DELAY]->getSample(out1, out2, in1, in2);
            break;
        case SPECTRINGS:
            instance->effects[SPECTRINGS]->getSample(out1, out2, in1, in2);
            instance->effects[REV]->getSample(out1, out2, out1, out2);
            break;

        default:
            out1 = out2 = 0.f;
        }
        out[0][i] = out1;
        out[1][i] = out2;
    }

    if (instance->mode == FILTER)
    {
        filter->getSamples(out[0], out[1], in[0], in[1], size);
    }
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
 * This method does not do anything.
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
 * @return int The current mode.
 */
int MultiVersio::getMode()
{
    int sw1, sw2;

    switch (this->versio.sw[daisy::DaisyVersio::SW_0].Read())
    {
    case 0:
        sw1 = 1;
        break;
    case 1:
        sw1 = 0;
        break;
    case 2:
        sw1 = 2;
        break;
    };

    switch (this->versio.sw[daisy::DaisyVersio::SW_1].Read())
    {
    case 0:
        sw2 = 3;
        break;
    case 1:
        sw2 = 0;
        break;
    case 2:
        sw2 = 6;
        break;
    };

    this->mode = sw1 + sw2;

    return mode;
}

void MultiVersio::UpdateKnobs()
{
    float blend = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_0);
    float speed = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_1);
    float tone = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_2);
    float index = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_3);
    float regen = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_4);
    float size = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_5);
    float dense = this->versio.GetKnobValue(daisy::DaisyVersio::KNOB_6);

    mode = this->getMode();

    if (mode != previous_mode)
    {
        previous_mode = mode;
        this->leds.Reset();
    }

    effects[mode]->run(blend, regen, tone, speed, size, index, dense, 0); // TODO: FSU

    this->versio.tap.Debounce();
}

void MultiVersio::Controls()
{
    Resonator *resonator = (Resonator *)effects[RESONATOR];
    Reverb *reverb = (Reverb *)effects[REV];

    resonator->resonator_feedback = 0;
    reverb->reverb_drywet = 0;

    this->versio.ProcessAnalogControls();
    // patch.ProcessDigitalControls();

    this->UpdateKnobs();

    // UpdateEncoder();
}

// void UpdateEncoder()
//{
//     mode = mode + patch.encoder.Increment();
//     mode = (mode % NUM_MODES + NUM_MODES) % NUM_MODES;
// }
