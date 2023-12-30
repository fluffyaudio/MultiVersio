#include "helpers.h"
#include <math.h>

/**
 * Clamps a value between a minimum and maximum range.
 *
 * @param value The value to be clamped.
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @return The clamped value.
 */
float clamp(float value, float min, float max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

/**
 * Maps a value from one range to another.
 *
 * @param value The value to be mapped.
 * @param start1 The start of the value's current range.
 * @param stop1 The end of the value's current range.
 * @param start2 The start of the value's target range.
 * @param stop2 The end of the value's target range.
 * @return The mapped value.
 */
float map(float value, float start1, float stop1, float start2, float stop2)
{
    return start2 + ((stop2 - start2) * (value - start1)) / (stop1 - start1);
};

/**
 * Left rotates an array by one position.
 *
 * @param arr The array to be rotated.
 * @param n The size of the array.
 */
void leftRotatebyOne(float arr[], int n)
{
    float temp = arr[0];
    for (int i = 0; i < n - 1; i++)
        arr[i] = arr[i + 1];
    arr[n - 1] = temp;
}

/**
 * Right rotates an array by one position.
 *
 * @param arr The array to be rotated.
 * @param n The size of the array.
 */
void rightRotatebyOne(float arr[], int n)
{
    float temp = arr[n - 1];
    for (int i = n - 1; i > 0; i--)
        arr[i] = arr[i - 1];
    arr[0] = temp;
}

/**
 * Left rotates an array by a given amount.
 *
 * @param arr The array to be rotated.
 * @param amount The amount of rotation.
 * @param n The size of the array.
 */
void leftRotate(float arr[], float amount, int n)
{
    for (int i = 0; i < amount * n; i++)
        leftRotatebyOne(arr, n);
}

/**
 * Right rotates an array by a given amount.
 *
 * @param arr The array to be rotated.
 * @param amount The amount of rotation.
 * @param n The size of the array.
 */
void rightRotate(float arr[], float amount, int n)
{
    for (int i = 0; i < amount * n; i++)
        rightRotatebyOne(arr, n);
}

/**
 * Finds the element in the array that is closest to the target value.
 *
 * @param arr The array to search in.
 * @param filter The filter array indicating which elements to consider.
 * @param n The size of the array.
 * @param target The target value.
 * @param offset The offset value.
 * @return The element closest to the target value.
 */
int findClosest(int arr[], bool filter[], int n, int target, int offset)
{
    int lower = 0;
    int higher = n;
    int reverse_counter = 0;
    for (int i = 0; i < n; i++)
    {
        if ((arr[i] < target) & filter[(i + offset) % 12])
        {
            lower = arr[i];
        }
        reverse_counter = (n - 1) - i;
        if ((arr[reverse_counter] > target) & filter[(reverse_counter + offset) % 12])
        {
            higher = arr[reverse_counter];
        }
    }
    return getClosest(lower, higher, target);
};

/**
 * Compares two values and returns the one closest to the target value.
 *
 * @param val1 The first value.
 * @param val2 The second value.
 * @param target The target value.
 * @return The value closest to the target value.
 */
int getClosest(int val1, int val2, int target)
{
    if (target - val1 >= val2 - target)
        return val2;
    else
        return val1;
}

/**
 * Swaps the values of two variables.
 *
 * @param xp Pointer to the first variable.
 * @param yp Pointer to the second variable.
 */
void swap(float *xp, float *yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

/**
 * Sorts an array in descending order using the bubble sort algorithm.
 *
 * @param arr The array to be sorted.
 * @param arr2 Another array to be sorted along with the first array.
 * @param n The size of the arrays.
 */
void bubbleSort(float arr[], float arr2[], int n)
{
    int i, j;
    for (i = 0; i < n - 1; i++)
        for (j = 0; j < n - i - 1; j++)
            if (arr[j] < arr[j + 1])
            {
                swap(&arr[j], &arr[j + 1]);
                swap(&arr2[j], &arr2[j + 1]);
            }
}

/**
 * Applies a window function to a value at a given position in an array.
 *
 * @param i The value to be multiplied by the window function.
 * @param pos The position of the value in the array.
 * @param FFT_SIZE The size of the array.
 * @return The value multiplied by the window function.
 */
float ApplyWindow(float i, size_t pos, size_t FFT_SIZE)
{
    float multiplier = 0.5 * (1 - cos(2 * M_PI * pos / (FFT_SIZE - 1)));
    return i * multiplier;
}

/**
 * Generates a random float value between 0 and 1.
 *
 * @return The random float value.
 */
float randomFloat()
{
    int randomNumber = rand() % 10000;
    return randomNumber / 10000.f;
}