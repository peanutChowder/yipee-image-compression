#include <vector>

#define MAX_WINDOW_HEIGHT 500
#define MAX_WINDOW_WIDTH 900

void calcOutputWindowSize(const int imageWidth, const int imageHeight, int &windowWidth, int &windowHeight);
void displayDecompressedImage(const std::vector<unsigned char>& imageData, int width, int height);