# Makefile

ifndef TURBO_DIR
TURBO_DIR := ../../turbo-devel-1.5.24
endif

RM = rm -f

SRC = ../../src/flash.c ../../src/string.c \
        ../../src/settings.c ../../src/pool.c \
            ../../src/schema.c ../../src/util.c

DEFINES = -D_MUVUKU_TINY_STRINGS

OBJ = $(SRC:.c=.o) muvuku.o
  
all: prototype

%.o : %.c 
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@

prototype:
	$(CC) $(DEFINES) -D_MUVUKU_PROTOTYPE \
        -fno-builtin -Wno-pointer-to-int-cast -Wno-attributes \
        -I../../src -g -o prototype $(SRC) prototype.c
clean:
	$(RM) *.o
	$(RM) *~
	$(RM) prototype
	$(RM) -r prototype.dSYM
	$(RM) *.stackdump
	$(RM) -r .cyg*

