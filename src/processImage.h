#include <vector>

bool decompressIDAT(const std::vector<unsigned char>& compressedData, std::vector<unsigned char> &decompressedData);

void printDecompressSummary(int compressedSize, int decompressedSize);

bool defilterIDAT(std::vector<unsigned char> &decompressedData, std::vector<unsigned char> &defilteredData, int width, int height, int colorType, int channelDepth);

int getBytesPerPixel(int colorType, int channelDepth);

void printFilterSummary(struct FilterCounts filterCounts);

void printGetFilterErr(int filter, int lineIndex, int colWidth, std::vector<unsigned char> decompressedData);