CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = -lm

all: image

image: image.c lodepng.c lodepng.h
	$(CC) $(CFLAGS) -o image image.c lodepng.c $(LIBS)

run: image
	./image

clean:
	rm -f image
