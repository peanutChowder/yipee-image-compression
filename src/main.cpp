#include <iostream>
#include <string>

#include "timer.h"
#include "processImage.h"
#include "displayImage.h"
#include "readImage.h"
#include "printUtils.h"

void printTimeElapsed(std::string taskName, double start, double end) {
    std::cout << "\t" << taskName << " took " << (end - start) << "s or " << (end - start) * 1000 << "ms." << std::endl;
}

int main()
{
    double start, end;
    double startGlobal, endGlobal;

    const char *filename = "../test-images/red-apple300x300.png"; 

    std::vector<unsigned char> compressedIDAT, decompressedIDAT, defilteredIDAT;
    struct ihdr ihdrData;

    GET_TIME(startGlobal);

    // read image bytes
    GET_TIME(start);
    if (!readPNGImage(filename, compressedIDAT, ihdrData)) {
        std::cerr << "Image reading failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    GET_TIME(end);
    printTimeElapsed("Image reading", start, end);

    // decompress image data (IDAT chunks)
    GET_TIME(start);
    if (!decompressIDAT(compressedIDAT, decompressedIDAT)) {
        std::cerr << "Image decompression failed" << std::endl;
    };
    GET_TIME(end);
    printTimeElapsed("Image decompression", start, end);

    // defilter image data (IDAT chunks)
    GET_TIME(start);
    if (!defilterIDAT(decompressedIDAT, defilteredIDAT, ihdrData.width, ihdrData.height, ihdrData.colorType, ihdrData.channelDepth)) {
        std::cerr << "Defiltering failed" << std::endl;
    }
    GET_TIME(end);
    printTimeElapsed("Image defiltering", start, end);

    GET_TIME(endGlobal);

    std::cout << PRINT_DIVIDER_BIG << std::endl;
    std::cout << "Final summary" << std::endl;
    printTimeElapsed("Total processing", startGlobal, endGlobal);

    // display image
    displayDecompressedImage(defilteredIDAT, ihdrData.width, ihdrData.height);
    

    return 0;
}