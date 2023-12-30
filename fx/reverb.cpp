#include "../core/helpers.h"
#include <daisy_versio.h>
#include "IMultiVersioCommon.h"
#include "reverb.h"
#include "mode.h"
#include "daisysp.h"

/**
 * @brief Construct a new Reverb::Reverb object
 *
 * @param mv A reference to the IMultiVersioCommon interface
 */
Reverb::Reverb(IMultiVersioCommon &mv) : mv(mv)
{
    this->mv.rev.SetLpFreq(9000.0f);
    this->mv.rev.SetFeedback(0.85f);

    // Reverb parameters
    reverb_shimmer_write_pos1l = 0;
    reverb_shimmer_write_pos1r = 0;
    reverb_shimmer_write_pos2 = 0;
    reverb_shimmer_play_pos1l = 0.0f;
    reverb_shimmer_play_pos1r = 0.0f;
    reverb_shimmer_play_pos2 = 0.f;

    reverb_shimmer_buffer_size1l = 24000 * 0.773;
    reverb_shimmer_buffer_size1r = 24000 * 0.802;
    reverb_shimmer_buffer_size2 = 48000 * 0.753 * 2;

    reverb_previous_inl = 0;
    reverb_previous_inr = 0;
    reverb_current_outl = 0;
    reverb_current_outr = 0;

    reverb_feedback_display = 0;
    reverb_current_RMS = 0.0f;
    reverb_target_RMS = 0.0f;
    reverb_feedback_RMS = 0.f;
    reverb_target_compression = 1.0f;
}

/**
 * @brief Writes the input samples to the first shimmer buffer.
 *
 * @param in_l The left channel input sample.
 * @param in_r The right channel input sample.
 */
void Reverb::WriteShimmerBuffer1(float in_l, float in_r)
{
    IMultiVersioCommon::mlooper_buf_1l[reverb_shimmer_write_pos1l] = in_l;
    IMultiVersioCommon::mlooper_buf_1r[reverb_shimmer_write_pos1r] = in_r;
    reverb_shimmer_write_pos1l = (reverb_shimmer_write_pos1l + 1) % reverb_shimmer_buffer_size1l;
    reverb_shimmer_write_pos1r = (reverb_shimmer_write_pos1r + 1) % reverb_shimmer_buffer_size1r;
}

/**
 * @brief Writes the input samples to the second shimmer buffer.
 *
 * @param in_l The left channel input sample.
 * @param in_r The right channel input sample.
 */
void Reverb::WriteShimmerBuffer2(float in_l, float in_r)
{
    // TODO why is only the left channel being written to?
    IMultiVersioCommon::mlooper_frozen_buf_1l[reverb_shimmer_write_pos2] = (in_r + in_l) / 2;
    reverb_shimmer_write_pos2 = (reverb_shimmer_write_pos2 + 1) % reverb_shimmer_buffer_size2;
}

/**
 * @brief Compresses the input sample.
 *
 * @param sample The input sample.
 * @return float The compressed sample.
 */
float Reverb::CompressSample(float sample)
{
    if (sample > 0.4)
    {
        sample = clamp(sample - map(sample, 0.4f, 5.0f, 0.0f, 0.6f), 0.0f, 2.0f);
    }
    if (sample < -0.4)
    {
        sample = clamp(sample - map(sample, -5.0f, -0.4f, -0.6f, 0.0f), -2.0f, 0.0f);
    }

    if (sample > 0.8)
    {
        sample = clamp(sample - map(sample, 0.8f, 2.0f, 0.0f, 0.1f), 0.0f, 0.9f);
    }
    if (sample < -0.8)
    {
        sample = clamp(sample - map(sample, -2.0f, -0.8f, -0.1f, 0.0f), -0.9f, 0.0f);
    }
    return sample;
}

/**
 * @brief Gets the next sample from the reverb effect.
 *
 * @param outl The left output sample.
 * @param outr The right output sample.
 * @param inl The left input sample.
 * @param inr The right input sample.
 */
void Reverb::getSample(float &outl, float &outr, float inl, float inr)
{
    float shimmer_l = 0.0f;
    float shimmer_r = 0.0f;

    if (((this->mv.mode == REV)) or (this->mv.mode == RESONATOR))
    {
        shimmer_l = IMultiVersioCommon::mlooper_buf_1l[reverb_shimmer_play_pos1l];
        shimmer_r = IMultiVersioCommon::mlooper_buf_1r[reverb_shimmer_play_pos1r];
        reverb_shimmer_play_pos1l = (reverb_shimmer_play_pos1l + 2) % reverb_shimmer_buffer_size1l;
        reverb_shimmer_play_pos1r = (reverb_shimmer_play_pos1r + 2) % reverb_shimmer_buffer_size1r;
        WriteShimmerBuffer1(inl, inr);
    }

    if (this->mv.mode == REV)
    {
        float octave2_shim = IMultiVersioCommon::mlooper_frozen_buf_1l[(int)reverb_shimmer_play_pos2] * 0.5f;
        shimmer_l += octave2_shim;
        shimmer_r += octave2_shim;
        // NOTE added the static cast in here to avoid a compiler error
        reverb_shimmer_play_pos2 = (static_cast<int>(reverb_shimmer_play_pos2) + 4) % reverb_shimmer_buffer_size2;
        if (reverb_shimmer_play_pos2 > reverb_shimmer_buffer_size2)
        {
            reverb_shimmer_play_pos2 -= reverb_shimmer_buffer_size2;
        }
        WriteShimmerBuffer2(inl, inr);
    }

    if (this->mv.mode == RESONATOR)
    {
        float octave2_shim = this->mv.mlooper_frozen_buf_1l[(int)reverb_shimmer_play_pos2];
        shimmer_l += octave2_shim;
        shimmer_r += octave2_shim;
        reverb_shimmer_play_pos2 = (reverb_shimmer_play_pos2 + 0.5);
        if (reverb_shimmer_play_pos2 > reverb_shimmer_buffer_size2)
        {
            reverb_shimmer_play_pos2 -= reverb_shimmer_buffer_size2;
        }
        WriteShimmerBuffer2(inl, inr);
    }

    reverb_rmsCount++;
    reverb_rmsCount %= (RMS_SIZE);

    if (reverb_rmsCount == 0)
    {
        reverb_target_RMS = reverb_averager.ProcessRMS();
    }

    daisysp::fonepole(reverb_current_RMS, reverb_target_RMS, .1f);
    daisysp::fonepole(reverb_feedback_RMS, reverb_target_RMS, .01f);

    if (this->mv.mode == REV)
    {
        // Set LED colors based on RMS values
        this->mv.leds.SetBaseColor(0, clamp(reverb_current_RMS, 0, 1), clamp(reverb_target_RMS, 0, 1),
                                   clamp(reverb_current_RMS, 0, 1) * clamp(reverb_current_RMS, 0, 1));
        this->mv.leds.SetBaseColor(1, clamp(reverb_target_RMS, 0, 1), clamp(reverb_target_RMS, 0, 1),
                                   clamp(reverb_target_RMS, 0, 1) * clamp(reverb_target_RMS, 0, 1));
        this->mv.leds.SetBaseColor(3, clamp(reverb_current_RMS, 0, 1), clamp(reverb_target_RMS, 0, 1),
                                   clamp(reverb_current_RMS, 0, 1) * clamp(reverb_current_RMS, 0, 1));
        this->mv.leds.SetBaseColor(2, clamp(reverb_target_RMS, 0, 1), clamp(reverb_target_RMS, 0, 1),
                                   clamp(reverb_target_RMS, 0, 1) * clamp(reverb_target_RMS, 0, 1));
    }

    this->mv.rev.SetFeedback(reverb_feedback - reverb_feedback_RMS * 0.75f);

    float sum_inl = (inl + shimmer_l * reverb_shimmer * (reverb_feedback * 0.5f + 0.5f) * (0.5f + reverb_current_RMS * 0.5f)) * 0.5f;
    float sum_inr = (inr + shimmer_r * reverb_shimmer * (reverb_feedback * 0.5f + 0.5f) * (0.5f + reverb_current_RMS * 0.5f)) * 0.5f;

    daisysp::fonepole(reverb_target_compression, this->mv.reverb_compression, .001f);

    sum_inl = CompressSample(sum_inl * reverb_target_compression);
    sum_inr = CompressSample(sum_inr * reverb_target_compression);

    this->mv.rev.Process(sum_inl, sum_inr, &outl, &outr);

    reverb_current_outl = outl;
    reverb_current_outr = outr;

    reverb_averager.Add((reverb_current_outl * reverb_current_outl + reverb_current_outr * reverb_current_outr) / 2);

    if (reverb_drywet > 0.98f)
    {
        reverb_drywet = 1.f;
    }

    outl = (sqrt(0.5f * (reverb_drywet * 2.0f)) * reverb_current_outl +
            sqrt(0.95f * (2.f - (reverb_drywet * 2))) * inl) *
           0.7f;
    outr = (sqrt(0.5f * (reverb_drywet * 2.0f)) * reverb_current_outr +
            sqrt(0.95f * (2.f - (reverb_drywet * 2))) * inr) *
           0.7f;
}

/**
 * @brief Runs the reverb effect with the specified parameters.
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
void Reverb::run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU)
{
    reverb_lowpass = this->mv.global_sample_rate * tone / 2.f;
    this->mv.rev.SetLpFreq(reverb_lowpass);
    reverb_shimmer = index;
    reverb_feedback = 0.8f + (std::log10(10 + regen * 90) - 1.000001f) * 0.4f;
    reverb_feedback_display = regen * 100;
    this->mv.rev.SetFeedback(reverb_feedback);
    this->mv.reverb_compression = dense + 0.5f;
    reverb_drywet = blend;
}
