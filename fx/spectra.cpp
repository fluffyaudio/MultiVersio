#include "libDaisy/src/daisy_versio.h"
#include "IMultiVersioCommon.h"
#include "spectra.h"
#include "OscBank.h"

/**
 * @brief Construct a new Spectra::Spectra object
 *
 * Initializes the oscilattor bank.
 *
 * @param mv A reference to the IMultiVersioCommon interface
 */
Spectra::Spectra(IMultiVersioCommon &mv) : mv(mv), spectra_oscbank(mv, *this)
{
}

/**
 * @brief Selects the spectra octave based on the given knob value.
 *
 * The spectra octave is determined as follows:
 * - If the knob value is less than 0.2, the spectra octave is set to -2.
 * - If the knob value is between 0.2 and 0.4, the spectra octave is set to -1.
 * - If the knob value is between 0.4 and 0.6, the spectra octave is set to 0.
 * - If the knob value is between 0.6 and 0.8, the spectra octave is set to 1.
 * - If the knob value is greater than 0.8, the spectra octave is set to 2.
 *
 * @param knob_value_1 The value of the knob used to select the spectra octave.
 */
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

/**
 * @brief Selects the spectra quality based on the given knob value.
 *
 * The spectra quality is determined as follows:
 * - If the knob value is less than 0.25, the spectra quality is set to 2.
 * - If the knob value is between 0.25 and 0.5, the spectra quality is set to 4.
 * - If the knob value is between 0.5 and 0.75, the spectra quality is set to 8.
 * - If the knob value is greater than 0.75, the spectra quality is set to 16.
 *
 * @param knob_value_1 The value of the knob used to select the spectra quality.
 */
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

/**
 * @brief Processes the input samples and writes the output samples.
 *
 * @param outl The left output sample.
 * @param outr The right output sample.
 * @param inl The left input sample.
 * @param inr The right input sample.
 */
void Spectra::processSample(float &outl, float &outr, float inl, float inr)
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

/**
 * @brief Runs the spectra effect with the specified parameters.
 *
 * @param blend The blend parameter for the effect.
 * @param regen The regen parameter for the effect.
 * @param tone The tone parameter for the effect.
 * @param speed The speed parameter for the effect.
 * @param size The size parameter for the effect.
 * @param index The index parameter for the effect.
 * @param dense The dense parameter for the effect.
 * @param gate Effect gate from the FSU input.
 */
void Spectra::run(float blend, float regen, float tone, float speed, float size, float index, float dense, bool gate)
{
    // blend = spectra drywet
    // speed = hop
    // index = transpose when quantizing.
    // tone = octave
    // size = number of waveforms + spread
    // regen = amount of reverb + feedback
    // dense = waveform kind
    // gate = clock / FSU

    SelectSpectraQuality(1.f - speed);

    // spectra_waveform = (dense + spectra_prev_knob_wave_knob)/0.5f;
    spectra_prev_knob_wave_knob = spectra_oscbank.SetAllWaveforms((int)((dense * 9 + spectra_prev_knob_wave_knob) * 0.1 * 9.f));

    spectra_num_active = ((int)(1 + (clamp(size * 2, 0.0f, 1.0f) * (spectra_max_num_frequencies - 0.5f))));

    spectra_spread = clamp(map(size - 0.5, 0.0f, 0.5f, 1.0f, 4.0f), 1.0f, 4.0f);
    spectra_lower_harmonics = spectra_spread * 0.25f;
    spectra_oscbank.SetNumActive(spectra_num_active);

    SelectSpectraOctave(tone);

    spectra_rotate_harmonics = 0;
    if (gate)
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

/**
 * @brief Returns whether or not the Spectra effect uses reverb.
 *
 * @return true
 */
bool Spectra::usesReverb()
{
    return true;
}

/**
 * @brief Fills the oscillator bank input buffer with the given samples.
 *
 * If the spectra_do_analysis flag is set, spectral analysis is performed.
 *
 * Overrides the IEffect::preProcess method.
 *
 * @param in1 The left input sample.
 * @param in2 The right input sample.
 * @param size The size of the input buffer.
 */
void Spectra::preProcess(const float *in1, const float *in2, size_t size)
{
    spectra_oscbank.FillInputBuffer(in1, in2, size);

    if (spectra_do_analysis)
    {
        spectra_do_analysis = false;
        spectra_oscbank.CalculateSpectralanalysis();
    }
}