#include "daisysp.h"
#include "daisy_versio.h"
#include <string>
#include "arm_math.h"
#include "stmlib/fft/shy_fft.h"
#include "stmlib/dsp/filter.h"

using namespace daisy;
using namespace daisysp;

DaisyVersio versio;

#define FFT_LENGTH 1024
#define MAX_SPECTRA_FREQUENCIES 6
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f)   //2.5 seconds max delay in the fast ram
#define LOOPER_MAX_SIZE (48000 * 60 * 1) // 1 minutes stereo of floats at 48 khz

//TO ADD: COMPLETE VOICE (ADSR + VCA + FILTER + REVERB)
//NATURAL GATE: LPG with SPECTRAL ANALISYS FOR NOTE HEIGHT, NOTE REPETITION DISTANCE
//CLOCKED DELAY NORMALE

#define REV 0
#define RESONATOR 1
#define FILTER 2
#define LOFI 3
#define MLOOPER 4
#define DELAY 5
#define SPECTRA 6
#define SPECTRINGS 7
#define NATURAL_GATE 8


#define NUM_MODES 9

const char *modes[NUM_MODES] = { "Reverb", "Resonator",
                             "Filter", "LO-FI", "MicroLooper",  "Delay", "Spectra", "Spectrings", "Natural Gate"};

#define NUM_DELAY_TIMES 17

#define RMS_SIZE 48
#define NUM_OF_STRINGS 2

const float delay_times[NUM_DELAY_TIMES] = {0.0078125,0.015625, 0.03125, 0.25/6.f, 0.046875, 0.0625,
                        0.25/3.f, 0.09375, 0.125, 0.5/3.f, 0.1875, 0.25, 1.f/3.f,
                        0.375, 0.5f, 0.75f, 1.f};
//These are reusable between effects to save memory
static ReverbSc                                  rev;

static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS dell;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delr;



static Svf                                       svfl;
static Svf                                       svfr;

static stmlib::Svf                               svf2l;
static stmlib::Svf                               svf2r;


static Tone                                      tonel;
static Tone                                      toner;

static Biquad                                   biquad;

static StringVoice                         string_voice[NUM_OF_STRINGS];

const size_t kMaxFftSize = FFT_LENGTH;
typedef stmlib::ShyFFT<float, kMaxFftSize, stmlib::RotationPhasor> FFT;


const size_t FFT_SIZE = FFT_LENGTH;
//Individual parameters for each effect
//static Parameter crusher_cutoff_par, crusher_crushrate_par;
static Parameter lofi_tone_par, lofi_rate_par;
static Parameter lofi_reverb_tone_par,lofi_reverb_rate_par;
static Parameter filter_cutoff_l_par, filter_cutoff_r_par;
static Parameter delay_cutoff_par;
static DcBlock dcblock_l, dcblock_r;
static DcBlock dcblock_2l, dcblock_2r;

int CHRM_SCALE[128] = {8176	,8662	,9177	,9723	,10301	,10913	,11562	,12250	,12978	,13750	,14568	,15434	,16352	,17324	,18354	,19445	,20602	,21827	,23125	,24500	,25957	,27500	,29135	,30868	,32703	,34648	,36708	,38891	,41203	,43654	,46249	,48999	,51913	,55000	,58270	,61735	,65406	,69296	,73416	,77782	,82407	,87307	,92499	,97999	,103826	,110000	,116541	,123471	,130813	,138591	,146832	,155563	,164814	,174614	,184997	,195998	,207652	,220000	,233082	,246942	,261626	,277183	,293665	,311127	,329628	,349228	,369994	,391995	,415305	,440000	,466164	,493883	,523251	,554365	,587330	,622254	,659255	,698456	,739989	,783991	,830609	,880000	,932328	,987767	,1046502	,1108731	,1174659	,1244508	,1318510	,1396913	,1479978	,1567982	,1661219	,1760000	,1864655	,1975533	,2093005	,2217461	,2349318	,2489016	,2637020	,2793826	,2959955	,3135963	,3322438	,3520000	,3729310	,3951066	,4186009	,4434922	,4698636	,4978032	,5274041	,5587652	,5919911	,6271927	,6644875	,7040000	,7458620	,7902133	,8372018	,8869844	,9397273	,9956063	,10548080	,11175300	,11839820	,12543850} ;
bool scale_12[12] = {1,1,1,1,1,1,1,1,1,1,1,1};
bool scale_7[12] = {1,0,1,0,1,1,0,1,0,1,0,1};
bool scale_6[12] = {1,0,1,0,1,1,0,1,0,1,0,0};
bool scale_5[12] = {1,0,1,0,0,1,0,1,0,1,0,0};
bool scale_4[12] = {1,0,1,0,0,1,0,1,0,0,0,0};
bool scale_3[12] = {1,0,0,0,0,1,0,1,0,0,0,0};
bool scale_2[12] = {1,0,0,0,0,0,0,1,0,0,0,0};
bool scale_1[12] = {1,0,0,0,0,0,0,0,0,0,0,0};



int mode = REV;
int previous_mode = mode;

float attack_lut[300];

//Individual Variables for each effect
float reverb_drywet, reverb_feedback,reverb_lowpass, reverb_shimmer = 0;
int reverb_shimmer_write_pos1l, reverb_shimmer_write_pos1r,
    reverb_shimmer_write_pos2 = 0;
int reverb_shimmer_play_pos1l, reverb_shimmer_play_pos1r;
float reverb_shimmer_play_pos2=0.f ;

int reverb_shimmer_buffer_size1l = 24000*0.773;
int reverb_shimmer_buffer_size1r = 24000*0.802;
int reverb_shimmer_buffer_size2 = 48000*0.753*2;


float reverb_previous_inl, reverb_previous_inr = 0;
float reverb_current_outl, reverb_current_outr = 0;

int reverb_feedback_display = 0;
int   reverb_rmsCount;
float reverb_current_RMS, reverb_target_RMS, reverb_feedback_RMS=0.f;
float reverb_target_compression, reverb_compression = 1.0f;


float resonator_note = 20.f;
float resonator_tone = 12000.f;
int resonator_feedback_display = 0;
float resonator_current_regen = 0.5f;
float target_resonator_feedback = 0.001f;
int   resonator_rmsCount;
float resonator_previous_l, resonator_previous_r = 0.f;

float resonator_current_RMS, resonator_target_RMS, resonator_feedback_RMS=0.f;
float resonator_current_delay, resonator_feedback, resonator_target, resonator_drywet =0.f;
int resonator_octave = 1;
float resonator_glide = 0.f;
int resonator_glide_mode = 0;

//int   crusher_crushmod, crusher_crushcount;
//float crusher_crushsl, crusher_crushsr;
//float crusher_cutoff;
float filter_mode_l = 0.f;
float filter_mode_r = 0.f;
float filter_path = 0.f;
float filter_target_l_freq, filter_target_r_freq, filter_current_l_freq, filter_current_r_freq = 0.5f;

float lofi_current_RMS, lofi_target_RMS;
float lofi_damp_speed, lofi_depth;
int   lofi_mod, lofi_rate_count;
int   lofi_rmsCount;
float lofi_cutoff, lofi_target_Lofi_LFO_Freq, lofi_current_Lofi_LFO_Freq;
float lofi_previous_variable_compressor;
float global_sample_rate;
float lofi_previous_left_saturation, lofi_previous_right_saturation;
float lofi_current_left_saturation, lofi_current_right_saturation;
float lofi_drywet = 0.0f;
float lofi_lpg_amount =1.0f;
float lofi_lpg_decay =1.0f;



bool mlooper_play  = false; //currently playing

float                 mlooper_pos_1 = 0;
float                 mlooper_pos_2 = 0;

int modified_buffer_length_l, modified_buffer_length_r;
int modified_frozen_buffer_length_l, modified_frozen_buffer_length_r;


float DSY_SDRAM_BSS mlooper_buf_1l[LOOPER_MAX_SIZE];
float DSY_SDRAM_BSS mlooper_buf_1r[LOOPER_MAX_SIZE];

float DSY_SDRAM_BSS mlooper_frozen_buf_1l[LOOPER_MAX_SIZE];
float DSY_SDRAM_BSS mlooper_frozen_buf_1r[LOOPER_MAX_SIZE];




int                 mlooper_len    = LOOPER_MAX_SIZE-1;
int                 mlooper_len_count = 0;


float               mlooper_drywet = 0.f;

float                 mlooper_frozen_pos_1 = 0;
float                 mlooper_frozen_pos_2 = 0;

int mlooper_frozen_len = LOOPER_MAX_SIZE-1;

bool mlooper_frozen = false;
int mlooper_writer_pos = 0;
int mlooper_writer_outside_pos = 0;

float mlooper_division_1 = 1.f;
float mlooper_division_2 = 1.f;

std::string mlooper_division_string_1 = "";
std::string mlooper_division_string_2 = "";

float mlooper_play_speed_1 = 1.f;
float mlooper_play_speed_2 = 1.f;
float mlooper_volume_att_1 = 1.f;
float mlooper_volume_att_2 = 1.f;
std::string mlooper_play_speed_string_1 = "";
std::string mlooper_play_speed_string_2 = "";

float delay_mult_l[2], delay_mult_r[2];
float delay_feedback = 0.0f;

int delay_time_count = 0;
int delay_write_pos = 0;
int delay_control_counter,delay_time_trig = 0;
int delay_control_latency_ms = 20;
int delay_pos_l[2], delay_pos_r[2];
int delay_time[2];
float delay_outl[2], delay_outr[2];
int delay_left_counter, delay_right_counter = 0;
int delay_left_counter_4, delay_right_counter_4 = 0;
int delay_main_counter = 0;
int delay_active = 0;
int delay_inactive = 1;
float delay_xfade_current = 0;
float delay_xfade_target = 0;


int delay_frozen_start;
int delay_frozen_end;
int delay_frozen_pos;
 float delay_target_cutoff =0.0f;
float delay_cutoff, delay_drywet = 0.f;
bool delay_frozen = false;
float delay_prev_sample_l, delay_prev_sample_r = 0.f;
bool delay_reduce_spikes_l, delay_reduce_spikes_r = false;
float delay_spike_counter_l, delay_spike_counter_r = 1.f;

int delay_rmsCount = 0;
float delay_target_RMS, delay_feedback_RMS, delay_fast_feedback_RMS = 0.0f;




int spectra_waveform = 0;
float spectra_r,spectra_g,spectra_b = 0.f;
float spectra_prev_knob_wave_knob = 0.f;
int spectra_num_active = MAX_SPECTRA_FREQUENCIES;
const int spectra_max_num_frequencies = MAX_SPECTRA_FREQUENCIES;
int spectra_oct = 0;
int spectra_hop = 1;
float spectra_drywet;
float spectra_lower_harmonics = 0.f;
float spectra_oct_mult;
std::string spectra_oct_string;
float spectra_reverb_amount= 0.f;
bool spectra_do_analisys = false;
int   spectra_rmsCount = 0;
float spectra_current_RMS, spectra_target_RMS = 1.f;
float spectra_spread= 1.0f;
float spectra_rotate_harmonics = 0.0f;
int spectra_transpose = 0;
int spectra_quantize = 0;
bool *spectra_selected_scale;

int spectrings_num_models = 2;
int spectrings_active_voices = 2;
int spectrings_current_voice = 0;
bool spectrings_trigger_next_cycle = false;
float spectrings_drywet = 0.0f;

size_t spectrings_attack_step[NUM_OF_STRINGS];
size_t spectrings_attack_last_step[NUM_OF_STRINGS];
float spectrings_accent_amount [NUM_OF_STRINGS];
float spectrings_decay_amount [NUM_OF_STRINGS];
float spectrings_pan_spread = 0.0f;
//Helper functions
void Controls();

void GetReverbSample(float &outl, float &outr, float inl, float inr);

void GetResonatorSample(float &outl, float &outr, float inl, float inr);

void GetFilterSamples(float outl[], float outr[], float inl[], float inr[], size_t size);

void GetLofiSample(float &outl, float &outr, float inl, float inr);

void GetLooperSample(float &out1l, float &out1r, float in1l, float in1r);

void GetDelaySample(float &out1l, float &out1r, float in1l, float in1r);

void GetSpectraSample(float &outl, float &outr, float inl, float inr);
void GetSpectringsSample(float &outl, float &outr, float inl, float inr);

void ResetLooperBuffer();
void FreezeLooperBuffer();
void SelectLooperDivision(float knob_value_1, float knob_value_2);
void SelectLooperPlaySpeed(float knob_value_1, float knob_value_2);
void SelectDelayDivision(float knob1, float knob2);

float clamp(float value,float min,float max) {
    if (value < min){
        return min;
    }
    if (value > max){
        return max;
    }
    return value;
};


float map(float value, float start1, float stop1, float start2, float stop2) {
    return start2 + ((stop2 - start2) * (value - start1) )/ (stop1 - start1);
};


void leftRotatebyOne(float arr[], int n)
{
    float temp = arr[0];
    for (int i = 0; i < n - 1; i++)
        arr[i] = arr[i + 1];
    arr[n-1] = temp;
}

void rightRotatebyOne(float arr[], int n)
{
    float temp = arr[n-1];
    for (int i = n-1; i > 0; i--)
        arr[i] = arr[i - 1];
    arr[0] = temp;
}

/*Function to left rotate arr[] of size n by d*/
void leftRotate(float arr[], float amount, int n)
{
    for (int i = 0; i < amount*n; i++)
        leftRotatebyOne(arr, n);
}

/*Function to left rotate arr[] of size n by d*/
void rightRotate(float arr[], float amount, int n)
{
    for (int i = 0; i < amount*n; i++)
        rightRotatebyOne(arr, n);
}

int getClosest(int, int, int);

// Returns element closest to target in arr[]
int findClosest(int arr[],bool filter[], int n, int target, int offset)
{
   int lower = 0;
   int higher = n;
   int reverse_counter = 0;
   for (int i = 0; i< n; i++) {
       if ((arr[i] < target) & filter[(i+offset)%12]) {
           lower = arr[i];
       }
       reverse_counter = (n-1)-i;
       if ((arr[reverse_counter] > target) & filter[(reverse_counter+offset)%12]) {
           higher = arr[reverse_counter];
       }

   }
   return getClosest(lower, higher, target);
};

// Method to compare which one is the more close.
// We find the closest by taking the difference
// between the target and both values. It assumes
// that val2 is greater than val1 and target lies
// between these two.
int getClosest(int val1, int val2,
               int target)
{
    if (target - val1 >= val2 - target)
        return val2;
    else
        return val1;
}



void SelectSpectraOctave(float knob_value_1){
    //sets the octave shift
    if (knob_value_1 < 0.2f){
        spectra_oct_mult = 0.25f;
        spectra_oct_string = "-2";
    } else if (knob_value_1 < 0.4f){
        spectra_oct_mult = 0.5f;
        spectra_oct_string = "-1";
    } else if (knob_value_1 < 0.6f){
        spectra_oct_mult = 1.f;
        spectra_oct_string = " 0";
    } else if (knob_value_1 < 0.8f){
        spectra_oct_mult = 2.f;
        spectra_oct_string = "+1";
    } else if (knob_value_1 > 0.8f){
        spectra_oct_mult = 4.f;
        spectra_oct_string = "+2";
    }
};

void swap(float *xp, float *yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// A function to implement bubble sort
void bubbleSort(float arr[],float arr2[],  int n)
{
    int i, j;
    for (i = 0; i < n-1; i++)

    // Last i elements are already in place
    for (j = 0; j < n-i-1; j++)
        if (arr[j] < arr[j+1]){
            swap(&arr[j], &arr[j+1]);
            swap(&arr2[j], &arr2[j+1]);
            }
}

float ApplyWindow(float i, size_t pos, size_t FFT_SIZE) {
        float multiplier = 0.5 * (1 - cos(2*PI*pos/(FFT_SIZE-1)));
        return i * multiplier;
}

class LedsControl {
    //Helper Class to handle leds easily.
    int times[4];

    float flash_color[4][3];
    float base_color[4][3];

    public:
    LedsControl(){
      Reset();
    };
    ~LedsControl() {};

    void Reset() {
        SwitchAllOff();
      //trick to initialize everything to black
      for (int i = 0; i < 4; i++) {
        SetForXCycles(i,-1,0,0,0);
      }
    }
    void SetForXCycles(int idx, int _times, float r, float g, float b) {
        flash_color[idx][0] = r;
        flash_color[idx][1] = g;
        flash_color[idx][2] = b;
        times[idx] = _times;
    };
    void SwitchAllOff() {
        SetAllLeds(0,0,0);
    };
    void SwitchOffLed(int idx) {
         versio.SetLed(idx,0,0,0);
    };

    void SetAllLeds(float r, float g, float b) {
        for (int i = 0; i < 4; i++) {
            SetBaseColor(i,r,g,b);
        }
    }

    void SetBaseColor(int idx, float r, float g, float b) {
        base_color[idx][0] = r;
        base_color[idx][1] = g;
        base_color[idx][2] = b;
    }

    void UpdateLeds() {
        //handle flashing leds
        for (int i = 0; i < 4; i++) {
            versio.SetLed(i,base_color[i][0],base_color[i][1],base_color[i][2]);
            if (times[i] > 0) {
                times[i]--;
                versio.SetLed(i, flash_color[i][0],flash_color[i][1],flash_color[i][2]);
            }
        }
        versio.UpdateLeds();
    };

};

class OscBank {
    static const int number_of_osc = spectra_max_num_frequencies;
    Oscillator osc[number_of_osc];
    float freq[number_of_osc];
    float magn[number_of_osc];

    float current_freq[number_of_osc];
    float current_magn[number_of_osc];
    size_t num_of_passes;
    float fftinbuff[FFT_SIZE];
    float window[FFT_SIZE];
    float window_fftinbuff[FFT_SIZE];
    float fftoutbuff[FFT_SIZE];
    float magni_fftoutbuff[FFT_SIZE/2];
    float bandSize;
    float maxAmp = 0;
    int num_active = spectra_num_active;
    float output_mult, prev_output_mult = 0.f;
    int firstUsableBin;
    float amp_attenuation = 1.f;
    int previous_wave = 0;
    int current_wave = 0;
    FFT fft;
    size_t attack_step[number_of_osc];
    bool mark_to_change_waveform[number_of_osc];

    public:
    size_t hop = 8;
    OscBank(){
      for (size_t t = FFT_SIZE; t > 1; t >>= 1) {
         ++num_of_passes;
         }
    };
    ~OscBank() {};

    void Init(float sample_rate) {
        for (int i = 0; i< number_of_osc; i++) {
            osc[i].Init(sample_rate);
            //float randomPhase = (rand() %1000)/1000.f;
            //osc[i].PhaseAdd(randomPhase);
            freq[i] = 0;
            magn[i] = 0;
            current_freq[i] = 0;
            current_magn[i] = 0;
            osc[i].SetWaveform(0);
            attack_step[i] = 0;
            mark_to_change_waveform[i] = false;
        };
        for (size_t i = 0; i < FFT_SIZE; i++) {
            fftinbuff[i] = 0;
            fftoutbuff[i] = 0;
            if (i < FFT_SIZE/2){
            magni_fftoutbuff[i] = 0;
            }
            window[i] = ApplyWindow(1.0f, i,FFT_SIZE );
        }
        fft.Init();


        bandSize = sample_rate/(FFT_SIZE*hop);
    };
    void SetFreq(int index, float frequency) {
        osc[index].SetFreq(frequency);
    };
    void SetAmp(int index, float amplitude) {
        osc[index].SetAmp(amplitude);
    };
    float SetAllWaveforms(int waveform) {
        current_wave = 0;
        float centre_of_position = 0.f;

        switch(waveform) {
            case 0:
                current_wave = 0;
                amp_attenuation = 1.f;
                centre_of_position = 0.5f/9.f;
                break;
            case 1:
                current_wave = 8;
                amp_attenuation = 1.f;
                centre_of_position = 1.5f/9.f;
                break;
            case 2:
                current_wave = 1;
                amp_attenuation = 0.9f;
                centre_of_position = 2.5f/9.f;
                break;
            case 3:
                current_wave = 5;
                amp_attenuation = 0.9f;
                centre_of_position = 3.5f/9.f;
                break;
            case 4:
                current_wave = 7;
                amp_attenuation = 0.35f;
                centre_of_position = 4.5f/9.f;
                break;
            case 5:
                current_wave = 2;
                amp_attenuation = 0.40f;
                centre_of_position = 5.5f/9.f;
                break;
            case 6:
                current_wave = 3;
                amp_attenuation = 0.45f;
                centre_of_position = 6.5f/9.f;
                break;
            case 7:
                current_wave = 6;
                amp_attenuation = 0.45f;
                centre_of_position = 7.5f/9.f;
                break;
            case 8:
                current_wave = 4;
                amp_attenuation = 0.4f;
                centre_of_position = 8.5f/9.f;
                break;
        }



            for (int i = 0; i< number_of_osc; i++) {
                //the second time it actually changes the waveform, so the attack lut can avoid clicks
                if (mark_to_change_waveform[i]) {
                    osc[i].SetWaveform(current_wave);
                    mark_to_change_waveform[i] = false;
                }
                //the first time it just marks the waveform to be changed
                if (previous_wave != current_wave) {

                    mark_to_change_waveform[i] = true;
                    attack_step[i] = 0;
                };
            }
            if (previous_wave != current_wave) {
                    previous_wave= current_wave;
            }
        return centre_of_position;
    };

    float Process() {
        float output = 0;
        for (int i = 0; i< spectra_max_num_frequencies; i++) {
            output_mult = ((0.5 + 0.2/num_active) + prev_output_mult*47.f) /48.f;
            output = output + osc[i].Process()* output_mult * attack_lut[attack_step[i]];
            attack_step[i] = clamp(attack_step[i]+1, 0, 299);
            prev_output_mult = output_mult;
        };
        return output;
    }
    size_t GetPasses() {
        return num_of_passes;
    }

    void FillInputBuffer(float *in1, float *in2, size_t size)
    {
        bandSize = global_sample_rate/(FFT_SIZE*hop);

        size_t real_size = size / hop;
        svfl.SetRes(0.1);
        svfl.SetFreq(global_sample_rate/(2*hop));
        svfr.SetRes(0.1);
        svfr.SetFreq(bandSize*(32/hop));
        //shift left the input array of "size" n of samples
        for (size_t i = 0; i<FFT_LENGTH-real_size; i++) {
            fftinbuff[i] = fftinbuff[i+real_size];
        };
        //add the samples to the input buffer


        for (size_t i = 0; i<real_size; i++) {
            float sum = 0.f;
            for (size_t j = 0; j < hop; j++) {
                float sample = (in1[i*hop + j] + in2[i*hop + j])*0.707;
                svfl.Process(sample);
                svfr.Process(svfl.Low());
                sum = sum + svfr.High() / hop;
            }
            float sample = sum;
            fftinbuff[i + FFT_SIZE - real_size] = sample;
        };
        for (size_t i = 0; i<FFT_SIZE; i++) {
            window_fftinbuff[i] = window[i]*fftinbuff[i];
        }
        fft.Direct(window_fftinbuff, fftoutbuff);
    }

    void CalculateSpectralAnalisys() {

        for (size_t i = 0; i<FFT_SIZE / 2; i++){
            if (i<32/hop){
                fftoutbuff[i] =fftoutbuff[i] * 0.5;
            }
            float real = fftoutbuff[i];
            float imag = fftoutbuff[i+(FFT_SIZE / 2)];
            magni_fftoutbuff[i] = sqrt(real*real+imag*imag);
        }
        const int N = sizeof(magni_fftoutbuff) / sizeof(float);
        int num_frequencies = MAX_SPECTRA_FREQUENCIES;
        float max_amp = 20.0f;

        for(int i = 0; i < spectra_max_num_frequencies; i++) {
            int a = std::distance(magni_fftoutbuff, std::max_element(magni_fftoutbuff, magni_fftoutbuff + (N/2)));

            freq[i] = a*bandSize*spectra_oct_mult;
            if (spectra_quantize >0) {

                freq[i] = findClosest(CHRM_SCALE, spectra_selected_scale,128, ((int)freq[i]*1000), spectra_transpose) /1000.f;
            };
            max_amp = std::max(magni_fftoutbuff[a], max_amp) ;
            magn[i] = ((magni_fftoutbuff[a]/max_amp)*(1-spectra_lower_harmonics) + spectra_lower_harmonics);

            if (freq[i] > global_sample_rate / 2) {
                freq[i] = 0.0f;
                magn[i] = 0.0f;
            }

            RemoveNearestBands(a*bandSize, a);

            }

        for(int i = num_active; i < num_frequencies; i++) {
            magn[i] = 0.f;
        }

        //rightRotate(magn,spectra_rotate_harmonics, num_active);
    }
    void RemoveNearestBands(float frequency, size_t start_band) {
        magni_fftoutbuff[start_band] = 0.f;
        float upper_frequency = mtof((int)((12.f*log2(frequency/440.f) + 69 + 1)));
        for (size_t i = start_band; (((i*bandSize/spectra_spread)< upper_frequency) & (i<FFT_SIZE/2) & (-i>0)) ; i++) {
            float mult = map((i*bandSize/spectra_spread),(1*bandSize/spectra_spread), upper_frequency, 0.0f, 1.0f);
            magni_fftoutbuff[i] = magni_fftoutbuff[i] * mult;
            magni_fftoutbuff[-i] = magni_fftoutbuff[-i] * mult;
        }

    }
    float getFrequency(int value) {
        return current_freq[value];
    }
    float getMagnitudo(int value) {
        return current_magn[value];
    }

    void updateFreqAndMagn() {
        for (int i = 0; i< spectra_max_num_frequencies; i++ ) {
            float new_freq = (freq[i] +  current_freq[i]*47)/48;
            SetFreq(i,new_freq);
            current_freq[i] = new_freq;

            float new_magn = ((magn[i] +  current_magn[i]*47)/48);
            SetAmp(i,current_magn[i]*amp_attenuation);
            current_magn[i] = new_magn;
        }
    }

    void calculatedSuggestedHop() {

        //if ((current_freq[0]/spectra_oct_mult) > 110) {
        hop = 16;
        //}
        //if ((current_freq[0]) > 880) {
        //    hop = 8;
        //}
        //if ((current_freq[0]) > 1760) {
        //    hop = 4;
        //}


    }
    void SetNumActive(int value) {
        num_active = value;
    };
};

class Averager {

    float buffer[RMS_SIZE];
    int cursor;
    public:

    Averager() {
        Clear();
    }
    ~Averager() {}

    float ProcessRMS() {
        float sum = 0.f;
        for (int i =0; i< cursor; i++) {
            sum = sum + buffer[i];
        }
        float result = sqrt(sum/cursor);
        Clear();
        return result;
    }
    void Clear() {
        for (int i =0; i< RMS_SIZE; i++) {
            buffer[i] = 0.f;
        }
        cursor = 0;
    }
    void Add(float sample){
        buffer[cursor] = sample;
        cursor++;
    }
};

static Averager lofi_averager;
static Averager reverb_averager;
static Averager delay_averager;



static Averager resonator_averager;
static OscBank spectra_oscbank;
static Averager spectra_averager;
static LedsControl leds;


void SelectResonatorOctave(float knob_value_1){
    //sets the octave shift
    if (knob_value_1 < 0.2f){
        resonator_octave = 1;
    } else if (knob_value_1 < 0.4f){
        resonator_octave = 2;
    } else if (knob_value_1 < 0.6f){
        resonator_octave = 4;
    } else if (knob_value_1 < 0.8f){
        resonator_octave = 8;
    } else if (knob_value_1 > 0.8f){
        resonator_octave = 16;
    }
};

void SelectSpectraQuality(float knob_value_1){
    //sets the octave shift
    if (knob_value_1 < 0.25f){
        spectra_oscbank.hop = 2;
    } else if (knob_value_1 < 0.5f){
        spectra_oscbank.hop = 4;
    } else if (knob_value_1 < 0.75f){
        spectra_oscbank.hop = 8;
    } else if (knob_value_1 > 0.75f){
        spectra_oscbank.hop = 16;
    }
};

//static void AudioCallback(AudioHandle::InputBuffer in,
//                          AudioHandle::OutputBuffer out,
//                          size_t size)
static void AudioCallback(float **in, float **out,
                          size_t size)
{

//    // AudioHandle::InputBuffer is const
//    float *bufIn1 = new float[size];
//    float *bufIn2 = new float[size];
//
//    // Copy data from in to temporary buffers
//    std::copy(in[0], in[0] + size, bufIn1);
//    std::copy(in[1], in[1] + size, bufIn2);
//

    float out1, out2, in1, in2;

    Controls();
    leds.UpdateLeds();

    if ((mode == SPECTRA) or (mode == SPECTRINGS)) {
        spectra_oscbank.FillInputBuffer(in[0], in[1], size);
        if (spectra_do_analisys) {
            spectra_do_analisys = false;
            spectra_oscbank.CalculateSpectralAnalisys();
        }

        if (mode == SPECTRINGS) {
        string_voice[spectrings_current_voice].SetFreq(spectra_oscbank.getFrequency(spectrings_current_voice));
        }
    };



    //audio
    for(size_t i = 0; i < size; i += 1)
    {
        in1 = in[0][i];
        in2 = in[1][i];
//        in1 = bufIn1[i];
//        in2 = bufIn2[i];

        out1 = 0.f;
        out2 = 0.f;

        switch(mode)
        {
            case REV: GetReverbSample(out1, out2, in1, in2); break;
            case RESONATOR: GetResonatorSample(out1, out2, in1, in2); break;
            case LOFI:
                GetReverbSample(out1, out2, in1, in2);
                GetLofiSample(out1, out2, out1, out2);
                break;
            case MLOOPER: GetLooperSample(out1, out2, in1, in2); break;
            case SPECTRA: GetSpectraSample(out1, out2, in1, in2);
                 GetReverbSample(out1, out2, out1, out2);
                 break;
            case DELAY: GetDelaySample(out1, out2, in1, in2);
                break;
            case SPECTRINGS: GetSpectringsSample(out1, out2, in1, in2);
                 GetReverbSample(out1, out2, out1, out2);
                 break;

            default: out1 = out2= 0.f;
        }
        out[0][i] = out1;
        out[1][i] = out2;

    }

    if (mode == FILTER) {
        GetFilterSamples(out[0], out[1], in[0], in[1], size);
        // GetFilterSamples(out[0], out[1], bufIn1, bufIn2, size);

    }

};

//void UpdateOled();

int main(void)
{
    float sample_rate;

    //Inits and sample rate
    versio.Init(true);
    sample_rate = versio.AudioSampleRate();
    rev.Init(sample_rate);
    dell.Init();
    delr.Init();

    tonel.Init(sample_rate);
    toner.Init(sample_rate);
    svfl.Init(sample_rate);
    svfl.SetFreq(0.0);
    svfl.SetRes(0.5);
    svfr.Init(sample_rate);
    svfr.SetFreq(0.0);
    svfr.SetRes(0.5);

    svf2l.Init();
    svf2r.Init();

    biquad.Init(sample_rate);
    biquad.SetCutoff(0.0);
    biquad.SetRes(0.5);

    //arm_rfft_fast_init_f32(&fft, FFT_SIZE);

    global_sample_rate = sample_rate;
    dcblock_l.Init(sample_rate);
    dcblock_r.Init(sample_rate);
    dcblock_2l.Init(sample_rate);
    dcblock_2r.Init(sample_rate);

    lofi_damp_speed = sample_rate;
    lofi_target_Lofi_LFO_Freq = lofi_current_Lofi_LFO_Freq = sample_rate;
    lofi_current_RMS = lofi_target_RMS = 0.f;
    lofi_previous_left_saturation = lofi_previous_right_saturation = 0.5f;
    lofi_current_left_saturation = lofi_current_right_saturation = 0.5f;
    lofi_previous_variable_compressor = 0.0f;


    //crusher_cutoff_par.Init(versio.knobs[DaisyVersio::KNOB_0], 60, 20000, crusher_cutoff_par.LOGARITHMIC);
    //crusher_crushrate_par.Init(versio.knobs[DaisyVersio::KNOB_1], 1, 50, crusher_crushrate_par.LOGARITHMIC);
    filter_cutoff_l_par.Init(versio.knobs[DaisyVersio::KNOB_0], 60, 20000, filter_cutoff_l_par.LOGARITHMIC);
    filter_cutoff_r_par.Init(versio.knobs[DaisyVersio::KNOB_4], 60, 20000, filter_cutoff_r_par.LOGARITHMIC);

    delay_cutoff_par.Init(versio.knobs[DaisyVersio::KNOB_1], 400, 20000, delay_cutoff_par.LOGARITHMIC);

    delay_pos_l[0] = 0;
    delay_pos_r[1] = 0;
    delay_time[0] = -1;
    delay_time[1] = -1;
    delay_outl[0] = 0;
    delay_outr[0] = 0;
    delay_outl[1] = 0;
    delay_outr[1] = 0;
    delay_mult_l[0] = 1;
    delay_mult_r[0] = 1;
    delay_mult_l[1] = 1;
    delay_mult_r[1] = 1;


    lofi_tone_par.Init(versio.knobs[DaisyVersio::KNOB_2], 20, 20000, lofi_tone_par.LOGARITHMIC);
    lofi_rate_par.Init(versio.knobs[DaisyVersio::KNOB_1], sample_rate*4, sample_rate/16, lofi_rate_par.LOGARITHMIC);

    //reverb parameters
    rev.SetLpFreq(9000.0f);
    rev.SetFeedback(0.85f);

    //delay parameters
    resonator_current_delay = resonator_target = sample_rate * 0.75f;
    dell.SetDelay(resonator_current_delay);
    delr.SetDelay(resonator_current_delay);

    for (size_t i = 0; i<300; i++) {
        if (i<48) {
        attack_lut[i] = map(i, 0, 48, 1.0f, 0.f);
        }
        else
        {
        attack_lut[i] = map(i, 48, 300, 0.0f, 1.f) ;
        }
    };


    ResetLooperBuffer();

    spectra_oscbank.Init(sample_rate);

    for (int i = 0; i < NUM_OF_STRINGS; i++)   {
    string_voice[i].Init(sample_rate);
    }

    // start callback
    versio.StartAdc();
    versio.StartAudio(AudioCallback);

    while(1) {
        //UpdateOled();
    }
}
float randomFloat() {
    int randomNumber = std::rand() % 10000;
    return randomNumber / 10000.f;
}

void UpdateKnobs()

{
    float blend = versio.GetKnobValue(DaisyVersio::KNOB_0);
    float speed = versio.GetKnobValue(DaisyVersio::KNOB_1);
    float tone = versio.GetKnobValue(DaisyVersio::KNOB_2);
    float index = versio.GetKnobValue(DaisyVersio::KNOB_3);
    float regen = versio.GetKnobValue(DaisyVersio::KNOB_4);
    float size = versio.GetKnobValue(DaisyVersio::KNOB_5);
    float dense = versio.GetKnobValue(DaisyVersio::KNOB_6);

    float tone_freq;

    int sw1,sw2;

    switch(versio.sw[DaisyVersio::SW_0].Read()) {
        case 0:
            sw1 = 1;
            break;
        case 1:
            sw1=0;
            break;
        case 2:
            sw1=2;
            break;
    };

    switch(versio.sw[DaisyVersio::SW_1].Read()) {
        case 0:
            sw2 = 3;
            break;
        case 1:
            sw2=0;
            break;
        case 2:
            sw2=6;
            break;
    };

    mode = sw1 + sw2;
    if (mode != previous_mode) {
        previous_mode = mode;
        leds.Reset();
    }


    switch(mode)
    {
        case REV:
            //blend = reverb wet/dry
            //tone = reverb_lowpass
            //speed =
            //index = shimmer
            //regen = reverb feedback
            //size =
            //dense = reverb compression


            reverb_lowpass = global_sample_rate*tone / 2.f;
            rev.SetLpFreq(reverb_lowpass);
            reverb_shimmer = index;
            reverb_feedback = 0.8f + (std::log10(10 + regen*90) -1.000001f)*0.4f;
            reverb_feedback_display = regen*100;
            rev.SetFeedback(reverb_feedback);

            reverb_compression = dense +0.5f;

            reverb_drywet = blend;
            break;
        case RESONATOR:
            //blend = resonator wet/dry
            //tone = resonator tone
            //speed = octave
            //index = resonator note
            //regen = resonator feedback
            //size = reverb shimmer
            //dense = reverb amount
            //tap

            if (versio.tap.RisingEdge()){
                resonator_glide_mode = (resonator_glide_mode + 1) % 10;
                resonator_glide = resonator_glide_mode*resonator_glide_mode*resonator_glide_mode;
            };


            SelectResonatorOctave(speed);
            resonator_note = 12.0f + index * 60.0f;
            resonator_note = static_cast<int32_t>(resonator_note); // Quantize to semitones


            resonator_tone = global_sample_rate*tone / 4.f;
            tone_freq = resonator_tone /2.f;
            //rev.SetLpFreq(resonator_tone);


            rev.SetLpFreq(resonator_tone*2.f);
            reverb_shimmer = size*2;
            reverb_feedback = 0.8f + (std::log10(10 + dense*90) -1.000001f)*1.4f;

            rev.SetFeedback(reverb_feedback);

            reverb_drywet = dense;


            tonel.SetFreq(tone_freq);
            toner.SetFreq(tone_freq);

            svfl.SetFreq(resonator_tone);
            svfr.SetFreq(resonator_tone);

            svfl.SetRes(0.001);
            svfr.SetRes(0.001);

            reverb_compression = dense*2 +0.5f;

            fonepole(resonator_current_regen,regen, 0.008) ;

            resonator_feedback =  (std::log10(10 + clamp((resonator_current_regen-0.5)*2.f,0,1)   *90) -1.000001f)*1.5f -(std::log10(10 + clamp((1 - resonator_current_regen*2.f),0,1)   *90) -1.000001f)*1.5f;

            resonator_drywet = blend*1.01;
            break;


        case FILTER:
            //blend = cutoff left
            //tone = mode left
            //speed = resonance left
            //index = mode right
            //regen = cutoff right
            //size = resonance right
            //dense = parallel -> series
            //dense = parallel -> series
            filter_target_l_freq = filter_cutoff_l_par.Process()/ (global_sample_rate);
            filter_target_r_freq = filter_cutoff_r_par.Process()/ (global_sample_rate);

            fonepole(filter_current_l_freq, filter_target_l_freq, 0.1f);
            fonepole(filter_current_r_freq, filter_target_r_freq, 0.1f);

            svf2l.set_f_q <stmlib::FREQUENCY_ACCURATE> (filter_current_l_freq, 1.f+ speed*speed*49.f);
            svf2r.set_f_q <stmlib::FREQUENCY_ACCURATE> (filter_current_r_freq,1.f+ size*size*49.f);

            filter_mode_l = tone;
            filter_mode_r = index;
            filter_path = dense;


            leds.SetBaseColor(0,blend*0.8,0,0);
            leds.SetBaseColor(1,tone,0,1-tone);
            leds.SetBaseColor(2,index,0,1-index);
            leds.SetBaseColor(3,regen*0.8,0,0);

            filter_path = dense;

            break;

        case LOFI:
            //blend = lofi drywet
            //tone = lofi lpg cutoff
            //speed = lofi rate
            //index = lofi_depth
            //regen = reverb amount
            //size = lpg amount
            //dense = lpg decay


            lofi_cutoff = lofi_tone_par.Process(); //tone
            lofi_depth = index*2;     //index
            tonel.SetFreq(lofi_cutoff);
            toner.SetFreq(lofi_cutoff);
            lofi_mod = (int)lofi_rate_par.Process();
            lofi_drywet = blend*1.01; //IMPLEMENT

            reverb_lowpass = lofi_cutoff;
            rev.SetLpFreq(reverb_lowpass);
            reverb_feedback = 0.7f + (std::log10(10 + regen*90) -1.000001f)*0.3f;
            rev.SetFeedback(reverb_feedback);

            lofi_lpg_amount = size*size*3;
            lofi_lpg_decay = clamp((1-((std::log10(10 + dense*90) -0.999991f)*0.4f +0.6f  ) ) *0.05f, 0.0001, 0.99999);
            //Shimmer
            reverb_shimmer = 0.0f;
            reverb_compression = 0.5f;
            //DRY WET
            reverb_drywet = regen*0.8f;

            break;

        case MLOOPER:
            //blend = looper division left
            //tone =
            //speed = looper octave left
            //index = buffer freeze
            //regen = looper division right
            //size = looper octave right
            //dense = dry/wet
            //FSU = clock

            //TONE = Amount of the two micro loopers
            //INDEX = AMOUNT OF RANDOM
            //DENSE = DRY WET //implement


            if (versio.gate.Trig())
            {
                mlooper_len = mlooper_len_count % LOOPER_MAX_SIZE;
                mlooper_len_count = 0;
                mlooper_play = true;
                mlooper_pos_1 = (mlooper_writer_pos - mlooper_len);
                mlooper_pos_2 = (mlooper_writer_pos - mlooper_len);
                leds.SetForXCycles(1,10,1,1,1);
                leds.SetForXCycles(2,10,1,1,1);
            };

            if (index > 0.5f)
            {
                if (mlooper_frozen == false)
                {
                    mlooper_frozen = true;
                    FreezeLooperBuffer();
                }

            } else
            {
              mlooper_frozen = false;
            }


            SelectLooperDivision(blend,regen);
            SelectLooperPlaySpeed(speed,size);

            mlooper_drywet = dense*1.01;

            break;

        case SPECTRA:
            // blend = spectra drywet
            // speed = hop
            // index = transpose when quantizing.
            // tone = octave
            // size = number of waveforms + spread
            // regen = amount of reverb + feedback
            // dense = waveform kind
            // tap = activate quantizer

            // FSU clock

            SelectSpectraQuality(1.f-speed);

            //spectra_waveform = (dense + spectra_prev_knob_wave_knob)/0.5f;
            spectra_prev_knob_wave_knob = spectra_oscbank.SetAllWaveforms((int)((dense*9 + spectra_prev_knob_wave_knob)*0.1 *9.f));

            spectra_num_active = ((int)(1+ (clamp(size*2, 0.0f, 1.0f)*(spectra_max_num_frequencies -0.5f))));


            spectra_spread = clamp(map(size-0.5, 0.0f, 0.5f, 1.0f, 4.0f), 1.0f, 4.0f);
            spectra_lower_harmonics = spectra_spread*0.25f;
            spectra_oscbank.SetNumActive(spectra_num_active);

            SelectSpectraOctave(tone);

            spectra_rotate_harmonics = 0;
            if (versio.gate.Trig()) {
                spectra_do_analisys = true;
                spectra_r = randomFloat();
                spectra_g = randomFloat();
                spectra_b = randomFloat();
                leds.SetBaseColor(1,spectra_r,spectra_g,spectra_b);
                leds.SetBaseColor(2,spectra_r,spectra_g,spectra_g);
            }

            reverb_lowpass = global_sample_rate*0.5 / 2.f;
            rev.SetLpFreq(reverb_lowpass);
            reverb_shimmer = 0.0f;
            reverb_feedback = 0.2f + (std::log10(10 + regen*90) -1.000001f)*1.0f;
            rev.SetFeedback(reverb_feedback);

            reverb_compression = 0.5f;
            spectra_drywet = blend;

            reverb_drywet = clamp(regen*1.1f, 0.0f, 1.0f);

            spectra_transpose = (int)std::round(index*12.f);
            if (versio.tap.RisingEdge()){
                spectra_quantize = (spectra_quantize + 1) % 9;
                if (spectra_quantize >0) {
                    leds.SetBaseColor(0,0,1,0);
                    switch (spectra_quantize)
                    {
                    case 1:
                        spectra_selected_scale = scale_12;
                        leds.SetBaseColor(3,0,0,1);
                        break;
                    case 2:
                        spectra_selected_scale = scale_7;
                        leds.SetBaseColor(3,0,0,0.8);
                        break;
                    case 3:
                        spectra_selected_scale = scale_6;
                        leds.SetBaseColor(3,0,0.3,0.6);
                        break;
                    case 4:
                        spectra_selected_scale = scale_5;
                        leds.SetBaseColor(3,0,0.4,0.4);
                        break;
                    case 5:
                        leds.SetBaseColor(3,0,0.6,0.3);
                        spectra_selected_scale = scale_4;
                        break;
                    case 6:
                        leds.SetBaseColor(3,0,0.7,0.2);
                        spectra_selected_scale = scale_3;
                        break;
                    case 7:
                        leds.SetBaseColor(3,0,0.4,0.1);
                        spectra_selected_scale = scale_2;
                        break;
                    case 8:
                        leds.SetBaseColor(3,0.4,0.4,0.0);
                        spectra_selected_scale = scale_1;
                        break;
                    default:
                        break;
                    }
                }
                else {
                    leds.SetBaseColor(0,0,0,0);
                    leds.SetBaseColor(3,0,0,0);
                    };
            };


            spectra_reverb_amount = reverb_drywet;
            break;


        case DELAY:
            //blend = delay speed division left
            //tone = feedback
            //speed = cutoff
            //index = buffer freeze
            //regen = delay speed division right
            //size = reverb
            //dense = dry/wet

            //FSU = clock

            if (versio.gate.Trig())
            {
                delay_time_trig = delay_time_count;
                delay_time_count = 0;
                leds.SetForXCycles(1,10,1,0.5f,0.5f);
                leds.SetForXCycles(2,10,1,0.5f,0.5f);
            };

            //Change delay length only if the difference is higher than 0.5 milliseconds
            //to avoid clicks for micro changes in the delay time
            if (delay_control_counter == 0)
            {   delay_time[delay_inactive] = delay_time[delay_active];
                delay_mult_l[delay_inactive] = delay_mult_l[delay_active];
                delay_mult_r[delay_inactive] = delay_mult_r[delay_active];

                if (delay_time_trig > 0)
                {

                    //if (abs(delay_time[delay_active] - ((delay_time_count*4) % LOOPER_MAX_SIZE)) > 48)

                    delay_time[delay_inactive] = (delay_time_trig *4) % LOOPER_MAX_SIZE;
                    //this line sets the crossfade to move to the other side
                    delay_xfade_target=delay_inactive;

                    delay_time_trig = 0;


                    if (delay_main_counter == 0) {
                        delay_left_counter = (delay_write_pos - delay_pos_l[delay_active]) / 4;
                        delay_right_counter = (delay_write_pos - delay_pos_r[delay_active]) / 4;
                        delay_right_counter_4 = delay_left_counter_4 = 0;

                    } ;
                    delay_main_counter = (delay_main_counter +1) % 4;
                };

                if (index > 0.5f)
                {
                    if (!delay_frozen) {
                        delay_frozen = true;
                        delay_frozen_end = delay_write_pos;
                        delay_frozen_start = (delay_write_pos-delay_time[delay_active]+ LOOPER_MAX_SIZE) % LOOPER_MAX_SIZE;
                        delay_frozen_pos= delay_frozen_start;
                        }
                } else
                {
                delay_frozen = false;
                }

                SelectDelayDivision(blend,regen);
                }

            delay_control_counter = (delay_control_counter + 1) % delay_control_latency_ms;


            //SelectLooperPlaySpeed(speed,size);
            delay_feedback = size*0.1 + (std::log10(10 + tone*90) -1.000001f)*0.9f;
            delay_drywet = (std::log10(10 + dense*90) -1.0f)*1.01;

            delay_target_cutoff = delay_cutoff_par.Process();
            fonepole(delay_cutoff, delay_target_cutoff, 0.1f);

            tonel.SetFreq(delay_cutoff);
            toner.SetFreq(delay_cutoff);
            //svf2l.set_f_q <stmlib::FREQUENCY_DIRTY> (delay_cutoff, 1.f);
            //svf2r.set_f_q <stmlib::FREQUENCY_DIRTY> (delay_cutoff, 1.f);

            reverb_lowpass = (global_sample_rate + global_sample_rate*(size))  / 4.f;
            rev.SetLpFreq(reverb_lowpass);
            reverb_feedback = 0.65f + (std::log10(10 + size*90) -1.000001f)*0.20f;
            rev.SetFeedback(reverb_feedback);
            //Shimmer
            reverb_shimmer = 0.0f;
            reverb_compression = 1.0f;
            //DRY WET
            reverb_drywet = size*0.75f;

            break;

        case SPECTRINGS:
            // blend = dry/wet
            // speed =  reverb
            // index = transpose when quantizing
            // tone = octave
            // size = damping
            // regen = brightness
            // dense = inharmonicity
            // FSU

            // tap = activate quantizer

            spectra_transpose = (int)std::round(index*12.f);
            if (versio.tap.RisingEdge()){
                spectra_quantize = (spectra_quantize + 1) % 9;
                if (spectra_quantize >0) {
                    leds.SetBaseColor(0,0,1,0);
                    switch (spectra_quantize)
                    {
                    case 1:
                        spectra_selected_scale = scale_12;
                        leds.SetBaseColor(3,0,0,1);
                        break;
                    case 2:
                        spectra_selected_scale = scale_7;
                        leds.SetBaseColor(3,0,0,0.8);
                        break;
                    case 3:
                        spectra_selected_scale = scale_6;
                        leds.SetBaseColor(3,0,0.3,0.6);
                        break;
                    case 4:
                        spectra_selected_scale = scale_5;
                        leds.SetBaseColor(3,0,0.4,0.4);
                        break;
                    case 5:
                        leds.SetBaseColor(3,0,0.6,0.3);
                        spectra_selected_scale = scale_4;
                        break;
                    case 6:
                        leds.SetBaseColor(3,0,0.7,0.2);
                        spectra_selected_scale = scale_3;
                        break;
                    case 7:
                        leds.SetBaseColor(3,0,0.4,0.1);
                        spectra_selected_scale = scale_2;
                        break;
                    case 8:
                        leds.SetBaseColor(3,0.4,0.4,0.0);
                        spectra_selected_scale = scale_1;
                        break;
                    default:
                        break;
                    }
                }
                else {
                    leds.SetBaseColor(0,0,0,0);
                    leds.SetBaseColor(3,0,0,0);
                    };
            };

            spectra_oscbank.calculatedSuggestedHop();
            //SelectSpectraQuality(0.8);
            for (int i = 0; i < NUM_OF_STRINGS; i++)   {
                string_voice[i].SetBrightness(spectra_oscbank.getMagnitudo(i)*regen);

                string_voice[i].SetAccent(spectra_oscbank.getMagnitudo(i));

                string_voice[i].SetStructure(dense);

            }

            //spectra_waveform = ((int)(dense*9.f));
            //spectra_oscbank.SetAllWaveforms(spectra_waveform);

            spectra_num_active = NUM_OF_STRINGS;

            spectra_spread = 4.f; //clamp(map(size-0.5, 0.0f, 0.5f, 1.0f, 4.0f), 1.0f, 4.0f);
            spectra_lower_harmonics = 0.0f;
            spectra_oscbank.SetNumActive(spectra_num_active);

            SelectSpectraOctave(tone);

            if (spectrings_trigger_next_cycle) {
                string_voice[spectrings_current_voice].SetDamping(spectrings_decay_amount[spectrings_current_voice]);
                string_voice[spectrings_current_voice].Trig();
                spectrings_trigger_next_cycle = false;
            }

            if (versio.gate.Trig()) {
                spectra_do_analisys = true;
                spectrings_current_voice = (spectrings_current_voice +1) % spectrings_active_voices;

                spectrings_trigger_next_cycle = true;
                spectrings_accent_amount[spectrings_current_voice] = spectra_oscbank.getMagnitudo(spectrings_current_voice) ;
                spectrings_decay_amount[spectrings_current_voice] = size;
                spectrings_attack_step[spectrings_current_voice] = 0;

                if (spectrings_current_voice == 0) {
                    leds.SetForXCycles(1,10,1,1,1);
                } else {
                    leds.SetForXCycles(2,10,1,1,1);
                };

                }

            spectrings_drywet = blend;
            spectrings_pan_spread = 1;
            reverb_lowpass = global_sample_rate*0.4*(1-speed*speed*0.6)  / 2.f;
            rev.SetLpFreq(reverb_lowpass);
            reverb_shimmer = 0.0f;
            reverb_feedback = 0.7f + (std::log10(10 + speed*90) -1.000001f)*0.299;
            rev.SetFeedback(reverb_feedback);

            reverb_compression = 0.5f;
            spectra_drywet = 1.0; //IMPLEMENT

            reverb_drywet = clamp(map(clamp(speed*1.1f, 0.0f, 1.0f)*0.95 , 0.0, 0.95, 0.7f,0.95f),0.0f,0.95f) ;

            break;
    };

versio.tap.Debounce();

}
//void UpdateEncoder()
//{
//    mode = mode + patch.encoder.Increment();
//    mode = (mode % NUM_MODES + NUM_MODES) % NUM_MODES;
//}

void Controls()
{

    resonator_target = resonator_feedback = reverb_drywet = 0;

    versio.ProcessAnalogControls();
    //patch.ProcessDigitalControls();

    UpdateKnobs();

    //UpdateEncoder();

}

void WriteShimmerBuffer1(float in_l, float in_r)
{
    //writes the input to the buffer
    mlooper_buf_1l[reverb_shimmer_write_pos1l] = in_l;
    mlooper_buf_1r[reverb_shimmer_write_pos1r] = in_r;
    reverb_shimmer_write_pos1l = (reverb_shimmer_write_pos1l +1) % reverb_shimmer_buffer_size1l;
    reverb_shimmer_write_pos1r = (reverb_shimmer_write_pos1r +1) % reverb_shimmer_buffer_size1r;
};

void WriteShimmerBuffer2(float in_l, float in_r)
{
    //writes the input to the buffer
    mlooper_frozen_buf_1l[reverb_shimmer_write_pos2] = (in_r + in_l)/2;
    reverb_shimmer_write_pos2 = (reverb_shimmer_write_pos2 +1) % reverb_shimmer_buffer_size2;
};
float CompressSample(float sample) {
    if (sample > 0.4) {
        sample = clamp(sample - map(sample, 0.4f, 5.0f, 0.0f, 0.6f), 0.0f, 2.0f);
    }
    if (sample < -0.4) {
        sample = clamp(sample - map(sample, -5.0f,-0.4f,  -0.6f, 0.0f), -2.0f, 0.0f);
    }

    if (sample > 0.8) {
        sample = clamp(sample - map(sample, 0.8f, 2.0f, 0.0f, 0.1f), 0.0f, 0.9f);
    }
    if (sample < -0.8) {
        sample = clamp(sample - map(sample, -2.0f,-0.8f,  -0.1f, 0.0f), -0.9f, 0.0f);
    }
    return sample;
}

void GetReverbSample(float &outl, float &outr, float inl, float inr)
{
    //Shimmer part: basically we write the buffer once every two frames and then we read it every frame at two
    //different speeds so two octaves are produced (the higher one is reduced in intensity)
    float shimmer_l = 0.0f;
    float shimmer_r = 0.0f;
    if (((mode == REV)) or (mode == RESONATOR) ){
        shimmer_l = mlooper_buf_1l[reverb_shimmer_play_pos1l] ;
        shimmer_r = mlooper_buf_1r[reverb_shimmer_play_pos1r];
        reverb_shimmer_play_pos1l = (reverb_shimmer_play_pos1l + 2) % reverb_shimmer_buffer_size1l;
        reverb_shimmer_play_pos1r = (reverb_shimmer_play_pos1r + 2) % reverb_shimmer_buffer_size1r;
        WriteShimmerBuffer1(inl,inr);
    }
    if (mode == REV) {
       float octave2_shim = mlooper_frozen_buf_1l[(int)reverb_shimmer_play_pos2]*0.5f;
       shimmer_l = shimmer_l + octave2_shim;
       shimmer_r = shimmer_r + octave2_shim;
       reverb_shimmer_play_pos2 = (reverb_shimmer_play_pos2 + 4);
       if (reverb_shimmer_play_pos2 > reverb_shimmer_buffer_size2) {
           reverb_shimmer_play_pos2 = reverb_shimmer_play_pos2-reverb_shimmer_buffer_size2;
       }
       WriteShimmerBuffer2(inl,inr);
    }
     if (mode == RESONATOR) {
       float octave2_shim = mlooper_frozen_buf_1l[(int)reverb_shimmer_play_pos2];
       shimmer_l = shimmer_l + octave2_shim;
       shimmer_r = shimmer_r + octave2_shim;
       reverb_shimmer_play_pos2 = (reverb_shimmer_play_pos2 + 0.5);
       if (reverb_shimmer_play_pos2 > reverb_shimmer_buffer_size2) {
           reverb_shimmer_play_pos2 = reverb_shimmer_play_pos2-reverb_shimmer_buffer_size2;
       }
       WriteShimmerBuffer2(inl,inr);
    }



    reverb_rmsCount++;
    reverb_rmsCount %= (RMS_SIZE);

    if (reverb_rmsCount == 0) {
        reverb_target_RMS = reverb_averager.ProcessRMS();
    }
    fonepole(reverb_current_RMS, reverb_target_RMS, .1f);
    fonepole(reverb_feedback_RMS, reverb_target_RMS, .01f);
    if (mode == REV) {
        leds.SetBaseColor(0,clamp(reverb_current_RMS,0,1),clamp(reverb_target_RMS,0,1),clamp(reverb_current_RMS,0,1)*clamp(reverb_current_RMS,0,1));
        leds.SetBaseColor(1,clamp(reverb_target_RMS,0,1),clamp(reverb_target_RMS,0,1),clamp(reverb_target_RMS,0,1)*clamp(reverb_target_RMS,0,1));
        leds.SetBaseColor(3,clamp(reverb_current_RMS,0,1),clamp(reverb_target_RMS,0,1),clamp(reverb_current_RMS,0,1)*clamp(reverb_current_RMS,0,1));
        leds.SetBaseColor(2,clamp(reverb_target_RMS,0,1),clamp(reverb_target_RMS,0,1),clamp(reverb_target_RMS,0,1)*clamp(reverb_target_RMS,0,1));
    }

        rev.SetFeedback(reverb_feedback -reverb_feedback_RMS*0.75f);
        //summing the output of the incoming audio, the previous input, and the shimmer
        float sum_inl = (inl + shimmer_l * reverb_shimmer*(reverb_feedback*0.5f + 0.5f)*(0.5f+reverb_current_RMS*0.5f))*0.5f;
        float sum_inr = (inr + shimmer_r * reverb_shimmer*(reverb_feedback*0.5f + 0.5f)*(0.5f+reverb_current_RMS*0.5f))*0.5f;

        fonepole(reverb_target_compression, reverb_compression, .001f);

        //basic sample based limiter to avoid overloading the output
        sum_inl = CompressSample(sum_inl*reverb_target_compression);
        sum_inr = CompressSample(sum_inr*reverb_target_compression);

        rev.Process(sum_inl, sum_inr, &outl, &outr);


        reverb_current_outl =  outl;
        reverb_current_outr =  outr;

    reverb_averager.Add((reverb_current_outl*reverb_current_outl + reverb_current_outr*reverb_current_outr)/2);
    //equal power crossfade dry wet
    if (reverb_drywet > 0.98f) {
        reverb_drywet = 1.f;
    }
    outl = (sqrt(0.5f * (reverb_drywet*2.0f))*reverb_current_outl + sqrt(0.95f * (2.f - (reverb_drywet*2))) * inl)*0.7f;
    outr = (sqrt(0.5f * (reverb_drywet*2.0f))*reverb_current_outr + sqrt(0.95f * (2.f - (reverb_drywet*2))) * inr)*0.7f;


}


void GetResonatorSample(float &outl, float &outr, float inl, float inr)
{
    //First we convert the resonator note to a Frequency
    float resonator_target = global_sample_rate / mtof(resonator_note);

    //The change is slightly smoothed to avoid abrupt changes in the delay line
    fonepole(resonator_current_delay, resonator_target/resonator_octave, 1/(1+resonator_glide*25));

    // The two delays are tuned to the note frequency
    delr.SetDelay(resonator_current_delay);
    dell.SetDelay(resonator_current_delay);

    //RMS Calculation every RMS_SIZE n of samples.
    resonator_rmsCount++;
    resonator_rmsCount %= (RMS_SIZE);

    if (resonator_rmsCount == 0) {
        resonator_target_RMS = resonator_averager.ProcessRMS();
    }

    //Setting two evelope followers at different speeds
    fonepole(resonator_current_RMS, resonator_target_RMS, .0001f);
    fonepole(resonator_feedback_RMS, resonator_target_RMS, .001f);

    leds.SetBaseColor(0,clamp(resonator_current_RMS,0,1),clamp(resonator_current_RMS,0,1)*clamp(resonator_current_RMS,0,0.1), (resonator_glide_mode/10.f));
    leds.SetBaseColor(1,clamp(resonator_feedback_RMS,0,1),clamp(resonator_feedback_RMS,0,1)*clamp(resonator_feedback_RMS,0,0.1), (resonator_glide_mode/10.f));
    leds.SetBaseColor(3,clamp(resonator_current_RMS,0,1),clamp(resonator_current_RMS,0,1)*clamp(resonator_current_RMS,0,0.1), (resonator_glide_mode/10.f));
    leds.SetBaseColor(2,clamp(resonator_feedback_RMS,0,1),clamp(resonator_feedback_RMS,0,1)*clamp(resonator_feedback_RMS,0,0.1), (resonator_glide_mode/10.f));


    //Setting the reverb feedback to scale according to the intensity of the audio
    //rev.SetFeedback(resonator_feedback -resonator_feedback_RMS*0.75f*(2.f-resonator_feedback));



    //Reading from the delay
    outl = dell.Read();
    outr = delr.Read();

    //Small saturation limiter
    //outl = CompressSample(outl);
    //outr = CompressSample(outr);


    svfl.Process(tonel.Process(outl));
    svfr.Process(toner.Process(outr));
    //Filtering the output of the delay
    float resonator_outl = svfl.Low();
    float resonator_outr = svfr.Low();


    float rev_outl, rev_outr;

    //Process the reverb on the incoming audio
    GetReverbSample(rev_outl, rev_outr, (inl*0.01 +  resonator_previous_l * 0.7f)*resonator_drywet  + (inl*0.999 +  resonator_previous_l * 0.001f)*(1-resonator_drywet),
                    (inr*0.01 + resonator_previous_r*0.7f)*resonator_drywet +  (inr*0.999 +  resonator_previous_r * 0.001f)*(1-resonator_drywet));

    //Adding samples to the RMS Averager
    resonator_averager.Add((resonator_outl*resonator_outl + resonator_outr*resonator_outr)/2);

    float delay_input_l = 0.f;
    float delay_input_r = 0.f;
    if (resonator_feedback > 0) {
        delay_input_l = dcblock_l.Process(((resonator_feedback-resonator_current_RMS*0.85f) * (resonator_outl + rev_outl*(0.15 + 0.85f*(1-resonator_drywet)))));
        delay_input_r = dcblock_r.Process(((resonator_feedback-resonator_current_RMS*0.85f) * (resonator_outr + rev_outr*(0.15 + 0.85f*(1-resonator_drywet)))));
    }
    else {
        delay_input_l = dcblock_l.Process(((resonator_feedback+resonator_current_RMS*0.85f) * (resonator_outl + rev_outl*(0.15 + 0.85f*(1-resonator_drywet)))));
        delay_input_r = dcblock_r.Process(((resonator_feedback+resonator_current_RMS*0.85f) * (resonator_outr + rev_outr*(0.15 + 0.85f*(1-resonator_drywet)))));
    };


    //Writing to the delay lines and ouputting the result.
    if (resonator_drywet > 0.98f) {
        resonator_drywet = 1.f;
    }
    dell.Write(delay_input_l);
    float reso_outl = resonator_outl;
    delr.Write(delay_input_r);
    float reso_outr = resonator_outr;


    resonator_previous_l = reso_outl;
    resonator_previous_r = reso_outr;

    outl = CompressSample(reso_outl*0.1f);
    outr = CompressSample(reso_outr*0.1f);

}

void GetFilterSamples(float outl[], float outr[], float inl[], float inr[], size_t size)
{
    svf2l.ProcessMultimode(inl, outl, size, filter_mode_l);

    for (size_t i = 0; i < size; i++)
    {
        inr[i] = sqrt(0.5f * (clamp((filter_path - 0.05f), 0.0f, 1.f) * 2.0f)) * outl[i] + sqrt(1.f * (2.f - (filter_path * 2))) * inr[i];
    }

    svf2r.ProcessMultimode(inr, outr, size, filter_mode_r);
}

void GetLofiSample(float &outl, float &outr, float inl, float inr)
{   inl = inl*0.8f;
    inr = inr*0.8f;
    //RMS calculation with smoothing for the envelope follower and the variable compressor.
    //RMS is calculated every RMS_SIZE samples
    lofi_rmsCount++;
    lofi_rmsCount %= (RMS_SIZE);

    if (lofi_rmsCount == 0) {
        lofi_target_RMS = lofi_averager.ProcessRMS() * lofi_lpg_amount*10.f;
    }

    if (lofi_target_RMS < lofi_current_RMS){
        fonepole(lofi_current_RMS, lofi_target_RMS, .005f * lofi_lpg_decay*10.f);
    }
    else {
        fonepole(lofi_current_RMS, lofi_target_RMS, .05f);
    }
    //RMS value is stored into the averager.
    lofi_averager.Add((inl*inl + inr*inr)/2);

    //envelope follower partfor opening the lowpass filter
    float lofi_envelope_follower = clamp(lofi_current_RMS*lofi_cutoff*13.0f, 20.f, 20000.f);
    leds.SetBaseColor(1,lofi_current_RMS,0,0);
    leds.SetBaseColor(0,lofi_current_RMS*lofi_current_RMS,0,0);
    leds.SetBaseColor(3,lofi_current_RMS*lofi_current_RMS,0,0);
    leds.SetBaseColor(2,lofi_current_RMS,0,0);


    tonel.SetFreq(lofi_envelope_follower);
    toner.SetFreq(lofi_envelope_follower);

    rev.SetLpFreq((lofi_envelope_follower*0.6) + (global_sample_rate*0.3));

    svf2l.set_f_q <stmlib::FREQUENCY_FAST> (lofi_envelope_follower/global_sample_rate, 1.f);
    svf2r.set_f_q <stmlib::FREQUENCY_FAST> (lofi_envelope_follower/global_sample_rate,1.f);
    //when knob1 is very low an hi-pass filter activates to create a more distant sound
    float lofi_hipass_freq = clamp(200.0 - (lofi_cutoff*2), 0.0f,200);
    svfl.SetFreq(lofi_hipass_freq);
    svfr.SetFreq(lofi_hipass_freq);


    //knob 2 sets how often the delay modulation changes. This time is variable between 0 and lofi_mod time.

    lofi_rate_count++;
    lofi_rate_count %= lofi_mod;
    if(lofi_rate_count == 0)
    {
        leds.SetForXCycles(1,10,0.0,0.0,0.0);
        leds.SetForXCycles(2,10,0.0,0.0,0.0);

        float r = (float) (rand() %lofi_mod);
        lofi_rate_count = rand() %(lofi_mod);
        lofi_target_Lofi_LFO_Freq = 0.001f + (r*lofi_depth)/5.f;
        lofi_damp_speed = lofi_rate_count;
    }
    //This smoothing allows for the delay time to change slowly so the pitch shifting effect is subtle
    fonepole(lofi_current_Lofi_LFO_Freq, lofi_target_Lofi_LFO_Freq, 1.0 / (1.2f * (lofi_damp_speed + (lofi_mod*3)/2)));

    dell.SetDelay(lofi_current_Lofi_LFO_Freq);
    delr.SetDelay(lofi_current_Lofi_LFO_Freq);

    //here we already read the contents of the delay and assign it to the ouputs.

    if (lofi_drywet > 0.98f) {
        lofi_drywet = 1.f;
    }
    outl = sqrt(0.5f * (lofi_drywet*2.0f))*dell.Read() + sqrt(0.95f * (2.f - (lofi_drywet*2))) * inl;
    outr = sqrt(0.5f * (lofi_drywet*2.0f))*delr.Read() + sqrt(0.95f * (2.f - (lofi_drywet*2))) * inr;


    //now we process the input and we add it to the delay line
    //first we filter it with the two filters: low-pass-gate and hi-pass
    //they work a bit differently. The Tone objects accepts the input as a parameter
    //and gives back a filtered sample. The SVF accepts an input and we retrive the
    //hi-pass ouput with High
    float filter_outl = svf2l.Process<stmlib::FILTER_MODE_LOW_PASS>(inl);
    float filter_outr = svf2r.Process<stmlib::FILTER_MODE_LOW_PASS>(inr);

    svfl.Process(tonel.Process(filter_outl));
    svfr.Process(toner.Process(filter_outr));
    float lofi_leftFilter = svfl.High();
    float lofi_rightFilter = svfr.High();

    //Here we calculate a basic compressor using the RMS values of the envelope follower
    //this way we compensate for the lack of volume when filtering
    //by interpolating it with the previous state of the compressor, we slightly smooth
    //the change avoiding potential clicks.
    float lofi_variable_compressor = (2.f*lofi_previous_variable_compressor + ((300.f-clamp(lofi_cutoff, 30.f, 300.f))/300.f)* (1.f-lofi_current_RMS)*1.f)*0.33f;
    lofi_previous_variable_compressor = lofi_variable_compressor;


    //This is a simple saturation that will contribute in enhancing the volume "vintage way"
    //when knob 1 is low
    float lofi_current_left_saturation = abs(inl * inl );
    lofi_previous_left_saturation = (lofi_current_left_saturation + lofi_previous_left_saturation*9.f)/10.f;
    float lofi_current_right_saturation =  abs(inr * inr);
    lofi_previous_right_saturation = (lofi_current_right_saturation + lofi_previous_right_saturation*9.f)/10.f;

    //Here we calculate the outputs, which are the filtered waveform plus, a certain amount of the other channel
    //to "monoize it" when knob 1 is low, plus a certain amount of compression and saturation.
    float lofi_left = lofi_leftFilter + (((200.f-clamp(lofi_cutoff, 20.f, 200.f))/200.f) * lofi_rightFilter) + lofi_leftFilter * lofi_variable_compressor + lofi_leftFilter * lofi_variable_compressor*lofi_current_left_saturation*0.01f;
    float lofi_right = lofi_rightFilter + (((200.f-clamp(lofi_cutoff, 20.f, 200.f))/200.f) * lofi_leftFilter) + lofi_rightFilter * lofi_variable_compressor + lofi_rightFilter * lofi_variable_compressor*lofi_current_right_saturation*0.01f;

    //we still add a certain amount of the other channel to further monoize the sound
    lofi_left = lofi_left + lofi_right* ((200.f-clamp(lofi_cutoff, 20.f, 200.f))/200.f);
    lofi_right = lofi_right + lofi_left* ((200.f-clamp(lofi_cutoff, 20.f, 200.f))/200.f);

    // this is a basic instantaneous saturation/limiter: if the sound is too loud (in either)
    // directions, we compress it to avoid digital clipping.
    if (lofi_left > 0.4) {
        lofi_left = clamp(lofi_left - map(lofi_left, 0.4f, 10.0f, 0.0f, 0.6f), 0.0f, 1.0f);
    }
    if (lofi_right > 0.4) {
        lofi_right = clamp(lofi_right - map(lofi_right, 0.4f, 10.0f, 0.0f, 0.6f), 0.0f, 1.0f);
    }

    if (lofi_left < -0.4) {
        lofi_left = clamp(lofi_left - map(lofi_left, -10.0f,-0.4f,  -0.6f, 0.0f), -1.0f, 0.0f);
    }
    if (lofi_right < -0.4) {
        lofi_right = clamp(lofi_right - map(lofi_right,  -10.0f,-0.4f,  -0.6f ,0.0f), -1.0f, 0.0f);
    }

    //we write all of this on the delayline
    dell.Write(lofi_left);
    delr.Write(lofi_right);
};





void ResetLooperBuffer()
{
    //initialize all settings
    mlooper_play  = false;
    mlooper_pos_1   = 0;
    mlooper_frozen_pos_1 = 0;
    mlooper_pos_2  = 0;
    mlooper_frozen_pos_2 = 0;
    mlooper_writer_pos = 0;
    mlooper_writer_outside_pos = 0;
    mlooper_len   = 0;
    mlooper_frozen_len = 0;
    mlooper_len_count = 0;
    for(int i = 0; i < LOOPER_MAX_SIZE; i++)
    {
        mlooper_buf_1l[i] = 0;
        mlooper_buf_1r[i] = 0;
        mlooper_frozen_buf_1l[i] = 0;
        mlooper_frozen_buf_1r[i] = 0;
    }
}

void WriteLooperBuffer(float in_1l, float in_1r)
{
    //writes the input to the buffer
    //mlooper_buf_1l[mlooper_writer_pos] = in_1l;
    //mlooper_buf_1r[mlooper_writer_pos] = in_1r;


    mlooper_buf_1l[mlooper_writer_pos] = in_1l;
    mlooper_buf_1r[mlooper_writer_pos] = in_1r;

    //this allows to fill the buffer when the next buffer is going to be bigger than the previous one
    if (mlooper_writer_outside_pos > mlooper_len) {
        mlooper_buf_1l[mlooper_writer_outside_pos] = in_1l;
        mlooper_buf_1r[mlooper_writer_outside_pos] = in_1r;
    }



    //if frozen is active, stop writing to the frozen buffer
    if(!mlooper_frozen) {
        mlooper_frozen_buf_1l[mlooper_writer_pos] = in_1l;
        mlooper_frozen_buf_1r[mlooper_writer_pos] = in_1r;
    }
};


void FreezeLooperBuffer() {
     //inherits the settings from the normal buffer when freezing
     mlooper_frozen_len = mlooper_len;
     mlooper_frozen_pos_1 = mlooper_pos_1;
     mlooper_frozen_pos_2 = mlooper_pos_2;

};
void SelectLooperDivision(float knob_value_1, float knob_value_2){
    //sets the amount of repetitions
    if (knob_value_1 < 0.2f){
        mlooper_division_1 = 1.f;
        mlooper_division_string_1 = " 1/1";
    } else if (knob_value_1 < 0.4f){
        mlooper_division_1 = 0.5f;
        mlooper_division_string_1 = " 1/2";
    } else if (knob_value_1 < 0.6f){
        mlooper_division_1 = 0.25f;
        mlooper_division_string_1 = " 1/4";
    } else if (knob_value_1 < 0.8f){
        mlooper_division_1 = 0.125f;
        mlooper_division_string_1 = " 1/8";
    } else if (knob_value_1 > 0.8f){
        mlooper_division_1 = 0.0625f;
        mlooper_division_string_1 = "1/16";
    }

    if (knob_value_2 < 0.2f){
        mlooper_division_2 = 1.f;
        mlooper_division_string_2 = " 1/1";
    } else if (knob_value_2 < 0.4f){
        mlooper_division_2 = 0.5f;
        mlooper_division_string_2 = " 1/2";
    } else if (knob_value_2 < 0.6f){
        mlooper_division_2 = 0.25f;
        mlooper_division_string_2 = " 1/4";
    } else if (knob_value_2 < 0.8f){
        mlooper_division_2 = 0.125f;
        mlooper_division_string_2 = " 1/8";
    } else if (knob_value_2 > 0.8f){
        mlooper_division_2 = 0.0625f;
        mlooper_division_string_2 = "1/16";
    }
};

void SelectLooperPlaySpeed(float knob_value_1, float knob_value_2){
    //sets the octave shift
    if (knob_value_1 < 0.2f){
        mlooper_play_speed_1 = 0.25f;
        mlooper_volume_att_1 = 1.0f;
        mlooper_play_speed_string_1 = "-2";
    } else if (knob_value_1 < 0.4f){
        mlooper_play_speed_1 = 0.5f;
        mlooper_volume_att_1 = 1.0f;
        mlooper_play_speed_string_1 = "-1";
    } else if (knob_value_1 < 0.6f){
        mlooper_play_speed_1 = 1.f;
        mlooper_volume_att_1 = 1.0f;
        mlooper_play_speed_string_1 = " 0";
    } else if (knob_value_1 < 0.8f){
        mlooper_play_speed_1 = 2.f;
        mlooper_volume_att_1 = 0.7f;
        mlooper_play_speed_string_1 = "+1";
    } else if (knob_value_1 > 0.8f){
        mlooper_play_speed_1 = 4.f;
        mlooper_volume_att_1 = 0.5f;
        mlooper_play_speed_string_1 = "+2";
    }

    if (knob_value_2 < 0.2f){
        mlooper_play_speed_2 = 0.25f;
        mlooper_volume_att_2 = 1.f;
        mlooper_play_speed_string_2 = "-2";
    } else if (knob_value_2 < 0.4f){
        mlooper_play_speed_2 = 0.5f;
        mlooper_volume_att_2 = 1.f;
        mlooper_play_speed_string_2 = "-1";
    } else if (knob_value_2 < 0.6f){
        mlooper_play_speed_2 = 1.f;
        mlooper_volume_att_2 = 1.f;
        mlooper_play_speed_string_2 = " 0";
    } else if (knob_value_2 < 0.8f){
        mlooper_play_speed_2 = 2.f;
        mlooper_volume_att_2 = 0.7f;
        mlooper_play_speed_string_2 = "+1";
    } else if (knob_value_2 > 0.8f){
        mlooper_play_speed_2 = 4.f;
        mlooper_volume_att_2 = 0.5f;
        mlooper_play_speed_string_2 = "+2";
    }
};

float GetSampleFromBuffer(float buffer[], float pos) {
    //linear interpolation that gives back one sample in a certain position in the buffer
    int32_t pos_integral   = static_cast<int32_t>(pos);
    float   pos_fractional = pos - static_cast<float>(pos_integral);
    float a = buffer[pos_integral % LOOPER_MAX_SIZE];
    float b = buffer[(pos_integral +1) % LOOPER_MAX_SIZE];
    return a + (b - a) * pos_fractional;
}

void GetLooperSample(float &out1l, float &out1r, float in1l, float in1r)
{   //writing the incoming input into the buffer
    out1l = out1r = 0;
    WriteLooperBuffer(in1l, in1r);
    //advance the buffer writing cursor and wrap it if it's longer than the buffer length
    mlooper_writer_pos++;
    mlooper_writer_pos = (mlooper_writer_pos + mlooper_len) % mlooper_len;
    if(mlooper_play) {
        if(!mlooper_frozen) //if the looper is not frozen
        {
            //if the buffer is not frozen we get one sample from the looper

            if(mlooper_play)
            {
                //now we change the play_pos according to the number of repetitions and the playing speed
                mlooper_pos_1 = mlooper_pos_1 + mlooper_play_speed_1;
                modified_buffer_length_l = (int)(mlooper_len * mlooper_division_1);
                if (mlooper_pos_1 > modified_buffer_length_l) {
                    leds.SetForXCycles(0,10,1,0,0);
                    mlooper_pos_1 = clamp(mlooper_pos_1 - modified_buffer_length_l, 0, mlooper_len);
                } else if (mlooper_pos_1<0.f){
                    mlooper_pos_1 = clamp(mlooper_pos_1 + modified_buffer_length_l, 0, mlooper_len);
                }

                mlooper_pos_2 = mlooper_pos_2 + mlooper_play_speed_2;
                modified_buffer_length_r =  (int)(mlooper_len * mlooper_division_2);
                if (mlooper_pos_2 > modified_buffer_length_r) {
                    leds.SetForXCycles(3,10,1,0,0);
                    mlooper_pos_2 = clamp(mlooper_pos_2 - modified_buffer_length_r, 0, mlooper_len);
                } else if (mlooper_pos_2<0.f){
                    mlooper_pos_2 = clamp(mlooper_pos_2 + modified_buffer_length_r, 0, mlooper_len);
                }

                out1l = GetSampleFromBuffer(mlooper_buf_1l,mlooper_pos_1)*mlooper_volume_att_1;
                out1r = GetSampleFromBuffer(mlooper_buf_1r,mlooper_pos_2)*mlooper_volume_att_2;

            };
        } else //Frozen Buffer
        {   //if the buffer is not frozen we get one sample from the frozen looper


            //out1l = GetSampleFromBuffer(mlooper_frozen_buf_1l,mlooper_frozen_pos_1)*mlooper_volume_att_1;
            //out1r = GetSampleFromBuffer(mlooper_frozen_buf_1r,mlooper_frozen_pos_2)*mlooper_volume_att_2;

            if(mlooper_play)
            {
                //now we change the play_pos according to the number of repetitions and the playing speed
                mlooper_frozen_pos_1 = mlooper_frozen_pos_1 + mlooper_play_speed_1;
                modified_frozen_buffer_length_l =  (int)(mlooper_frozen_len * mlooper_division_1);
                if (mlooper_frozen_pos_1 > modified_frozen_buffer_length_l) {
                    leds.SetForXCycles(0,10,0,0,1);
                    mlooper_frozen_pos_1 = clamp(mlooper_frozen_pos_1 - modified_frozen_buffer_length_l, 0, mlooper_len) ;
                } else if (mlooper_frozen_pos_1<0.f){
                    mlooper_frozen_pos_1 = clamp(mlooper_frozen_pos_1 + modified_frozen_buffer_length_l, 0, mlooper_len);
                }

                mlooper_frozen_pos_2 = mlooper_frozen_pos_2 + mlooper_play_speed_2;
                modified_frozen_buffer_length_r = (int)(mlooper_frozen_len * mlooper_division_2);
                if (mlooper_frozen_pos_2 > modified_frozen_buffer_length_r) {
                    leds.SetForXCycles(3,10,0,0,1);
                    mlooper_frozen_pos_2 = clamp(mlooper_frozen_pos_2 - modified_frozen_buffer_length_r, 0, mlooper_len) ;
                } else if (mlooper_frozen_pos_2<0.f){
                    mlooper_frozen_pos_2 = clamp(mlooper_frozen_pos_2 + modified_frozen_buffer_length_r, 0, mlooper_len);
                }

            out1l = GetSampleFromBuffer(mlooper_frozen_buf_1l,mlooper_frozen_pos_1)*mlooper_volume_att_1;
            out1r = GetSampleFromBuffer(mlooper_frozen_buf_1r,mlooper_frozen_pos_2)*mlooper_volume_att_2;

            }
        };
    }
    //advance the counter for calculating the length of the next incoming buffer
    mlooper_len_count++;

    mlooper_writer_outside_pos++;
    mlooper_writer_outside_pos = ((mlooper_writer_outside_pos + mlooper_len_count) % (mlooper_len_count)) % LOOPER_MAX_SIZE;

    //automatic looptime
    if (mlooper_len >= LOOPER_MAX_SIZE)
    {
          mlooper_len   = LOOPER_MAX_SIZE-1;
    }

    if (mlooper_drywet > 0.98f) {
        mlooper_drywet = 1.f;
    }
    out1l = sqrt(0.5f * (mlooper_drywet*2.0f))*out1l + sqrt(0.95f * (2.f - (mlooper_drywet*2))) * in1l;
    out1r = sqrt(0.5f * (mlooper_drywet*2.0f))*out1r + sqrt(0.95f * (2.f - (mlooper_drywet*2))) * in1r;

};


void SelectDelayDivision(float knob1, float knob2) {
    //sets the octave shift
    float new_delay_mult_l;
    new_delay_mult_l = delay_times[(int)(knob1*NUM_DELAY_TIMES)];
    float new_delay_mult_r;
    new_delay_mult_r = delay_times[(int)(knob2*NUM_DELAY_TIMES)];
    //if (delay_time[delay_active] == -1){
    //    delay_mult_l[delay_active] = new_delay_mult_l;
    //    delay_mult_r[delay_active] = new_delay_mult_r;
    //    };

    if ((new_delay_mult_l != delay_mult_l[delay_active]) or (new_delay_mult_r != delay_mult_r[delay_active])) {

        delay_mult_l[delay_inactive] = new_delay_mult_l;
        delay_mult_r[delay_inactive] = new_delay_mult_r;

        delay_xfade_target=delay_inactive;
    }

}
/*
void ResetDelayBuffer()
{
    for(int i = 0; i < LOOPER_MAX_SIZE; i++)
    {
        mlooper_buf_1l[i] = 0;
        mlooper_buf_1r[i] = 0;
        mlooper_frozen_buf_1l[i] = 0;
        mlooper_frozen_buf_1r[i] = 0;
    }
}
*/
void WriteDelayBuffer(float in_1l, float in_1r)
{

    mlooper_buf_1l[delay_write_pos] = in_1l;
    mlooper_buf_1r[delay_write_pos] = in_1r;

    //if frozen is active, stop writing to the frozen buffer
    if(!delay_frozen) {
        mlooper_frozen_buf_1l[delay_write_pos] = in_1l;
        mlooper_frozen_buf_1r[delay_write_pos] = in_1r;
    }
};

void GetDelaySample(float &out1l, float &out1r, float in1l, float in1r)
{
    out1l = out1r = 0;

        delay_rmsCount++;
        delay_rmsCount %= (RMS_SIZE);

        if (delay_rmsCount == 0) {
            delay_target_RMS = delay_averager.ProcessRMS();
        }

        fonepole(delay_feedback_RMS, delay_target_RMS, .001f *(1/(0.5+delay_feedback_RMS)) );

        fonepole(delay_fast_feedback_RMS, delay_target_RMS, .0005f *(1/(0.7+delay_fast_feedback_RMS)));


        float input_l = dcblock_2l.Process(delay_prev_sample_l*delay_feedback*(1-delay_feedback_RMS*0.3) + in1l*(clamp(1-delay_feedback,0.5,1)))*(1-delay_fast_feedback_RMS*0.4);
        float input_r = dcblock_2r.Process(delay_prev_sample_r*delay_feedback*(1-delay_feedback_RMS*0.3) + in1r*(clamp(1-delay_feedback,0.5,1)))*(1-delay_fast_feedback_RMS*0.4);


        WriteDelayBuffer(input_l, input_r);

    if (delay_time[delay_inactive] > 0) {
        if(!delay_frozen) {

            delay_pos_l[0] =((int)((delay_write_pos - (delay_time[0]*delay_mult_l[0])) + LOOPER_MAX_SIZE) )% LOOPER_MAX_SIZE;
            delay_pos_r[0] =((int)((delay_write_pos - (delay_time[0]*delay_mult_r[0])) + LOOPER_MAX_SIZE)) % LOOPER_MAX_SIZE;

            delay_outl[0] = mlooper_buf_1l[delay_pos_l[0]];
            delay_outr[0] = mlooper_buf_1r[delay_pos_r[0]];

            delay_pos_l[1] =((int)((delay_write_pos - (delay_time[1]*delay_mult_l[1])) + LOOPER_MAX_SIZE) )% LOOPER_MAX_SIZE;
            delay_pos_r[1] =((int)((delay_write_pos - (delay_time[1]*delay_mult_r[1])) + LOOPER_MAX_SIZE)) % LOOPER_MAX_SIZE;

            delay_outl[1] = mlooper_buf_1l[delay_pos_l[1]];
            delay_outr[1] = mlooper_buf_1r[delay_pos_r[1]];

        }
        else {
            delay_pos_l[0] =((int)((delay_frozen_pos - (delay_time[0]*delay_mult_l[0])) + LOOPER_MAX_SIZE)) % LOOPER_MAX_SIZE;
            delay_pos_r[0] =((int)((delay_frozen_pos - (delay_time[0]*delay_mult_r[0])) + LOOPER_MAX_SIZE)) % LOOPER_MAX_SIZE;

            delay_outl[0] = mlooper_frozen_buf_1l[delay_pos_l[0]];
            delay_outr[0] = mlooper_frozen_buf_1r[delay_pos_r[0]];

            delay_pos_l[1] =((int)((delay_frozen_pos - (delay_time[1]*delay_mult_l[1])) + LOOPER_MAX_SIZE)) % LOOPER_MAX_SIZE;
            delay_pos_r[1] =((int)((delay_frozen_pos - (delay_time[1]*delay_mult_r[1])) + LOOPER_MAX_SIZE)) % LOOPER_MAX_SIZE;

            delay_outl[1] = mlooper_frozen_buf_1l[delay_pos_l[1]];
            delay_outr[1] = mlooper_frozen_buf_1r[delay_pos_r[1]];

            delay_frozen_pos++;
            delay_frozen_pos = delay_frozen_pos % LOOPER_MAX_SIZE;
            if (delay_frozen_pos == delay_frozen_end) {
                delay_frozen_pos = delay_frozen_start;
                }
            }

            if (delay_xfade_current> delay_xfade_target) {
                delay_xfade_current = clamp(delay_xfade_current - (1.f/(47.f*delay_control_latency_ms)),0,1);
            } else if (delay_xfade_current < delay_xfade_target){
                delay_xfade_current = clamp(delay_xfade_current + (1.f/(47.f*delay_control_latency_ms)), 0, 1);
            } else {
                delay_active = (int) delay_xfade_target ;
                delay_inactive = (delay_active+1) % 2;
                //delay_left_counter = delay_pos_l[delay_active] - delay_write_pos;
                //delay_right_counter = delay_pos_r[delay_active] - delay_write_pos;
            }

        };



    //leds
    delay_left_counter = (delay_left_counter -1);
    if (delay_left_counter <=0) {

        if (delay_left_counter_4 == 0) {
            leds.SetForXCycles(0, 3, 1*!delay_frozen,0,delay_frozen);
        }
        delay_left_counter_4 = (delay_left_counter_4 +1) % 4;
        delay_left_counter = (delay_write_pos - delay_pos_l[delay_active]) / 4 ;
    }
    delay_right_counter = (delay_right_counter -1);
    if (delay_right_counter <=0) {
        if (delay_right_counter_4 == 0) {
            leds.SetForXCycles(3, 3, 1*!delay_frozen ,0,delay_frozen);
        }
        delay_right_counter_4 = (delay_right_counter_4 +1) % 4;

        delay_right_counter = (delay_write_pos - delay_pos_r[delay_active]) / 4;
    }


    delay_time_count++;
    delay_write_pos++;
    delay_write_pos = delay_write_pos % LOOPER_MAX_SIZE;




    float delay_outputl, delay_outputr;
    delay_outputl = sqrt(0.5f * (delay_xfade_current*2.0f))*delay_outl[1] + sqrt(0.95f * (2.f - (delay_xfade_current*2))) * delay_outl[0];
    delay_outputr = sqrt(0.5f * (delay_xfade_current*2.0f))*delay_outr[1] + sqrt(0.95f * (2.f - (delay_xfade_current*2))) * delay_outr[0];



    delay_outputl = dcblock_l.Process(delay_outputl);
    delay_outputr = dcblock_r.Process(delay_outputr);
    delay_outputl = tonel.Process(delay_outputl);
    delay_outputr = toner.Process(delay_outputr);
    //float filter_out1l = svf2l.Process<stmlib::FILTER_MODE_LOW_PASS>(delay_out1l);
    //float filter_out1r = svf2r.Process<stmlib::FILTER_MODE_LOW_PASS>(delay_out1r);


    float reverb_outl, reverb_outr;
    GetReverbSample(reverb_outl, reverb_outr, delay_outputl, delay_outputr);

    delay_prev_sample_l = reverb_outl*0.85f;// + delay_outputl*0.1f;
    delay_prev_sample_r = reverb_outr*0.85f; // + delay_outputr*0.1f;

    delay_averager.Add((delay_prev_sample_l*delay_prev_sample_l + delay_prev_sample_r*delay_prev_sample_r)/2);


    if (delay_drywet > 0.99f) {
         delay_drywet = 1.f ;
    }
    out1l = sqrt(0.5f * (delay_drywet*2.0f))*delay_prev_sample_l + sqrt(0.7f * (2.f - (delay_drywet*2))) * in1l;
    out1r = sqrt(0.5f * (delay_drywet*2.0f))*delay_prev_sample_r + sqrt(0.7f * (2.f - (delay_drywet*2))) * in1r;

};

void GetSpectraSample(float &outl, float &outr, float inl, float inr) {
    float spectra_outl;
    float spectra_outr;
    spectra_oscbank.updateFreqAndMagn();
    float spectra_output = spectra_oscbank.Process();

    spectra_output = spectra_output; //(sqrt(0.5f * (spectra_dynamics*2.0f))*filtered_out + sqrt(0.95f * (2.f - (spectra_dynamics*2))) * spectra_output)*0.5f;


    spectra_outl = spectra_output;

    spectra_outr = spectra_outl;
    if (spectra_drywet > 0.98f) {
        spectra_drywet = (1.f + spectra_drywet)*0.5f;
    }



    outl = (sqrt(0.5f * (spectra_drywet*2.0f))*spectra_outl + sqrt(0.95f * (2.f - (spectra_drywet*2))) * inl)*0.5f;
    outr = (sqrt(0.5f * (spectra_drywet*2.0f))*spectra_outr + sqrt(0.95f * (2.f - (spectra_drywet*2))) * inr)*0.5f;


}

void GetSpectringsSample(float &outl, float &outr, float inl, float inr) {
        spectra_oscbank.updateFreqAndMagn();
        float rings_1 = string_voice[0].Process()* (spectrings_accent_amount[0]*attack_lut[spectrings_attack_step[0]] + (1-spectrings_accent_amount[0]) );
        float rings_2 = string_voice[1].Process()* (spectrings_accent_amount[1]*attack_lut[spectrings_attack_step[1]] + (1-spectrings_accent_amount[1]) );
        //float rings_3 = string_voice[2].Process()* (spectrings_accent_amount[2]*spectrings_attack_lut[spectrings_attack_step[2]] + (1-spectrings_accent_amount[2]) );
        //float rings_4 = string_voice[3].Process()* (spectrings_accent_amount[3]*spectrings_attack_lut[spectrings_attack_step[3]] + (1-spectrings_accent_amount[3]) );

        float spectrings_outl = (rings_1 + rings_2 * spectrings_pan_spread)*(0.7 + (1-spectrings_pan_spread)*0.3);
        float spectrings_outr = (rings_2 + rings_1 * spectrings_pan_spread)*(0.7 + (1-spectrings_pan_spread)*0.3);

        spectrings_attack_step[0] = clamp(spectrings_attack_step[0]+1, 0, 299);
        spectrings_attack_step[1] = clamp(spectrings_attack_step[1]+1, 0, 299);
        //spectrings_attack_step[2] = clamp(spectrings_attack_step[2]+1, 0, 299);
        //spectrings_attack_step[3] = clamp(spectrings_attack_step[3]+1, 0, 299);


    if (spectrings_drywet > 0.98f) {
        spectrings_drywet = 1.f;
    }
    outl = (sqrt(0.5f * (spectrings_drywet*2.0f))*spectrings_outl + sqrt(0.95f * (2.f - (spectrings_drywet*2))) * inl)*0.5f;
    outr = (sqrt(0.5f * (spectrings_drywet*2.0f))*spectrings_outr + sqrt(0.95f * (2.f - (spectrings_drywet*2))) * inr)*0.5f;

}
