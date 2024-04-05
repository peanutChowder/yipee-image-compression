#include <string>

// PNG chunk headers
#define PNG_HEADER                                     \
    {                                                  \
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a \
    }
#define IHDR_HEADER            \
    {                          \
        0x49, 0x48, 0x44, 0x52 \
    }
#define IDAT_HEADER            \
    {                          \
        0x49, 0x44, 0x41, 0x54 \
    }
#define IEND_HEADER            \
    {                          \
        0x49, 0x45, 0x4e, 0x44 \
    }

#define PRINT_DIVIDER "----------" 

struct ihdr {
    int width;
    int height;
    int channelDepth;
    int colorType;
    int compressionMethod;
};

int compareHeaders(unsigned char header[], std::string headerType);

int byteArrayToInt(unsigned char byteArr[], int len);

void printChunkInfo(int sizeBytes, int offset, unsigned char chunkHeader[]);

/**
 * Appends IDAT chunk data into the provided vector.
 * 
 * Starts from the actual chunk start, i.e. including the IDAT chunk's metadata (byte size and IDAT tag). 
 * The provided size should NOT include the 4 CRC bytes at the end. PNG chunk size metadata does not count
 * the CRC bytes anyways.
 * 
 * @param fd The file descriptor of the png
 * @param start The starting point of the chunk including its header
 * @param imageRGBA The vector to append data to.
 * @return true if successful. false otherwise.
*/
bool readIDAT(int fd, int start, int size, std::vector<unsigned char> &imageRGBA);

bool readIHDR(int fd, int start, int size, struct ihdr &ihdrData);

bool readPNGImage(const char *filename, std::vector<unsigned char> &imageRGBA, struct ihdr &ihdrData);


