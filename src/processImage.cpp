#include <zlib.h>
#include <vector>
#include <iostream>
#include <algorithm>

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

bool defilterIDAT(std::vector<unsigned char> &decompressedData, int width, int height, int colorType, int channelDepth) {
    int bytesPerPixel, colWidth;
    int filter;

    if ((bytesPerPixel = getBytesPerPixel(colorType, channelDepth)) == -1) {
        return false;
    }

    // each line is occupied by pixel data + 1 byte for the filter
    colWidth = width * bytesPerPixel + 1;

    for (int lineIndex = 0; lineIndex < height; lineIndex++) {
        filter = decompressedData[lineIndex * colWidth]; // get the filter which is located in the first byte of each line

        for (int colIndex = 0; colIndex < colWidth; colIndex++) {
            // TODO
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