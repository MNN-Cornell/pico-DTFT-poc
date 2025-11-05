#ifndef OUTPUT_H
#define OUTPUT_H

/**
 * Plot DTFT spectrum in terminal with adaptive scaling
 * @param magnitudes Array of DTFT magnitudes
 * @param num_points Number of frequency points in the magnitudes array
 */
void plot_dtft_spectrum(float *magnitudes, int num_points);

/**
 * Print DTFT complex values in MATLAB-friendly format
 * @param complex_values Array of complex values [real1, imag1, real2, imag2, ...]
 * @param num_points Number of frequency points
 */
void print_dtft_complex_for_matlab(float *complex_values, int num_points);

#endif // OUTPUT_H
