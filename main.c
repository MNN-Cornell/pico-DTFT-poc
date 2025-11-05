/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// Module includes
#include "lib/lut.h"
#include "lib/dtft.h"
#include "lib/gpio_control.h"
#include "lib/signal.h"
#include "lib/output.h"
#include "lib/image_data.h"

// Configuration: Number of pixels to transmit (set to IMAGE_SIZE for full image)
// Start with a smaller number for testing (e.g., 100-1000 pixels)
#define PIXELS_TO_TRANSMIT 2000  // Adjust this value

// Array to store reconstructed image pixels
static uint8_t reconstructed_image[PIXELS_TO_TRANSMIT];

/**
 * Process and reconstruct a single pixel value
 * @param pixel_value Original pixel value (0-255)
 * @return Reconstructed pixel value
 */
uint8_t process_pixel(uint8_t pixel_value) {
    // Transmit on GPIO2, clock on GPIO4, sample received bits on GPIO3
    uint8_t *bits_recv = send_receive_data(pixel_value, 8);
    uint8_t reconstructed = 0;
    
    if (bits_recv) {
        // Process pattern and get reconstructed value
        reconstructed = process_pattern_return_value(bits_recv);
        free(bits_recv);
    }
    
    return reconstructed;
}

/**
 * Transmit and reconstruct image
 * Sends pixels one by one and reconstructs the image
 */
void transmit_reconstruct_image(void) {
    printf("\n========== IMAGE TRANSMISSION ==========\n");
    printf("Image size: %dx%d = %d pixels\n", IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_SIZE);
    printf("Transmitting: %d pixels\n", PIXELS_TO_TRANSMIT);
    printf("========================================\n\n");
    
    absolute_time_t start_time = get_absolute_time();
    
    // Transmit and reconstruct each pixel
    for (int i = 0; i < PIXELS_TO_TRANSMIT; i++) {
        // Calculate (x, y) position in image
        int x = i % IMAGE_WIDTH;
        int y = i / IMAGE_WIDTH;
        
        uint8_t original = image_data[i];
        
        // Print what we're sending
        printf("\n[Pixel %d] Position: (%d, %d)\n", i, x, y);
        printf("  SENDING: 0x%02X (0b", original);
        for (int b = 7; b >= 0; b--) {
            printf("%d", (original >> b) & 1);
        }
        printf(", decimal: %d)\n", original);
        
        // Transmit and reconstruct
        uint8_t reconstructed = process_pixel(original);
        reconstructed_image[i] = reconstructed;
        
        // Print reconstruction result
        printf("  RECONSTRUCTED: 0x%02X (0b", reconstructed);
        for (int b = 7; b >= 0; b--) {
            printf("%d", (reconstructed >> b) & 1);
        }
        printf(", decimal: %d) %s\n", reconstructed,
               (original == reconstructed) ? "✓ MATCH" : "✗ MISMATCH");
        
        // Progress update every 10 pixels
        if ((i + 1) % 10 == 0) {
            printf("\n>>> Progress: %d/%d pixels (%.1f%%) <<<\n", 
                   i + 1, PIXELS_TO_TRANSMIT, 
                   (float)(i + 1) * 100.0f / PIXELS_TO_TRANSMIT);
        }
    }
    
    absolute_time_t end_time = get_absolute_time();
    int64_t total_time = absolute_time_diff_us(start_time, end_time);
    
    // Calculate accuracy
    int correct = 0;
    int total_error = 0;
    for (int i = 0; i < PIXELS_TO_TRANSMIT; i++) {
        if (image_data[i] == reconstructed_image[i]) {
            correct++;
        } else {
            total_error += abs((int)image_data[i] - (int)reconstructed_image[i]);
        }
    }
    
    printf("\n========== RECONSTRUCTION RESULTS ==========\n");
    printf("Pixels transmitted: %d\n", PIXELS_TO_TRANSMIT);
    printf("Correct reconstructions: %d/%d (%.2f%%)\n", 
           correct, PIXELS_TO_TRANSMIT, 
           (float)correct * 100.0f / PIXELS_TO_TRANSMIT);
    printf("Average error per incorrect pixel: %.2f\n", 
           (PIXELS_TO_TRANSMIT - correct) > 0 ? 
           (float)total_error / (PIXELS_TO_TRANSMIT - correct) : 0.0f);
    printf("Total time: %.2f seconds\n", total_time / 1000000.0f);
    printf("Average time per pixel: %.2f ms\n", 
           total_time / (float)PIXELS_TO_TRANSMIT / 1000.0f);
    printf("Estimated time for full image: %.2f hours\n",
           (total_time / (float)PIXELS_TO_TRANSMIT * IMAGE_SIZE) / 3600000000.0f);
    printf("============================================\n\n");
    
    // Output reconstructed image data in machine-readable format
    printf("\n========== RECONSTRUCTED IMAGE DATA ==========\n");
    printf("IMAGE_DATA_START\n");
    printf("WIDTH=%d\n", IMAGE_WIDTH);
    printf("HEIGHT=%d\n", IMAGE_HEIGHT);
    printf("PIXELS=%d\n", PIXELS_TO_TRANSMIT);
    printf("DATA_HEX\n");
    
    // Print as hex values, 16 per line for readability
    for (int i = 0; i < PIXELS_TO_TRANSMIT; i++) {
        printf("%02X", reconstructed_image[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        } else if (i < PIXELS_TO_TRANSMIT - 1) {
            printf(" ");
        }
    }
    if (PIXELS_TO_TRANSMIT % 16 != 0) {
        printf("\n");
    }
    
    printf("IMAGE_DATA_END\n");
    printf("==============================================\n\n");
}

/**
 * Test a pattern by sending, receiving, and reconstructing
 * @param pattern 8-bit pattern to test
 */
void test_pattern(uint8_t pattern) {
    printf("\n=== Testing Pattern ===\n");
    printf("Sent: 0x%02X (0b", pattern);
    for (int i = 7; i >= 0; i--) {
        printf("%d", (pattern >> i) & 1);
    }
    printf(", decimal: %d)\n", pattern);
    
    // Transmit on GPIO2, clock on GPIO4, sample received bits on GPIO3
    uint8_t *bits_recv = send_receive_data(pattern, 8);
    if (bits_recv) {
        printf("Received bits: ");
        for (int i = 1; i <= bits_recv[0]; i++) {
            printf("%d", bits_recv[i]);
        }
        printf("\n");
        process_pattern(bits_recv);
        free(bits_recv);
    }
}


int main() {
    // Initialize stdio only in DEBUG to avoid USB overhead in performance runs
    stdio_init_all();
    
    // Initialize cycle counter for performance measurement
    init_cycle_counter();
    
    // Initialize trigonometric look-up tables
    init_trig_lut();
    
    // Initialize Core1 for parallel DTFT computation
    init_core1_dtft();
    printf("Dual-core DTFT enabled (Core0 + Core1)\n");

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    
    // Initialize GPIO pins for signal transmission
    init_signal_gpio();
    
    // Choose mode: 0 = single pattern test, 1 = image transmission
    int mode = 1;  // Change to 0 for single pattern testing
    
    if (mode == 0) {
        // Single pattern testing mode
        while (true) {
            printf("\n=== Starting new pattern sequence ===\n");
            
            // Test different patterns - just change the values here!
            // test_pattern(0x4C);   // 0b01001100
            test_pattern(0x4F);   // 0b01001111
            
            // Uncomment to test more patterns:
            // test_pattern(0xAA);   // 0b10101010
            // test_pattern(0x55);   // 0b01010101
            // test_pattern(0xFF);   // 0b11111111
            // test_pattern(0x00);   // 0b00000000
            // test_pattern(0x0F);   // 0b00001111
            // test_pattern(0xF0);   // 0b11110000
            
            // Wait before starting the sequence again
            sleep_ms(3000);  // 3 second interval
        }
    } else {
        // Image transmission mode
        while (true) {
            transmit_reconstruct_image();
            sleep_ms(60000);
        }
    }
}
