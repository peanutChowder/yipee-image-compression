#include <zlib.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "processImage.h"
#include "printUtils.h"

struct FilterCounts {
    int none = 0;
    int sub = 0;
    int up = 0;
    int average = 0;
    int paeth = 0;
};

bool decompressIDAT(const std::vector<unsigned char>& compressedData, std::vector<unsigned char> &decompressedData) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = compressedData.size();
    stream.next_in = const_cast<unsigned char*>(compressedData.data());

    // Set up the inflate stream
    if (inflateInit(&stream) != Z_OK) {
        std::cerr << "Error initializing zlib inflate stream" << std::endl;
        return false;
    }

    // Create buffer to store decompressed data
    std::vector<unsigned char> buffer(4096); // Initial size

    // Decompress IDAT data
    int ret;
    do {
        stream.avail_out = buffer.size();
        stream.next_out = buffer.data();
        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret < 0 && ret != Z_STREAM_END) {
            std::cerr << "Error decompressing IDAT data, ret status: " << ret << std::endl;
            inflateEnd(&stream);
            return false;
        }
        // Append decompressed data to result
        decompressedData.insert(decompressedData.end(), buffer.begin(), buffer.end() - stream.avail_out);
        buffer.clear();
        buffer.resize(4096);
    } while (ret != Z_STREAM_END);

    // Clean up and return result
    inflateEnd(&stream);

    printDecompressSummary(compressedData.size(), decompressedData.size());

    return true;
}

void printDecompressSummary(int compressedSize, int decompressedSize) {
    std::cout << PRINT_DIVIDER_BIG << std::endl;
    std::cout << "Decompression summary:" << std::endl;
    std::cout << "\tCompressed len: " << compressedSize << std::endl;
    std::cout << "\tDecompressed len: " << decompressedSize << std::endl;
}

unsigned char paethPredictor(unsigned char left, unsigned char up, unsigned char leftUp) {
    int p, pa, pb, pc;
    p = left + up - leftUp;
    pa = abs(p - left);
    pb = abs(p - up);
    pc = abs(p - leftUp);
    if (pa <= pb && pa <= pc) {
        return left;
    } else if (pb <= pc) {
        return up;
    } else {
        return leftUp;
    }
}

bool defilterIDAT(std::vector<unsigned char> &decompressedData, std::vector<unsigned char> &defilteredData, int width, int height, int colorType, int channelDepth) {
    int bytesPerPixel, colWidth;
    int filter;
    int currByteIndex;
    int defilteredCurr, defilteredCurrIndex, defilteredLeft, defilteredUp, defilteredLeftUp;
    struct FilterCounts filterCounts;

    if ((bytesPerPixel = getBytesPerPixel(colorType, channelDepth)) == -1) {
        return false;
    }

    // each line is occupied by pixel data + 1 byte for the filter
    colWidth = width * bytesPerPixel + 1;

    // Reserve the output vector. We subtract 'height' to account for  
    // discarding the filter byte at the start of each line.
    defilteredData.reserve(colWidth * height - height);

    for (int lineIndex = 0; lineIndex < height; lineIndex++) {
        filter = decompressedData[lineIndex * colWidth]; // get the filter which is located in the first byte of each line

        // apply filter to current line. we skip the first byte
        // since it is occupied by the filter data.
        // We perform 256 modulo on each filter calc to ensure
        // no pixel data overflow.
        for (int colIndex = 1; colIndex < colWidth; colIndex++) { 
            currByteIndex = lineIndex * colWidth + colIndex;
            defilteredCurrIndex = currByteIndex - lineIndex - 1; // Defiltered data does not include the 1 byte filter

            switch (filter) {
                // no filter -- add byte directly
                case 0:
                    defilteredCurr = decompressedData[currByteIndex];

                    filterCounts.none += 1;
                    break;

                // sub filter: defiltered byte = curr filtered + defiltered left
                case 1: 
                    defilteredLeft = (colIndex < bytesPerPixel) ? 0 : defilteredData[defilteredCurrIndex - bytesPerPixel];
                    defilteredCurr = (decompressedData[currByteIndex] + defilteredLeft) % 256;

                    filterCounts.sub += 1;
                    break;

                // up filter: defiltered byte = curr filtered + defiltered up
                case 2:
                    defilteredUp = (lineIndex == 0) ? 0 : defilteredData[defilteredCurrIndex - width * bytesPerPixel];
                    defilteredCurr = (decompressedData[currByteIndex] + defilteredUp) % 256; 

                    filterCounts.up += 1;
                    break;

                // average filter: defiltered byte = curr filtered + floor((defiltered left + defiltered up) / 2)
                case 3:
                    defilteredLeft = (colIndex < bytesPerPixel) ? 0 : defilteredData[defilteredCurrIndex - bytesPerPixel];
                    defilteredUp = (lineIndex == 0) ? 0 : defilteredData[defilteredCurrIndex - width * bytesPerPixel];
                    defilteredCurr = (decompressedData[currByteIndex] + (defilteredLeft + defilteredUp) / 2) % 256;

                    filterCounts.average += 1;
                    break;

                // paeth filter: defiltered byte = curr filtered + paethPredictor(defiltered left + defiltered up + defiltered left up (diagonal))
                case 4:
                    defilteredLeft = (colIndex < bytesPerPixel) ? 0 : defilteredData[defilteredCurrIndex - bytesPerPixel];
                    defilteredUp = (lineIndex == 0) ? 0 : defilteredData[defilteredCurrIndex - width * bytesPerPixel];
                    defilteredLeftUp = (lineIndex == 0 || colIndex < bytesPerPixel) ? 0 : defilteredData[defilteredCurrIndex - width * bytesPerPixel - bytesPerPixel];
                    defilteredCurr = (decompressedData[currByteIndex] + paethPredictor(defilteredLeft, defilteredUp, defilteredLeftUp)) % 256;

                    filterCounts.paeth += 1;
                    break;

                default:
                    printGetFilterErr(filter, lineIndex, colWidth, decompressedData);
                    exit(-1);
                    break;
            }

            defilteredData.push_back(defilteredCurr);
        }

        
    }

    printFilterSummary(filterCounts);

    return true;
}

void printFilterSummary(struct FilterCounts filterCounts) {
    std::cout << PRINT_DIVIDER_BIG << std::endl;
    std::cout << "Filter summary: " << std::endl;
    std::cout << "\tNone: " << filterCounts.none << std::endl;
    std::cout << "\tSub: " << filterCounts.sub << std::endl;
    std::cout << "\tUp: " << filterCounts.up << std::endl;
    std::cout << "\tAverage: " << filterCounts.average << std::endl;
    std::cout << "\tPaeth: " << filterCounts.paeth << std::endl;

}

void printGetFilterErr(int filter, int lineIndex, int colWidth, std::vector<unsigned char> decompressedData) {
    int lineStart = lineIndex >= 2 ? lineIndex - 2 : lineIndex;
    int lineEnd = lineStart + 4;
    std::cerr << "Error: invalid row filter '" << filter << "' at row " << lineIndex << ", byte " << lineIndex * colWidth;
    std::cerr << ". Contents: " << std::endl;
    for (int i = lineStart; i < lineEnd; i++) {
        std::cerr << "Row " << i << ":";
        for (int j = 0; j < colWidth; j++) {
            int debugIndex = i * colWidth + j;
            if (debugIndex % colWidth == 0 || (debugIndex - 1) % colWidth == 0 || (debugIndex - 2) % colWidth == 0 || (debugIndex + 3) % colWidth == 0 || (debugIndex + 2) % colWidth == 0 || (debugIndex + 1) % colWidth == 0 ) {
                std::cerr << " " << (int) decompressedData[debugIndex];
            }  
            if ((debugIndex - 2) % colWidth == 0) {
                std::cerr << " ...";
            }
        }
        std::cerr << std::endl;
    }
}

int getBytesPerPixel(int colorType, int channelDepth) {
    int bytesPerChannel;

    if (channelDepth < 8) {
        std::cerr << "Channel depths less than 1 byte currently unsupported" << std::endl;
        return -1;
    }

    bytesPerChannel = channelDepth / 8;

    switch (colorType) {
        // greyscale image, 1 channel per pixel
        case 0:
            return bytesPerChannel * 1;
        // Truecolor image, 3 channels per pixel (RGB)
        case 2:
            return bytesPerChannel * 3;
        // Indexed color image, 1 channel per pixel (need indexing)
        case 3:
            return bytesPerChannel * 1;
        // Greyscale with alpha, 2 channels per pixel
        case 4:
            return bytesPerChannel * 2;
        // Truecolor with alpha, 4 channels per pixel (RGBA)
        case 6:
            return bytesPerChannel * 4;
        default:
            std::cerr << "Unsupported color type '" << colorType << "'." << std::endl;
            return -1;
    }
}