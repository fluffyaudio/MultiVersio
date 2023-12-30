#include "delay.h"
#include "IMultiVersioCommon.h"

DelayEffect::DelayEffect(IMultiVersioCommon &mv) : mv(mv)
{
}

void DelayEffect::SelectDelayDivision(float knob1, float knob2)
{
    // sets the octave shift
    float new_delay_mult_l;
    new_delay_mult_l = delay_times[(int)(knob1 * NUM_DELAY_TIMES)];
    float new_delay_mult_r;
    new_delay_mult_r = delay_times[(int)(knob2 * NUM_DELAY_TIMES)];

    if ((new_delay_mult_l != delay_mult_l[delay_active]) or (new_delay_mult_r != delay_mult_r[delay_active]))
    {
        delay_mult_l[delay_inactive] = new_delay_mult_l;
        delay_mult_r[delay_inactive] = new_delay_mult_r;
    }
}

void DelayEffect::getSample(float &outl, float &outr, float inl, float inr){
    // TODO
};

void DelayEffect::run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU){
    // TODO
};