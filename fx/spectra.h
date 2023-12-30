#pragma once

#include "IEffect.h"
#include "OscBank.h"
#include "ISpectra.h"
#include "IMultiVersioCommon.h"

const bool scale_12[12] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const bool scale_7[12] = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
const bool scale_6[12] = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0};
const bool scale_5[12] = {1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0};
const bool scale_4[12] = {1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0};
const bool scale_3[12] = {1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0};
const bool scale_2[12] = {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};
const bool scale_1[12] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int CHRM_SCALE[128] = {8176, 8662, 9177, 9723, 10301, 10913, 11562, 12250, 12978, 13750, 14568, 15434, 16352, 17324, 18354, 19445, 20602, 21827, 23125, 24500, 25957, 27500, 29135, 30868, 32703, 34648, 36708, 38891, 41203, 43654, 46249, 48999, 51913, 55000, 58270, 61735, 65406, 69296, 73416, 77782, 82407, 87307, 92499, 97999, 103826, 110000, 116541, 123471, 130813, 138591, 146832, 155563, 164814, 174614, 184997, 195998, 207652, 220000, 233082, 246942, 261626, 277183, 293665, 311127, 329628, 349228, 369994, 391995, 415305, 440000, 466164, 493883, 523251, 554365, 587330, 622254, 659255, 698456, 739989, 783991, 830609, 880000, 932328, 987767, 1046502, 1108731, 1174659, 1244508, 1318510, 1396913, 1479978, 1567982, 1661219, 1760000, 1864655, 1975533, 2093005, 2217461, 2349318, 2489016, 2637020, 2793826, 2959955, 3135963, 3322438, 3520000, 3729310, 3951066, 4186009, 4434922, 4698636, 4978032, 5274041, 5587652, 5919911, 6271927, 6644875, 7040000, 7458620, 7902133, 8372018, 8869844, 9397273, 9956063, 10548080, 11175300, 11839820, 12543850};

/**
 * @brief Implements the Spectra effect
 */
class Spectra : public IEffect, public ISpectra
{
private:
    IMultiVersioCommon &mv;

public:
    Spectra(IMultiVersioCommon &mv);
    void run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU);
    void SelectSpectraOctave(float knob_value_1);
    void SelectSpectraQuality(float knob_value_1);
    void processSample(float &outl, float &outr, float inl, float inr);
    bool usesReverb();
    void preProcess(const float *in1, const float *in2, size_t size);

    int spectra_waveform = 0;
    float spectra_r, spectra_g, spectra_b = 0.f;
    float spectra_prev_knob_wave_knob = 0.f;
    int spectra_num_active = MAX_SPECTRA_FREQUENCIES;
    const int spectra_max_num_frequencies = MAX_SPECTRA_FREQUENCIES;
    int spectra_oct = 0;
    int spectra_hop = 1;
    float spectra_drywet;
    float spectra_lower_harmonics = 0.f;
    float spectra_oct_mult;
    std::string spectra_oct_string;
    float spectra_reverb_amount = 0.f;
    bool spectra_do_analysis = false;
    int spectra_rmsCount = 0;
    float spectra_current_RMS, spectra_target_RMS = 1.f;
    float spectra_spread = 1.0f;
    float spectra_rotate_harmonics = 0.0f;
    int spectra_transpose = 0;
    int spectra_quantize = 0;
    const bool *spectra_selected_scale;

    Averager spectra_averager;
    OscBank spectra_oscbank;
};
