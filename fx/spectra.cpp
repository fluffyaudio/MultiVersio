#include <daisy_versio.h>
#include "IMultiVersioCommon.h"
#include "spectra.h"
#include "OscBank.h"

Spectra::Spectra(IMultiVersioCommon &mv) : mv(mv), spectra_oscbank(mv, *this)
{
}

void Spectra::SelectSpectraOctave(float knob_value_1)
{
    // sets the octave shift
    if (knob_value_1 < 0.2f)
    {
        spectra_oct_mult = 0.25f;
        spectra_oct_string = "-2";
    }
    else if (knob_value_1 < 0.4f)
    {
        spectra_oct_mult = 0.5f;
        spectra_oct_string = "-1";
    }
    else if (knob_value_1 < 0.6f)
    {
        spectra_oct_mult = 1.f;
        spectra_oct_string = " 0";
    }
    else if (knob_value_1 < 0.8f)
    {
        spectra_oct_mult = 2.f;
        spectra_oct_string = "+1";
    }
    else if (knob_value_1 > 0.8f)
    {
        spectra_oct_mult = 4.f;
        spectra_oct_string = "+2";
    }
};

void Spectra::SelectSpectraQuality(float knob_value_1)
{
    // sets the octave shift
    if (knob_value_1 < 0.25f)
    {
        spectra_oscbank.hop = 2;
    }
    else if (knob_value_1 < 0.5f)
    {
        spectra_oscbank.hop = 4;
    }
    else if (knob_value_1 < 0.75f)
    {
        spectra_oscbank.hop = 8;
    }
    else if (knob_value_1 > 0.75f)
    {
        spectra_oscbank.hop = 16;
    }
};

void Spectra::getSample(float &outl, float &outr, float inl, float inr)
{
    float spectra_outl;
    float spectra_outr;
    spectra_oscbank.updateFreqAndMagn();
    float spectra_output = spectra_oscbank.Process();

    spectra_output = spectra_output; //(sqrt(0.5f * (spectra_dynamics*2.0f))*filtered_out + sqrt(0.95f * (2.f - (spectra_dynamics*2))) * spectra_output)*0.5f;

    spectra_outl = spectra_output;

    spectra_outr = spectra_outl;
    if (spectra_drywet > 0.98f)
    {
        spectra_drywet = (1.f + spectra_drywet) * 0.5f;
    }

    outl = (sqrt(0.5f * (spectra_drywet * 2.0f)) * spectra_outl + sqrt(0.95f * (2.f - (spectra_drywet * 2))) * inl) * 0.5f;
    outr = (sqrt(0.5f * (spectra_drywet * 2.0f)) * spectra_outr + sqrt(0.95f * (2.f - (spectra_drywet * 2))) * inr) * 0.5f;
}

void Spectra::run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU)
{
    // blend = spectra drywet
    // speed = hop
    // index = transpose when quantizing.
    // tone = octave
    // size = number of waveforms + spread
    // regen = amount of reverb + feedback
    // dense = waveform kind
    // tap = activate quantizer

    // FSU clock

    SelectSpectraQuality(1.f - speed);

    // spectra_waveform = (dense + spectra_prev_knob_wave_knob)/0.5f;
    spectra_prev_knob_wave_knob = spectra_oscbank.SetAllWaveforms((int)((dense * 9 + spectra_prev_knob_wave_knob) * 0.1 * 9.f));

    spectra_num_active = ((int)(1 + (clamp(size * 2, 0.0f, 1.0f) * (spectra_max_num_frequencies - 0.5f))));

    spectra_spread = clamp(map(size - 0.5, 0.0f, 0.5f, 1.0f, 4.0f), 1.0f, 4.0f);
    spectra_lower_harmonics = spectra_spread * 0.25f;
    spectra_oscbank.SetNumActive(spectra_num_active);

    SelectSpectraOctave(tone);

    spectra_rotate_harmonics = 0;
    if (this->mv.versio.gate.Trig())
    {
        spectra_do_analysis = true;
        spectra_r = randomFloat();
        spectra_g = randomFloat();
        spectra_b = randomFloat();
        this->mv.leds.SetBaseColor(1, spectra_r, spectra_g, spectra_b);
        this->mv.leds.SetBaseColor(2, spectra_r, spectra_g, spectra_g);
    }

    this->mv.reverb_lowpass = this->mv.sample_rate * 0.5 / 2.f;
    this->mv.rev.SetLpFreq(this->mv.reverb_lowpass);
    this->mv.reverb_shimmer = 0.0f;
    this->mv.reverb_feedback = 0.2f + (std::log10(10 + regen * 90) - 1.000001f) * 1.0f;
    this->mv.rev.SetFeedback(this->mv.reverb_feedback);

    this->mv.reverb_compression = 0.5f;
    spectra_drywet = blend;

    this->mv.reverb_drywet = clamp(regen * 1.1f, 0.0f, 1.0f);

    spectra_transpose = (int)std::round(index * 12.f);
    if (this->mv.versio.tap.RisingEdge())
    {
        spectra_quantize = (spectra_quantize + 1) % 9;
        if (spectra_quantize > 0)
        {
            this->mv.leds.SetBaseColor(0, 0, 1, 0);
            switch (spectra_quantize)
            {
            case 1:
                spectra_selected_scale = scale_12;
                this->mv.leds.SetBaseColor(3, 0, 0, 1);
                break;
            case 2:
                spectra_selected_scale = scale_7;
                this->mv.leds.SetBaseColor(3, 0, 0, 0.8);
                break;
            case 3:
                spectra_selected_scale = scale_6;
                this->mv.leds.SetBaseColor(3, 0, 0.3, 0.6);
                break;
            case 4:
                spectra_selected_scale = scale_5;
                this->mv.leds.SetBaseColor(3, 0, 0.4, 0.4);
                break;
            case 5:
                this->mv.leds.SetBaseColor(3, 0, 0.6, 0.3);
                spectra_selected_scale = scale_4;
                break;
            case 6:
                this->mv.leds.SetBaseColor(3, 0, 0.7, 0.2);
                spectra_selected_scale = scale_3;
                break;
            case 7:
                this->mv.leds.SetBaseColor(3, 0, 0.4, 0.1);
                spectra_selected_scale = scale_2;
                break;
            case 8:
                this->mv.leds.SetBaseColor(3, 0.4, 0.4, 0.0);
                spectra_selected_scale = scale_1;
                break;
            default:
                break;
            }
        }
        else
        {
            this->mv.leds.SetBaseColor(0, 0, 0, 0);
            this->mv.leds.SetBaseColor(3, 0, 0, 0);
        };
    };

    spectra_reverb_amount = this->mv.reverb_drywet;
}
