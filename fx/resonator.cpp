/**
 * @brief Implementation of the Resonator class.
 *
 * The Resonator class is responsible for generating resonator effects based on the input audio signals.
 * It provides methods for selecting the resonator octave, processing audio samples, and adjusting parameters.
 */
#include "../core/helpers.h"
#include "resonator.h"
#include "IMultiVersioCommon.h"
#include "mode.h"

/**
 * @brief Construct a new Resonator::Resonator object
 *
 * @param mv A reference to the IMultiVersioCommon interface
 */
Resonator::Resonator(IMultiVersioCommon &mv) : mv(mv)
{
    // Initialize resonator parameters
    resonator_note = 20.f;
    resonator_tone = 12000.f;
    resonator_feedback_display = 0;
    resonator_current_regen = 0.5f;
    target_resonator_feedback = 0.001f;
    resonator_rmsCount = 0;
    resonator_previous_l = 0.f;
    resonator_previous_r = 0.f;
    resonator_current_RMS = 0.f;
    resonator_target_RMS = 0.f;
    resonator_feedback_RMS = 0.f;
    resonator_current_delay = 0.f;
    resonator_feedback = 0.f;
    resonator_target = 0.f;
    resonator_drywet = 0.f;
    resonator_octave = 1;
    resonator_glide = 0.f;
    resonator_glide_mode = 0;
}

/**
 * @brief Selects the resonator octave based on the given knob value.
 *
 * The resonator octave is determined as follows:
 * - If the knob value is less than 0.2, the resonator octave is set to 1.
 * - If the knob value is between 0.2 and 0.4, the resonator octave is set to 2.
 * - If the knob value is between 0.4 and 0.6, the resonator octave is set to 4.
 * - If the knob value is between 0.6 and 0.8, the resonator octave is set to 8.
 * - If the knob value is greater than 0.8, the resonator octave is set to 16.
 *
 * @param knob_value_1 The value of the knob used to select the resonator octave.
 */
void Resonator::SelectResonatorOctave(float knob_value_1)
{
    if (knob_value_1 < 0.2f)
    {
        resonator_octave = 1;
    }
    else if (knob_value_1 < 0.4f)
    {
        resonator_octave = 2;
    }
    else if (knob_value_1 < 0.6f)
    {
        resonator_octave = 4;
    }
    else if (knob_value_1 < 0.8f)
    {
        resonator_octave = 8;
    }
    else if (knob_value_1 > 0.8f)
    {
        resonator_octave = 16;
    }
}

/**
 * @brief Calculates the output samples for the Resonator effect.
 *
 * This function takes the input samples and calculates the output samples
 * based on the current state of the Resonator effect. It applies various
 * processing steps such as delay, filtering, and feedback to create the
 * resonator effect.
 *
 * @param[out] outl The left output sample.
 * @param[out] outr The right output sample.
 * @param inl The left input sample.
 * @param inr The right input sample.
 */
void Resonator::processSample(float &outl, float &outr, float inl, float inr)
{
    float resonator_target = this->mv.global_sample_rate / daisysp::mtof(resonator_note);

    daisysp::fonepole(resonator_current_delay, resonator_target / resonator_octave, 1 / (1 + resonator_glide * 25));

    IMultiVersioCommon::delr.SetDelay(resonator_current_delay);
    IMultiVersioCommon::dell.SetDelay(resonator_current_delay);

    resonator_rmsCount++;
    resonator_rmsCount %= (RMS_SIZE);

    if (resonator_rmsCount == 0)
    {
        resonator_target_RMS = resonator_averager.ProcessRMS();
    }

    daisysp::fonepole(resonator_current_RMS, resonator_target_RMS, .0001f);
    daisysp::fonepole(resonator_feedback_RMS, resonator_target_RMS, .001f);

    this->mv.leds.SetBaseColor(0, clamp(resonator_current_RMS, 0, 1), clamp(resonator_current_RMS, 0, 1) * clamp(resonator_current_RMS, 0, 0.1), (resonator_glide_mode / 10.f));
    this->mv.leds.SetBaseColor(1, clamp(resonator_feedback_RMS, 0, 1), clamp(resonator_feedback_RMS, 0, 1) * clamp(resonator_feedback_RMS, 0, 0.1), (resonator_glide_mode / 10.f));
    this->mv.leds.SetBaseColor(3, clamp(resonator_current_RMS, 0, 1), clamp(resonator_current_RMS, 0, 1) * clamp(resonator_current_RMS, 0, 0.1), (resonator_glide_mode / 10.f));
    this->mv.leds.SetBaseColor(2, clamp(resonator_feedback_RMS, 0, 1), clamp(resonator_feedback_RMS, 0, 1) * clamp(resonator_feedback_RMS, 0, 0.1), (resonator_glide_mode / 10.f));

    outl = IMultiVersioCommon::dell.Read();
    outr = IMultiVersioCommon::delr.Read();

    this->mv.svfl.Process(this->mv.tonel.Process(outl));
    this->mv.svfr.Process(this->mv.toner.Process(outr));

    float resonator_outl = this->mv.svfl.Low();
    float resonator_outr = this->mv.svfr.Low();

    float rev_outl, rev_outr;
    this->mv.effects[REV]->processSample(rev_outl, rev_outr, (inl * 0.01 + resonator_previous_l * 0.7f) * resonator_drywet + (inl * 0.999 + resonator_previous_l * 0.001f) * (1 - resonator_drywet),
                                     (inr * 0.01 + resonator_previous_r * 0.7f) * resonator_drywet + (inr * 0.999 + resonator_previous_r * 0.001f) * (1 - resonator_drywet));

    resonator_averager.Add((resonator_outl * resonator_outl + resonator_outr * resonator_outr) / 2);

    float delay_input_l = 0.f;
    float delay_input_r = 0.f;

    if (resonator_feedback > 0)
    {
        delay_input_l = this->mv.dcblock_l.Process(((resonator_feedback - resonator_current_RMS * 0.85f) * (resonator_outl + rev_outl * (0.15 + 0.85f * (1 - resonator_drywet)))));
        delay_input_r = this->mv.dcblock_r.Process(((resonator_feedback - resonator_current_RMS * 0.85f) * (resonator_outr + rev_outr * (0.15 + 0.85f * (1 - resonator_drywet)))));
    }
    else
    {
        delay_input_l = this->mv.dcblock_l.Process(((resonator_feedback + resonator_current_RMS * 0.85f) * (resonator_outl + rev_outl * (0.15 + 0.85f * (1 - resonator_drywet)))));
        delay_input_r = this->mv.dcblock_r.Process(((resonator_feedback + resonator_current_RMS * 0.85f) * (resonator_outr + rev_outr * (0.15 + 0.85f * (1 - resonator_drywet)))));
    }
}

/**
 * @brief Runs the resonator effect with the specified parameters.
 *
 * @param blend The blend parameter for the effect.
 * @param regen The regen parameter for the effect.
 * @param tone The tone parameter for the effect.
 * @param speed The speed parameter for the effect.
 * @param size The size parameter for the effect.
 * @param index The index parameter for the effect.
 * @param dense The dense parameter for the effect.
 * @param FSU The FSU parameter for the effect.
 */
void Resonator::run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU)
{
    if (this->mv.versio.tap.RisingEdge())
    {
        resonator_glide_mode = (resonator_glide_mode + 1) % 10;
        resonator_glide = resonator_glide_mode * resonator_glide_mode * resonator_glide_mode;
    }

    SelectResonatorOctave(speed);
    resonator_note = 12.0f + index * 60.0f;
    resonator_note = static_cast<int32_t>(resonator_note); // Quantize to semitones

    resonator_tone = this->mv.global_sample_rate * tone / 4.f;

    this->mv.rev.SetLpFreq(resonator_tone * 2.f);
    this->mv.reverb_shimmer = size * 2;
    this->mv.reverb_feedback = 0.8f + (std::log10(10 + dense * 90) - 1.000001f) * 1.4f;

    this->mv.rev.SetFeedback(this->mv.reverb_feedback);
    this->mv.reverb_drywet = dense;

    this->mv.tonel.SetFreq(resonator_tone / 2.f);
    this->mv.toner.SetFreq(resonator_tone / 2.f);

    this->mv.svfl.SetFreq(resonator_tone);
    this->mv.svfr.SetFreq(resonator_tone);

    this->mv.svfl.SetRes(0.001);
    this->mv.svfr.SetRes(0.001);

    this->mv.reverb_compression = dense * 2 + 0.5f;

    daisysp::fonepole(resonator_current_regen, regen, 0.008);

    resonator_feedback = (std::log10(10 + clamp((resonator_current_regen - 0.5) * 2.f, 0, 1) * 90) - 1.000001f) * 1.5f -
                         (std::log10(10 + clamp((1 - resonator_current_regen * 2.f), 0, 1) * 90) - 1.000001f) * 1.5f;

    resonator_drywet = blend * 1.01;
}

/**
 * @brief Returns whether or not the Resonator effect uses reverb.
 *
 * @return true
 */
bool Resonator::usesReverb()
{
    return true;
}