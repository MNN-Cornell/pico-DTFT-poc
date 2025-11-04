/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "pico/time.h"
#include "pico/multicore.h"

// Size of the sine/cosine lookup tables (must be power of 2)
#define LUT_SIZE 256
#define LUT_MASK (LUT_SIZE - 1)
#define LUT_SCALE (LUT_SIZE / (2.0f * M_PI))  // Precomputed scaling factor

// Pre-computed sine and cosine lookup tables (aligned for faster access)
// ARM Cortex-M33 benefits from 8-byte alignment for float arrays
float sin_lut[LUT_SIZE] __attribute__((aligned(8)));
float cos_lut[LUT_SIZE] __attribute__((aligned(8)));

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

static core1_dtft_params_t core1_params;

// Initialize lookup tables
void init_trig_lut(void) {
    for (int i = 0; i < LUT_SIZE; i++) {
        float angle = (2.0f * M_PI * i) / LUT_SIZE;
        sin_lut[i] = sinf(angle);
        cos_lut[i] = cosf(angle);
    }
}

// Fast lookup of sine/cosine values with forced inlining
static inline __attribute__((always_inline)) float fast_sin(float angle) {
    // Normalize angle to 0-2π range and convert to lookup table index
    int index = (int)(angle * LUT_SCALE) & LUT_MASK;
    return sin_lut[index];
}

static inline __attribute__((always_inline)) float fast_cos(float angle) {
    // Normalize angle to 0-2π range and convert to lookup table index
    int index = (int)(angle * LUT_SCALE) & LUT_MASK;
    return cos_lut[index];
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

// Pico W devices use a GPIO on the WIFI chip for the LED,
// so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 10
#endif

#define SIGNAL_GPIO 2
#define RECEIVER_GPIO 3
#define TX_ACTIVE_GPIO 4
#define BIT_DELAY_MS 10    // Reduced from 100ms to 1ms for faster transmission

// Perform initialisation
int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#endif
}

// Turn the led on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}

// Send a single bit on GPIO2 with LED blink and clock pulse
void send_bit(uint8_t bit) {
    // Set data bit
    gpio_put(SIGNAL_GPIO, bit);
    
    // Clock pulse: high for half the bit time
    gpio_put(RECEIVER_GPIO, 1);
    pico_set_led(true);
    sleep_ms(BIT_DELAY_MS / 2);
    
    // Clock pulse: low for the other half
    gpio_put(RECEIVER_GPIO, 0);
    pico_set_led(false);
    sleep_ms(BIT_DELAY_MS / 2);
}

// Send data on GPIO2 (1-16 bits)
// data: the value to send
// num_bits: number of bits to send (1-16)
// Returns: array of bits sent (caller must free). First element is num_bits, rest are the bits.
uint8_t* send_data(uint16_t data, uint8_t num_bits) {
    if (num_bits < 1 || num_bits > 16) {
        perf_printf("Error: num_bits must be between 1 and 16\n");
        return NULL;
    }

    // Allocate array: first element stores num_bits, rest store the actual bits
    uint8_t *bits_sent = malloc((num_bits + 1) * sizeof(uint8_t));
    bits_sent[0] = num_bits;  // Store the length in first element

#if DEBUG
    printf("Send pattern: ");
    for (int i = num_bits - 1; i >= 0; i--) {
        printf("%d", (data >> i) & 1);
    }
    printf("\n");
#endif

    // Set TX_ACTIVE high to indicate transmission is starting
    gpio_put(TX_ACTIVE_GPIO, 1);

    // Send bits from MSB to LSB
    for (int i = num_bits - 1; i >= 0; i--) {
        uint8_t bit = (data >> i) & 1;
        bits_sent[num_bits - i] = bit;  // Store starting at index 1
        send_bit(bit);
    }
    
    // Reset GPIO2 to 0 after transmission
    gpio_put(SIGNAL_GPIO, 0);
    sleep_ms(10);  // Give it time to settle

    // Set TX_ACTIVE low to indicate transmission is complete
    gpio_put(TX_ACTIVE_GPIO, 0);

    
    return bits_sent;
}

// Repeat a bit pattern N times to create a signal buffer
// pattern: input bit pattern (first element contains the pattern length)
// repetitions: number of times to repeat
// Returns: buffer of size pattern_len * repetitions (caller must free)
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

// Compute DTFT magnitude at a specific frequency (optimized)
// x: input signal array
// N: length of signal
// omega: normalized frequency (radians/sample, 0 to 2*pi)
// Returns: magnitude of DTFT at that frequency
float compute_dtft_magnitude(uint8_t * restrict x, int N, float omega) {
    float real_part = 0.0f;
    float imag_part = 0.0f;
    float angle = 0.0f;
    const float neg_omega = -omega;
    
    // Loop unrolling (4x)
    int n = 0;
    int N_unroll = N & ~3;
    
    for (; n < N_unroll; n += 4) {
        float angle1 = angle;
        float angle2 = angle + neg_omega;
        float angle3 = angle + 2.0f * neg_omega;
        float angle4 = angle + 3.0f * neg_omega;
        
        float cos1 = fast_cos(angle1);
        float sin1 = fast_sin(angle1);
        float cos2 = fast_cos(angle2);
        float sin2 = fast_sin(angle2);
        float cos3 = fast_cos(angle3);
        float sin3 = fast_sin(angle3);
        float cos4 = fast_cos(angle4);
        float sin4 = fast_sin(angle4);
        
        real_part += x[n]   * cos1 + x[n+1] * cos2 + x[n+2] * cos3 + x[n+3] * cos4;
        imag_part += x[n]   * sin1 + x[n+1] * sin2 + x[n+2] * sin3 + x[n+3] * sin4;
        
        angle += 4.0f * neg_omega;
    }
    
    // Handle remaining samples
    for (; n < N; n++) {
        real_part += x[n] * fast_cos(angle);
        imag_part += x[n] * fast_sin(angle);
        angle += neg_omega;
    }
    
    return sqrtf(real_part * real_part + imag_part * imag_part);
}

// Compute DTFT magnitudes for a range of frequencies
// x: input signal array
// N: length of signal
// num_points: number of frequency points to compute
// Returns: array of magnitudes (caller must free)
float* calculate_dtft(uint8_t * restrict x, int N, int num_points) {
    float *magnitudes = malloc(num_points * sizeof(float));
    if (!magnitudes) {
        printf("Error: Failed to allocate memory for DTFT magnitudes\n");
        return NULL;
    }

    const float omega_scale = (2.0f * M_PI) / num_points;
    for (int k = 0; k < num_points; k++) {
        float omega = omega_scale * k;  // 0 to 2*pi
        magnitudes[k] = compute_dtft_magnitude(x, N, omega);
    }

    return magnitudes;
}

#if DEBUG
// Plot DTFT spectrum in terminal with adaptive scaling
// magnitudes: array of DTFT magnitudes
// num_points: number of frequency points in the magnitudes array
void plot_dtft_spectrum(float *magnitudes, int num_points) {
    printf("\n========== DTFT SPECTRUM ==========\n");
    
    // Find max magnitude
    float max_magnitude = 0.0f;
    for (int k = 0; k < num_points; k++) {
        if (magnitudes[k] > max_magnitude) {
            max_magnitude = magnitudes[k];
        }
    }
    
    // Normalize for display
    if (max_magnitude == 0.0f) max_magnitude = 1.0f;
    
    // Adaptive display height based on data
    int max_height = 25;  // Maximum height for display
    
    // Convert to bar heights (0 to max_height for display)
    int bar_heights[num_points];
    for (int k = 0; k < num_points; k++) {
        bar_heights[k] = (int)((magnitudes[k] / max_magnitude) * max_height);
    }
    
    // Plot spectrum vertically - magnitude on Y-axis, frequency on X-axis (0 to π only)
    int half_points = num_points / 2;
    for (int height = max_height; height > 0; height--) {
        for (int k = 0; k < half_points; k++) {
            if (bar_heights[k] >= height) {
                printf("#");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    
    // X-axis labels (frequency in radians/sample, 0 to π)
    for (int k = 0; k < half_points; k++) {
        printf("-");
    }
    printf("\n");
    
    // Print frequency labels at major intervals (aligned to column positions)
    // Labels at: 0, 0.25π, 0.5π, 0.75π, 1.0π (covering 0 to π)
    char label_line[half_points + 1];
    memset(label_line, ' ', half_points);
    label_line[half_points] = '\0';
    
    // Place labels at appropriate positions matching histogram columns
    // For half_points columns (0 to half_points-1), labels at 0, 0.25π, 0.5π, 0.75π, π
    // correspond to positions: 0, half_points/4, half_points/2, 3*half_points/4, half_points-1
    int label_positions[] = {0, half_points/4, half_points/2, 3*half_points/4, half_points-1};
    const char *labels[] = {"0", "0.25π", "0.5π", "0.75π", "π"};

    for (int i = 0; i < 5; i++) {
        int pos = label_positions[i];
        int len = strlen(labels[i]);
        // Right-align the last label (π) at the end of the axis
        int start = (i == 4) ? (half_points - len) : pos;
        if (start >= 0 && start + len <= half_points) {
            for (int j = 0; j < len; j++) {
                label_line[start + j] = labels[i][j];
            }
        }
    }
    label_line[half_points] = '\0';
    printf("%s\n", label_line);
    
    // Print statistics
    printf("Max magnitude: %.6f | Height: %d\n", max_magnitude, max_height);
    printf("===================================\n\n");
}
#endif

// Core1 worker function for parallel DTFT computation
void core1_dtft_worker(void) {
    while (true) {
        // Wait for work (with memory barrier)
        while (!core1_params.signal) {
            __dmb();  // Data memory barrier to ensure we see Core0's write
            tight_loop_contents();
        }
        
        const float omega_scale = (2.0f * M_PI) / core1_params.num_points;
        
        // Compute DTFT for assigned frequency range
        for (int k = core1_params.start_freq; k < core1_params.end_freq; k++) {
            float real_part = 0.0f;
            float imag_part = 0.0f;
            const float omega = omega_scale * k;
            const float neg_omega = -omega;
            float angle = 0.0f;
            
            int n = 0;
            int N_unroll = core1_params.signal_len & ~3;
            
            for (; n < N_unroll; n += 4) {
                float angle1 = angle;
                float angle2 = angle + neg_omega;
                float angle3 = angle + 2.0f * neg_omega;
                float angle4 = angle + 3.0f * neg_omega;
                
                float cos1 = fast_cos(angle1);
                float sin1 = fast_sin(angle1);
                float cos2 = fast_cos(angle2);
                float sin2 = fast_sin(angle2);
                float cos3 = fast_cos(angle3);
                float sin3 = fast_sin(angle3);
                float cos4 = fast_cos(angle4);
                float sin4 = fast_sin(angle4);
                
                real_part += core1_params.signal[n]   * cos1 + core1_params.signal[n+1] * cos2 + 
                             core1_params.signal[n+2] * cos3 + core1_params.signal[n+3] * cos4;
                imag_part += core1_params.signal[n]   * sin1 + core1_params.signal[n+1] * sin2 + 
                             core1_params.signal[n+2] * sin3 + core1_params.signal[n+3] * sin4;
                
                angle += 4.0f * neg_omega;
            }
            
            for (; n < core1_params.signal_len; n++) {
                real_part += core1_params.signal[n] * fast_cos(angle);
                imag_part += core1_params.signal[n] * fast_sin(angle);
                angle += neg_omega;
            }
            
            core1_params.output[2*k] = real_part;
            core1_params.output[2*k + 1] = imag_part;
        }
        
        // Signal completion
        __dmb();  // Ensure all writes complete before signaling done
        core1_params.done = true;
        core1_params.signal = NULL;
    }
}

// Compute DTFT with both magnitude and phase (dual-core optimized version)
// x: input signal array
// N: length of signal
// num_points: number of frequency points to compute
// Returns: array of complex values [real1, imag1, real2, imag2, ...] (caller must free)
float* calculate_dtft_complex(uint8_t * restrict x, int N, int num_points) {
    float *complex_values = malloc(num_points * 2 * sizeof(float));
    if (!complex_values) {
        printf("Error: Failed to allocate memory for DTFT complex values\n");
        return NULL;
    }

    // Split work between cores: Core0 handles first half, Core1 handles second half
    int split_point = num_points / 2;
    
    // Launch Core1 work
    core1_params.signal_len = N;
    core1_params.start_freq = split_point;
    core1_params.end_freq = num_points;
    core1_params.num_points = num_points;
    core1_params.output = complex_values;
    core1_params.done = false;
    __dmb();  // Memory barrier before signaling
    core1_params.signal = x;  // This triggers Core1 to start
    
    // Core0 computes first half
    const float omega_scale = (2.0f * M_PI) / num_points;
    
    for (int k = 0; k < split_point; k++) {
        float real_part = 0.0f;
        float imag_part = 0.0f;
        const float omega = omega_scale * k;
        const float neg_omega = -omega;
        float angle = 0.0f;
        
        // Main loop with manual unrolling (4x) for better ILP
        int n = 0;
        int N_unroll = N & ~3;  // Round down to multiple of 4
        
        for (; n < N_unroll; n += 4) {
            // Process 4 samples at once
            float angle1 = angle;
            float angle2 = angle + neg_omega;
            float angle3 = angle + 2.0f * neg_omega;
            float angle4 = angle + 3.0f * neg_omega;
            
            float cos1 = fast_cos(angle1);
            float sin1 = fast_sin(angle1);
            float cos2 = fast_cos(angle2);
            float sin2 = fast_sin(angle2);
            float cos3 = fast_cos(angle3);
            float sin3 = fast_sin(angle3);
            float cos4 = fast_cos(angle4);
            float sin4 = fast_sin(angle4);
            
            real_part += x[n]   * cos1 + x[n+1] * cos2 + x[n+2] * cos3 + x[n+3] * cos4;
            imag_part += x[n]   * sin1 + x[n+1] * sin2 + x[n+2] * sin3 + x[n+3] * sin4;
            
            angle += 4.0f * neg_omega;
        }
        
        // Handle remaining samples
        for (; n < N; n++) {
            real_part += x[n] * fast_cos(angle);
            imag_part += x[n] * fast_sin(angle);
            angle += neg_omega;
        }
        
        complex_values[2*k] = real_part;
        complex_values[2*k + 1] = imag_part;
    }
    
    // Wait for Core1 to finish
    while (!core1_params.done) {
        __dmb();  // Memory barrier to see Core1's update
        tight_loop_contents();
    }
    __dmb();  // Ensure we see all of Core1's writes

    return complex_values;
}

// Print DTFT complex values in MATLAB-friendly format
void print_dtft_complex_for_matlab(float *complex_values, int num_points) {
    printf("\n========== DTFT COMPLEX VALUES FOR MATLAB ==========\n");
    printf("dtft_real = [\n");
    for (int k = 0; k < num_points; k++) {
        printf("    %.6f", complex_values[2*k]);
        if (k < num_points - 1) {
            printf(";");
        }
        printf("\n");
    }
    printf("];\n");
    
    printf("dtft_imag = [\n");
    for (int k = 0; k < num_points; k++) {
        printf("    %.6f", complex_values[2*k + 1]);
        if (k < num_points - 1) {
            printf(";");
        }
        printf("\n");
    }
    printf("];\n");
    printf("====================================================\n\n");
}

// Process a single pattern: compute DTFT and plot spectrum
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

int main() {
    // Initialize stdio only in DEBUG to avoid USB overhead in performance runs
    stdio_init_all();
    
    // Initialize trigonometric look-up tables
    init_trig_lut();
    
    // Initialize Core1 parameters
    core1_params.signal = NULL;
    core1_params.done = true;
    
    // Launch Core1 for parallel DTFT computation
    multicore_launch_core1(core1_dtft_worker);
    printf("Dual-core DTFT enabled (Core0 + Core1)\n");

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    
    // Initialize GPIO2 for signal generation, GPIO3 for receiver, GPIO4 for TX_ACTIVE
    gpio_init(SIGNAL_GPIO);
    gpio_set_dir(SIGNAL_GPIO, GPIO_OUT);
    gpio_put(SIGNAL_GPIO, 0);
    gpio_init(RECEIVER_GPIO);
    gpio_set_dir(RECEIVER_GPIO, GPIO_OUT);
    gpio_put(RECEIVER_GPIO, 0);
    gpio_init(TX_ACTIVE_GPIO);
    gpio_set_dir(TX_ACTIVE_GPIO, GPIO_OUT);
    gpio_put(TX_ACTIVE_GPIO, 0);
    
    while (true) {
        printf("\n=== Starting new pattern sequence ===\n");     

        // Pattern 3: 8 bits
        printf("\nPattern 3 (8 bits):\n");
        uint8_t *bits_sent = send_data(0b01001100, 8); // 8 bits: impulse (single 1). radians/sample = 2*pi/8
        if (bits_sent) {
            process_pattern(bits_sent);
            free(bits_sent);
        }
        sleep_ms(3000);  // 3 second interval
        
        // Wait before starting the sequence again
        sleep_ms(3000);  // 3 second interval
    }
}
