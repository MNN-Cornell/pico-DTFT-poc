#include "signal.h"
#include "dtft.h"
#include "output.h"
#include "pico/time.h"
#include "lib/dtft_lookup_n10.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include "pico/stdlib.h"

// ARM Cortex-M33 DWT (Data Watchpoint and Trace) cycle counter
// Provides accurate CPU cycle counting for performance measurement
#define DWT_CTRL    (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004)
#define DEM_CR      (*(volatile uint32_t *)0xE000EDFC)

/**
 * Initialize the DWT cycle counter
 * Call this once at startup before measuring cycles
 */
void init_cycle_counter(void) {
    DEM_CR |= 0x01000000;  // Enable DWT
    DWT_CTRL |= 0x00000001;  // Enable cycle counter
}

/**
 * Get current cycle count from DWT
 * @return Current CPU cycle count
 */
static inline uint32_t get_cycle_count(void) {
    return DWT_CYCCNT;
}

// DEBUG control: set to 1 for verbose logging/plotting, 0 for performance runs
#ifndef DEBUG
#define DEBUG 1
#endif

// Disable printf overhead in non-DEBUG builds
#if !DEBUG
#undef printf
#define printf(...) ((void)0)

static inline void perf_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
#else
#define perf_printf printf
#endif

/**
 * Calculate squared Euclidean distance between two magnitude spectrums
 * (sqrt is omitted since we only need to compare distances, not compute actual values)
 * @param spectrum1 First spectrum (computed DTFT magnitudes)
 * @param spectrum2 Second spectrum (lookup table magnitudes)
 * @param num_points Number of frequency points
 * @return Squared Euclidean distance
 */
float calculate_euclidean_distance(const float *spectrum1, const float *spectrum2, int num_points) {
    float sum = 0.0f;
    for (int i = 0; i < num_points; i++) {
        float diff = spectrum1[i] - spectrum2[i];
        sum += diff * diff;
    }
    return sum;  // Return squared distance (sqrt is unnecessary for comparisons)
}

/**
 * Reconstruct pixel value from DTFT spectrum using Euclidean distance
 * @param computed_magnitudes Computed DTFT magnitudes (41 points)
 * @param num_frequencies Number of frequency points (should be 41)
 * @return Best matching pixel value (0-255)
 */
uint8_t reconstruct_pixel_value(const float *computed_magnitudes, int num_frequencies) {
    if (num_frequencies != 41) {
        printf("Warning: Expected 41 frequency points, got %d\n", num_frequencies);
        return 0;
    }
    
    float min_distance = INFINITY;
    uint8_t best_match = 0;
    
    // Compare with all 256 lookup table entries
    for (int value = 0; value < 256; value++) {
        // Extract magnitudes from lookup table
        float lookup_magnitudes[41];
        for (int freq = 0; freq < 41; freq++) {
            lookup_magnitudes[freq] = dtft_lookup_n10[value][freq].magnitude;
        }
        
        // Calculate Euclidean distance
        float distance = calculate_euclidean_distance(computed_magnitudes, lookup_magnitudes, 41);
        
        // Update best match
        if (distance < min_distance) {
            min_distance = distance;
            best_match = value;
        }
    }
    
    perf_printf("\n=== Pixel Value Reconstruction (Euclidean Distance) ===\n");
    perf_printf("Best match: 0x%02X (0b", best_match);
    for (int i = 7; i >= 0; i--) {
        perf_printf("%d", (best_match >> i) & 1);
    }
    perf_printf(", decimal: %d)\n", best_match);
    perf_printf("Minimum distance: %.6f\n", min_distance);
    
    // Show top 5 matches for debugging
    perf_printf("\nTop 5 matches:\n");
    typedef struct {
        uint8_t value;
        float distance;
    } Match;
    Match matches[256];
    
    for (int value = 0; value < 256; value++) {
        float lookup_magnitudes[41];
        for (int freq = 0; freq < 41; freq++) {
            lookup_magnitudes[freq] = dtft_lookup_n10[value][freq].magnitude;
        }
        matches[value].value = value;
        matches[value].distance = calculate_euclidean_distance(computed_magnitudes, lookup_magnitudes, 41);
    }
    
    // Simple bubble sort for top 5 (sufficient for this purpose)
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 256; j++) {
            if (matches[j].distance < matches[i].distance) {
                Match temp = matches[i];
                matches[i] = matches[j];
                matches[j] = temp;
            }
        }
    }
    
    for (int i = 0; i < 5; i++) {
        perf_printf("  %d. 0x%02X (0b", i + 1, matches[i].value);
        for (int b = 7; b >= 0; b--) {
            perf_printf("%d", (matches[i].value >> b) & 1);
        }
        perf_printf(", decimal: %3d) - distance: %.6f\n", matches[i].value, matches[i].distance);
    }
    perf_printf("===================================================\n\n");
    
    return best_match;
}

uint8_t* repeat_pattern(uint8_t *pattern, int repetitions) {
    int pattern_len = pattern[0];  // Extract length from first element
    int total_len = pattern_len * repetitions;
    uint8_t *buffer = malloc(total_len * sizeof(uint8_t));
    
    for (int rep = 0; rep < repetitions; rep++) {
        for (int i = 0; i < pattern_len; i++) {
            buffer[rep * pattern_len + i] = pattern[i + 1];  // Skip first element (length)
        }
    }
    
    return buffer;
}

void process_pattern(uint8_t *bits_sent) {
    if (!bits_sent) return;
    
    // Extract pattern length and show the received bits before repetition
    int pattern_len = bits_sent[0];
#if DEBUG
    printf("Received pattern: ");
    for (int i = 1; i <= pattern_len; i++) {
        printf("%d", bits_sent[i]);
    }
    printf("\n");
#endif

    // Repeat the pattern 10 times for DTFT analysis
    uint8_t *signal_buffer = repeat_pattern(bits_sent, 10);
    
    // Get total buffer size
    int total_len = pattern_len * 10;
    
    // Print the signal buffer for verification
    printf("Signal buffer (pattern repeated 10x): ");
    for (int i = 0; i < total_len; i++) {
        printf("%d", signal_buffer[i]);
    }
    printf("\n");
    
    // Compute DTFT with 41 frequency points from 0 to π to match lookup table
    // Note: For real-valued signals, DTFT is symmetric around π (conjugate symmetry)
    // Therefore we only need to compute 0 to π; the π to 2π range would be redundant
    absolute_time_t start_time = get_absolute_time();
    uint32_t start_cycles = get_cycle_count();
    
    float *complex_values = malloc(41 * 2 * sizeof(float));
    if (!complex_values) {
        free(signal_buffer);
        return;
    }
    
    // Compute DTFT for frequencies 0 to π ONLY (41 points with spacing π/40)
    // This matches the lookup table generation exactly
    for (int k = 0; k < 41; k++) {
        float omega = (M_PI * k) / 40.0f;  // Frequency: 0, π/40, 2π/40, ..., 40π/40=π
        float real_part = 0.0f;
        float imag_part = 0.0f;
        
        for (int n = 0; n < total_len; n++) {
            float angle = -omega * n;
            real_part += signal_buffer[n] * cosf(angle);
            imag_part += signal_buffer[n] * sinf(angle);
        }
        
        complex_values[2*k] = real_part;
        complex_values[2*k + 1] = imag_part;
    }
    
    absolute_time_t end_time = get_absolute_time();
    uint32_t end_cycles = get_cycle_count();
    int64_t execution_time = absolute_time_diff_us(start_time, end_time);
    uint32_t total_cycles = end_cycles - start_cycles;
    perf_printf("%d bits data: DTFT calculation took %lld microseconds (%.2f ms) | %u cycles\n", pattern_len, execution_time, execution_time / 1000.0f, total_cycles);
    
    // Print complex DTFT values for MATLAB
    if (complex_values) {
#if DEBUG
        print_dtft_complex_for_matlab(complex_values, 41);
#endif
        
        // Compute squared magnitudes from complex values
        // (no sqrt needed - lookup table now stores squared magnitudes)
        float *magnitudes = malloc(41 * sizeof(float));
        if (magnitudes) {
            for (int k = 0; k < 41; k++) {
                float real = complex_values[2*k];
                float imag = complex_values[2*k + 1];
                magnitudes[k] = real * real + imag * imag;  // Squared magnitude
            }
            
            // Print first few magnitudes for debugging
            printf("\nComputed spectrum (first 10 magnitudes):\n");
            for (int i = 0; i < 10 && i < 41; i++) {
                printf("  freq[%d]: %.6f\n", i, magnitudes[i]);
            }
            
            // Print lookup table values for 0x4C for comparison
            printf("\nLookup table 0x4C (first 10 magnitudes):\n");
            for (int i = 0; i < 10; i++) {
                printf("  freq[%d]: %.6f\n", i, dtft_lookup_n10[0x4C][i].magnitude);
            }
            
            // Reconstruct pixel value using Euclidean distance
            absolute_time_t recon_start_time = get_absolute_time();
            uint32_t recon_start_cycles = get_cycle_count();
            
            uint8_t reconstructed_value = reconstruct_pixel_value(magnitudes, 41);
            
            absolute_time_t recon_end_time = get_absolute_time();
            uint32_t recon_end_cycles = get_cycle_count();
            int64_t recon_execution_time = absolute_time_diff_us(recon_start_time, recon_end_time);
            uint32_t recon_total_cycles = recon_end_cycles - recon_start_cycles;
            perf_printf("Pixel value reconstruction took %lld microseconds (%.2f ms) | %u cycles\n", recon_execution_time, recon_execution_time / 1000.0f, recon_total_cycles);
            
#if DEBUG
            plot_dtft_spectrum(magnitudes, 41);
#endif
            free(magnitudes);
        }
        
        free(complex_values);
    }
    
    // Free signal buffer
    free(signal_buffer);
}

uint8_t process_pattern_return_value(uint8_t *bits_sent) {
    if (!bits_sent) return 0;
    
    // Extract pattern length
    int pattern_len = bits_sent[0];

    // Repeat the pattern 10 times for DTFT analysis
    uint8_t *signal_buffer = repeat_pattern(bits_sent, 10);
    
    // Get total buffer size
    int total_len = pattern_len * 10;
    
    // Compute DTFT with 41 frequency points from 0 to π
    float *complex_values = malloc(41 * 2 * sizeof(float));
    if (!complex_values) {
        free(signal_buffer);
        return 0;
    }
    
    // Compute DTFT for frequencies 0 to π ONLY (41 points with spacing π/40)
    for (int k = 0; k < 41; k++) {
        float omega = (M_PI * k) / 40.0f;
        float real_part = 0.0f;
        float imag_part = 0.0f;
        
        for (int n = 0; n < total_len; n++) {
            float angle = -omega * n;
            real_part += signal_buffer[n] * cosf(angle);
            imag_part += signal_buffer[n] * sinf(angle);
        }
        
        complex_values[2*k] = real_part;
        complex_values[2*k + 1] = imag_part;
    }
    
    // Compute squared magnitudes from complex values
    float *magnitudes = malloc(41 * sizeof(float));
    uint8_t reconstructed_value = 0;
    
    if (magnitudes) {
        for (int k = 0; k < 41; k++) {
            float real = complex_values[2*k];
            float imag = complex_values[2*k + 1];
            magnitudes[k] = real * real + imag * imag;  // Squared magnitude
        }
        
        // Reconstruct pixel value using Euclidean distance
        reconstructed_value = reconstruct_pixel_value(magnitudes, 41);
        
        free(magnitudes);
    }
    
    free(complex_values);
    free(signal_buffer);
    
    return reconstructed_value;
}
