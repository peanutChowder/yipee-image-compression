CC = g++
CC_FLAGS = -g

readImage: readImage.cpp
	$(CC) $(CC_FLAGS) -o readImage readImage.cpp -lglfw -lGLEW -lGLU -lGL -lm -lXrandr -lXi -lX11 -lpthread -ldl