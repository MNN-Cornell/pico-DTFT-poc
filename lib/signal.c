#include "signal.h"
#include "dtft.h"
#include "output.h"
#include "pico/time.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>

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
    
    // Compute DTFT (complex values)
    absolute_time_t start_time = get_absolute_time();
    float *complex_values = calculate_dtft_complex(signal_buffer, total_len, 128);
    absolute_time_t end_time = get_absolute_time();
    int64_t execution_time = absolute_time_diff_us(start_time, end_time);
    perf_printf("%d bits data: DTFT calculation took %lld milliseconds.\n", pattern_len, execution_time / 1000);
    
    // Print complex DTFT values for MATLAB
    if (complex_values) {
        print_dtft_complex_for_matlab(complex_values, 128);
        
        // Also compute and plot magnitudes for visualization
        float *magnitudes = malloc(128 * sizeof(float));
        if (magnitudes) {
            for (int k = 0; k < 128; k++) {
                float real = complex_values[2*k];
                float imag = complex_values[2*k + 1];
                magnitudes[k] = sqrtf(real * real + imag * imag);
            }
#if DEBUG
            plot_dtft_spectrum(magnitudes, 128);
#endif
            free(magnitudes);
        }
        
        free(complex_values);
    }
    
    // Free signal buffer
    free(signal_buffer);
}
