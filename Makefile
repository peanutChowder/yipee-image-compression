CC = g++
CC_FLAGS = -g

readImage: readImage.cpp
	$(CC) $(CC_FLAGS) readImage.cpp -fopenmp -o readImage