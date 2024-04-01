#include <zlib.h>
#include <vector>
#include <iostream>
#include <algorithm>

#include "decompressIDAT.h"

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
        decompressedData.insert(decompressedData.end(), buffer.begin(), buffer.end());
        buffer.clear();
        buffer.resize(4096);
    } while (ret != Z_STREAM_END);

    // Clean up and return result
    inflateEnd(&stream);

    // perform a flip on the x axis (vertical flip) to match openGL's bottom left (0,0)
    std::reverse(decompressedData.begin(), decompressedData.end());
    return true;
}