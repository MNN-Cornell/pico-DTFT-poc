#!/usr/bin/env python3
"""
Analyze reconstruction quality for only the transmitted pixels
"""
from PIL import Image
import numpy as np

# Load images
original = Image.open("image.jpg").convert('L')
reconstructed = Image.open("reconstructed_image.png").convert('L')

# Get pixel arrays
orig_array = np.array(original).flatten()
recon_array = np.array(reconstructed).flatten()

# Only compare the first 1000 transmitted pixels
num_transmitted = 1000

orig_transmitted = orig_array[:num_transmitted]
recon_transmitted = recon_array[:num_transmitted]

# Calculate metrics
correct = np.sum(orig_transmitted == recon_transmitted)
accuracy = correct / num_transmitted * 100

# Calculate error for mismatches
errors = np.abs(orig_transmitted.astype(int) - recon_transmitted.astype(int))
avg_error = np.mean(errors)
max_error = np.max(errors)

# MSE and PSNR for transmitted pixels only
mse = np.mean(errors ** 2)
psnr = 10 * np.log10(255**2 / mse) if mse > 0 else float('inf')

print("=" * 60)
print("RECONSTRUCTION QUALITY (1000 transmitted pixels only)")
print("=" * 60)
print(f"Pixels transmitted: {num_transmitted}")
print(f"Exact matches: {correct}/{num_transmitted} ({accuracy:.2f}%)")
print(f"Average error: {avg_error:.2f} grayscale levels")
print(f"Maximum error: {max_error} grayscale levels")
print(f"MSE: {mse:.4f}")
print(f"PSNR: {psnr:.2f} dB")
print("=" * 60)

# Show some examples
print("\nFirst 20 pixels comparison:")
print("Pixel | Original | Reconstructed | Match")
print("-" * 50)
for i in range(min(20, num_transmitted)):
    match = "✓" if orig_transmitted[i] == recon_transmitted[i] else "✗"
    print(f"{i:5d} | 0x{orig_transmitted[i]:02X} ({orig_transmitted[i]:3d}) | "
          f"0x{recon_transmitted[i]:02X} ({recon_transmitted[i]:3d}) | {match}")

# Count mismatches by error magnitude
error_bins = [0, 0, 0, 0, 0]  # 0, 1-5, 6-10, 11-20, >20
for e in errors:
    if e == 0:
        error_bins[0] += 1
    elif e <= 5:
        error_bins[1] += 1
    elif e <= 10:
        error_bins[2] += 1
    elif e <= 20:
        error_bins[3] += 1
    else:
        error_bins[4] += 1

print(f"\nError distribution:")
print(f"  Exact match (0): {error_bins[0]} ({error_bins[0]/num_transmitted*100:.1f}%)")
print(f"  Close (1-5): {error_bins[1]} ({error_bins[1]/num_transmitted*100:.1f}%)")
print(f"  Medium (6-10): {error_bins[2]} ({error_bins[2]/num_transmitted*100:.1f}%)")
print(f"  Large (11-20): {error_bins[3]} ({error_bins[3]/num_transmitted*100:.1f}%)")
print(f"  Very large (>20): {error_bins[4]} ({error_bins[4]/num_transmitted*100:.1f}%)")
