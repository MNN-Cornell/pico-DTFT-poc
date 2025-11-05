#ifndef SIGNAL_H
#define SIGNAL_H

#include <stdint.h>

/**
 * Repeat a bit pattern N times to create a signal buffer
 * @param pattern Input bit pattern (first element contains the pattern length)
 * @param repetitions Number of times to repeat
 * @return Buffer of size pattern_len * repetitions (caller must free)
 */
uint8_t* repeat_pattern(uint8_t *pattern, int repetitions);

/**
 * Process a single pattern: compute DTFT and plot spectrum
 * @param bits_sent Bit pattern array (first element is length)
 */
void process_pattern(uint8_t *bits_sent);

/**
 * Process a single pattern and return reconstructed pixel value
 * @param bits_sent Bit pattern array (first element is length)
 * @return Reconstructed pixel value (0-255)
 */
uint8_t process_pattern_return_value(uint8_t *bits_sent);

/**
 * Process pattern and output DTFT magnitude spectrum (for PC-side reconstruction)
 * @param bits_sent Array where first element is length, rest are bit values
 * @param pixel_idx Pixel index in image
 * @param x X position in image
 * @param y Y position in image
 */
void process_pattern_output_spectrum(uint8_t *bits_sent, int pixel_idx, int x, int y);

/**
 * Initialize the ARM DWT cycle counter for performance measurement
 * Call this once at startup before measuring cycles
 */
void init_cycle_counter(void);

#endif // SIGNAL_H
