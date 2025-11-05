#include "lut.h"

// Pre-computed sine and cosine lookup tables (aligned for faster access)
// ARM Cortex-M33 benefits from 8-byte alignment for float arrays
float sin_lut[LUT_SIZE] __attribute__((aligned(8)));
float cos_lut[LUT_SIZE] __attribute__((aligned(8)));

void init_trig_lut(void) {
    for (int i = 0; i < LUT_SIZE; i++) {
        float angle = (2.0f * M_PI * i) / LUT_SIZE;
        sin_lut[i] = sinf(angle);
        cos_lut[i] = cosf(angle);
    }
}
