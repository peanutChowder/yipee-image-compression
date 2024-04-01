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

#include "readImage.h"

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

void printChunkInfo(int sizeBytes, int offset, unsigned char chunkHeader[])
{
    std::cout << "----------" << std::endl;
    std::cout << "Chunk header: " << chunkHeader << std::endl;
    std::cout << std::endl
              << "Chunk size (bytes): " << sizeBytes << std::endl;
    std::cout << "Starting at: " << offset << " bytes" << std::endl;
    std::cout << std::endl
              << "----------" << std::endl;
}

/**
 * Appends IDAT chunk data into the provided vector.
 * 
 * Starts from the actual chunk start, i.e. including the IDAT chunk's metadata (byte size and IDAT tag). 
 * The provided size should NOT include the 4 CRC bytes at the end. PNG chunk size metadata does not count
 * the CRC bytes anyways.
 * 
 * @param fd The file descriptor of the png
 * @param start The starting point of the chunk including its header
 * @param imageRGBA The vector to append data to.
 * @return true if successful. false otherwise.
*/
bool readIDAT(int fd, int start, int size, std::vector<unsigned char> &imageRGBA)
{ 
    unsigned char data;

    // Skip the chunk header (size + tag).
    start += 8;
    for (int i = 0; i < size; i++)
    {
         if (pread(fd, &data, 1, start + i) != 1) {
            return false;
         }

        // add data to our RGBA vector
        imageRGBA.push_back(data);
    }

    return true;
}

bool readIHDR(int fd, int start, int size, int &pixelWidth, int &pixelHeight) {
    unsigned char widthBuff[4], heightBuff[4];

    // 'start' begins at the start of the IHDR chunk.
    // we skip the first 8 bytes (chunk name and chunk size) to get the width.
    // we skip another 4 bytes to get the height, since image width & height are 4 bytes each.
    if (pread(fd, &widthBuff, 4, start + 8) != 4) {
        return false;
    }
    if (pread(fd, &heightBuff, 4, start + 12) != 4) {
        return false;
    }
    

    pixelWidth = byteArrayToInt(widthBuff, 4);
    pixelHeight = byteArrayToInt(heightBuff, 4);

    return true;
}

bool readPNGImage(const char *filename, std::vector<unsigned char> &imageRGBA, int &width, int &height)
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
    int offset = 8; // First 8 bytes determine the png image format -- we already checked this
    width = -1;
    height = -1;

    // Iterate over file until we encounter the IEND chunk or
    // the start of the chunk indicates a 0 byte length.
    // The IEND chunk should normally have a size of 0 bytes.
    while (1)
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
        printChunkInfo(sizeBytes, offset, chunkHeader);

        if (strcmp((char *) chunkHeader, "IHDR") == 0) {
            bool success = readIHDR(fd, offset, sizeBytes, width, height);

            if (!success) {
                std::cerr << "Error reading IHDR" << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        if (strcmp((char *)chunkHeader, "IDAT") == 0)
        {
            bool success = readIDAT(fd, offset, sizeBytes, imageRGBA);

            if (!success) {
                std::cerr << "Error reading IDAT" << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        if (strcmp((char *) chunkHeader, "IEND") == 0 || sizeBytes == 0) {
            break;
        }

        offset += sizeBytes + 12; // 12 bytes reserved for chunk metadata (size, name, CRC)
    }

    // ensure we obtained the width and height from the IHDR chunk
    if (width == -1 || height == -1) {
        std::cerr << "Failed to get image height and width from IHDR" << std::endl;
        return false;
    }

    return true;
}
