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
    
    // Initialize trigonometric look-up tables
    init_trig_lut();
    
    // Initialize Core1 for parallel DTFT computation
    init_core1_dtft();
    printf("Dual-core DTFT enabled (Core0 + Core1)\n");

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    
    // Initialize GPIO pins for signal transmission
    init_signal_gpio();
    
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
}
