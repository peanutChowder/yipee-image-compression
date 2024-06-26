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
#include "printUtils.h"

bool printVerbose = false;

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

    for (int i = 0; i < len; ++i)
    {
        // Assume big endian, bit shift each hex entry and cast to int
        result += static_cast<int>(byteArr[i] << (8 * (3 - i)));
    }
    return result;
}

void printChunkInfo(int sizeBytes, int offset, unsigned char chunkHeader[])
{
    if (!printVerbose) {
        return;
    }

    std::cout << PRINT_DIVIDER << std::endl;
    std::cout << "Chunk header: " << chunkHeader << std::endl;
    std::cout << std::endl
              << "Chunk size (bytes): " << sizeBytes << std::endl;
    std::cout << "Starting at: " << offset << " bytes" << std::endl;
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
    unsigned char buffer[size];
    
    // Skip the chunk header (size + tag).
    start += 8;

    if (pread(fd, &buffer, size, start) != size) {
        return false;
    }

    imageRGBA.insert(imageRGBA.end(), buffer, buffer + size);

    return true;
}

void printReadSummary(struct ihdr ihdrData) {
    std::cout << PRINT_DIVIDER_BIG << std::endl;
    std::cout << "Image reading summary" << std::endl;
    std::cout << "\tDimensions: " << ihdrData.width << " x " << ihdrData.height << std::endl;
    std::cout << "\tChannel depth: " << ihdrData.channelDepth << std::endl;
    std::cout << "\tColor type: " << ihdrData.colorType << std::endl;
    std::cout << "\tCompression method: " << ihdrData.compressionMethod << std::endl;
    std::cout << "\tFilter method: " << ihdrData.filterMethod << std::endl;
    std::cout << "\tInterlace method: " << ihdrData.interlaceMethod << std::endl;

}

bool readIHDR(int fd, int start, int size, struct ihdr &ihdrData) {
    unsigned char widthBuff[4], heightBuff[4];
    unsigned char channelDepth, colorType, compressionMethod, filterMethod, interlaceMethod;

    // 'start' begins at the start of the IHDR chunk.
    // we skip the first 8 bytes (chunk name and chunk size) to get the width.
    // we skip another 4 bytes to get the height, since image width & height are 4 bytes each.
    if (pread(fd, &widthBuff, 4, start + 8) != 4) {
        return false;
    }
    if (pread(fd, &heightBuff, 4, start + 12) != 4) {
        return false;
    }

    // get bit depth per channel (1 byte)
    if (pread(fd, &channelDepth, 1, start + 16) != 1) {
        return false;
    }

    // get color type (1 byte)
    if (pread(fd, &colorType, 1, start + 17) != 1) {
        return false;
    }

    // get compression method (1 byte)
    if (pread(fd, &compressionMethod, 1, start + 18) != 1) {
        return false;
    }    

    // get filter method (1 byte)
    if (pread(fd, &filterMethod, 1, start + 19) != 1) {
        return false;
    }   

    // get interlace method (1 byte)
    if (pread(fd, &interlaceMethod, 1, start + 20) != 1) {
        return false;
    }   

    ihdrData.width = byteArrayToInt(widthBuff, 4);
    ihdrData.height = byteArrayToInt(heightBuff, 4);

    // single byte data can be casted to int
    ihdrData.channelDepth = (int) channelDepth;
    ihdrData.colorType = (int) colorType;
    ihdrData.compressionMethod = (int) compressionMethod;
    ihdrData.filterMethod = (int) filterMethod;
    ihdrData.interlaceMethod = (int) interlaceMethod;

    return true;
}

bool readPNGImage(const char *filename, std::vector<unsigned char> &imageRGBA, struct ihdr &ihdrData)
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

    // Set all ihdr fields to -1. We check at the end if any values are still -1. If so, we return
    // in error.
    ihdrData.width = -1;
    ihdrData.height = -1;
    ihdrData.channelDepth = -1;
    ihdrData.colorType = -1;
    ihdrData.compressionMethod = -1;
    ihdrData.filterMethod = -1;
    ihdrData.interlaceMethod = -1;

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


        if (strcmp((char *) chunkHeader, "IHDR") == 0) {
            bool success = readIHDR(fd, offset, sizeBytes, ihdrData);

            if (!success) {
                std::cerr << "Error reading IHDR" << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        // print size and chunk name
        printChunkInfo(sizeBytes, offset, chunkHeader);

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

    // ensure all IHDR values are initialized
    if (ihdrData.width == -1 || ihdrData.height == -1 || ihdrData.channelDepth == -1 || ihdrData.colorType == -1 || ihdrData.compressionMethod == -1 || ihdrData.filterMethod == -1 || ihdrData.interlaceMethod == -1) {
        std::cerr << "Failed to get metadata from IHDR" << std::endl;
        return false;
    }

    // Temporary block since we currently cannot guarantee the code is built
    // for color type 6, i.e. bytes/pixel != 4 (i.e. RGBA)
    if (ihdrData.colorType != 6) {
        std::cerr << "Unsupported color type. Only type 6 is supported!" << std::endl;
        return false;
    }

    printReadSummary(ihdrData);

    return true;
}
