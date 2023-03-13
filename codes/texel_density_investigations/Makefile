PKGS=sdl2 gl
CFLAGS+=-O0
#CFLAGS+=-O2
CFLAGS+=$(shell pkg-config --cflags $(PKGS))
LDLIBS+=-lm  $(shell pkg-config --libs $(PKGS))

all: main dense fakemip

main: main.o
dense: dense.o
fakemip: fakemip.o

clean:
	rm -f *.o main dense
