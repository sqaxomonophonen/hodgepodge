PLATFORM_CFLAGS=$(shell pkg-config --cflags gl sdl2)
PLATFORM_LDLIBS=$(shell pkg-config --libs gl sdl2) -pthread -ldl
OPT=-O2
CFLAGS=-std=c99 $(OPT) $(PLATFORM_CFLAGS) -Inanovg/src
LDLIBS=$(PLATFORM_LDLIBS) -Lnanovg/build -lnanovg -lm

bins=main
all: $(bins)

main: main.o nanovg_gl.o miniaudio.o

clean:
	rm -f $(bins) *.o
