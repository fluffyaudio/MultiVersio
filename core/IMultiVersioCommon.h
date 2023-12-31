/**
 * @class IMultiVersioCommon
 * @brief Interface for the common functionality of the MultiVersio system.
 *
 * This class provides an interface for the common functionality of the MultiVersio system.
 * It includes member variables and functions that are shared among different effects.
 *
 * The class contains a constructor that takes a reference to a DaisyVersio object and initializes
 * the DaisyVersio and LedsControl member variables.
 *
 * The class also includes an array of pointers to IEffect instances, which can hold up to 8 effects.
 *
 * The class defines static variables for delay lines, buffers, and various audio processing modules
 * such as reverb, filters, tone, biquad, and DC block.
 *
 * It also includes member variables for mode, previous mode, sample rate, and global sample rate.
 *
 * The class provides virtual functions for running the MultiVersio system and getting the current mode.
 *
 * Additionally, it includes reverb parameters that are used across all effects, as well as an attack lookup table.
 */
#pragma once

#include "DaisySP/Source/daisysp.h"
#include "libDaisy/src/daisy_versio.h"
#include "helpers.h"
#include "leds.h"
#include "stmlib/dsp/filter.h"
#include "fx/IEffect.h"

/**
 * @class IMultiVersioCommon
 * @brief Interface for the MultiVersio common functionality.
 *
 * This class defines the common functionality that is shared among different versions of MultiVersio.
 * It provides access to the DaisyVersio object, LED control, and various audio processing components.
 * Subclasses of IMultiVersioCommon must implement the run() and getMode() methods.
 */
class IMultiVersioCommon
{
public:
    /**
     * @brief Constructor.
     *
     * Initializes the IMultiVersioCommon object with the given DaisyVersio instance.
     *
     * @param versio The DaisyVersio instance.
     */
    IMultiVersioCommon(daisy::DaisyVersio &versio) : versio(versio), leds(versio) {}

    daisy::DaisyVersio versio; /**< The DaisyVersio instance. */
    LedsControl leds;          /**< The LedsControl instance. */

    IEffect *effects[8]; /**< Array for 8 IEffect instances. */

    static daisysp::DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS dell;    /**< Static DelayLine instance for left channel. */
    static daisysp::DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delr;    /**< Static DelayLine instance for right channel. */
    static float DSY_SDRAM_BSS mlooper_buf_1l[LOOPER_MAX_SIZE];        /**< Static buffer for left channel looper. */
    static float DSY_SDRAM_BSS mlooper_buf_1r[LOOPER_MAX_SIZE];        /**< Static buffer for right channel looper. */
    static float DSY_SDRAM_BSS mlooper_frozen_buf_1l[LOOPER_MAX_SIZE]; /**< Static buffer for left channel frozen looper. */
    static float DSY_SDRAM_BSS mlooper_frozen_buf_1r[LOOPER_MAX_SIZE]; /**< Static buffer for right channel frozen looper. */

    daisysp::ReverbSc rev;       /**< ReverbSc instance. */
    daisysp::Svf svfl, svfr;     /**< Svf instances for left and right channels. */
    stmlib::Svf svf2l, svf2r;    /**< Svf instances for left and right channels. */
    daisysp::Tone tonel, toner;  /**< Tone instances for left and right channels. */
    daisysp::Biquad biquad;      /**< Biquad instance. */
    daisysp::DcBlock dcblock_l;  /**< DcBlock instance for left channel. */
    daisysp::DcBlock dcblock_r;  /**< DcBlock instance for right channel. */
    daisysp::DcBlock dcblock_2l; /**< DcBlock instance for left channel. */
    daisysp::DcBlock dcblock_2r; /**< DcBlock instance for right channel. */

    int mode;               /**< Current mode. */
    int previous_mode;      /**< Previous mode. */
    int sample_rate;        /**< Sample rate. */
    int global_sample_rate; /**< Global sample rate. */

    // reverb parameters used across all effects
    float reverb_drywet, reverb_feedback, reverb_lowpass, reverb_shimmer = 0;
    float reverb_compression = 1.0f;

    float attack_lut[300]; /**< Attack lookup table. */

    /**
     * @brief Pure virtual function to run the MultiVersio functionality.
     */
    virtual void run() = 0;

    /**
     * @brief Pure virtual function to get the current mode.
     *
     * @return The current mode.
     */
    virtual int getMode() = 0;
};
