OPT=-O2
CFLAGS=-std=c99 $(OPT) -Wall

all: isofun

isofun: isofun.o stb_image_write.o
	$(CC) $^ -o $@

clean:
	rm -f *.o isofun out.png
