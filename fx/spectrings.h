#pragma once

#include "IEffect.h"
#include "daisysp.h"
#include "IMultiVersioCommon.h"
#include "spectra.h"

#define NUM_OF_STRINGS 2

/**
 * @brief Implements the Spectrings effect.
 */
class Spectrings : public IEffect
{
public:
    Spectrings(IMultiVersioCommon &mv, Spectra &spectra, int sample_rate);
    void run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU);
    void getSample(float &outl, float &outr, float inl, float inr);

    int spectrings_num_models;
    int spectrings_active_voices;
    int spectrings_current_voice;
    bool spectrings_trigger_next_cycle;
    float spectrings_drywet;

    size_t spectrings_attack_step[NUM_OF_STRINGS];
    size_t spectrings_attack_last_step[NUM_OF_STRINGS];
    float spectrings_accent_amount[NUM_OF_STRINGS];
    float spectrings_decay_amount[NUM_OF_STRINGS];
    float spectrings_pan_spread;

    daisysp::StringVoice string_voice[NUM_OF_STRINGS];

private:
    IMultiVersioCommon &mv;
    Spectra &spectra;
};