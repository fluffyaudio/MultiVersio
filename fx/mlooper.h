#pragma once
#include "daisy_patch.h"
#include "daisysp.h"

#include <string>
#include "IEffect.h"
#include "IMultiVersioCommon.h"

/**
 * @brief Implements a looper effect.
 */
class MLooper : public IEffect
{
public:
    size_t mlooper_len = LOOPER_MAX_SIZE - 1;
    int mlooper_len_count = 0;

    float mlooper_drywet = 0.f;

    float mlooper_frozen_pos_1 = 0;
    float mlooper_frozen_pos_2 = 0;

    bool mlooper_play = false; // currently playing

    float mlooper_pos_1 = 0;
    float mlooper_pos_2 = 0;

    int modified_buffer_length_l, modified_buffer_length_r;
    int modified_frozen_buffer_length_l, modified_frozen_buffer_length_r;

    int mlooper_frozen_len = LOOPER_MAX_SIZE - 1;

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

    IMultiVersioCommon &mv;

    MLooper(IMultiVersioCommon &mv);
    void ResetLooperBuffer();
    void WriteLooperBuffer(float in_1l, float in_1r);
    void FreezeLooperBuffer();
    void SelectLooperDivision(float knob_value_1, float knob_value_2);
    void SelectLooperPlaySpeed(float knob_value_1, float knob_value_2);
    float GetSampleFromBuffer(float buffer[], float pos);
    void processSample(float &out1l, float &out1r, float in1l, float in1r);
    void run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU);
    bool usesReverb();
};
