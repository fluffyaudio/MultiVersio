/**
 * @file OscBank.cpp
 * @brief Implementation of the OscBank class.
 *
 * The OscBank class represents a bank of oscillators used for spectral analysis and synthesis.
 * It provides methods for initializing the oscillators, setting their frequency and amplitude,
 * selecting waveforms, processing the oscillators, and performing spectral analysis.
 */
#include "libDaisy/src/daisy_versio.h"
#include "DaisySP/Source/daisysp.h"
#include "IMultiVersioCommon.h"
#include "ISpectra.h"
#include "OscBank.h"

/**
 * @brief Initializes the OscBank.
 *
 * @param mv The IMultiVersioCommon object.
 * @param spectra The ISpectra object.
 */
OscBank::OscBank(IMultiVersioCommon &mv, ISpectra &spectra) : mv(mv), spectra(spectra)
{
    this->num_active = spectra.spectra_num_active;

    for (size_t t = FFT_LENGTH; t > 1; t >>= 1)
    {
        ++num_of_passes;
    }
};

/**
 * @brief Initializes the oscillators.
 *
 * @param sample_rate The sample rate.
 */
void OscBank::Init(float sample_rate)
{
    for (int i = 0; i < number_of_osc; i++)
    {
        osc[i].Init(sample_rate);
        // float randomPhase = (rand() %1000)/1000.f;
        // osc[i].PhaseAdd(randomPhase);
        freq[i] = 0;
        magn[i] = 0;
        current_freq[i] = 0;
        current_magn[i] = 0;
        osc[i].SetWaveform(0);
        attack_step[i] = 0;
        mark_to_change_waveform[i] = false;
    };
    for (size_t i = 0; i < FFT_LENGTH; i++)
    {
        fftinbuff[i] = 0;
        fftoutbuff[i] = 0;
        if (i < FFT_LENGTH / 2)
        {
            magni_fftoutbuff[i] = 0;
        }
        window[i] = ApplyWindow(1.0f, i, FFT_LENGTH);
    }
    this->fft.Init();

    bandSize = sample_rate / (FFT_LENGTH * hop);
};

/**
 * @brief Sets the frequency of an oscillator.
 *
 * @param index The index of the oscillator.
 * @param frequency The frequency of the oscillator.
 */
void OscBank::SetFreq(int index, float frequency)
{
    osc[index].SetFreq(frequency);
};

/**
 * @brief Sets the amplitude of an oscillator.
 *
 * @param index The index of the oscillator.
 * @param amplitude The amplitude of the oscillator.
 */
void OscBank::SetAmp(int index, float amplitude)
{
    osc[index].SetAmp(amplitude);
};

/**
 * @brief Sets the waveform of all oscillators
 *
 * @param index The index of the oscillator.
 * @param waveform The waveform of the oscillator.
 */
float OscBank::SetAllWaveforms(int waveform)
{
    current_wave = 0;
    float centre_of_position = 0.f;

    switch (waveform)
    {
    case 0:
        current_wave = 0;
        amp_attenuation = 1.f;
        centre_of_position = 0.5f / 9.f;
        break;
    case 1:
        current_wave = 8;
        amp_attenuation = 1.f;
        centre_of_position = 1.5f / 9.f;
        break;
    case 2:
        current_wave = 1;
        amp_attenuation = 0.9f;
        centre_of_position = 2.5f / 9.f;
        break;
    case 3:
        current_wave = 5;
        amp_attenuation = 0.9f;
        centre_of_position = 3.5f / 9.f;
        break;
    case 4:
        current_wave = 7;
        amp_attenuation = 0.35f;
        centre_of_position = 4.5f / 9.f;
        break;
    case 5:
        current_wave = 2;
        amp_attenuation = 0.40f;
        centre_of_position = 5.5f / 9.f;
        break;
    case 6:
        current_wave = 3;
        amp_attenuation = 0.45f;
        centre_of_position = 6.5f / 9.f;
        break;
    case 7:
        current_wave = 6;
        amp_attenuation = 0.45f;
        centre_of_position = 7.5f / 9.f;
        break;
    case 8:
        current_wave = 4;
        amp_attenuation = 0.4f;
        centre_of_position = 8.5f / 9.f;
        break;
    }

    for (int i = 0; i < number_of_osc; i++)
    {
        // the second time it actually changes the waveform, so the attack lut can avoid clicks
        if (mark_to_change_waveform[i])
        {
            osc[i].SetWaveform(current_wave);
            mark_to_change_waveform[i] = false;
        }
        // the first time it just marks the waveform to be changed
        if (previous_wave != current_wave)
        {

            mark_to_change_waveform[i] = true;
            attack_step[i] = 0;
        };
    }
    if (previous_wave != current_wave)
    {
        previous_wave = current_wave;
    }
    return centre_of_position;
};

float OscBank::Process()
{
    float output = 0;
    for (int i = 0; i < spectra_max_num_frequencies; i++)
    {
        output_mult = ((0.5 + 0.2 / num_active) + prev_output_mult * 47.f) / 48.f;
        output = output + osc[i].Process() * output_mult * this->mv.attack_lut[attack_step[i]];
        attack_step[i] = clamp(attack_step[i] + 1, 0, 299);
        prev_output_mult = output_mult;
    };
    return output;
}

/**
 * @brief Gets the number of passes of the FFT
 * @return size_t
 */
size_t OscBank::GetPasses()
{
    return num_of_passes;
}

/**
 * @brief Fills the input buffer with the input samples.
 *
 * @param in1 The first input buffer.
 * @param in2 The second input buffer.
 * @param size The size of the input buffers.
 */
void OscBank::FillInputBuffer(const float *in1, const float *in2, size_t size)
{
    bandSize = this->mv.global_sample_rate / (FFT_LENGTH * hop);

    size_t real_size = size / hop;
    this->mv.svfl.SetRes(0.1);
    this->mv.svfl.SetFreq(this->mv.global_sample_rate / (2 * hop));
    this->mv.svfr.SetRes(0.1);
    this->mv.svfr.SetFreq(bandSize * (32 / hop));
    // shift left the input array of "size" n of samples
    for (size_t i = 0; i < FFT_LENGTH - real_size; i++)
    {
        fftinbuff[i] = fftinbuff[i + real_size];
    };
    // add the samples to the input buffer

    for (size_t i = 0; i < real_size; i++)
    {
        float sum = 0.f;
        for (size_t j = 0; j < hop; j++)
        {
            float sample = (in1[i * hop + j] + in2[i * hop + j]) * 0.707;
            this->mv.svfl.Process(sample);
            this->mv.svfr.Process(this->mv.svfl.Low());
            sum = sum + this->mv.svfr.High() / hop;
        }
        float sample = sum;
        fftinbuff[i + FFT_LENGTH - real_size] = sample;
    };
    for (size_t i = 0; i < FFT_LENGTH; i++)
    {
        window_fftinbuff[i] = window[i] * fftinbuff[i];
    }
    this->fft.Direct(window_fftinbuff, fftoutbuff);
}

/**
 * @brief Calculates the spectral analysis.
 *
 * This function calculates the spectral analysis of the input buffer.
 */
void OscBank::CalculateSpectralanalysis()
{

    for (size_t i = 0; i < FFT_LENGTH / 2; i++)
    {
        if (i < 32 / hop)
        {
            fftoutbuff[i] = fftoutbuff[i] * 0.5;
        }
        float real = fftoutbuff[i];
        float imag = fftoutbuff[i + (FFT_LENGTH / 2)];
        magni_fftoutbuff[i] = sqrt(real * real + imag * imag);
    }
    const int N = sizeof(magni_fftoutbuff) / sizeof(float);
    int num_frequencies = MAX_SPECTRA_FREQUENCIES;
    float max_amp = 20.0f;

    for (int i = 0; i < spectra_max_num_frequencies; i++)
    {
        int a = std::distance(magni_fftoutbuff, std::max_element(magni_fftoutbuff, magni_fftoutbuff + (N / 2)));

        freq[i] = a * bandSize * this->spectra.spectra_oct_mult;
        if (this->spectra.spectra_quantize > 0)
        {

            freq[i] = findClosest(this->spectra.CHRM_SCALE, this->spectra.spectra_selected_scale, 128, ((int)freq[i] * 1000), this->spectra.spectra_transpose) / 1000.f;
        };
        max_amp = std::max(magni_fftoutbuff[a], max_amp);
        magn[i] = ((magni_fftoutbuff[a] / max_amp) * (1 - this->spectra.spectra_lower_harmonics) + this->spectra.spectra_lower_harmonics);
        if (freq[i] > this->mv.global_sample_rate / 2)
        {
            freq[i] = 0.0f;
            magn[i] = 0.0f;
        }

        RemoveNearestBands(a * bandSize, a);
    }

    for (int i = num_active; i < num_frequencies; i++)
    {
        magn[i] = 0.f;
    }

    // rightRotate(magn,spectra_rotate_harmonics, num_active);
}

/**
 * @brief Removes the nearest bands.
 *
 * @param frequency The frequency.
 * @param start_band The start band.
 */
void OscBank::RemoveNearestBands(float frequency, size_t start_band)
{
    magni_fftoutbuff[start_band] = 0.f;
    float upper_frequency = daisysp::mtof((int)((12.f * log2(frequency / 440.f) + 69 + 1)));
    for (size_t i = start_band; (((i * bandSize / this->spectra.spectra_spread) < upper_frequency) & (i < FFT_LENGTH / 2) & (-i > 0)); i++)
    {
        float mult = map((i * bandSize / this->spectra.spectra_spread), (1 * bandSize / this->spectra.spectra_spread), upper_frequency, 0.0f, 1.0f);
        magni_fftoutbuff[i] = magni_fftoutbuff[i] * mult;
        magni_fftoutbuff[-i] = magni_fftoutbuff[-i] * mult;
    }
}

/**
 * @brief Gets the frequency of an oscillator.
 *
 * @param value The index of the oscillator.
 * @return float
 */
float OscBank::getFrequency(int value)
{
    return current_freq[value];
}

/**
 * @brief Gets the magnitude of an oscillator.
 *
 * @param value The index of the oscillator.
 * @return float
 */
float OscBank::getMagnitudo(int value)
{
    return current_magn[value];
}

/**
 * @brief Updates the frequency and magnitude of the oscillators.
 *
 * This function updates the frequency and magnitude of the oscillators.
 * It is used to avoid clicks when changing the frequency and magnitude of the oscillators.
 *
 * @return void
 */
void OscBank::updateFreqAndMagn()
{
    for (int i = 0; i < spectra_max_num_frequencies; i++)
    {
        float new_freq = (freq[i] + current_freq[i] * 47) / 48;
        SetFreq(i, new_freq);
        current_freq[i] = new_freq;

        float new_magn = ((magn[i] + current_magn[i] * 47) / 48);
        SetAmp(i, current_magn[i] * amp_attenuation);
        current_magn[i] = new_magn;
    }
}

/**
 * @brief Calculates the suggested hop.
 *
 * This function calculates the suggested hop.
 * It is used to avoid clicks when changing the frequency and magnitude of the oscillators.
 *
 * @return void
 */
void OscBank::calculatedSuggestedHop()
{

    // if ((current_freq[0]/spectra_oct_mult) > 110) {
    hop = 16;
    //}
    // if ((current_freq[0]) > 880) {
    //    hop = 8;
    //}
    // if ((current_freq[0]) > 1760) {
    //    hop = 4;
    //}
}

/**
 * @brief Sets the number of active oscillators.
 *
 * @param value The number of active oscillators.
 * @return void
 */
void OscBank::SetNumActive(int value)
{
    num_active = value;
};
