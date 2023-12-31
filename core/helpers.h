#pragma once

#include <cstddef>

#define RMS_SIZE 48
#define FFT_LENGTH 1024
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f)         // 2.5 seconds max delay in the fast ram
#define LOOPER_MAX_SIZE static_cast<size_t>(48000 * 60 * 1) // 1 minutes stereo of floats at 48 khz
#define MAX_SPECTRA_FREQUENCIES 6

/**
 * @brief The Averager class calculates the root mean square (RMS) value of a set of samples.
 *
 * The Averager class maintains a buffer of samples and provides methods to add samples, calculate the RMS value,
 * and clear the buffer.
 */
class Averager
{
    float buffer[RMS_SIZE]; /**< The buffer to store the samples. */
    int cursor;             /**< The current position in the buffer. */

public:
    /**
     * @brief Constructs an Averager object and clears the buffer.
     */
    Averager()
    {
        Clear();
    }

    /**
     * @brief Calculates the root mean square (RMS) of the samples in the buffer.
     * @return The RMS value.
     */
    float ProcessRMS()
    {
        float sum = 0.f;
        for (int i = 0; i < cursor; i++)
        {
            sum = sum + buffer[i];
        }
        float result = sqrt(sum / cursor);
        Clear();
        return result;
    }

    /**
     * @brief Clears the buffer by setting all elements to 0.
     */
    void Clear()
    {
        for (int i = 0; i < RMS_SIZE; i++)
        {
            buffer[i] = 0.f;
        }
        cursor = 0;
    }

    /**
     * @brief Adds a sample to the buffer.
     * @param sample The sample to be added.
     */
    void Add(float sample)
    {
        buffer[cursor] = sample;
        cursor++;
    }
};

float clamp(float value, float min, float max);
float map(float value, float start1, float stop1, float start2, float stop2);
void leftRotatebyOne(float arr[], int n);
void rightRotatebyOne(float arr[], int n);
void leftRotate(float arr[], float amount, int n);
void rightRotate(float arr[], float amount, int n);
int findClosest(int arr[], bool filter[], int n, int target, int offset);
int getClosest(int val1, int val2, int target);
void swap(float *xp, float *yp);
void bubbleSort(float arr[], float arr2[], int n);
float ApplyWindow(float i, size_t pos, size_t FFT_SIZE);
float randomFloat();