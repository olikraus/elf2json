#
#  elf2json Makefile
#  (c) 2024 Oliver Kraus
#  https://github.com/olikraus/c-object
#  CC BY-SA 3.0  https://creativecommons.org/licenses/by-sa/3.0/
#
#  targets
#	clean		clear all objects and executables
#	debug		build debug version (default)
#	sanitize	build debug version with gcc sanitize enabled
#	release		build release version
#


debug: CFLAGS = -g -Wall -I./co -DLIBDWARF_STATIC
sanitize: CFLAGS = -g -Wall -fsanitize=address -I./co -DLIBDWARF_STATIC
release: CFLAGS = -O4 -DNDEBUG -Wall -I./co -DLIBDWARF_STATIC
#gprof: CFLAGS = -g -pg -DCO_USE_ZLIB -Wall -I./co



ifeq ($(shell uname -s),Linux)
LDFLAGS = -lelf -ldwarf -lm
else
# assume minge64
LDFLAGS = -L../msys2/mingw64 -Wl,-Bstatic -lelf -ldwarf -lm
debug: CFLAGS +=  -I../msys2/mingw64
sanitize: CFLAGS +=  -I../msys2/mingw64
release: CFLAGS +=  -I../msys2/mingw64
endif

#SRC = $(shell ls *.c)
#OBJ = $(SRC:.c=.o)

debug: all

sanitize: all 

release: all
	
#gprof: all

all: elf2json elf2bin elf2obj elfdwarf
	
elf2json: elf2json.o 
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

elf2bin: elf2bin.o 
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

elf2obj: elf2obj.o 
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

elfdwarf: elfdwarf.o 
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)


clean:
	-rm elf2json.o elf2bin.o elf2obj.o elfdwarf.o elf2json elf2bin elfdwarf elf2obj $(OBJ)
	