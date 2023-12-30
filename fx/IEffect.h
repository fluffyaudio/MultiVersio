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
    virtual void processSample(float &outl, float &outr, float inl, float inr) = 0;

    /**
     * @brief Indicates whether the effect uses reverb.
     *
     * Some effects also use the reverb effect. This method indicates whether the effect uses reverb or not.
     *
     * The base implementation returns false.
     *
     * @return True if the effect uses reverb, false otherwise.
     */
    bool usesReverb()
    {
        return false;
    }

    /**
     * @brief Processes the input samples before the effect is run.
     *
     * This method is called before the effect is run. It can be used to process the input samples before the effect is run.
     *
     * The base implementation does nothing.
     *
     * @param in1 The left input sample.
     * @param in2 The right input sample.
     * @param size The number of samples.
     */
    void preProcess(const float *in1, const float *in2, size_t size)
    {
    }

    /**
     * @brief Processes the output samples after the effect is run.
     *
     * This method is called after the effect is run. It can be used to process the output samples after the effect is run.
     *
     * The base implementation does nothing.
     *
     * @param outl The left output samples
     * @param outr The right output samples
     * @param inl The left input samples
     * @param inr The right input samples
     * @param size The number of samples.
     */
    void postProcess(float outl[], float outr[], const float inl[], const float inr[], size_t size)
    {
    }
};
