#pragma once

#include "../core/helpers.h"
#include <daisy_versio.h>
#include "mlooper.h"
#include "IEffect.h"
#include "IMultiVersioCommon.h"

/**
 * @brief Implements a reverb effect.  This is also used by a few other effects.
 */
class Reverb : public IEffect
{
public:
    Reverb(IMultiVersioCommon &mv);
    void WriteShimmerBuffer1(float in_l, float in_r);
    void WriteShimmerBuffer2(float in_l, float in_r);
    float CompressSample(float sample);
    void getSample(float &outl, float &outr, float inl, float inr);
    void run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU);

    float reverb_drywet, reverb_feedback, reverb_lowpass, reverb_shimmer;
    int reverb_shimmer_write_pos1l, reverb_shimmer_write_pos1r, reverb_shimmer_write_pos2;
    int reverb_shimmer_play_pos1l, reverb_shimmer_play_pos1r;
    float reverb_shimmer_play_pos2;

    int reverb_shimmer_buffer_size1l;
    int reverb_shimmer_buffer_size1r;
    int reverb_shimmer_buffer_size2;

    float reverb_previous_inl, reverb_previous_inr;
    float reverb_current_outl, reverb_current_outr;

    int reverb_feedback_display = 0;
    int reverb_rmsCount;
    float reverb_current_RMS, reverb_target_RMS, reverb_feedback_RMS = 0.f;
    float reverb_target_compression, reverb_compression = 1.0f;

private:
    IMultiVersioCommon &mv;
    Averager reverb_averager;
};