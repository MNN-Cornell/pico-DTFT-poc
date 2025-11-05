#include "output.h"
#include <stdio.h>
#include <string.h>

void plot_dtft_spectrum(float *magnitudes, int num_points) {
    printf("\n========== DTFT SPECTRUM ==========\n");
    
    // Find max magnitude
    float max_magnitude = 0.0f;
    for (int k = 0; k < num_points; k++) {
        if (magnitudes[k] > max_magnitude) {
            max_magnitude = magnitudes[k];
        }
    }
    
    // Normalize for display
    if (max_magnitude == 0.0f) max_magnitude = 1.0f;
    
    // Adaptive display height based on data
    int max_height = 25;  // Maximum height for display
    
    // Convert to bar heights (0 to max_height for display)
    int bar_heights[num_points];
    for (int k = 0; k < num_points; k++) {
        bar_heights[k] = (int)((magnitudes[k] / max_magnitude) * max_height);
    }
    
    // Plot spectrum vertically - magnitude on Y-axis, frequency on X-axis (0 to π)
    // num_points already represents 0 to π range (41 points for n=10)
    for (int height = max_height; height > 0; height--) {
        for (int k = 0; k < num_points; k++) {
            if (bar_heights[k] >= height) {
                printf("#");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    
    // X-axis labels (frequency in radians/sample, 0 to π)
    for (int k = 0; k < num_points; k++) {
        printf("-");
    }
    printf("\n");
    
    // Print frequency labels at major intervals (aligned to column positions)
    // Labels at: 0, 0.25π, 0.5π, 0.75π, 1.0π (covering 0 to π)
    char label_line[num_points + 1];
    memset(label_line, ' ', num_points);
    label_line[num_points] = '\0';
    
    // Place labels at appropriate positions matching histogram columns
    // For num_points columns (0 to num_points-1), labels at 0, 0.25π, 0.5π, 0.75π, π
    // correspond to positions: 0, num_points/4, num_points/2, 3*num_points/4, num_points-1
    int label_positions[] = {0, num_points/4, num_points/2, 3*num_points/4, num_points-1};
    const char *labels[] = {"0", "0.25π", "0.5π", "0.75π", "π"};

    for (int i = 0; i < 5; i++) {
        int pos = label_positions[i];
        int len = strlen(labels[i]);
        // Right-align the last label (π) at the end of the axis
        int start = (i == 4) ? (num_points - len) : pos;
        if (start >= 0 && start + len <= num_points) {
            for (int j = 0; j < len; j++) {
                label_line[start + j] = labels[i][j];
            }
        }
    }
    label_line[num_points] = '\0';
    printf("%s\n", label_line);
    
    // Print statistics
    printf("Max magnitude: %.6f | Height: %d\n", max_magnitude, max_height);
    printf("===================================\n\n");
}

void print_dtft_complex_for_matlab(float *complex_values, int num_points) {
    printf("\n========== DTFT COMPLEX VALUES FOR MATLAB ==========\n");
    printf("dtft_real = [\n");
    for (int k = 0; k < num_points; k++) {
        printf("    %.6f", complex_values[2*k]);
        if (k < num_points - 1) {
            printf(";");
        }
        printf("\n");
    }
    printf("];\n");
    
    printf("dtft_imag = [\n");
    for (int k = 0; k < num_points; k++) {
        printf("    %.6f", complex_values[2*k + 1]);
        if (k < num_points - 1) {
            printf(";");
        }
        printf("\n");
    }
    printf("];\n");
    printf("====================================================\n\n");
}
