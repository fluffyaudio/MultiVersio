#include "delay.h"
#include "IMultiVersioCommon.h"

/**
 * @brief Construct a new DelayEffect::DelayEffect object
 *
 * @param mv A reference to the IMultiVersioCommon interface
 */
DelayEffect::DelayEffect(IMultiVersioCommon &mv) : mv(mv)
{
}

/**
 * @brief Selects the delay division based on the given knob values.
 *
 * The delay division is determined as follows:
 *
 * - If the knob value is less than 0.2, the delay division is set to 1/4.
 * - If the knob value is between 0.2 and 0.4, the delay division is set to 1/8.
 * - If the knob value is between 0.4 and 0.6, the delay division is set to 1/16.
 * - If the knob value is between 0.6 and 0.8, the delay division is set to 1/32.
 * - If the knob value is greater than 0.8, the delay division is set to 1/64.
 *
 * @param knob1 The value of the first knob used to select the delay division.
 * @param knob2 The value of the second knob used to select the delay division.
 */
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

/**
 * @brief Gets the next sample from the delay effect.
 *
 * @todo This method is not implemented
 * @param outl The left output sample.
 * @param outr The right output sample.
 * @param inl The left input sample.
 * @param inr The right input sample.
 */
void DelayEffect::getSample(float &outl, float &outr, float inl, float inr){
    // TODO
};

/**
 * @brief Runs the delay effect.
 *
 * @todo This method is not implemented
 * @param blend The value of the blend knob.
 * @param regen The value of the regen knob.
 * @param tone The value of the tone knob.
 * @param speed The value of the speed knob.
 * @param size The value of the size knob.
 * @param index The value of the index knob.
 * @param dense The value of the dense knob.
 * @param FSU The value of the FSU knob.
 */
void DelayEffect::run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU){
    // TODO
};

/**
 * @brief Indicates whether the delay effect uses reverb.
 *
 * @return True, as the delay effect uses reverb
 */
bool DelayEffect::usesReverb()
{
    return true;
}