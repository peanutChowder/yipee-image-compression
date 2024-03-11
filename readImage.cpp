#include <iostream>

#include <vector>
#include <cstring>

// file read
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

// parallelization
#include <omp.h>

#include <iomanip>

#include "timer.h"

// PNG chunk headers
#define PNG_HEADER                                     \
    {                                                  \
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a \
    }
#define IHDR_HEADER            \
    {                          \
        0x49, 0x48, 0x44, 0x52 \
    }
#define IDAT_HEADER            \
    {                          \
        0x49, 0x44, 0x41, 0x54 \
    }
#define IEND_HEADER            \
    {                          \
        0x49, 0x45, 0x4e, 0x44 \
    }

struct RGBPixel
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

int compareHeaders(unsigned char header[], std::string headerType)
{
    if (headerType == "PNG")
    {
        unsigned char pngHeader[8] = PNG_HEADER;
        return std::memcmp(header, pngHeader, 8);
    }
    else if (headerType == "IHDR")
    {
        unsigned char ihdrHeader[4] = IHDR_HEADER;
        return std::memcmp(header, ihdrHeader, 4);
    }
    else if (headerType == "IDAT")
    {
        unsigned char idatHeader[4] = IDAT_HEADER;
        return std::memcmp(header, idatHeader, 4);
    }
    else if (headerType == "IEND")
    {
        unsigned char iendHeader[4] = IEND_HEADER;
        return std::memcmp(header, iendHeader, 4);
    }
    else
    {
        std::cerr << "Invalid header supplied" << std::endl;
        exit(EXIT_FAILURE);
    }
}

int byteArrayToInt(unsigned char byteArr[], int len)
{
    int result = 0;

    for (int i = 0; i < 4; ++i)
    {
        // Assume big endian, bit shift each hex entry and cast to int
        result += static_cast<int>(byteArr[i] << (8 * (3 - i)));
    }
    return result;
}

void printChunkInfo(int sizeBytes, unsigned char chunkHeader[])
{
    std::cout << "----------" << std::endl;
    std::cout << "Chunk header: " << chunkHeader << std::endl;
    std::cout << std::endl
              << "Chunk size (bytes): " << sizeBytes << std::endl;
    std::cout << std::endl
              << "----------" << std::endl;
}

void readIDAT(int fd, int start, int size)
{ 
    char data[5];
    for (int i = 0; (i * 4) < size; i++)
    {
        pread(fd, &(data), 4, start + (i * 4));

        std::cout << "Data: ";
        for (int j = 0; j < 4; j++)
        {
            std::cout << data[j] << " ";
        }
        std::cout << std::endl;
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
    unsigned char header[8];
    if (pread(fd, header, 8, 0) != 8 || compareHeaders(header, "PNG") != 0)
    {
        std::cerr << "Mismatching image headers" << std::endl;
        exit(EXIT_FAILURE);
    }

    bool reachedIDAT = false;
    int offset = 8;
    for (int i = 0; i < 6; i++)
    {
        unsigned char size[4], chunkHeader[5];

        if (pread(fd, size, 4, offset) != 4)
        {
            std::cerr << "Error reading size of chunk" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (pread(fd, chunkHeader, 4, offset + 4) != 4)
        {
            std::cerr << "Error reading header of chunk" << std::endl;
            exit(EXIT_FAILURE);
        }

        int sizeBytes = byteArrayToInt(size, 4);
        // print size and chunk name
        printChunkInfo(sizeBytes, chunkHeader);

        if (strcmp((char *)chunkHeader, "IDAT") == 0)
        {
            readIDAT(fd, offset, sizeBytes);
        }

        offset += sizeBytes + 12; // 12 bytes reserved for chunk metadata (size, name, etc)
    }

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
    // std::cout << "First 10 pixel values: ";
    // for (int i = 0; i < 10; ++i)
    // {
    //     std::cout << static_cast<int>(pixels[i].red) << " "
    //               << static_cast<int>(pixels[i].green) << " "
    //               << static_cast<int>(pixels[i].blue) << " ";
    // }
    // std::cout << std::endl;

    return 0;
}
