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


debug: CFLAGS = -g -Wall -I./co 
sanitize: CFLAGS = -g -Wall -fsanitize=address -I./co
release: CFLAGS = -O4 -DNDEBUG -Wall -I./co
#gprof: CFLAGS = -g -pg -DCO_USE_ZLIB -Wall -I./co



ifeq ($(shell uname -s),Linux)
LDFLAGS = -lelf -lm
else
LDFLAGS = -Wl,-Bstatic -lelf -lm
endif

#SRC = $(shell ls *.c)
#OBJ = $(SRC:.c=.o)

debug: all

sanitize: all 

release: all
	
#gprof: all

all: elf2json elf2bin elf2obj
	
elf2json: elf2json.o 
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

elf2bin: elf2bin.o 
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

elf2obj: elf2obj.o 
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

clean:
	-rm elf2json.o elf2bin.o elf2obj.o elf2json elf2bin elf2obj $(OBJ)
	