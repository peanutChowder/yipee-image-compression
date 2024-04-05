#include <vector>

bool decompressIDAT(const std::vector<unsigned char>& compressedData, std::vector<unsigned char> &decompressedData);

int getBytesPerPixel(int colorType, int channelDepth);