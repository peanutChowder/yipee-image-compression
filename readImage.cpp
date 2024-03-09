#include <iostream>
#include <fstream>
#include <vector>
#include <cstring> 

struct RGBPixel {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};


bool readPNGImage(const char* filename, std::vector<RGBPixel>& pixels, int& width, int& height) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open image file." << std::endl;
        return false;
    }

    // Check PNG file signature (first 8 bytes)
    unsigned char png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    unsigned char header[8];
    file.read(reinterpret_cast<char*>(header), 8);
    if (std::memcmp(header, png_signature, 8) != 0) {
        std::cerr << "Error: Invalid PNG file format." << std::endl;
        return false;
    }

    // Read IHDR chunk to extract image dimensions
    char ihdr_chunk[25]; // IHDR chunk is 25 bytes
    file.read(ihdr_chunk, 25);

    // use bit arithmetic to cherry pick bytes containing width/height data
    width = (ihdr_chunk[3] << 24) | (ihdr_chunk[4] << 16) | (ihdr_chunk[5] << 8) | ihdr_chunk[6];
    height = (ihdr_chunk[7] << 24) | (ihdr_chunk[8] << 16) | (ihdr_chunk[9] << 8) | ihdr_chunk[10];

    // Move to the beginning of pixel data
    // Start of pixel data after IHDR chunk (25 bytes) + CRC (4 bytes) + IDAT chunk header (4 bytes)
    // Start reading at image size data
    file.seekg(33); 

    // Allocate memory for pixel data
    pixels.resize(width * height);

    // Read pixel data (assuming RGB format)
    for (int i = 0; i < width * height; ++i) {
        file.read(reinterpret_cast<char*>(&pixels[i].red), sizeof(pixels[i].red));
        file.read(reinterpret_cast<char*>(&pixels[i].green), sizeof(pixels[i].green));
        file.read(reinterpret_cast<char*>(&pixels[i].blue), sizeof(pixels[i].blue));
        // Skip alpha channel if present (assuming RGB format)
        file.seekg(1, std::ios::cur);
    }

    return true;
}

int main() {
    const char* filename = "./test-images/green-apple.png"; // Replace with your PNG image file path
    std::vector<RGBPixel> pixels;
    int width, height;

    if (readPNGImage(filename, pixels, width, height)) {
        // Example: Print the first 10 pixel values
        std::cout << "First 10 pixel values: ";
        for (int i = 0; i < 10; ++i) {
            std::cout << static_cast<int>(pixels[i].red) << " "
                      << static_cast<int>(pixels[i].green) << " "
                      << static_cast<int>(pixels[i].blue) << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
