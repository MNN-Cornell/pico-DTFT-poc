#include "dtft.h"
#include "lut.h"
#include <stdlib.h>
#include <math.h>
#include "pico/multicore.h"
#include "pico/time.h"

// Global Core1 parameters
static core1_dtft_params_t core1_params;

// Core1 worker function for parallel DTFT computation
static void core1_dtft_worker(void) {
    while (true) {
        // Wait for work (with memory barrier)
        while (!core1_params.signal) {
            __dmb();  // Data memory barrier to ensure we see Core0's write
            tight_loop_contents();
        }
        
        const float omega_scale = (2.0f * M_PI) / core1_params.num_points;
        
        // Compute DTFT for assigned frequency range
        for (int k = core1_params.start_freq; k < core1_params.end_freq; k++) {
            float real_part = 0.0f;
            float imag_part = 0.0f;
            const float omega = omega_scale * k;
            const float neg_omega = -omega;
            float angle = 0.0f;
            
            int n = 0;
            int N_unroll = core1_params.signal_len & ~3;
            
            for (; n < N_unroll; n += 4) {
                float angle1 = angle;
                float angle2 = angle + neg_omega;
                float angle3 = angle + 2.0f * neg_omega;
                float angle4 = angle + 3.0f * neg_omega;
                
                float cos1 = fast_cos(angle1);
                float sin1 = fast_sin(angle1);
                float cos2 = fast_cos(angle2);
                float sin2 = fast_sin(angle2);
                float cos3 = fast_cos(angle3);
                float sin3 = fast_sin(angle3);
                float cos4 = fast_cos(angle4);
                float sin4 = fast_sin(angle4);
                
                real_part += core1_params.signal[n]   * cos1 + core1_params.signal[n+1] * cos2 + 
                             core1_params.signal[n+2] * cos3 + core1_params.signal[n+3] * cos4;
                imag_part += core1_params.signal[n]   * sin1 + core1_params.signal[n+1] * sin2 + 
                             core1_params.signal[n+2] * sin3 + core1_params.signal[n+3] * sin4;
                
                angle += 4.0f * neg_omega;
            }
            
            for (; n < core1_params.signal_len; n++) {
                real_part += core1_params.signal[n] * fast_cos(angle);
                imag_part += core1_params.signal[n] * fast_sin(angle);
                angle += neg_omega;
            }
            
            core1_params.output[2*k] = real_part;
            core1_params.output[2*k + 1] = imag_part;
        }
        
        // Signal completion
        __dmb();  // Ensure all writes complete before signaling done
        core1_params.done = true;
        core1_params.signal = NULL;
    }
}

void init_core1_dtft(void) {
    core1_params.signal = NULL;
    core1_params.done = true;
    multicore_launch_core1(core1_dtft_worker);
}

float compute_dtft_magnitude(uint8_t * restrict x, int N, float omega) {
    float real_part = 0.0f;
    float imag_part = 0.0f;
    float angle = 0.0f;
    const float neg_omega = -omega;
    
    // Loop unrolling (4x)
    int n = 0;
    int N_unroll = N & ~3;
    
    for (; n < N_unroll; n += 4) {
        float angle1 = angle;
        float angle2 = angle + neg_omega;
        float angle3 = angle + 2.0f * neg_omega;
        float angle4 = angle + 3.0f * neg_omega;
        
        float cos1 = fast_cos(angle1);
        float sin1 = fast_sin(angle1);
        float cos2 = fast_cos(angle2);
        float sin2 = fast_sin(angle2);
        float cos3 = fast_cos(angle3);
        float sin3 = fast_sin(angle3);
        float cos4 = fast_cos(angle4);
        float sin4 = fast_sin(angle4);
        
        real_part += x[n]   * cos1 + x[n+1] * cos2 + x[n+2] * cos3 + x[n+3] * cos4;
        imag_part += x[n]   * sin1 + x[n+1] * sin2 + x[n+2] * sin3 + x[n+3] * sin4;
        
        angle += 4.0f * neg_omega;
    }
    
    // Handle remaining samples
    for (; n < N; n++) {
        real_part += x[n] * fast_cos(angle);
        imag_part += x[n] * fast_sin(angle);
        angle += neg_omega;
    }
    
    return sqrtf(real_part * real_part + imag_part * imag_part);
}

float* calculate_dtft(uint8_t * restrict x, int N, int num_points) {
    float *magnitudes = malloc(num_points * sizeof(float));
    if (!magnitudes) {
        return NULL;
    }

    const float omega_scale = (2.0f * M_PI) / num_points;
    for (int k = 0; k < num_points; k++) {
        float omega = omega_scale * k;  // 0 to 2*pi
        magnitudes[k] = compute_dtft_magnitude(x, N, omega);
    }

    return magnitudes;
}

float* calculate_dtft_complex(uint8_t * restrict x, int N, int num_points) {
    float *complex_values = malloc(num_points * 2 * sizeof(float));
    if (!complex_values) {
        return NULL;
    }

    // Split work between cores: Core0 handles first half, Core1 handles second half
    int split_point = num_points / 2;
    
    // Launch Core1 work
    core1_params.signal_len = N;
    core1_params.start_freq = split_point;
    core1_params.end_freq = num_points;
    core1_params.num_points = num_points;
    core1_params.output = complex_values;
    core1_params.done = false;
    __dmb();  // Memory barrier before signaling
    core1_params.signal = x;  // This triggers Core1 to start
    
    // Core0 computes first half
    const float omega_scale = (2.0f * M_PI) / num_points;
    
    for (int k = 0; k < split_point; k++) {
        float real_part = 0.0f;
        float imag_part = 0.0f;
        const float omega = omega_scale * k;
        const float neg_omega = -omega;
        float angle = 0.0f;
        
        // Main loop with manual unrolling (4x) for better ILP
        int n = 0;
        int N_unroll = N & ~3;  // Round down to multiple of 4
        
        for (; n < N_unroll; n += 4) {
            // Process 4 samples at once
            float angle1 = angle;
            float angle2 = angle + neg_omega;
            float angle3 = angle + 2.0f * neg_omega;
            float angle4 = angle + 3.0f * neg_omega;
            
            float cos1 = fast_cos(angle1);
            float sin1 = fast_sin(angle1);
            float cos2 = fast_cos(angle2);
            float sin2 = fast_sin(angle2);
            float cos3 = fast_cos(angle3);
            float sin3 = fast_sin(angle3);
            float cos4 = fast_cos(angle4);
            float sin4 = fast_sin(angle4);
            
            real_part += x[n]   * cos1 + x[n+1] * cos2 + x[n+2] * cos3 + x[n+3] * cos4;
            imag_part += x[n]   * sin1 + x[n+1] * sin2 + x[n+2] * sin3 + x[n+3] * sin4;
            
            angle += 4.0f * neg_omega;
        }
        
        // Handle remaining samples
        for (; n < N; n++) {
            real_part += x[n] * fast_cos(angle);
            imag_part += x[n] * fast_sin(angle);
            angle += neg_omega;
        }
        
        complex_values[2*k] = real_part;
        complex_values[2*k + 1] = imag_part;
    }
    
    // Wait for Core1 to finish
    while (!core1_params.done) {
        __dmb();  // Memory barrier to see Core1's update
        tight_loop_contents();
    }
    __dmb();  // Ensure we see all of Core1's writes

    return complex_values;
}
