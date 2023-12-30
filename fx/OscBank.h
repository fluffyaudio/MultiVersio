#pragma once

#include "daisy_versio.h"
#include "daisysp.h"
#include "IMultiVersioCommon.h"
#include "ISpectra.h"
#include "shy_fft.h"

typedef stmlib::ShyFFT<float, FFT_LENGTH, stmlib::RotationPhasor> FFT;

class OscBank
{
public:
    static const int number_of_osc = MAX_SPECTRA_FREQUENCIES;
    static const int spectra_max_num_frequencies = MAX_SPECTRA_FREQUENCIES;
    daisysp::Oscillator osc[number_of_osc];
    float freq[number_of_osc];
    float magn[number_of_osc];

    float current_freq[number_of_osc];
    float current_magn[number_of_osc];
    size_t num_of_passes;
    float fftinbuff[FFT_LENGTH];
    float window[FFT_LENGTH];
    float window_fftinbuff[FFT_LENGTH];
    float fftoutbuff[FFT_LENGTH];
    float magni_fftoutbuff[FFT_LENGTH / 2];
    float bandSize;
    float maxAmp = 0;
    int num_active;
    float output_mult, prev_output_mult = 0.f;
    int firstUsableBin;
    float amp_attenuation = 1.f;
    int previous_wave = 0;
    int current_wave = 0;
    FFT fft;
    size_t attack_step[number_of_osc];
    bool mark_to_change_waveform[number_of_osc];

    size_t hop = 8;

    OscBank(IMultiVersioCommon &mv, ISpectra &spectra);
    float getFrequency(int index);
    float getMagnitudo(int index);
    size_t GetPasses();
    float Process();
    float SetAllWaveforms(int waveform);
    void calculatedSuggestedHop();
    void CalculateSpectralanalysis();
    void FillInputBuffer(const float *in1, const float *in2, size_t size);
    void Init(float sample_rate);
    void RemoveNearestBands(float frequency, size_t start_band);
    void SetAmp(int index, float amplitude);
    void SetFreq(int index, float frequency);
    void SetNumActive(int num_active);
    void updateFreqAndMagn();

private:
    IMultiVersioCommon &mv;
    ISpectra &spectra;
};
