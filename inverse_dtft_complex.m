%% Inverse DTFT with Phase Information
% This script takes DTFT complex values (real + imaginary) and recovers the original bit pattern
% Using phase information gives much better reconstruction than magnitude-only

clear all; close all;

% ========== INPUT: DTFT Complex Values ==========
% Copy dtft_real and dtft_imag arrays from Pico output
% Look for: ========== DTFT COMPLEX VALUES FOR MATLAB ==========
dtft_real = [
    40.000000;
    -5.422434;
    6.023712;
    -2.058626;
    2.036586;
    1.679697;
    -0.467443;
    2.764438;
    -0.039930;
    1.408290;
    1.915181;
    0.116517;
    2.868337;
    0.475534;
    2.701977;
    4.374987;
    10.007401;
    -4.929978;
    1.394233;
    -0.922329;
    0.673270;
    0.775013;
    -0.137637;
    1.735470;
    0.009657;
    1.186429;
    1.604222;
    0.064486;
    3.052884;
    0.747214;
    2.519299;
    9.244361;
    0.123008;
    -8.509957;
    -2.076949;
    -0.600349;
    -2.228182;
    -0.164682;
    -0.687702;
    -0.983291;
    0.017925;
    -0.722210;
    -0.697838;
    0.139742;
    -1.210630;
    -0.760764;
    -0.120195;
    -5.852535;
    9.922716;
    2.932602;
    2.813079;
    -0.374332;
    1.719236;
    0.591828;
    0.265115;
    1.318630;
    0.000300;
    0.780383;
    0.751303;
    0.057017;
    1.077437;
    0.163778;
    0.403441;
    0.843289;
    0.001205;
    0.917053;
    0.452702;
    0.135417;
    0.990126;
    0.065376;
    0.742118;
    0.755304;
    0.003766;
    1.317665;
    0.158298;
    0.605029;
    1.647698;
    -0.333288;
    2.827259;
    2.720531;
    10.111462;
    -5.942259;
    0.063394;
    -0.715906;
    -1.182810;
    0.095742;
    -0.599018;
    -0.706546;
    0.030453;
    -1.000590;
    -0.689316;
    -0.216782;
    -2.150978;
    -0.521126;
    -2.128614;
    -8.380008;
    -0.292989;
    9.275308;
    2.435093;
    0.909170;
    2.974222;
    0.043874;
    1.667245;
    1.194965;
    0.017440;
    1.656046;
    -0.156447;
    0.767419;
    0.546499;
    -0.926761;
    1.417182;
    -5.058693;
    9.751877;
    4.544383;
    2.669701;
    0.471888;
    2.786772;
    0.032920;
    1.899643;
    1.331977;
    0.030744;
    2.682383;
    -0.495274;
    1.556140;
    2.088917;
    -2.088140;
    5.897459;
    -5.644099
];
dtft_imag = [
    0.000000;
    -18.146128;
    -4.008826;
    -1.776041;
    -4.854614;
    0.219140;
    -2.611954;
    -1.311207;
    -0.008453;
    -2.225770;
    0.634466;
    -1.034060;
    -0.419568;
    1.233620;
    -1.487829;
    5.548453;
    -10.017267;
    -4.161850;
    -2.162972;
    -0.467187;
    -2.113309;
    0.050647;
    -1.440134;
    -0.644233;
    -0.123278;
    -1.615293;
    0.603788;
    -1.069500;
    -0.703446;
    1.148177;
    -3.134638;
    3.385790;
    -20.023033;
    2.890989;
    -2.526685;
    0.851891;
    -0.216800;
    -0.568578;
    0.569961;
    -0.382051;
    0.070118;
    0.553859;
    -0.297777;
    0.505020;
    0.926743;
    -0.322576;
    2.376705;
    2.430751;
    9.954172;
    -6.366350;
    0.117599;
    -0.990472;
    -1.307070;
    0.279711;
    -1.094055;
    -0.158796;
    -0.024542;
    -0.836419;
    0.321533;
    -0.307700;
    -0.269240;
    0.372185;
    -0.508035;
    0.311045;
    -0.098166;
    -0.300575;
    0.546608;
    -0.272431;
    0.203288;
    0.400003;
    -0.381453;
    0.789054;
    0.024927;
    0.165849;
    1.004081;
    -0.337604;
    1.426276;
    0.869988;
    -0.067540;
    6.466654;
    -9.910619;
    -2.474895;
    -2.325184;
    0.340741;
    -0.993719;
    -0.453237;
    0.285312;
    -0.592382;
    0.015087;
    0.335017;
    -0.658425;
    0.578622;
    0.143017;
    -0.988228;
    2.482330;
    -3.269865;
    19.873680;
    -3.316712;
    3.137130;
    -1.288470;
    0.830072;
    0.998475;
    -0.497088;
    1.635682;
    -0.006975;
    0.721942;
    1.358837;
    -0.078586;
    2.140440;
    0.381015;
    2.188906;
    4.109187;
    10.217858;
    -5.582522;
    1.522896;
    -1.215680;
    0.479417;
    1.106706;
    -0.477638;
    2.253942;
    -0.110173;
    1.385547;
    2.550409;
    -0.139624;
    4.890352;
    1.664829;
    4.179062;
    17.918648
];
% ========== PARAMETERS ==========
N = 80;  % Total signal length (8 bits * 10 repetitions)
num_freq_points = 128;  % Number of frequency points
pattern_len = 8;  % Original pattern length in bits

% ========== INVERSE DTFT WITH PHASE ==========
% Reconstruct signal using inverse DTFT formula with complex values
% x[n] = (1/N) * sum(X[k] * exp(j*2*pi*k*n/N)) for k=0 to N-1

recovered_signal = zeros(1, N);

for n = 0:N-1
    for k = 0:num_freq_points-1
        % Frequency
        omega = (2*pi * k) / num_freq_points;
        
        % Complex exponential: exp(j*omega*n) = cos(omega*n) + j*sin(omega*n)
        cos_part = cos(omega * n);
        sin_part = sin(omega * n);
        
        % Multiply DTFT value (real + j*imag) by complex exponential
        % (a + j*b) * (c + j*d) = (a*c - b*d) + j*(a*d + b*c)
        real_product = dtft_real(k+1) * cos_part - dtft_imag(k+1) * sin_part;
        
        % Sum only the real part
        recovered_signal(n+1) = recovered_signal(n+1) + real_product;
    end
end

% Normalize
recovered_signal = recovered_signal / num_freq_points;

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

% Plot 2: DTFT Magnitude Spectrum
subplot(1, 4, 2);
magnitude = sqrt(dtft_real.^2 + dtft_imag.^2);
omega_axis = (2*pi * (0:length(dtft_real)-1)) / length(dtft_real);
stem(omega_axis, magnitude, 'b', 'filled');
xlabel('Frequency (rad/sample)');
ylabel('Magnitude');
title('DTFT Magnitude Spectrum');
grid on;

% Plot 3: DTFT Phase Spectrum
subplot(1, 4, 3);
phase = atan2(dtft_imag, dtft_real);
stem(omega_axis, phase, 'g', 'filled');
xlabel('Frequency (rad/sample)');
ylabel('Phase (radians)');
title('DTFT Phase Spectrum');
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

sgtitle('Inverse DTFT Analysis with Phase Information');
