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

#endif // SIGNAL_H
