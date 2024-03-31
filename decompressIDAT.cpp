#include <zlib.h>
#include <vector>
#include <iostream>

std::vector<unsigned char> decompressIDAT(const std::vector<unsigned char>& idatData) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = idatData.size();
    stream.next_in = const_cast<unsigned char*>(idatData.data());

    // Set up the inflate stream
    if (inflateInit(&stream) != Z_OK) {
        std::cerr << "Error initializing zlib inflate stream" << std::endl;
        return std::vector<unsigned char>();
    }

    // Create buffer to store decompressed data
    std::vector<unsigned char> decompressedData(4096); // Initial size

    // Decompress IDAT data
    std::vector<unsigned char> result;
    int ret;
    do {
        stream.avail_out = decompressedData.size();
        stream.next_out = decompressedData.data();
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret < 0) {
            std::cerr << "Error decompressing IDAT data: " << ret << std::endl;
            inflateEnd(&stream);
            return std::vector<unsigned char>();
        }
        // Append decompressed data to result
        result.insert(result.end(), decompressedData.begin(), decompressedData.begin() + stream.total_out);
    } while (ret != Z_STREAM_END);

    // Clean up and return result
    inflateEnd(&stream);
    return result;
}