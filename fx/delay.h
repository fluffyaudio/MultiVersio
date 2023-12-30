#pragma once

#include <daisy_versio.h>
#include "helpers.h"
#include "IEffect.h"
#include "IMultiVersioCommon.h"

#define NUM_DELAY_TIMES 17
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f) // 2.5 seconds max delay in the fast ram

class DelayEffect : public IEffect
{
public:
    float delay_mult_l[2], delay_mult_r[2];
    float delay_feedback;

    int delay_time_count;
    int delay_write_pos;
    int delay_control_counter, delay_time_trig;
    int delay_control_latency_ms;
    int delay_pos_l[2], delay_pos_r[2];
    int delay_time[2];
    float delay_outl[2], delay_outr[2];
    int delay_left_counter, delay_right_counter;
    int delay_left_counter_4, delay_right_counter_4;
    int delay_main_counter;
    int delay_active;
    int delay_inactive;
    float delay_xfade_current;
    float delay_xfade_target;

    int delay_frozen_start;
    int delay_frozen_end;
    int delay_frozen_pos;
    float delay_target_cutoff;
    float delay_cutoff, delay_drywet;
    bool delay_frozen;
    float delay_prev_sample_l, delay_prev_sample_r;
    bool delay_reduce_spikes_l, delay_reduce_spikes_r;

    float delay_spike_counter_l, delay_spike_counter_r;

    int delay_rmsCount;
    float delay_target_RMS, delay_feedback_RMS, delay_fast_feedback_RMS;

    float delay_times[NUM_DELAY_TIMES];

    Averager delay_averager;
    daisy::Parameter delay_cutoff_par;

    DelayEffect(IMultiVersioCommon &mv);
    void SelectDelayDivision(float knob1, float knob2);
    void run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU);
    void getSample(float &outl, float &outr, float inl, float inr);

private:
    IMultiVersioCommon &mv;
};
