CFLAGS+=-O2
CFLAGS+=-mavx2
LDLIBS=-lm
all: w2d
w2d: w2d.o w2dproc.o
w2dproc.s: w2dproc.c
	cc $(CFLAGS) -S $<
clean:
	rm -f w2d *.o
