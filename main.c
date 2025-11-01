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

// DEBUG control: set to 1 for verbose logging/plotting, 0 for performance runs
#ifndef DEBUG
#define DEBUG 0
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
#define LED_DELAY_MS 250
#endif

#define SIGNAL_GPIO 2
#define RECEIVER_GPIO 3
#define TX_ACTIVE_GPIO 4
#define BIT_DELAY_MS 100
#define PATTERN_DELAY_MS 1000

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

// Compute DTFT magnitude at a specific frequency
// x: input signal array
// N: length of signal
// omega: normalized frequency (radians/sample, 0 to 2*pi)
// Returns: magnitude of DTFT at that frequency
float compute_dtft_magnitude(uint8_t *x, int N, float omega) {
    float real_part = 0.0f;
    float imag_part = 0.0f;
    
    for (int n = 0; n < N; n++) {
        float angle = -omega * n;
        real_part += x[n] * cosf(angle);
        imag_part += x[n] * sinf(angle);
    }
    
    return sqrtf(real_part * real_part + imag_part * imag_part);
}

// Compute DTFT magnitudes for a range of frequencies
// x: input signal array
// N: length of signal
// num_points: number of frequency points to compute
// Returns: array of magnitudes (caller must free)
float* calculate_dtft(uint8_t *x, int N, int num_points) {
    float *magnitudes = malloc(num_points * sizeof(float));
    if (!magnitudes) {
        printf("Error: Failed to allocate memory for DTFT magnitudes\n");
        return NULL;
    }

    for (int k = 0; k < num_points; k++) {
        float omega = (2.0f * M_PI * k) / num_points;  // 0 to 2*pi
        magnitudes[k] = compute_dtft_magnitude(x, N, omega);
    }

    return magnitudes;
}

#if DEBUG
// Plot DTFT spectrum in terminal
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
    
    // Convert to bar heights (0-30 for display)
    int bar_heights[num_points];
    for (int k = 0; k < num_points; k++) {
        bar_heights[k] = (int)((magnitudes[k] / max_magnitude) * 30);
    }
    
    // Plot spectrum vertically - magnitude on Y-axis, frequency on X-axis (0 to π only)
    int half_points = num_points / 2;
    for (int height = 30; height > 0; height--) {
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
    // These correspond to k values: 0, 8, 16, 24, 32 (for 64 points covering 0 to π)
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
    
    printf("===================================\n\n");
}
#endif

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
    
    // Compute DTFT
    absolute_time_t start_time = get_absolute_time();
    float *magnitudes = calculate_dtft(signal_buffer, total_len, 128);
    absolute_time_t end_time = get_absolute_time();
    int64_t execution_time = absolute_time_diff_us(start_time, end_time);
    perf_printf("%d bits data: DTFT calculation took %lld milliseconds.\n", pattern_len, execution_time / 1000);
    
    // Plot DTFT spectrum (DEBUG only)
    if (magnitudes) {
#if DEBUG
        plot_dtft_spectrum(magnitudes, 128);
#endif
        free(magnitudes);
    }
    
    // Free signal buffer
    free(signal_buffer);
}

int main() {
    // Initialize stdio only in DEBUG to avoid USB overhead in performance runs
    stdio_init_all();

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
        
        // Pattern 1: 2 bits
        printf("\nPattern 1 (2 bits):\n");
        uint8_t *bits_sent = send_data(0b10, 2);       // 2 bits: 10. radians/sample = 2*pi/2
        if (bits_sent) {
            // Process pattern (DTFT calculation and plotting)
            process_pattern(bits_sent);
            free(bits_sent);
        }
        sleep_ms(5000);  // 5 second interval
        
        // Pattern 2: 4 bits
        printf("\nPattern 2 (4 bits):\n");
        bits_sent = send_data(0b1000, 4);     // 4 bits: 1000. radians/sample = 2*pi/4
        if (bits_sent) {
            process_pattern(bits_sent);
            free(bits_sent);
        }
        sleep_ms(5000);  // 5 second interval
        
        // Pattern 3: 8 bits
        printf("\nPattern 3 (8 bits):\n");
        bits_sent = send_data(0b10000000, 8); // 8 bits: 10000000. radians/sample = 2*pi/8
        if (bits_sent) {
            process_pattern(bits_sent);
            free(bits_sent);
        }
        sleep_ms(5000);  // 5 second interval
        
        // Pattern 4: 16 bits
        printf("\nPattern 4 (16 bits):\n");
        bits_sent = send_data(0x8000, 16);    // 16 bits: 1000000000000000. radians/sample = 2*pi/16
        
        if (bits_sent) {
            process_pattern(bits_sent);
            free(bits_sent);
        }
        
        // Wait before starting the sequence again
        sleep_ms(5000);  // 5 second interval
    }
}
