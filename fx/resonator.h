#pragma once

#include "IEffect.h"
#include "IMultiVersioCommon.h"

/**
 * @brief Implements a resonator effect.
 */
class Resonator : public IEffect
{
public:
    Resonator(IMultiVersioCommon &mv);
    void run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU);
    void getSample(float &outl, float &outr, float inl, float inr);
    void SelectResonatorOctave(float speed);
    bool usesReverb();

    float resonator_feedback;
    float resonator_target;

private:
    float resonator_note;
    float resonator_tone;
    int resonator_feedback_display;
    float resonator_current_regen;
    float target_resonator_feedback;
    int resonator_rmsCount;
    float resonator_previous_l;
    float resonator_previous_r;
    float resonator_current_RMS;
    float resonator_target_RMS;
    float resonator_feedback_RMS;
    float resonator_current_delay;
    float resonator_drywet;
    int resonator_octave;
    float resonator_glide;
    int resonator_glide_mode;
    Averager resonator_averager;

    IMultiVersioCommon &mv;
};