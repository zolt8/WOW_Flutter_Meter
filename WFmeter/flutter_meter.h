#ifndef FLUTTER_METER_H
#define FLUTTER_METER_H

#define DLL_EXPORT __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the flutter meter processing module.
 *
 * This function configures the internal processing state, sets the input
 * sample rate, and defines the test signal's expected nominal frequency.
 * It must be called before any samples are processed.
 *
 * @param sample_rate     Input signal sample rate in Hz.
 * @param test_frequency  Expected test tone frequency in Hz.
 */
DLL_EXPORT void flutterMeter_init(int sample_rate, double test_frequency);

/**
 * @brief Processes a block of audio samples using the selected filter type.
 *
 * This function applies the flutter measurement algorithm to the provided
 * samples. It may be called repeatedly with consecutive blocks until the
 * entire signal is processed.
 *
 * @param samples      Pointer to an array of input samples (double-precision).
 * @param num_samples  Number of samples in the provided buffer.
 * @param filter_type  Filter selector (e.g., DIN, JIS, or unweighted), depending
 *                     on the implementation's enumeration or definition.
 */
DLL_EXPORT int process_samples(const int* samples, int num_samples, int filter_type);

/**
 * @brief Retrieves the computed flutter results.
 *
 * After processing the desired amount of audio, this function returns
 * the peak flutter value, RMS flutter value, and measured frequency deviation.
 *
 * @param peak   Pointer to a double that receives the peak flutter value.
 * @param rms    Pointer to a double that receives the RMS flutter value.
 * @param freq   Pointer to a double that receives the measured frequency (Hz).
 */
DLL_EXPORT void get_results(double* peak, double* rms, double* freq);

#ifdef __cplusplus
}
#endif

#endif
