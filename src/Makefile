CC = g++
CC_FLAGS = -g -fopenmp

main: main.cpp readImage.cpp processImage.o displayImage.o readImage.o
	$(CC) $(CC_FLAGS) -o main main.cpp processImage.o displayImage.o readImage.o -lglfw -lGLEW -lGLU -lGL -lm -lXrandr -lXi -lX11 -lpthread -ldl -lz

processImage.o: processImage.cpp processImage.h
	$(CC) $(CC_FLAGS) -c processImage.cpp -o processImage.o -lz

displayImage.o: displayImage.cpp displayImage.h
	$(CC) $(CC_FLAGS) -c displayImage.cpp -o displayImage.o -lglfw -lGLEW -lGLU -lGL -lm -lXrandr -lXi -lX11 -lpthread -ldl

readImage.o: readImage.cpp readImage.h
	$(CC) $(CC_FLAGS) -c readImage.cpp -o readImage.o 

clean:
	rm *.o main