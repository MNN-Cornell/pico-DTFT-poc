#!/usr/bin/env python3
"""
Extract reconstructed pixel values from Pico serial output
Saves to a binary file for later processing
"""
import sys
import re

def extract_pixels_from_output(input_file):
    """
    Extract reconstructed pixel values from serial output
    """
    original_pixels = []
    reconstructed_pixels = []
    
    with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            # Look for lines like: "Pixel[0]: Original=0xD3, Reconstructed=0xD3"
            match = re.search(r'Pixel\[(\d+)\]:\s*Original=0x([0-9A-Fa-f]{2}),\s*Reconstructed=0x([0-9A-Fa-f]{2})', line)
            if match:
                index = int(match.group(1))
                original = int(match.group(2), 16)
                reconstructed = int(match.group(3), 16)
                
                original_pixels.append((index, original))
                reconstructed_pixels.append((index, reconstructed))
    
    return original_pixels, reconstructed_pixels

def save_pixels_binary(pixels, output_file):
    """
    Save pixel values to binary file
    """
    with open(output_file, 'wb') as f:
        for _, pixel_value in pixels:
            f.write(bytes([pixel_value]))

def save_pixels_text(pixels, output_file):
    """
    Save pixel values to text file (one per line, hex format)
    """
    with open(output_file, 'w') as f:
        for index, pixel_value in pixels:
            f.write(f"{index}: 0x{pixel_value:02X}\n")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 extract_pixels.py <pico_output.txt>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    print(f"Extracting pixels from {input_file}...")
    original_pixels, reconstructed_pixels = extract_pixels_from_output(input_file)
    
    if len(reconstructed_pixels) == 0:
        print("No pixel data found in file!")
        print("Make sure the file contains lines like:")
        print("  Pixel[0]: Original=0xD3, Reconstructed=0xD3")
        sys.exit(1)
    
    print(f"Found {len(reconstructed_pixels)} reconstructed pixels")
    
    # Save in multiple formats
    save_pixels_binary(reconstructed_pixels, "reconstructed_pixels.bin")
    save_pixels_text(reconstructed_pixels, "reconstructed_pixels.txt")
    save_pixels_binary(original_pixels, "original_pixels.bin")
    save_pixels_text(original_pixels, "original_pixels.txt")
    
    print(f"\nSaved files:")
    print(f"  reconstructed_pixels.bin - Binary format")
    print(f"  reconstructed_pixels.txt - Text format (hex)")
    print(f"  original_pixels.bin - Original pixel values")
    print(f"  original_pixels.txt - Original pixel values (hex)")
    
    # Calculate accuracy
    correct = sum(1 for i in range(len(reconstructed_pixels)) 
                  if original_pixels[i][1] == reconstructed_pixels[i][1])
    accuracy = correct / len(reconstructed_pixels) * 100
    
    print(f"\nAccuracy: {correct}/{len(reconstructed_pixels)} ({accuracy:.2f}%)")
    
    # Calculate average error
    total_error = sum(abs(original_pixels[i][1] - reconstructed_pixels[i][1]) 
                      for i in range(len(reconstructed_pixels)))
    avg_error = total_error / len(reconstructed_pixels)
    
    print(f"Average error: {avg_error:.2f} grayscale levels")
