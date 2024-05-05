#include <iostream>
#include <string>
#include <iomanip>

#include "timer.h"
#include "processImage.h"
#include "displayImage.h"
#include "readImage.h"
#include "printUtils.h"

void printTimeElapsed(std::string taskName, double start, double end) {
    std::cout << "\t" << taskName << " took " << (end - start) << "s or " << (end - start) * 1000 << "ms." << std::endl;
}

int modeTiming() {
double start, end;
    double startGlobal, endGlobal;

    const char *filename = "../test-images/forest-3584x2048.png"; 
    const int trials = 7;
    std::vector<double> readImageTime, decompressImageTime, filterImageTime, globalTime;
    float avgReadImageTime, avgDecompressImageTime, avgFilterImageTime, avgGlobalTime;


    for (int i = 0; i < trials; i++) {
        std::vector<unsigned char> compressedIDAT, decompressedIDAT, defilteredIDAT;
        struct ihdr ihdrData;

        std::cout << "Beginning trial " << i << std::endl;

        GET_TIME(startGlobal);

        // read image bytes
        GET_TIME(start);
        if (!readPNGImage(filename, compressedIDAT, ihdrData)) {
            std::cerr << "Image reading failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        GET_TIME(end);
        readImageTime.push_back(end - start);

        // decompress image data (IDAT chunks)
        GET_TIME(start);
        if (!decompressIDAT(compressedIDAT, decompressedIDAT)) {
            std::cerr << "Image decompression failed" << std::endl;
        };
        GET_TIME(end);
        decompressImageTime.push_back(end - start);

        // defilter image data (IDAT chunks)
        GET_TIME(start);
        if (!defilterIDAT(decompressedIDAT, defilteredIDAT, ihdrData.width, ihdrData.height, ihdrData.colorType, ihdrData.channelDepth)) {
            std::cerr << "Defiltering failed" << std::endl;
        }
        GET_TIME(end);
        filterImageTime.push_back(end - start);

        GET_TIME(endGlobal);
        globalTime.push_back(endGlobal - startGlobal);

        printTimeElapsed("Trial " + i, startGlobal, endGlobal);
    }

    // Display image reading times:
    avgReadImageTime = 0.0;
    std::cout << "Image reading:\t\t";
    for (double time: readImageTime) {
        std::cout << std::setprecision(5) << time << std::fixed << " ";
        avgReadImageTime += time;
    }
    avgReadImageTime = avgReadImageTime / trials;
    std::cout << " | Avg: " << avgReadImageTime << " s" << std::endl;

    // Display image decompression times:
    avgDecompressImageTime = 0.0;
    std::cout << "Image decompression:\t";
    for (double time: decompressImageTime) {
        std::cout << time << std::setprecision(5) << std::fixed  << " ";
        avgDecompressImageTime += time;
    }
    avgDecompressImageTime = avgDecompressImageTime / trials;
    std::cout << " | Avg: " << avgDecompressImageTime << " s" << std::endl;

    // Display image filtering times:
    avgFilterImageTime = 0.0;
    std::cout << "Image filtering:\t";
    for (double time: filterImageTime) {
        std::cout << time << std::setprecision(5) << std::fixed  << " ";
        avgFilterImageTime += time;
    }
    avgFilterImageTime = avgFilterImageTime / trials;
    std::cout << " | Avg: " << avgFilterImageTime << " s" << std::endl;

    // Display image overall times:
    avgGlobalTime = 0.0;
    std::cout << "Image overall:\t\t";
    for (double time: globalTime) {
        std::cout << time << std::setprecision(5) << std::fixed  << " ";
        avgGlobalTime += time;
    }
    avgGlobalTime = avgGlobalTime / trials;
    std::cout << " | Avg: " << avgGlobalTime << " s" << std::endl;
    return 0;
}

int modeRegular() {
 double start, end;
    double startGlobal, endGlobal;

    const char *filename = "../test-images/forest-3584x2048.png"; 

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

int main()
{
   modeTiming();
}