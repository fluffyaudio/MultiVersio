#include "filter.h"
#include "IEffect.h"
#include "IMultiVersioCommon.h"

Filter::Filter(IMultiVersioCommon &mv) : mv(mv)
{
    filter_cutoff_l_par.Init(this->mv.versio.knobs[daisy::DaisyVersio::KNOB_0], 60, 20000, filter_cutoff_l_par.LOGARITHMIC);
    filter_cutoff_r_par.Init(this->mv.versio.knobs[daisy::DaisyVersio::KNOB_4], 60, 20000, filter_cutoff_r_par.LOGARITHMIC);
}

void Filter::getSample(float &outl, float &outr, float inl, float inr)
{
}

void Filter::getSamples(float outl[], float outr[], const float inl[], const float inr[], size_t size)
{
    this->mv.svf2l.ProcessMultimode(inl, outl, size, filter_mode_l);
    // TODO why does this manipulate the input buffer on the right channel only?
    // for (size_t i = 0; i < size; i++)
    //{
    //     inr[i] = sqrt(0.5f * (clamp((filter_path - 0.05f), 0.0f, 1.f) * 2.0f)) * outl[i] +
    //              sqrt(1.f * (2.f - (filter_path * 2))) * inr[i];
    // }
    this->mv.svf2r.ProcessMultimode(inr, outr, size, filter_mode_r);
}

void Filter::run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU)
{
    // Calculate filter frequencies
    filter_target_l_freq = filter_cutoff_l_par.Process() / this->mv.global_sample_rate;
    filter_target_r_freq = filter_cutoff_r_par.Process() / this->mv.global_sample_rate;

    // Update filter frequencies using one-pole filters
    daisysp::fonepole(filter_current_l_freq, filter_target_l_freq, 0.1f);
    daisysp::fonepole(filter_current_r_freq, filter_target_r_freq, 0.1f);

    // Set filter frequencies and Q values
    this->mv.svf2l.set_f_q<stmlib::FREQUENCY_ACCURATE>(filter_current_l_freq, 1.f + speed * speed * 49.f);
    this->mv.svf2r.set_f_q<stmlib::FREQUENCY_ACCURATE>(filter_current_r_freq, 1.f + size * size * 49.f);

    // Update filter mode and path
    filter_mode_l = tone;
    filter_mode_r = index;
    filter_path = dense;

    // Set LED colors based on filter parameters
    this->mv.leds.SetBaseColor(0, blend * 0.8, 0, 0);
    this->mv.leds.SetBaseColor(1, tone, 0, 1 - tone);
    this->mv.leds.SetBaseColor(2, index, 0, 1 - index);
    this->mv.leds.SetBaseColor(3, regen * 0.8, 0, 0);

    // Update filter path
    filter_path = dense;
}
