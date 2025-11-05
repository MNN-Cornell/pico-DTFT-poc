#ifndef DTFT_H
#define DTFT_H

#include <stdint.h>
#include <stdbool.h>

// Dual-core DTFT computation state
typedef struct {
    uint8_t *signal;
    int signal_len;
    int start_freq;
    int end_freq;
    int num_points;
    float *output;
    volatile bool done;
} core1_dtft_params_t;

/**
 * Initialize Core1 DTFT worker
 * Must be called once at startup
 */
void init_core1_dtft(void);

/**
 * Compute DTFT magnitude at a specific frequency (optimized)
 * @param x Input signal array
 * @param N Length of signal
 * @param omega Normalized frequency (radians/sample, 0 to 2*pi)
 * @return Magnitude of DTFT at that frequency
 */
float compute_dtft_magnitude(uint8_t * restrict x, int N, float omega);

/**
 * Compute DTFT magnitudes for a range of frequencies (single-core)
 * @param x Input signal array
 * @param N Length of signal
 * @param num_points Number of frequency points to compute
 * @return Array of magnitudes (caller must free)
 */
float* calculate_dtft(uint8_t * restrict x, int N, int num_points);

/**
 * Compute DTFT with both magnitude and phase (dual-core optimized version)
 * @param x Input signal array
 * @param N Length of signal
 * @param num_points Number of frequency points to compute
 * @return Array of complex values [real1, imag1, real2, imag2, ...] (caller must free)
 */
float* calculate_dtft_complex(uint8_t * restrict x, int N, int num_points);

#endif // DTFT_H
