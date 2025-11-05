#!/usr/bin/env python3
"""
Convert image.jpg to grayscale 8-bit C header file
"""
from PIL import Image
import sys

def convert_image_to_header(input_path, output_path):
    # Load image
    img = Image.open(input_path)
    
    # Convert to grayscale
    img_gray = img.convert('L')
    
    # Get dimensions
    width, height = img_gray.size
    
    # Get pixel data as 8-bit values (0-255)
    pixels = list(img_gray.getdata())
    
    # Write C header file
    with open(output_path, 'w') as f:
        f.write("// Auto-generated from image.jpg\n")
        f.write("#ifndef IMAGE_DATA_H\n")
        f.write("#define IMAGE_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define IMAGE_WIDTH {width}\n")
        f.write(f"#define IMAGE_HEIGHT {height}\n")
        f.write(f"#define IMAGE_SIZE {len(pixels)}\n\n")
        f.write("// Grayscale pixel data (8-bit per pixel)\n")
        f.write("const uint8_t image_data[IMAGE_SIZE] = {\n")
        
        # Write pixels in rows of 16 for readability
        for i in range(0, len(pixels), 16):
            row = pixels[i:i+16]
            f.write("    ")
            f.write(", ".join(f"0x{p:02X}" for p in row))
            if i + 16 < len(pixels):
                f.write(",")
            f.write("\n")
        
        f.write("};\n\n")
        f.write("#endif // IMAGE_DATA_H\n")
    
    print(f"Converted {input_path} to {output_path}")
    print(f"Image dimensions: {width}x{height} = {len(pixels)} pixels")
    print(f"Grayscale 8-bit values (0-255)")

if __name__ == "__main__":
    input_file = "image.jpg"
    output_file = "lib/image_data.h"
    
    try:
        convert_image_to_header(input_file, output_file)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
