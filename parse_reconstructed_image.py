#!/usr/bin/env python3
"""
Parse structured reconstructed image data from Pico serial output
Creates PNG image from the hex data block
"""
from PIL import Image
import numpy as np
import sys
import re

def parse_image_data(input_file):
    """
    Parse the structured IMAGE_DATA block from serial output
    """
    with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Find the IMAGE_DATA block
    match = re.search(r'IMAGE_DATA_START\s+WIDTH=(\d+)\s+HEIGHT=(\d+)\s+PIXELS=(\d+)\s+DATA_HEX\s+(.*?)\s+IMAGE_DATA_END', 
                     content, re.DOTALL)
    
    if not match:
        print("Error: Could not find IMAGE_DATA_START...IMAGE_DATA_END block")
        return None, None, None
    
    width = int(match.group(1))
    height = int(match.group(2))
    num_pixels = int(match.group(3))
    hex_data = match.group(4)
    
    # Parse hex values
    hex_values = re.findall(r'[0-9A-Fa-f]{2}', hex_data)
    
    if len(hex_values) != num_pixels:
        print(f"Warning: Expected {num_pixels} pixels, found {len(hex_values)}")
    
    # Convert to pixel array
    pixels = [int(h, 16) for h in hex_values]
    
    return width, height, pixels

def create_image(width, height, pixels, output_path):
    """
    Create PNG image from pixel data
    """
    # Create array
    num_pixels = len(pixels)
    pixel_array = np.array(pixels, dtype=np.uint8)
    
    # If we have fewer pixels than a full image, pad with black
    if num_pixels < width * height:
        full_array = np.zeros(width * height, dtype=np.uint8)
        full_array[:num_pixels] = pixel_array
    else:
        full_array = pixel_array[:width * height]
    
    # Reshape to 2D
    img_array = full_array.reshape(height, width)
    
    # Create and save image
    img = Image.fromarray(img_array, mode='L')
    img.save(output_path)
    
    print(f"Created reconstructed image: {output_path}")
    print(f"Dimensions: {width}x{height}")
    print(f"Pixels: {num_pixels}")
    
    return img

def create_comparison(original_path, reconstructed_img, output_path):
    """
    Create side-by-side comparison
    """
    try:
        img_original = Image.open(original_path).convert('L')
        
        # Resize original to match reconstructed if needed
        if img_original.size != reconstructed_img.size:
            img_original = img_original.resize(reconstructed_img.size)
        
        # Create side-by-side
        width, height = reconstructed_img.size
        comparison = Image.new('L', (width * 2, height))
        comparison.paste(img_original, (0, 0))
        comparison.paste(reconstructed_img, (width, 0))
        
        comparison.save(output_path)
        print(f"Created comparison: {output_path}")
        
        # Calculate metrics
        original_array = np.array(img_original).flatten()
        reconstructed_array = np.array(reconstructed_img).flatten()
        
        num_pixels = min(len(original_array), len(reconstructed_array))
        correct = np.sum(original_array[:num_pixels] == reconstructed_array[:num_pixels])
        accuracy = correct / num_pixels * 100
        
        mse = np.mean((original_array[:num_pixels] - reconstructed_array[:num_pixels]) ** 2)
        psnr = 10 * np.log10(255**2 / mse) if mse > 0 else float('inf')
        
        print(f"\nQuality Metrics:")
        print(f"  Accuracy: {correct}/{num_pixels} ({accuracy:.2f}%)")
        print(f"  MSE: {mse:.4f}")
        print(f"  PSNR: {psnr:.2f} dB")
        
    except Exception as e:
        print(f"Could not create comparison: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 parse_reconstructed_image.py <pico_output.txt>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    print(f"Parsing {input_file}...")
    width, height, pixels = parse_image_data(input_file)
    
    if pixels is None:
        print("\nMake sure your serial output contains the structured data block:")
        print("  IMAGE_DATA_START")
        print("  WIDTH=180")
        print("  HEIGHT=180")
        print("  PIXELS=32400")
        print("  DATA_HEX")
        print("  D3 D4 D5 ...")
        print("  IMAGE_DATA_END")
        sys.exit(1)
    
    # Create reconstructed image
    reconstructed_img = create_image(width, height, pixels, "reconstructed_image.png")
    
    # Create comparison with original
    create_comparison("image.jpg", reconstructed_img, "reconstructed_comparison.png")
    
    print("\n✓ Done! Check the generated images.")
