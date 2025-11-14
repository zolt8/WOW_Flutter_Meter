/**
 * @file flutter_meter.c
 * @brief Wow and flutter measurement implementation for audio signals
 *
 * This module measures speed variations (wow and flutter) in audio playback
 * by analyzing zero-crossings of a test tone (typically 3150 Hz).
 *
 * Measurement methodology:
 * - Tracks timing between zero-crossings of filtered signal
 * - Calculates deviation from expected interval
 * - Applies weighting filters (DIN, wow, flutter, or unweighted)
 * - Computes RMS and quasi-peak values over 10-second windows
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DLL_EXPORT __declspec(dllexport)

// Filter function declarations
void reset_filters();
double process_2nd_order(register double val);
double process_DIN(register double val);
double process_unweighted(register double val);
double process_wow(register double val);
double process_flutter(register double val);

// ============================================================================
// STATE VARIABLES - Zero-crossing interval tracking
// ============================================================================

/** Previous sample value for zero-crossing detection */
static int previous_sample = 0;

/** Current interval duration in nanoseconds between zero-crossings */
static double current_interval_ns = 0;

/** Fractional remainder from last zero-crossing interpolation */
static double interval_remainder_ns = 0;

/** Time between samples in nanoseconds */
static double nanoseconds_per_sample = 0;

// ============================================================================
// CONFIGURATION VARIABLES - Test signal parameters
// ============================================================================

/** Expected half-period of test frequency in nanoseconds */
static double expected_half_period_ns = 0.5 * 1.0e9 / 3150;

/** Center frequency of test signal (Hz) */
static int test_frequency_hz = 3150;

/** Minimum acceptable zero-crossings per 100ms window */
static int min_zero_crossings = 0;

/** Maximum acceptable zero-crossings per 100ms window */
static int max_zero_crossings = 0;

/** Number of samples in a 100ms window */
static int samples_per_100ms = 0;

// ============================================================================
// FILTER STATE - Intermediate processing values
// ============================================================================

/** Input value to 2nd order filter */
static double filter_input = 0;

/** Output value from 2nd order filter */
static double filter_output = 0;

// ============================================================================
// ACCUMULATION STATE - Statistical computation
// ============================================================================

/** Flag indicating first buffer, used to skip initial unstable values */
static int is_first_buffer;

/** Count of valid samples processed */
static int valid_sample_count;

/** Sum of all interval durations */
static double interval_sum_ns;

/** Average interval duration */
static double average_interval_ns;

/** Measured frequency from interval analysis */
static double measured_frequency_hz;

// ============================================================================
// BUFFER MANAGEMENT - 10-second window tracking
// ============================================================================

/** Current buffer index (0-9) for 1-second windows */
static int rms_1sec_buffer_index;

/** RMS sum of squares for each 1-second buffer */
static double buffer_rms_1sec_sums[10];

/** Peak values for each 100ms window (50 per 5 seconds) */
static double peak_values_100ms[50];

/** Index into peak_values_100ms array */
static int peak_index_100ms = 0;

/** Maximum RMS values for 5-second window tracking */
static double max_rms_array[50];

/** Current quasi-peak value (persistent across windows) */
static double current_quasi_peak = 0.0;

// ============================================================================
// RESULTS - Output values
// ============================================================================

/** Maximum RMS value in 10-second window (percentage) */
static double result_rms_percent = 0;

/** Maximum quasi-peak value in 10-second window */
static double result_quasi_peak = 0;

/** Measured center frequency (Hz) */
static double result_frequency_hz = 0;

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

/**
 * @brief Initialize the flutter meter with specified parameters
 *
 * @param sample_rate Sample rate in Hz (e.g., 48000)
 * @param test_frequency Expected test tone frequency in Hz (typically 3150)
 */
DLL_EXPORT void flutterMeter_init(int sample_rate, double test_frequency)
{
    // Reset output results
    result_rms_percent = 0;
    result_quasi_peak = 0;
    result_frequency_hz = 0;

    // Initialize signal processing filters
    reset_filters();

    // Configure test signal parameters
    test_frequency_hz = test_frequency;
    expected_half_period_ns = 0.5 * 1.0e9 / test_frequency;

    // Set acceptable zero-crossing range (±5% of expected count)
    // In 100ms at test_frequency Hz, expect (test_frequency / 5) crossings
    min_zero_crossings = test_frequency_hz / 5 * 0.95;
    max_zero_crossings = test_frequency_hz / 5 * 1.05;

    // Calculate samples per measurement window
    samples_per_100ms = sample_rate / 10;
    nanoseconds_per_sample = 1.0e9 / sample_rate;

    // Initialize state variables
    is_first_buffer = 1;
    rms_1sec_buffer_index = 0;
    interval_sum_ns = 0.0;
    average_interval_ns = 0.0;
    current_interval_ns = 0;
    interval_remainder_ns = 0;
    previous_sample = 0;
    current_quasi_peak = 0.0;

    // Clear buffer arrays
    for (int i = 0; i < 10; i++)
    {
        buffer_rms_1sec_sums[i] = 0.0;
    }

    for (int i = 0; i < 50; i++)
    {
        peak_values_100ms[i] = 0.0;
        max_rms_array[i] = 0.0;
    }

    peak_index_100ms = 0;
}

/**
 * @brief Process audio samples and compute wow/flutter measurements
 *
 * Processes 10 seconds of audio (100 x 100ms windows) and updates results.
 *
 * @param samples Pointer to array of audio samples (16-bit integer values)
 * @param num_samples Total number of samples in array
 * @param filter_type Filter type: 0=Unweighted, 1=DIN, 2=Wow, 3=Flutter
 * @return 0 on success, -1 if insufficient samples
 */
DLL_EXPORT int process_samples(const int *samples, int num_samples,
        int filter_type)
{
    static short previous_sample_raw = 0;
    double freq_sum_5sec = 0.0;
    int freq_count_5sec = 0;

    // Verify we have enough samples for 10 seconds of processing
    if (num_samples < samples_per_100ms * 100)
    {
        return -1; // Not enough samples
    }

    // Process 100 windows of 100ms each (10 seconds total)
    for (int window_100ms = 0; window_100ms < 100; window_100ms++)
    {
        double sum_of_squares = 0.0;
        int max_amplitude = 0;
        int zero_crossing_count = 0;

        // First pass: Validate signal quality
        // Check amplitude level and zero-crossing rate
        for (int i = 0; i < samples_per_100ms; i++)
        {
            short sample = samples[i];

            // Track maximum amplitude
            if (sample > max_amplitude)
            {
                max_amplitude = sample;
            }

            // Count zero-crossings
            if (((sample >= 0) && (previous_sample_raw < 0))
                    || ((sample < 0) && (previous_sample_raw >= 0)))
            {
                zero_crossing_count++;
            }
            previous_sample_raw = sample;
        }

        // Skip if signal is too weak (below threshold)
        if (max_amplitude < 50)
        {
            samples += samples_per_100ms;
            continue;
        }

        // Skip if frequency is out of acceptable range
        if ((zero_crossing_count < min_zero_crossings)
                || (zero_crossing_count > max_zero_crossings))
        {
            samples += samples_per_100ms;
            continue;
        }

        // Second pass: Process samples for wow/flutter measurement
        double max_quasi_peak = 0.0;

        for (int i = 0; i < samples_per_100ms; i++)
        {
            short sample = samples[i];

            // Apply 2nd order bandpass filter
            filter_input = sample;
            filter_output = process_2nd_order(filter_input);

            int current_sample_value = (int) (filter_output);
            int is_zero_crossing = 0;

            // Detect zero-crossing with linear interpolation
            if (((current_sample_value > 0) && (previous_sample < 0))
                    || ((current_sample_value < 0)
                            && (previous_sample > 0)))
            {
                // Interpolate exact zero-crossing time
                double denom = current_sample_value - previous_sample;
                //Prevent divide by zero in case of same samples.
                if (fabs(denom) < 1e-9)
                {
                    denom = (denom >= 0 ? 1e-9 : -1e-9);
                }

                double crossing_offset_ns = -previous_sample * nanoseconds_per_sample / denom;
                current_interval_ns += crossing_offset_ns;
                interval_remainder_ns = nanoseconds_per_sample - crossing_offset_ns;
                is_zero_crossing = 1;
            }
            else
            {
                current_interval_ns += nanoseconds_per_sample;
            }

            // Handle exact zero case
            if (current_sample_value == 0)
            {
                interval_remainder_ns = 0;
                is_zero_crossing = 1;
            }

            previous_sample = current_sample_value;

            // Process zero-crossing event
            if (is_zero_crossing)
            {
                // Skip first buffer to allow filters to stabilize
                if (is_first_buffer)
                {
                    valid_sample_count = 0;
                    is_first_buffer = 0;
                    continue;
                }

                // Calculate timing error as percentage deviation
                double timing_error_percent = (expected_half_period_ns
                        - current_interval_ns) / expected_half_period_ns;

                // Apply selected weighting filter
                switch (filter_type)
                {
                    case 0: // Unweighted
                        timing_error_percent = process_unweighted(
                                timing_error_percent);
                    break;
                    case 1: // DIN weighting
                        timing_error_percent = process_DIN(
                                timing_error_percent);
                    break;
                    case 2: // Wow filter (low frequency)
                        timing_error_percent = process_wow(
                                timing_error_percent);
                    break;
                    case 3: // Flutter filter (high frequency)
                        timing_error_percent = process_flutter(
                                timing_error_percent);
                    break;
                    default:
                        timing_error_percent = process_unweighted(
                                timing_error_percent);
                    break;
                }

                // Convert to measurement units (empirical calibration)
                double measurement_value = fabs(timing_error_percent) * 10000 / 85;

                // Update quasi-peak detector with different attack/decay times
                if (measurement_value > current_quasi_peak)
                    current_quasi_peak += (measurement_value
                            - current_quasi_peak) / 500; // Fast attack
                else
                    current_quasi_peak += (measurement_value
                            - current_quasi_peak) / 6000; // Slow decay

                max_quasi_peak = current_quasi_peak;

                // Accumulate for RMS calculation
                sum_of_squares += timing_error_percent * timing_error_percent;
                valid_sample_count++;

                // Accumulate interval for frequency measurement
                interval_sum_ns += (double) current_interval_ns;
                current_interval_ns = interval_remainder_ns;

                // Calculate average frequency
                average_interval_ns = interval_sum_ns
                        / (double) valid_sample_count;
                measured_frequency_hz = 1000000000 / average_interval_ns / 2;
                freq_sum_5sec += measured_frequency_hz;
                freq_count_5sec++;
            }
        }

        // Move to next 100ms window
        samples += samples_per_100ms;

        // Store results for this 100ms window
        buffer_rms_1sec_sums[rms_1sec_buffer_index] = sum_of_squares;
        peak_values_100ms[peak_index_100ms] = max_quasi_peak;

        // Wrap peak index at 50 (5 seconds)
        if (++peak_index_100ms == 50) peak_index_100ms = 0;

        // Process complete 1-second buffer (10 x 100ms)
        if (++rms_1sec_buffer_index == 10)
        {
            // Calculate RMS over the 1-second window
            double total_sum_of_squares = 0.0;
            for (int i = 0; i < 10; i++)
            {
                total_sum_of_squares += buffer_rms_1sec_sums[i];
            }

            // Store RMS result in the appropriate array slot
            // Using static array to track max RMS values
            max_rms_array[peak_index_100ms] = sqrt(total_sum_of_squares / valid_sample_count) * 100;

            // Find maximum RMS and peak values in 5-second window
            double max_rms_10sec = 0.0;
            double max_peak_10sec = 0.0;

            // Scan through 5-second history (50 x 100ms windows)
            for (int i = 0; i < 50; i++)
            {
                // Track maximum RMS
                if (max_rms_array[i] > max_rms_10sec)
                {
                    max_rms_10sec = max_rms_array[i];
                }

                // Track maximum peak
                if (peak_values_100ms[i] > max_peak_10sec)
                {
                    max_peak_10sec = peak_values_100ms[i];
                }
            }

            // Update output results
            result_rms_percent = max_rms_10sec;
            result_quasi_peak = max_peak_10sec;
            if (freq_count_5sec > 0)
            {
                result_frequency_hz = freq_sum_5sec / freq_count_5sec;
            }

            // Reset for next measurement cycle
            valid_sample_count = 0;
            rms_1sec_buffer_index = 0;
            interval_sum_ns = 0.0;
        }
    }

    return 0;
}

/**
 * @brief Retrieve the latest measurement results
 *
 * @param[out] peak Pointer to store quasi-peak value
 * @param[out] rms Pointer to store RMS value (percentage)
 * @param[out] freq Pointer to store measured frequency (Hz)
 */
DLL_EXPORT void get_results(double *peak, double *rms, double *freq)
{
    *peak = result_quasi_peak;
    *rms = result_rms_percent;
    *freq = result_frequency_hz;
}
