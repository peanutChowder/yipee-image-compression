#include <zlib.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "processImage.h"

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

    // perform a flip on the x axis (vertical flip) to match openGL's bottom left (0,0)
    std::reverse(decompressedData.begin(), decompressedData.end());
    return true;
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
    int defilteredCurr, defilteredLeft, defilteredUp, defilteredLeftUp;

    if ((bytesPerPixel = getBytesPerPixel(colorType, channelDepth)) == -1) {
        return false;
    }

    // each line is occupied by pixel data + 1 byte for the filter
    colWidth = width * bytesPerPixel + 1;

    for (int lineIndex = 0; lineIndex < height; lineIndex++) {
        filter = decompressedData[lineIndex * colWidth]; // get the filter which is located in the first byte of each line

        // apply filter to current line. we skip the first byte
        // since it is occupied by the filter data.
        // We perform 256 modulo on each filter calc to ensure
        // no pixel data overflow.
        for (int colIndex = 1; colIndex < colWidth; colIndex++) { 
            currByteIndex = lineIndex * colWidth + colIndex;
            switch (filter) {
                // no filter -- add byte directly
                case 0:
                    defilteredCurr = decompressedData[currByteIndex];
                    break;

                // sub filter: defiltered byte = curr filtered + defiltered left
                case 1: 
                    defilteredLeft = (colIndex < bytesPerPixel + 1) ? 0 : defilteredData[currByteIndex - bytesPerPixel];
                    defilteredCurr = (decompressedData[currByteIndex] + defilteredLeft) % 256;
                    break;

                // up filter: defiltered byte = curr filtered + defiltered up
                case 2:
                    defilteredUp = (lineIndex == 0) ? 0 : defilteredData[currByteIndex - colWidth];
                    defilteredCurr = (decompressedData[currByteIndex] + defilteredUp) % 256; 
                    break;

                // average filter: defiltered byte = curr filtered + floor((defiltered left + defiltered up) / 2)
                case 3:
                    defilteredLeft = (colIndex == 1) ? 0 : defilteredData[currByteIndex - bytesPerPixel];
                    defilteredUp = (lineIndex == 0) ? 0 : defilteredData[currByteIndex - colWidth];
                    defilteredCurr = (decompressedData[currByteIndex] + (defilteredLeft + defilteredUp) / 2) % 256;
                    break;

                // paeth filter: defiltered byte = curr filtered + paethPredictor(defiltered left + defiltered up + defiltered left up (diagonal))
                case 4:
                    defilteredLeft = (colIndex == 1) ? 0 : defilteredData[currByteIndex - bytesPerPixel];
                    defilteredUp = (lineIndex == 0) ? 0 : defilteredData[currByteIndex - colWidth];
                    defilteredLeftUp = (lineIndex == 0 || colIndex == 1) ? 0 : defilteredData[currByteIndex - colWidth - bytesPerPixel];
                    defilteredCurr = (decompressedData[currByteIndex] + paethPredictor(defilteredLeft, defilteredUp, defilteredLeftUp)) % 256;
                    break;

                default:
                    std::cerr << "Error: invalid row filter '" << filter << "' at row " << lineIndex << ", byte " << lineIndex * colWidth;
                    // std::cerr << ". Contents: ";
                    // for (int i = lineIndex * colWidth; i < lineIndex * colWidth + colWidth; i++) {
                    //     std::cerr << " " << (int) decompressedData[i];
                    // }
                    std::cerr << std::endl;
                    exit(-1);
                    break;
            }

            defilteredData.push_back(defilteredCurr);
        }
    }

    return true;
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