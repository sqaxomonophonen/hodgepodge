PKGS=sdl2
CFLAGS+=-O2 $(shell pkg-config --cflags $(PKGS))
LDLIBS+=-lm  $(shell pkg-config --libs $(PKGS))

all: main dense

main: main.o
dense: dense.o

clean:
	rm -f *.o main dense
