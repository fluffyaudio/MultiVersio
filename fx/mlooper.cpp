#include "mlooper.h"
#include "IMultiVersioCommon.h"

/**
 * @brief Construct a new MLooper::MLooper object
 *
 * Resets the looper buffer.
 *
 * @param mv A reference to the IMultiVersioCommon interface
 */
MLooper::MLooper(IMultiVersioCommon &mv) : mv(mv)
{
    ResetLooperBuffer();
}

/**
 * @brief Resets the looper buffer.
 *
 * Initializes all looper settings and clears the looper buffer.
 */
void MLooper::ResetLooperBuffer()
{
    // initialize all settings
    mlooper_play = false;
    mlooper_pos_1 = 0;
    mlooper_frozen_pos_1 = 0;
    mlooper_pos_2 = 0;
    mlooper_frozen_pos_2 = 0;
    mlooper_writer_pos = 0;
    mlooper_writer_outside_pos = 0;
    mlooper_len = 0;
    mlooper_frozen_len = 0;
    mlooper_len_count = 0;
    for (size_t i = 0; i < LOOPER_MAX_SIZE; i++)
    {
        IMultiVersioCommon::mlooper_buf_1l[i] = 0;
        IMultiVersioCommon::mlooper_buf_1r[i] = 0;
        IMultiVersioCommon::mlooper_frozen_buf_1l[i] = 0;
        IMultiVersioCommon::mlooper_frozen_buf_1r[i] = 0;
    }
}

/**
 * @brief Writes the input to the looper buffer.
 *
 * @param in_1l The left input sample.
 * @param in_1r The right input sample.
 */
void MLooper::WriteLooperBuffer(float in_1l, float in_1r)
{
    // writes the input to the buffer
    // mlooper_buf_1l[mlooper_writer_pos] = in_1l;
    // mlooper_buf_1r[mlooper_writer_pos] = in_1r;

    IMultiVersioCommon::mlooper_buf_1l[mlooper_writer_pos] = in_1l;
    IMultiVersioCommon::mlooper_buf_1r[mlooper_writer_pos] = in_1r;

    // this allows to fill the buffer when the next buffer is going to be bigger than the previous one
    if (mlooper_writer_outside_pos > mlooper_len)
    {
        IMultiVersioCommon::mlooper_buf_1l[mlooper_writer_outside_pos] = in_1l;
        IMultiVersioCommon::mlooper_buf_1r[mlooper_writer_outside_pos] = in_1r;
    }

    // if frozen is active, stop writing to the frozen buffer
    if (!mlooper_frozen)
    {
        IMultiVersioCommon::mlooper_frozen_buf_1l[mlooper_writer_pos] = in_1l;
        IMultiVersioCommon::mlooper_frozen_buf_1r[mlooper_writer_pos] = in_1r;
    }
};

/**
 * @brief Freezes the looper buffer.
 *
 * Inherits the settings from the normal buffer when freezing.
 */
void MLooper::FreezeLooperBuffer()
{
    // inherits the settings from the normal buffer when freezing
    mlooper_frozen_len = mlooper_len;
    mlooper_frozen_pos_1 = mlooper_pos_1;
    mlooper_frozen_pos_2 = mlooper_pos_2;
};

/**
 * @brief Selects the looper division based on the given knob values.
 *
 * The looper division is determined as follows:
 *
 * - If the knob value is less than 0.2, the looper division is set to 1/1.
 * - If the knob value is between 0.2 and 0.4, the looper division is set to 1/2.
 * - If the knob value is between 0.4 and 0.6, the looper division is set to 1/4.
 * - If the knob value is between 0.6 and 0.8, the looper division is set to 1/8.
 * - If the knob value is greater than 0.8, the looper division is set to 1/16.
 *
 * @param knob_value_1 Left channel looper division knob value.
 * @param knob_value_2 Right channel looper division knob value.
 */
void MLooper::SelectLooperDivision(float knob_value_1, float knob_value_2)
{
    // sets the amount of repetitions
    if (knob_value_1 < 0.2f)
    {
        mlooper_division_1 = 1.f;
        mlooper_division_string_1 = " 1/1";
    }
    else if (knob_value_1 < 0.4f)
    {
        mlooper_division_1 = 0.5f;
        mlooper_division_string_1 = " 1/2";
    }
    else if (knob_value_1 < 0.6f)
    {
        mlooper_division_1 = 0.25f;
        mlooper_division_string_1 = " 1/4";
    }
    else if (knob_value_1 < 0.8f)
    {
        mlooper_division_1 = 0.125f;
        mlooper_division_string_1 = " 1/8";
    }
    else if (knob_value_1 > 0.8f)
    {
        mlooper_division_1 = 0.0625f;
        mlooper_division_string_1 = "1/16";
    }

    if (knob_value_2 < 0.2f)
    {
        mlooper_division_2 = 1.f;
        mlooper_division_string_2 = " 1/1";
    }
    else if (knob_value_2 < 0.4f)
    {
        mlooper_division_2 = 0.5f;
        mlooper_division_string_2 = " 1/2";
    }
    else if (knob_value_2 < 0.6f)
    {
        mlooper_division_2 = 0.25f;
        mlooper_division_string_2 = " 1/4";
    }
    else if (knob_value_2 < 0.8f)
    {
        mlooper_division_2 = 0.125f;
        mlooper_division_string_2 = " 1/8";
    }
    else if (knob_value_2 > 0.8f)
    {
        mlooper_division_2 = 0.0625f;
        mlooper_division_string_2 = "1/16";
    }
};

/**
 * @brief Selects the looper play speed based on the given knob values.
 *
 * The looper play speed is determined as follows:
 *
 * - If the knob value is less than 0.2, the looper play speed is set to -2.
 * - If the knob value is between 0.2 and 0.4, the looper play speed is set to -1.
 * - If the knob value is between 0.4 and 0.6, the looper play speed is set to 0.
 * - If the knob value is between 0.6 and 0.8, the looper play speed is set to 1.
 * - If the knob value is greater than 0.8, the looper play speed is set to 2.
 *
 * @param knob_value_1 Left channel looper play speed knob value.
 * @param knob_value_2 Right channel looper play speed knob value.
 */
void MLooper::SelectLooperPlaySpeed(float knob_value_1, float knob_value_2)
{
    // sets the octave shift
    if (knob_value_1 < 0.2f)
    {
        mlooper_play_speed_1 = 0.25f;
        mlooper_volume_att_1 = 1.0f;
        mlooper_play_speed_string_1 = "-2";
    }
    else if (knob_value_1 < 0.4f)
    {
        mlooper_play_speed_1 = 0.5f;
        mlooper_volume_att_1 = 1.0f;
        mlooper_play_speed_string_1 = "-1";
    }
    else if (knob_value_1 < 0.6f)
    {
        mlooper_play_speed_1 = 1.f;
        mlooper_volume_att_1 = 1.0f;
        mlooper_play_speed_string_1 = " 0";
    }
    else if (knob_value_1 < 0.8f)
    {
        mlooper_play_speed_1 = 2.f;
        mlooper_volume_att_1 = 0.7f;
        mlooper_play_speed_string_1 = "+1";
    }
    else if (knob_value_1 > 0.8f)
    {
        mlooper_play_speed_1 = 4.f;
        mlooper_volume_att_1 = 0.5f;
        mlooper_play_speed_string_1 = "+2";
    }

    if (knob_value_2 < 0.2f)
    {
        mlooper_play_speed_2 = 0.25f;
        mlooper_volume_att_2 = 1.f;
        mlooper_play_speed_string_2 = "-2";
    }
    else if (knob_value_2 < 0.4f)
    {
        mlooper_play_speed_2 = 0.5f;
        mlooper_volume_att_2 = 1.f;
        mlooper_play_speed_string_2 = "-1";
    }
    else if (knob_value_2 < 0.6f)
    {
        mlooper_play_speed_2 = 1.f;
        mlooper_volume_att_2 = 1.f;
        mlooper_play_speed_string_2 = " 0";
    }
    else if (knob_value_2 < 0.8f)
    {
        mlooper_play_speed_2 = 2.f;
        mlooper_volume_att_2 = 0.7f;
        mlooper_play_speed_string_2 = "+1";
    }
    else if (knob_value_2 > 0.8f)
    {
        mlooper_play_speed_2 = 4.f;
        mlooper_volume_att_2 = 0.5f;
        mlooper_play_speed_string_2 = "+2";
    }
};

/**
 * @brief Gets the next sample from the looper effect.
 *
 * @param out1l The left output sample.
 * @param out1r The right output sample.
 * @param in1l The left input sample.
 * @param in1r The right input sample.
 */
float MLooper::GetSampleFromBuffer(float buffer[], float pos)
{
    // linear interpolation that gives back one sample in a certain position in the buffer
    int32_t pos_integral = static_cast<int32_t>(pos);
    float pos_fractional = pos - static_cast<float>(pos_integral);
    float a = buffer[pos_integral % LOOPER_MAX_SIZE];
    float b = buffer[(pos_integral + 1) % LOOPER_MAX_SIZE];
    return a + (b - a) * pos_fractional;
}

/**
 * @brief Applies looper processing to the input audio samples and generates the output samples.
 *
 * The looper effect includes RMS calculation, envelope following, filtering, compression, saturation, and delay.
 *
 * @param out1l The left output sample.
 * @param out1r The right output sample.
 * @param in1l The left input sample.
 * @param in1r The right input sample.
 */
void MLooper::processSample(float &out1l, float &out1r, float in1l, float in1r)
{ // writing the incoming input into the buffer
    out1l = out1r = 0;
    WriteLooperBuffer(in1l, in1r);
    // advance the buffer writing cursor and wrap it if it's longer than the buffer length
    mlooper_writer_pos++;
    mlooper_writer_pos = (mlooper_writer_pos + mlooper_len) % mlooper_len;
    if (mlooper_play)
    {
        if (!mlooper_frozen) // if the looper is not frozen
        {
            // if the buffer is not frozen we get one sample from the looper

            if (mlooper_play)
            {
                // now we change the play_pos according to the number of repetitions and the playing speed
                mlooper_pos_1 = mlooper_pos_1 + mlooper_play_speed_1;
                modified_buffer_length_l = (int)(mlooper_len * mlooper_division_1);
                if (mlooper_pos_1 > modified_buffer_length_l)
                {
                    this->mv.leds.SetForXCycles(0, 10, 1, 0, 0);
                    mlooper_pos_1 = clamp(mlooper_pos_1 - modified_buffer_length_l, 0, mlooper_len);
                }
                else if (mlooper_pos_1 < 0.f)
                {
                    mlooper_pos_1 = clamp(mlooper_pos_1 + modified_buffer_length_l, 0, mlooper_len);
                }

                mlooper_pos_2 = mlooper_pos_2 + mlooper_play_speed_2;
                modified_buffer_length_r = (int)(mlooper_len * mlooper_division_2);
                if (mlooper_pos_2 > modified_buffer_length_r)
                {
                    this->mv.leds.SetForXCycles(3, 10, 1, 0, 0);
                    mlooper_pos_2 = clamp(mlooper_pos_2 - modified_buffer_length_r, 0, mlooper_len);
                }
                else if (mlooper_pos_2 < 0.f)
                {
                    mlooper_pos_2 = clamp(mlooper_pos_2 + modified_buffer_length_r, 0, mlooper_len);
                }

                out1l = GetSampleFromBuffer(IMultiVersioCommon::mlooper_buf_1l, mlooper_pos_1) * mlooper_volume_att_1;
                out1r = GetSampleFromBuffer(IMultiVersioCommon::mlooper_buf_1r, mlooper_pos_2) * mlooper_volume_att_2;
            };
        }
        else // Frozen Buffer
        {    // if the buffer is not frozen we get one sample from the frozen looper

            // out1l = GetSampleFromBuffer(mlooper_frozen_buf_1l,mlooper_frozen_pos_1)*mlooper_volume_att_1;
            // out1r = GetSampleFromBuffer(mlooper_frozen_buf_1r,mlooper_frozen_pos_2)*mlooper_volume_att_2;

            if (mlooper_play)
            {
                // now we change the play_pos according to the number of repetitions and the playing speed
                mlooper_frozen_pos_1 = mlooper_frozen_pos_1 + mlooper_play_speed_1;
                modified_frozen_buffer_length_l = (int)(mlooper_frozen_len * mlooper_division_1);
                if (mlooper_frozen_pos_1 > modified_frozen_buffer_length_l)
                {
                    this->mv.leds.SetForXCycles(0, 10, 0, 0, 1);
                    mlooper_frozen_pos_1 = clamp(mlooper_frozen_pos_1 - modified_frozen_buffer_length_l, 0, mlooper_len);
                }
                else if (mlooper_frozen_pos_1 < 0.f)
                {
                    mlooper_frozen_pos_1 = clamp(mlooper_frozen_pos_1 + modified_frozen_buffer_length_l, 0, mlooper_len);
                }

                mlooper_frozen_pos_2 = mlooper_frozen_pos_2 + mlooper_play_speed_2;
                modified_frozen_buffer_length_r = (int)(mlooper_frozen_len * mlooper_division_2);
                if (mlooper_frozen_pos_2 > modified_frozen_buffer_length_r)
                {
                    this->mv.leds.SetForXCycles(3, 10, 0, 0, 1);
                    mlooper_frozen_pos_2 = clamp(mlooper_frozen_pos_2 - modified_frozen_buffer_length_r, 0, mlooper_len);
                }
                else if (mlooper_frozen_pos_2 < 0.f)
                {
                    mlooper_frozen_pos_2 = clamp(mlooper_frozen_pos_2 + modified_frozen_buffer_length_r, 0, mlooper_len);
                }

                out1l = GetSampleFromBuffer(this->mv.mlooper_frozen_buf_1l, mlooper_frozen_pos_1) * mlooper_volume_att_1;
                out1r = GetSampleFromBuffer(this->mv.mlooper_frozen_buf_1r, mlooper_frozen_pos_2) * mlooper_volume_att_2;
            }
        };
    }
    // advance the counter for calculating the length of the next incoming buffer
    mlooper_len_count++;

    mlooper_writer_outside_pos++;
    mlooper_writer_outside_pos = ((mlooper_writer_outside_pos + mlooper_len_count) % (mlooper_len_count)) % LOOPER_MAX_SIZE;

    // automatic looptime
    if (mlooper_len >= LOOPER_MAX_SIZE)
    {
        mlooper_len = LOOPER_MAX_SIZE - 1;
    }

    if (mlooper_drywet > 0.98f)
    {
        mlooper_drywet = 1.f;
    }
    out1l = sqrt(0.5f * (mlooper_drywet * 2.0f)) * out1l + sqrt(0.95f * (2.f - (mlooper_drywet * 2))) * in1l;
    out1r = sqrt(0.5f * (mlooper_drywet * 2.0f)) * out1r + sqrt(0.95f * (2.f - (mlooper_drywet * 2))) * in1r;
};

/**
 * @brief Runs the looper effect with the specified parameters.
 *
 * @param blend The blend parameter for the effect.
 * @param regen The regen parameter for the effect.
 * @param tone The tone parameter for the effect.
 * @param speed The speed parameter for the effect.
 * @param size The size parameter for the effect.
 * @param index The index parameter for the effect.
 * @param dense The dense parameter for the effect.
 * @param FSU The FSU parameter for the effect.
 */
void MLooper::run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU)
{
    // blend = looper division left
    // tone =
    // speed = looper octave left
    // index = buffer freeze
    // regen = looper division right
    // size = looper octave right
    // dense = dry/wet
    // FSU = clock

    // TONE = Amount of the two micro loopers
    // INDEX = AMOUNT OF RANDOM
    // DENSE = DRY WET //implement

    if (this->mv.versio.gate.Trig())
    {
        mlooper_len = mlooper_len_count % LOOPER_MAX_SIZE;
        mlooper_len_count = 0;
        mlooper_play = true;
        mlooper_pos_1 = (mlooper_writer_pos - mlooper_len);
        mlooper_pos_2 = (mlooper_writer_pos - mlooper_len);
        this->mv.leds.SetForXCycles(1, 10, 1, 1, 1);
        this->mv.leds.SetForXCycles(2, 10, 1, 1, 1);
    };

    if (index > 0.5f)
    {
        if (mlooper_frozen == false)
        {
            mlooper_frozen = true;
            FreezeLooperBuffer();
        }
    }
    else
    {
        mlooper_frozen = false;
    }

    SelectLooperDivision(blend, regen);
    SelectLooperPlaySpeed(speed, size);

    mlooper_drywet = dense * 1.01;
}