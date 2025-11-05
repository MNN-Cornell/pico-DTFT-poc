#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

#define SIGNAL_GPIO 2
#define RECEIVER_GPIO 3
#define TX_ACTIVE_GPIO 4
#define BIT_DELAY_MS 10

/**
 * Initialize LED GPIO
 * @return PICO_OK on success
 */
int pico_led_init(void);

/**
 * Set LED state
 * @param led_on True to turn on, false to turn off
 */
void pico_set_led(bool led_on);

/**
 * Initialize GPIO pins for signal transmission
 */
void init_signal_gpio(void);

/**
 * Send a single bit on GPIO2 with LED blink and clock pulse
 * @param bit The bit value to send (0 or 1)
 */
void send_bit(uint8_t bit);

/**
 * Send data on GPIO2 (1-16 bits)
 * @param data The value to send
 * @param num_bits Number of bits to send (1-16)
 * @return Array of bits sent (caller must free). First element is num_bits, rest are the bits.
 */
uint8_t* send_data(uint16_t data, uint8_t num_bits);

#endif // GPIO_CONTROL_H
