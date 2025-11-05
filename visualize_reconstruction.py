#!/usr/bin/env python3
"""
Visualize reconstructed image from Pico output
Reads the original image and reconstructed pixel values to create comparison
"""
from PIL import Image
import numpy as np
import sys

def create_reconstructed_image(original_path, reconstructed_pixels, output_path):
    """
    Create reconstructed image from pixel values
    
    Args:
        original_path: Path to original image.jpg
        reconstructed_pixels: List of reconstructed pixel values (0-255)
        output_path: Where to save the reconstructed image
    """
    # Load original image to get dimensions
    img_original = Image.open(original_path).convert('L')
    width, height = img_original.size
    
    # Create array for reconstructed image
    num_pixels = len(reconstructed_pixels)
    reconstructed_array = np.array(reconstructed_pixels, dtype=np.uint8)
    
    # If we have fewer pixels than the full image, create a partial reconstruction
    if num_pixels < width * height:
        # Fill remaining with zeros (black) or copy from original
        full_array = np.zeros(width * height, dtype=np.uint8)
        full_array[:num_pixels] = reconstructed_array
    else:
        full_array = reconstructed_array[:width * height]
    
    # Reshape to 2D image
    img_reconstructed = Image.fromarray(full_array.reshape(height, width), mode='L')
    
    # Save reconstructed image
    img_reconstructed.save(output_path)
    print(f"Saved reconstructed image to {output_path}")
    
    # Create side-by-side comparison if we have enough pixels
    if num_pixels >= width:  # At least one row
        rows_reconstructed = min(num_pixels // width, height)
        
        # Crop both images to the reconstructed region
        original_crop = img_original.crop((0, 0, width, rows_reconstructed))
        reconstructed_crop = img_reconstructed.crop((0, 0, width, rows_reconstructed))
        
        # Create side-by-side comparison
        comparison = Image.new('L', (width * 2, rows_reconstructed))
        comparison.paste(original_crop, (0, 0))
        comparison.paste(reconstructed_crop, (width, 0))
        
        comparison_path = output_path.replace('.png', '_comparison.png')
        comparison.save(comparison_path)
        print(f"Saved comparison to {comparison_path}")
        
    # Calculate metrics
    original_array = np.array(img_original).flatten()
    mse = np.mean((original_array[:num_pixels] - reconstructed_array) ** 2)
    psnr = 10 * np.log10(255**2 / mse) if mse > 0 else float('inf')
    accuracy = np.sum(original_array[:num_pixels] == reconstructed_array) / num_pixels * 100
    
    print(f"\nReconstruction Metrics:")
    print(f"  Pixels reconstructed: {num_pixels}")
    print(f"  Accuracy (exact match): {accuracy:.2f}%")
    print(f"  MSE: {mse:.4f}")
    print(f"  PSNR: {psnr:.2f} dB")

def parse_pico_output(output_file):
    """
    Parse Pico serial output to extract reconstructed pixel values
    Expected format: "Pixel[i]: Original=0xXX, Reconstructed=0xYY"
    """
    reconstructed_pixels = []
    
    with open(output_file, 'r') as f:
        for line in f:
            if 'Reconstructed=0x' in line:
                # Extract hex value after "Reconstructed=0x"
                parts = line.split('Reconstructed=0x')
                if len(parts) > 1:
                    hex_val = parts[1].split()[0]
                    pixel_val = int(hex_val, 16)
                    reconstructed_pixels.append(pixel_val)
    
    return reconstructed_pixels

def manual_input_mode():
    """
    Manually input reconstructed pixel values
    """
    print("Manual input mode")
    print("Enter reconstructed pixel values (hex, e.g., D3) one per line.")
    print("Enter 'done' when finished.")
    
    reconstructed_pixels = []
    while True:
        line = input(f"Pixel[{len(reconstructed_pixels)}]: ").strip()
        if line.lower() == 'done':
            break
        try:
            # Try parsing as hex
            if line.startswith('0x'):
                pixel_val = int(line, 16)
            else:
                pixel_val = int(line, 16)  # Assume hex without 0x prefix
            
            if 0 <= pixel_val <= 255:
                reconstructed_pixels.append(pixel_val)
            else:
                print("Value must be 0-255")
        except ValueError:
            print("Invalid input, enter hex value (e.g., D3 or 0xD3)")
    
    return reconstructed_pixels

if __name__ == "__main__":
    original_image = "image.jpg"
    
    if len(sys.argv) > 1:
        # Parse from file
        output_file = sys.argv[1]
        print(f"Parsing reconstructed pixels from {output_file}")
        reconstructed_pixels = parse_pico_output(output_file)
    else:
        # Manual input or demo mode
        print("No input file specified.")
        print("Usage: python3 visualize_reconstruction.py <pico_output.txt>")
        print("\nDemo mode: Using first 100 pixels of original as 'reconstructed' for testing")
        
        img = Image.open(original_image).convert('L')
        reconstructed_pixels = list(img.getdata())[:100]
    
    if len(reconstructed_pixels) == 0:
        print("No reconstructed pixels found!")
        sys.exit(1)
    
    print(f"Found {len(reconstructed_pixels)} reconstructed pixels")
    
    output_image = "reconstructed_image.png"
    create_reconstructed_image(original_image, reconstructed_pixels, output_image)
