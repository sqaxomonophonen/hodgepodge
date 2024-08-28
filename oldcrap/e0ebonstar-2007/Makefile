# makefile

C++ = c++
CCFLAGS = -O2 -Wall -I/usr/local/include/SDL -I/usr/local/include
LINK = -L/usr/local/lib -lz -lSDLmain -lSDL -lGL -lvorbis -logg

O=.o

OWNOBJS = stb_image.o font1.o gs_menu.o gs_game.o gs_game_logic.o gs_game_draw.o map.o main.o fileop.o console.o audio.o

all: ebonstar

stb_image.o: stb_image.c stb_image.h
	$(CC) $(CCFLAGS) -c stb_image.c

font1.o: font1.cc font1.h read_png.h
	$(C++) $(CCFLAGS) -c font1.cc
	
fileop.o: fileop.cc fileop.h
	$(C++) $(CCFLAGS) -c fileop.cc
	
gs_menu.o: gs_menu.cc gs_menu.h font1.h main.h
	$(C++) $(CCFLAGS) -c gs_menu.cc
	
gs_game.o: gs_game.cc gs_game.h gs_game_i.h main.h read_png.h
	$(C++) $(CCFLAGS) -c gs_game.cc
	
gs_game_logic.o: gs_game_logic.cc gs_game.h gs_game_i.h main.h
	$(C++) $(CCFLAGS) -c gs_game_logic.cc

gs_game_draw.o: gs_game_draw.cc gs_game.h gs_game_i.h main.h
	$(C++) $(CCFLAGS) -c gs_game_draw.cc

map.o: map.cc map.h
	$(C++) $(CCFLAGS) -c map.cc

console.o: console.cc console.h font1.h read_png.h
	$(C++) $(CCFLAGS) -c console.cc
	
audio.o: audio.cc audio.h fileop.h
	$(C++) $(CCFLAGS) -c audio.cc
	
main.o: main.cc main.h gs_game.h gs_menu.h font1.h fileop.h console.h audio.h
	$(C++) $(CCFLAGS) -c main.cc


ebonstar: $(OWNOBJS)
	$(C++) -o ebonstar $(OWNOBJS) $(LINK)

clean:
	rm *.o
	
# end of Makefile.
