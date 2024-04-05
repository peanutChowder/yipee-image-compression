#include <iostream>

// header files
#include "timer.h"
#include "processImage.h"
#include "displayImage.h"
#include "readImage.h"

int main()
{
    double start, end;

    const char *filename = "../test-images/red-apple300x300.png"; 

    std::vector<unsigned char> compressedIDAT, decompressedIDAT, defilteredIDAT;
    struct ihdr ihdrData;

    GET_TIME(start);
    bool res = readPNGImage(filename, compressedIDAT, ihdrData);
    GET_TIME(end);

    if (!res)
    {
        printf("Image reading failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Image loaded\nLoading took %fs / %fms\n", (end - start), (end - start) * 1000);
    printf("Image pixels: %d width x %d height\n", ihdrData.width, ihdrData.height);

    std::cout << "Decompressing data..." << std::endl;
    decompressIDAT(compressedIDAT, decompressedIDAT);

    std::cout << "============================" << std::endl;
    std::cout << "Decompression results:" << std::endl;
    std::cout << "Compressed len: " << compressedIDAT.size() << std::endl;
    std::cout << "Decompressed len: " << decompressedIDAT.size() << std::endl;

    defilterIDAT(decompressedIDAT, defilteredIDAT, ihdrData.width, ihdrData.height, ihdrData.colorType, ihdrData.channelDepth);

    displayDecompressedImage(defilteredIDAT, ihdrData.width, ihdrData.height);
    

    return 0;
}