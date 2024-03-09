#include <iostream>

#include <vector>
#include <cstring>

// file read
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

// parallelization
#include <omp.h>

#include "timer.h"

// PNG chunk headers
#define PNG_HEADER {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}
#define IHDR_HEADER {0x49, 0x48, 0x44, 0x52}
#define IDAT_HEADER {0x49, 0x44, 0x41, 0x54}
#define IEND_HEADER {0x49, 0x45, 0x4e, 0x44}

struct RGBPixel
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

int headersMatch(unsigned char header[], std::string headerType) {
    if (headerType == "PNG") {
        unsigned char pngHeader[8] = PNG_HEADER;
        return std::memcmp(header, pngHeader, 8);

    } else if (headerType == "IHDR") {
        unsigned char ihdrHeader[4] = IHDR_HEADER;
        return std::memcmp(header, ihdrHeader, 4);

    } else if (headerType == "IDAT") {
        unsigned char idatHeader[4] = IDAT_HEADER;
        return std::memcmp(header, idatHeader, 4);

    } else if (headerType == "IEND") {
        unsigned char iendHeader[4] = IEND_HEADER;
        return std::memcmp(header, iendHeader, 4);

    } else {
        std::cerr << "Invalid header supplied" << std::endl;
        exit(EXIT_FAILURE);
    }
}

bool readPNGImage(const char *filename, std::vector<RGBPixel> &pixels, int &width, int &height)
{
    int fd = open(filename, O_RDONLY);

    if (fd == -1)
    {
        std::cerr << "Error opening image file" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Check PNG and read file signature (first 8 bytes)
    unsigned char png_signature[8] = PNG_HEADER;
    unsigned char header[8];

    if (pread(fd, header, 8, 0) != 8 || std::memcmp(header, png_signature, 8) != 0)
    {
        std::cerr << "Mismatching image headers" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Read IHDR chunk to extract image dimensions
    char ihdr_chunk[25]; // IHDR chunk is 25 bytes
    if (pread(fd, ihdr_chunk, 25, 0) != 25)
    {
        std::cerr << "Error reading IHDR chunk" << std::endl;
        exit(EXIT_FAILURE);
    }

    // use bit arithmetic to cherry pick bytes containing width/height data
    width = (ihdr_chunk[3] << 24) | (ihdr_chunk[4] << 16) | (ihdr_chunk[5] << 8) | ihdr_chunk[6];
    height = (ihdr_chunk[7] << 24) | (ihdr_chunk[8] << 16) | (ihdr_chunk[9] << 8) | ihdr_chunk[10];

    // Allocate memory for pixel data
    pixels.resize(width * height);

    long int fileSize;
    if ((fileSize = lseek(fd, 0, SEEK_END)) == -1)
    {
        std::cerr << "Failed to get file size" << std::endl;
        exit(EXIT_FAILURE);
    }

    printf("Image w: %d, h: %d\nFile size: %ld\n", width, height, fileSize);
    printf("Beginning reading Image data.\n");

    // Read pixel data assuming RGB format
    bool failure = false;
    // #pragma omp parallel for shared(failure)
    for (int i = 33; i < fileSize; i += sizeof(RGBPixel))
    {
        if (failure)
        {
            continue;
        }

        // Calculate offset for current pixel
        off_t offset = i;

        if (offset + sizeof(RGBPixel) >= fileSize)
        {
            continue;
        }

        std::cout << offset << std::endl;

        // Read pixel data (RGB) from file at the calculated offset
        if (pread(fd, &pixels[i], sizeof(RGBPixel), offset) != sizeof(RGBPixel))
        {
            std::cerr << "Error: Unable to read pixel data." << std::endl;
            failure = true;
        }

        // Skip alpha channel if present (assuming RGB format)
        // Update offset to skip one byte for alpha channel
        offset++;
        lseek(fd, offset, SEEK_SET);
    }

    close(fd);
    if (failure)
        exit(EXIT_FAILURE);

    return true;
}

int main()
{
    double start, end;

    const char *filename = "./test-images/green-apple.png"; // Replace with your PNG image file path
    std::vector<RGBPixel> pixels;
    int width, height;

    GET_TIME(start);
    bool res = readPNGImage(filename, pixels, width, height);
    GET_TIME(end);

    if (!res)
    {
        printf("Image reading failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Image loaded\nLoading took %fs / %fms\n", (end - start), (end - start) * 1000);
    std::cout << "First 10 pixel values: ";
    for (int i = 0; i < 10; ++i)
    {
        std::cout << static_cast<int>(pixels[i].red) << " "
                  << static_cast<int>(pixels[i].green) << " "
                  << static_cast<int>(pixels[i].blue) << " ";
    }
    std::cout << std::endl;

    return 0;
}
