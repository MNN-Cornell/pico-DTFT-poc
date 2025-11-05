%% Inverse DTFT with Phase Information (0 to π range)
% This script takes DTFT complex values (real + imaginary) and recovers the original bit pattern
% Using phase information gives much better reconstruction than magnitude-only
%
% NOTE: For real-valued signals, DTFT has conjugate symmetry, so we only need
% frequencies from 0 to π. This script handles 41 frequency points (0 to π).

clear all; close all;

% ========== INPUT: DTFT Complex Values ==========
% Copy dtft_real and dtft_imag arrays from Pico output
% Look for: ========== DTFT COMPLEX VALUES FOR MATLAB ==========
dtft_real = [
    50.000000;
    0.000002;
    0.000000;
    0.000001;
    0.000003;
    -0.000001;
    -0.000002;
    -0.000001;
    -0.000003;
    -0.000003;
    -2.928916;
    0.000002;
    0.000001;
    -0.000003;
    0.000004;
    -0.000019;
    0.000004;
    -0.000022;
    -0.000001;
    0.000001;
    -0.000009;
    -0.000006;
    0.000002;
    0.000012;
    -0.000002;
    -0.000008;
    -0.000007;
    -0.000009;
    -0.000001;
    -0.000002;
    -17.071075;
    0.000012;
    -0.000013;
    0.000005;
    0.000028;
    0.000000;
    0.000017;
    0.000001;
    -0.000007;
    -0.000023;
    -10.000000
];
dtft_imag = [
    0.000000;
    0.000001;
    0.000001;
    0.000000;
    0.000005;
    0.000000;
    -0.000001;
    0.000002;
    -0.000004;
    0.000001;
    17.071070;
    0.000002;
    0.000004;
    0.000021;
    -0.000015;
    -0.000001;
    -0.000009;
    -0.000003;
    -0.000005;
    -0.000007;
    -10.000000;
    -0.000002;
    -0.000003;
    0.000014;
    -0.000016;
    0.000001;
    -0.000015;
    0.000000;
    0.000011;
    -0.000029;
    -2.928931;
    0.000014;
    0.000003;
    0.000002;
    -0.000024;
    -0.000001;
    -0.000005;
    -0.000001;
    -0.000002;
    0.000019;
    0.000005
];
% ========== PARAMETERS ==========
N = 80;  % Total signal length (8 bits * 10 repetitions)
num_freq_points = 41;  % Number of frequency points (0 to π only)
pattern_len = 8;  % Original pattern length in bits

% ========== INVERSE DTFT WITH PHASE (0 to π range) ==========
% Reconstruct signal using inverse DTFT formula with complex values
% For real signals, we only need 0 to π frequencies due to conjugate symmetry
% x[n] = DC/N + (2/N) * sum(Re{X[k] * exp(j*omega*n)}) for k=1 to π

recovered_signal = zeros(1, N);

for n = 0:N-1
    % DC component (k=0)
    recovered_signal(n+1) = dtft_real(1);
    
    % Sum contributions from k=1 to 40 (frequencies π/40 to π)
    for k = 1:num_freq_points-1
        % Frequency from 0 to π
        omega = (pi * k) / (num_freq_points - 1);
        
        % Complex exponential: exp(j*omega*n) = cos(omega*n) + j*sin(omega*n)
        cos_part = cos(omega * n);
        sin_part = sin(omega * n);
        
        % Multiply DTFT value (real + j*imag) by complex exponential
        % (a + j*b) * (c + j*d) = (a*c - b*d) + j*(a*d + b*c)
        real_product = dtft_real(k+1) * cos_part - dtft_imag(k+1) * sin_part;
        
        % For 0 to π range, multiply by 2 (since we're using half the spectrum)
        recovered_signal(n+1) = recovered_signal(n+1) + 2 * real_product;
    end
end

% Normalize by signal length
recovered_signal = recovered_signal / N;

% ========== EXTRACT BITS ==========
% Threshold to convert to binary (0 or 1)
threshold = max(recovered_signal) / 2;
recovered_bits = recovered_signal > threshold;

% Extract the first pattern
first_pattern = recovered_bits(1:pattern_len);

% ========== OUTPUT RESULTS ==========
fprintf('\n========== INVERSE DTFT RESULTS (WITH PHASE) ==========\n');
fprintf('Recovered signal (first 16 samples):\n');
for i = 1:min(16, length(recovered_signal))
    fprintf('%.4f  ', recovered_signal(i));
end
fprintf('\n\n');

fprintf('Recovered bits (first 16 samples):\n');
for i = 1:min(16, length(recovered_bits))
    fprintf('%d', recovered_bits(i));
end
fprintf('\n\n');

fprintf('Extracted first pattern (%d bits):\n', pattern_len);
pattern_str = sprintf('%d', first_pattern);
fprintf('%s\n', pattern_str);

% Convert to decimal
pattern_decimal = bin2dec(pattern_str);
fprintf('\nPattern in decimal: %d\n', pattern_decimal);
fprintf('Pattern in binary: 0b%s\n', pattern_str);

fprintf('======================================================\n\n');

% ========== VISUALIZATION ==========
figure('Position', [100, 100, 1400, 500]);

% Plot 1: Recovered signal
subplot(1, 4, 1);
plot(recovered_signal, 'b-', 'LineWidth', 1.5);
hold on;
plot(recovered_bits * max(recovered_signal), 'r--', 'LineWidth', 1);
xlabel('Sample');
ylabel('Amplitude');
title('Recovered Signal vs Thresholded Bits');
legend('Signal', 'Thresholded Bits');
grid on;

% Plot 2: DTFT Magnitude Spectrum (0 to π)
subplot(1, 4, 2);
magnitude = sqrt(dtft_real.^2 + dtft_imag.^2);
omega_axis = (pi * (0:length(dtft_real)-1)) / (length(dtft_real) - 1);  % 0 to π
stem(omega_axis, magnitude, 'b', 'filled');
xlabel('Frequency (rad/sample)');
ylabel('Magnitude');
title('DTFT Magnitude Spectrum (0 to π)');
xlim([0 pi]);
xticks([0 pi/4 pi/2 3*pi/4 pi]);
xticklabels({'0', 'π/4', 'π/2', '3π/4', 'π'});
grid on;

% Plot 3: DTFT Phase Spectrum (0 to π)
subplot(1, 4, 3);
phase = atan2(dtft_imag, dtft_real);
stem(omega_axis, phase, 'g', 'filled');
xlabel('Frequency (rad/sample)');
ylabel('Phase (radians)');
title('DTFT Phase Spectrum (0 to π)');
xlim([0 pi]);
xticks([0 pi/4 pi/2 3*pi/4 pi]);
xticklabels({'0', 'π/4', 'π/2', '3π/4', 'π'});
grid on;

% Plot 4: First pattern zoomed
subplot(1, 4, 4);
plot(1:pattern_len, recovered_signal(1:pattern_len), 'bo-', 'LineWidth', 2, 'MarkerSize', 8);
hold on;
plot(1:pattern_len, recovered_bits(1:pattern_len) * max(recovered_signal), 'r^--', 'LineWidth', 2, 'MarkerSize', 8);
xlabel('Bit Index');
ylabel('Amplitude');
title(sprintf('First Pattern (%d bits)', pattern_len));
legend('Signal', 'Recovered Bits');
grid on;
xticks(1:pattern_len);

sgtitle('Inverse DTFT Analysis with Phase Information (0 to π)');
