// display images
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include <vector>
#include <cstring>

// file read
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

// parallelization
#include <omp.h>

#include <iomanip>



// timing execution time
#include "timer.h"

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

int compareHeaders(unsigned char header[], std::string headerType)
{
    if (headerType == "PNG")
    {
        unsigned char pngHeader[8] = PNG_HEADER;
        return std::memcmp(header, pngHeader, 8);
    }
    else if (headerType == "IHDR")
    {
        unsigned char ihdrHeader[4] = IHDR_HEADER;
        return std::memcmp(header, ihdrHeader, 4);
    }
    else if (headerType == "IDAT")
    {
        unsigned char idatHeader[4] = IDAT_HEADER;
        return std::memcmp(header, idatHeader, 4);
    }
    else if (headerType == "IEND")
    {
        unsigned char iendHeader[4] = IEND_HEADER;
        return std::memcmp(header, iendHeader, 4);
    }
    else
    {
        std::cerr << "Invalid header supplied" << std::endl;
        exit(EXIT_FAILURE);
    }
}

int byteArrayToInt(unsigned char byteArr[], int len)
{
    int result = 0;

    for (int i = 0; i < 4; ++i)
    {
        // Assume big endian, bit shift each hex entry and cast to int
        result += static_cast<int>(byteArr[i] << (8 * (3 - i)));
    }
    return result;
}

void printChunkInfo(int sizeBytes, unsigned char chunkHeader[])
{
    std::cout << "----------" << std::endl;
    std::cout << "Chunk header: " << chunkHeader << std::endl;
    std::cout << std::endl
              << "Chunk size (bytes): " << sizeBytes << std::endl;
    std::cout << std::endl
              << "----------" << std::endl;
}

void readIDAT(int fd, int start, int size, std::vector<unsigned char> &imageRGBA)
{ 
    unsigned char data;
    for (int i = 0; i < size; i++)
    {
        pread(fd, &data, 1, start + i);

        // add data to our RGBA vector
        imageRGBA.push_back(data);
    }
}

bool readIHDR(int fd, int start, int size, int &pixelWidth, int &pixelHeight) {
    unsigned char widthBuff[4], heightBuff[4];

    // 'start' begins at the start of the IHDR chunk.
    // we skip the first 8 bytes (chunk name and chunk size) to get the width.
    // we skip another 4 bytes to get the height, since image width & height are 4 bytes each.
    pread(fd, &widthBuff, 4, start + 8);
    pread(fd, &heightBuff, 4, start + 12);

    pixelWidth = byteArrayToInt(widthBuff, 4);
    pixelHeight = byteArrayToInt(heightBuff, 4);
}

bool readPNGImage(const char *filename, std::vector<unsigned char> &imageRGBA, int &width, int &height)
{
    int fd = open(filename, O_RDONLY);

    if (fd == -1)
    {
        std::cerr << "Error opening image file" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Check PNG and read file signature (first 8 bytes)
    unsigned char header[8];
    if (pread(fd, header, 8, 0) != 8 || compareHeaders(header, "PNG") != 0)
    {
        std::cerr << "Mismatching image headers" << std::endl;
        exit(EXIT_FAILURE);
    }

    bool reachedIDAT = false;
    int offset = 8;
    width = -1;
    height = -1;
    for (int i = 0; i < 6; i++)
    {
        unsigned char size[4], chunkHeader[5];

        if (pread(fd, size, 4, offset) != 4)
        {
            std::cerr << "Error reading size of chunk" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (pread(fd, chunkHeader, 4, offset + 4) != 4)
        {
            std::cerr << "Error reading header of chunk" << std::endl;
            exit(EXIT_FAILURE);
        }

        int sizeBytes = byteArrayToInt(size, 4);
        // print size and chunk name
        printChunkInfo(sizeBytes, chunkHeader);

        if (strcmp((char *) chunkHeader, "IHDR") == 0) {
            readIHDR(fd, offset, sizeBytes, width, height);
        }

        if (strcmp((char *)chunkHeader, "IDAT") == 0)
        {
            readIDAT(fd, offset, sizeBytes, imageRGBA);
        }

        offset += sizeBytes + 12; // 12 bytes reserved for chunk metadata (size, name, etc)
    }

    // ensure we obtained the width and height from the IHDR chunk
    if (width == -1 || height == -1) {
        std::cerr << "Failed to get image height and width from IHDR" << std::endl;
        return false;
    }

    return true;
}

void displayRGBA(const std::vector<unsigned char>& rgbaData, int width, int height) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(width, height, "Pixels", NULL, NULL);
    if (!window) {
        glfwTerminate();
        fprintf(stderr, "Failed to create GLFW window\n");
        return;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return;
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the framebuffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw pixels from rgbaData
        glBegin(GL_POINTS);
        for (int i = 0; i < width * height; ++i) {
            int index = 4 * i;
            glColor4ub(rgbaData[index], rgbaData[index + 1], rgbaData[index + 2], rgbaData[index + 3]);
            float x = (i % width) / static_cast<float>(width) * 2.0f - 1.0f;
            float y = (i / width) / static_cast<float>(height) * 2.0f - 1.0f;
            glVertex2f(x, y);
        }
        glEnd();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();
}

int main()
{
    double start, end;

    const char *filename = "./test-images/green-apple.png"; // Replace with your PNG image file path
    std::vector<unsigned char> imageRGBA;
    int width, height;

    GET_TIME(start);
    bool res = readPNGImage(filename, imageRGBA, width, height);
    GET_TIME(end);

    if (!res)
    {
        printf("Image reading failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Image loaded\nLoading took %fs / %fms\n", (end - start), (end - start) * 1000);
    printf("Image pixels: %d width x %d\n height", width, height);

    // std::cout << "First 10 pixel values: ";
    // for (int i = 0; i < 10; ++i)
    // {
    //     std::cout << static_cast<int>(pixels[i].red) << " "
    //               << static_cast<int>(pixels[i].green) << " "
    //               << static_cast<int>(pixels[i].blue) << " ";
    // }
    // std::cout << std::endl;

    return 0;
}
