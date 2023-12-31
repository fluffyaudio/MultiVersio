#include "core/helpers.h"
#include "libDaisy/src/daisy_versio.h"
#include "DaisySP/Source/daisysp.h"
#include "core/IMultiVersioCommon.h"
#include "fx/lofi.h"

/**
 * @brief Construct a new Lofi::Lofi object
 *
 * @param mv A reference to the IMultiVersioCommon interface
 */
Lofi::Lofi(IMultiVersioCommon &mv) : mv(mv)
{
    // Initialize member variables
    lofi_rmsCount = 0;
    lofi_target_RMS = 0.0f;
    lofi_current_RMS = 0.0f;
    lofi_rate_count = 0;
    lofi_target_Lofi_LFO_Freq = 0.0f;
    lofi_current_Lofi_LFO_Freq = 0.0f;
    lofi_damp_speed = 0;
    lofi_drywet = 0.0f;
    lofi_previous_variable_compressor = 0.0f;

    lofi_damp_speed = mv.versio.AudioSampleRate();
    lofi_target_Lofi_LFO_Freq = lofi_current_Lofi_LFO_Freq = mv.versio.AudioSampleRate();
    lofi_current_RMS = lofi_target_RMS = 0.f;
    lofi_previous_left_saturation = lofi_previous_right_saturation = 0.5f;
    lofi_current_left_saturation = lofi_current_right_saturation = 0.5f;
    lofi_previous_variable_compressor = 0.0f;

    lofi_tone_par.Init(mv.versio.knobs[daisy::DaisyVersio::KNOB_2], 20, 20000, lofi_tone_par.LOGARITHMIC);
    lofi_rate_par.Init(mv.versio.knobs[daisy::DaisyVersio::KNOB_1], mv.versio.AudioSampleRate() * 4, mv.versio.AudioSampleRate() / 16, lofi_rate_par.LOGARITHMIC);
}

/**
 * Applies lo-fi processing to the input audio samples and generates the output samples.
 * The lo-fi effect includes RMS calculation, envelope following, filtering, compression, saturation, and delay.
 *
 * @param outl The left channel output sample.
 * @param outr The right channel output sample.
 * @param inl The left channel input sample.
 * @param inr The right channel input sample.
 */
void Lofi::processSample(float &outl, float &outr, float inl, float inr)
{
    inl = inl * 0.8f;
    inr = inr * 0.8f;
    // RMS calculation with smoothing for the envelope follower and the variable compressor.
    // RMS is calculated every RMS_SIZE samples
    lofi_rmsCount++;
    lofi_rmsCount %= (RMS_SIZE);

    if (lofi_rmsCount == 0)
    {
        lofi_target_RMS = lofi_averager.ProcessRMS() * lofi_lpg_amount * 10.f;
    }

    if (lofi_target_RMS < lofi_current_RMS)
    {
        daisysp::fonepole(lofi_current_RMS, lofi_target_RMS, .005f * lofi_lpg_decay * 10.f);
    }
    else
    {
        daisysp::fonepole(lofi_current_RMS, lofi_target_RMS, .05f);
    }
    // RMS value is stored into the averager.
    lofi_averager.Add((inl * inl + inr * inr) / 2);

    // envelope follower partfor opening the lowpass filter
    float lofi_envelope_follower = clamp(lofi_current_RMS * lofi_cutoff * 13.0f, 20.f, 20000.f);
    this->mv.leds.SetBaseColor(1, lofi_current_RMS, 0, 0);
    this->mv.leds.SetBaseColor(0, lofi_current_RMS * lofi_current_RMS, 0, 0);
    this->mv.leds.SetBaseColor(3, lofi_current_RMS * lofi_current_RMS, 0, 0);
    this->mv.leds.SetBaseColor(2, lofi_current_RMS, 0, 0);

    this->mv.tonel.SetFreq(lofi_envelope_follower);
    this->mv.toner.SetFreq(lofi_envelope_follower);

    this->mv.rev.SetLpFreq((lofi_envelope_follower * 0.6) + (global_sample_rate * 0.3));

    this->mv.svf2l.set_f_q<stmlib::FREQUENCY_FAST>(lofi_envelope_follower / global_sample_rate, 1.f);
    this->mv.svf2r.set_f_q<stmlib::FREQUENCY_FAST>(lofi_envelope_follower / global_sample_rate, 1.f);
    // when knob1 is very low an hi-pass filter activates to create a more distant sound
    float lofi_hipass_freq = clamp(200.0 - (lofi_cutoff * 2), 0.0f, 200);
    this->mv.svfl.SetFreq(lofi_hipass_freq);
    this->mv.svfr.SetFreq(lofi_hipass_freq);

    // knob 2 sets how often the delay modulation changes. This time is variable between 0 and lofi_mod time.

    lofi_rate_count++;
    lofi_rate_count %= lofi_mod;
    if (lofi_rate_count == 0)
    {
        this->mv.leds.SetForXCycles(1, 10, 0.0, 0.0, 0.0);
        this->mv.leds.SetForXCycles(2, 10, 0.0, 0.0, 0.0);

        float r = (float)(rand() % lofi_mod);
        lofi_rate_count = rand() % (lofi_mod);
        lofi_target_Lofi_LFO_Freq = 0.001f + (r * lofi_depth) / 5.f;
        lofi_damp_speed = lofi_rate_count;
    }
    // This smoothing allows for the delay time to change slowly so the pitch shifting effect is subtle
    daisysp::fonepole(lofi_current_Lofi_LFO_Freq, lofi_target_Lofi_LFO_Freq, 1.0 / (1.2f * (lofi_damp_speed + (lofi_mod * 3) / 2)));

    IMultiVersioCommon::dell.SetDelay(lofi_current_Lofi_LFO_Freq);
    IMultiVersioCommon::delr.SetDelay(lofi_current_Lofi_LFO_Freq);

    // here we already read the contents of the delay and assign it to the ouputs.

    if (lofi_drywet > 0.98f)
    {
        lofi_drywet = 1.f;
    }
    outl = sqrt(0.5f * (lofi_drywet * 2.0f)) * IMultiVersioCommon::dell.Read() + sqrt(0.95f * (2.f - (lofi_drywet * 2))) * inl;
    outr = sqrt(0.5f * (lofi_drywet * 2.0f)) * IMultiVersioCommon::delr.Read() + sqrt(0.95f * (2.f - (lofi_drywet * 2))) * inr;

    // now we process the input and we add it to the delay line
    // first we filter it with the two filters: low-pass-gate and hi-pass
    // they work a bit differently. The Tone objects accepts the input as a parameter
    // and gives back a filtered sample. The SVF accepts an input and we retrive the
    // hi-pass ouput with High
    float filter_outl = this->mv.svf2l.Process<stmlib::FILTER_MODE_LOW_PASS>(inl);
    float filter_outr = this->mv.svf2r.Process<stmlib::FILTER_MODE_LOW_PASS>(inr);

    this->mv.svfl.Process(this->mv.tonel.Process(filter_outl));
    this->mv.svfr.Process(this->mv.toner.Process(filter_outr));
    float lofi_leftFilter = this->mv.svfl.High();
    float lofi_rightFilter = this->mv.svfr.High();

    // Here we calculate a basic compressor using the RMS values of the envelope follower
    // this way we compensate for the lack of volume when filtering
    // by interpolating it with the previous state of the compressor, we slightly smooth
    // the change avoiding potential clicks.
    float lofi_variable_compressor = (2.f * lofi_previous_variable_compressor + ((300.f - clamp(lofi_cutoff, 30.f, 300.f)) / 300.f) * (1.f - lofi_current_RMS) * 1.f) * 0.33f;
    lofi_previous_variable_compressor = lofi_variable_compressor;

    // This is a simple saturation that will contribute in enhancing the volume "vintage way"
    // when knob 1 is low
    float lofi_current_left_saturation = abs(inl * inl);
    lofi_previous_left_saturation = (lofi_current_left_saturation + lofi_previous_left_saturation * 9.f) / 10.f;
    float lofi_current_right_saturation = abs(inr * inr);
    lofi_previous_right_saturation = (lofi_current_right_saturation + lofi_previous_right_saturation * 9.f) / 10.f;

    // Here we calculate the outputs, which are the filtered waveform plus, a certain amount of the other channel
    // to "monoize it" when knob 1 is low, plus a certain amount of compression and saturation.
    float lofi_left = lofi_leftFilter + (((200.f - clamp(lofi_cutoff, 20.f, 200.f)) / 200.f) * lofi_rightFilter) + lofi_leftFilter * lofi_variable_compressor + lofi_leftFilter * lofi_variable_compressor * lofi_current_left_saturation * 0.01f;
    float lofi_right = lofi_rightFilter + (((200.f - clamp(lofi_cutoff, 20.f, 200.f)) / 200.f) * lofi_leftFilter) + lofi_rightFilter * lofi_variable_compressor + lofi_rightFilter * lofi_variable_compressor * lofi_current_right_saturation * 0.01f;

    // we still add a certain amount of the other channel to further monoize the sound
    lofi_left = lofi_left + lofi_right * ((200.f - clamp(lofi_cutoff, 20.f, 200.f)) / 200.f);
    lofi_right = lofi_right + lofi_left * ((200.f - clamp(lofi_cutoff, 20.f, 200.f)) / 200.f);

    // this is a basic instantaneous saturation/limiter: if the sound is too loud (in either)
    // directions, we compress it to avoid digital clipping.
    if (lofi_left > 0.4)
    {
        lofi_left = clamp(lofi_left - map(lofi_left, 0.4f, 10.0f, 0.0f, 0.6f), 0.0f, 1.0f);
    }
    if (lofi_right > 0.4)
    {
        lofi_right = clamp(lofi_right - map(lofi_right, 0.4f, 10.0f, 0.0f, 0.6f), 0.0f, 1.0f);
    }

    if (lofi_left < -0.4)
    {
        lofi_left = clamp(lofi_left - map(lofi_left, -10.0f, -0.4f, -0.6f, 0.0f), -1.0f, 0.0f);
    }
    if (lofi_right < -0.4)
    {
        lofi_right = clamp(lofi_right - map(lofi_right, -10.0f, -0.4f, -0.6f, 0.0f), -1.0f, 0.0f);
    }

    // we write all of this on the delayline
    IMultiVersioCommon::dell.Write(lofi_left);
    IMultiVersioCommon::delr.Write(lofi_right);
};

/**
 * Runs the lo-fi effect with the specified parameters.
 *
 * @param blend The blend parameter for the effect.
 * @param regen The regen parameter for the effect.
 * @param tone The tone parameter for the effect.
 * @param speed The speed parameter for the effect.
 * @param size The size parameter for the effect.
 * @param index The index parameter for the effect.
 * @param dense The dense parameter for the effect.
 * @param gate Effect gate from the FSU input.
 */
void Lofi::run(float blend, float regen, float tone, float speed, float size, float index, float dense, bool gate)
{

    // blend = lofi drywet
    // tone = lofi lpg cutoff
    // speed = lofi rate
    // index = lofi_depth
    // regen = reverb amount
    // size = lpg amount
    // dense = lpg decay

    lofi_cutoff = lofi_tone_par.Process(); // tone
    lofi_depth = index * 2;                // index
    this->mv.tonel.SetFreq(lofi_cutoff);
    this->mv.toner.SetFreq(lofi_cutoff);
    lofi_mod = (int)lofi_rate_par.Process();
    lofi_drywet = blend * 1.01; // IMPLEMENT

    this->mv.reverb_lowpass = lofi_cutoff;
    this->mv.rev.SetLpFreq(this->mv.reverb_lowpass);
    this->mv.reverb_feedback = 0.7f + (std::log10(10 + regen * 90) - 1.000001f) * 0.3f;
    this->mv.rev.SetFeedback(this->mv.reverb_feedback);

    lofi_lpg_amount = size * size * 3;
    lofi_lpg_decay = clamp((1 - ((std::log10(10 + dense * 90) - 0.999991f) * 0.4f + 0.6f)) * 0.05f, 0.0001, 0.99999);
    // Shimmer
    this->mv.reverb_shimmer = 0.0f;
    this->mv.reverb_compression = 0.5f;
    // DRY WET
    this->mv.reverb_drywet = regen * 0.8f;
}

/**
 * Indicates whether the lo-fi effect uses reverb.
 *
 * @return true  as the lo-fi effect uses reverb.
 */
bool Lofi::usesReverb()
{
    return true;
}