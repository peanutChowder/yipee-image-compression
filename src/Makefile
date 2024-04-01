CC = g++
CC_FLAGS = -g

readImage: readImage.cpp decompressIDAT.o displayImage.o 
	$(CC) $(CC_FLAGS) -o readImage readImage.cpp decompressIDAT.o displayImage.o -lglfw -lGLEW -lGLU -lGL -lm -lXrandr -lXi -lX11 -lpthread -ldl -lz

decompressIDAT.o: decompressIDAT.cpp decompressIDAT.h
	$(CC) $(CC_FLAGS) -c decompressIDAT.cpp -o decompressIDAT.o -lz

displayImage.o: displayImage.cpp displayImage.h
	$(CC) $(CC_FLAGS) -c displayImage.cpp -o displayImage.o -lglfw -lGLEW -lGLU -lGL -lm -lXrandr -lXi -lX11 -lpthread -ldl

clean:
	rm *.o readImage