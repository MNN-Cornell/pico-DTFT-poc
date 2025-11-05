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

        // Pattern 3: 8 bits
        printf("\nPattern 3 (8 bits):\n");
        // Transmit on GPIO2, clock on GPIO4, sample received bits on GPIO3
        uint8_t *bits_recv = send_receive_data(0b01001100, 8);
        if (bits_recv) {
            process_pattern(bits_recv);
            free(bits_recv);
        }
        
        // Wait before starting the sequence again
        sleep_ms(3000);  // 3 second interval
    }
}
