#pragma once

/**
 * @brief The interface for an effect.
 *
 * This interface defines the methods that an effect class must implement.
 * The methods allow controlling the effect parameters and processing audio samples.
 */
class IEffect
{
public:
    /**
     * @brief Runs the effect with the specified parameters.
     *
     * @param blend The blend parameter.
     * @param regen The regen parameter.
     * @param tone The tone parameter.
     * @param speed The speed parameter.
     * @param size The size parameter.
     * @param index The index parameter.
     * @param dense The dense parameter.
     * @param FSU The FSU parameter.
     */
    virtual void run(float blend, float regen, float tone, float speed, float size, float index, float dense, int FSU) = 0;

    /**
     * @brief Processes the input audio samples and produces the output samples.
     *
     * @param outl The left output sample.
     * @param outr The right output sample.
     * @param inl The left input sample.
     * @param inr The right input sample.
     */
    virtual void getSample(float &outl, float &outr, float inl, float inr) = 0;
};
