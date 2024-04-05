#include <vector>

bool decompressIDAT(const std::vector<unsigned char>& compressedData, std::vector<unsigned char> &decompressedData);

bool defilterIDAT(std::vector<unsigned char> &decompressedData, std::vector<unsigned char> &defilteredData, int width, int height, int colorType, int channelDepth);

int getBytesPerPixel(int colorType, int channelDepth);