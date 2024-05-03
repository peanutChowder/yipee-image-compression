// display images
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#include "displayImage.h"

void calcOutputWindowSize(const int imageWidth, const int imageHeight, int &windowWidth, int &windowHeight) {
    int heightScale = imageHeight / MAX_WINDOW_HEIGHT;
    int widthScale = imageWidth / MAX_WINDOW_WIDTH;

    if (heightScale == 1 && widthScale == 1) {
        windowHeight = imageHeight;
        windowWidth = imageWidth;
        return;
    }
    int scale = std::max(heightScale, widthScale);

    windowHeight = imageHeight / scale;
    windowWidth = imageWidth / scale;
}

void displayDecompressedImage(const std::vector<unsigned char>& imageData, int width, int height) {
    int windowWidth, windowHeight;
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    calcOutputWindowSize(width, height, windowWidth, windowHeight);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Decompressed Image", NULL, NULL);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window" << std::endl;
        return;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return;
    }

    // Enable texture
    glEnable(GL_TEXTURE_2D);

    // Generate texture ID
    GLuint textureID;
    glGenTextures(1, &textureID);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Upload image data to texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the framebuffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw textured quad
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
        glEnd();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Delete texture
    glDeleteTextures(1, &textureID);

    // Terminate GLFW
    glfwTerminate();
}