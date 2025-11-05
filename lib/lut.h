#ifndef LUT_H
#define LUT_H

#include <stdint.h>
#include <math.h>

// Size of the sine/cosine lookup tables (must be power of 2)
#define LUT_SIZE 1024
#define LUT_MASK (LUT_SIZE - 1)
#define LUT_SCALE (LUT_SIZE / (2.0f * M_PI))

// Pre-computed sine and cosine lookup tables
extern float sin_lut[LUT_SIZE];
extern float cos_lut[LUT_SIZE];

/**
 * Initialize trigonometric lookup tables
 */
void init_trig_lut(void);

/**
 * Fast lookup of sine value with linear interpolation and forced inlining
 * @param angle Angle in radians
 * @return sin(angle)
 */
static inline __attribute__((always_inline)) float fast_sin(float angle) {
    // Normalize angle to 0-2π range and convert to lookup table index
    float scaled = angle * LUT_SCALE;
    int index = (int)scaled;
    float frac = scaled - index;
    index &= LUT_MASK;
    int next_index = (index + 1) & LUT_MASK;
    
    // Linear interpolation between two LUT entries
    return sin_lut[index] + frac * (sin_lut[next_index] - sin_lut[index]);
}

/**
 * Fast lookup of cosine value with linear interpolation and forced inlining
 * @param angle Angle in radians
 * @return cos(angle)
 */
static inline __attribute__((always_inline)) float fast_cos(float angle) {
    // Normalize angle to 0-2π range and convert to lookup table index
    float scaled = angle * LUT_SCALE;
    int index = (int)scaled;
    float frac = scaled - index;
    index &= LUT_MASK;
    int next_index = (index + 1) & LUT_MASK;
    
    // Linear interpolation between two LUT entries
    return cos_lut[index] + frac * (cos_lut[next_index] - cos_lut[index]);
}

#endif // LUT_H
