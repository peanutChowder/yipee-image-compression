CC = g++
CC_FLAGS = -g

main: readImage.cpp decompressIDAT.o displayImage.o readImage.o
	$(CC) $(CC_FLAGS) -o main main.cpp decompressIDAT.o displayImage.o readImage.o -lglfw -lGLEW -lGLU -lGL -lm -lXrandr -lXi -lX11 -lpthread -ldl -lz

decompressIDAT.o: decompressIDAT.cpp decompressIDAT.h
	$(CC) $(CC_FLAGS) -c decompressIDAT.cpp -o decompressIDAT.o -lz

displayImage.o: displayImage.cpp displayImage.h
	$(CC) $(CC_FLAGS) -c displayImage.cpp -o displayImage.o -lglfw -lGLEW -lGLU -lGL -lm -lXrandr -lXi -lX11 -lpthread -ldl

readImage.o: readImage.cpp readImage.h
	$(CC) $(CC_FLAGS) -c readImage.cpp -o readImage.o 

clean:
	rm *.o main