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

debug: CFLAGS = -g -DCO_USE_ZLIB -Wall -I./co 
sanitize: CFLAGS = -g -DCO_USE_ZLIB -Wall -fsanitize=address -I./co
release: CFLAGS = -O4 -DNDEBUG -DCO_USE_ZLIB -Wall -I./co
#gprof: CFLAGS = -g -pg -DCO_USE_ZLIB -Wall -I./co

ifeq ($(shell uname -s),Linux)
LDFLAGS = -lelf -lm
else
LDFLAGS = -Wl,-Bstatic -llibelf -lm
endif

COSRC = $(shell ls *.c)
COOBJ = $(COSRC:.c=.o)

debug: all

sanitize: all 

release: all
	strip a2l_info.exe
	strip a2l_search.exe
	
#gprof: all

all: elf2json
	
elf2json: $(COOBJ) 
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

clean:
	-rm $(COOBJ)
	