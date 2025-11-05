#include "gpio_control.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    return cyw43_arch_init();
#endif
}

void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}

void init_signal_gpio(void) {
    gpio_init(SIGNAL_GPIO);
    gpio_set_dir(SIGNAL_GPIO, GPIO_OUT);
    gpio_put(SIGNAL_GPIO, 0);
    
    gpio_init(RECEIVER_GPIO);
    gpio_set_dir(RECEIVER_GPIO, GPIO_OUT);
    gpio_put(RECEIVER_GPIO, 0);
    
    gpio_init(TX_ACTIVE_GPIO);
    gpio_set_dir(TX_ACTIVE_GPIO, GPIO_OUT);
    gpio_put(TX_ACTIVE_GPIO, 0);
}

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

uint8_t* send_data(uint16_t data, uint8_t num_bits) {
    if (num_bits < 1 || num_bits > 16) {
        printf("Error: num_bits must be between 1 and 16\n");
        return NULL;
    }

    // Allocate array: first element stores num_bits, rest store the actual bits
    uint8_t *bits_sent = malloc((num_bits + 1) * sizeof(uint8_t));
    bits_sent[0] = num_bits;  // Store the length in first element

#ifdef DEBUG
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
