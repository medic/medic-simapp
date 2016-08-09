# Makefile

ifndef TURBO_DIR
TURBO_DIR := ../../turbo-devel-1.5.24
endif

TURBO_TAG = --turbo-manifest "Vendor: Medic Mobile"
INCDIR  = -I$(TURBO_DIR)/include -I.
LIBDIR = $(TURBO_DIR)/lib

CC = avr-gcc
LD = avr-ld
RM = rm -f

TRG = muvuku
SRC = flash.c util.c string.c settings.c \
        actions.c transport.c pool.c schema.c

LIB = 

DEFINES = -D_MUVUKU_TINY_STRINGS -D_ENABLE_STORAGE_INFO \
    -D_SCHEMA_INCLUDE_DATES -D_SCHEMA_DISABLE_SPECIAL_DELIMITERS \
    -D_ENABLE_STORAGE_CLEAR

CFLAGS = $(DEFINES) -Os -Wall -fno-strict-aliasing -std=gnu99 \
    -fomit-frame-pointer -mmcu=atmega128 -mno-tablejump \
    -Wimplicit-function-declaration -fno-builtin

LDFLAGS = -L$(LIBDIR) -T $(TURBO_DIR)/lib/turbo.lds -d -r \
    --emit-relocs -R $(TURBO_DIR)/lib/public_calls $(LIB) -lc \
    `avr-gcc -print-libgcc-file-name`

OBJ = $(SRC:.c=.o) muvuku.o
  
all: links $(TRG).trb

%.o : %.c 
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@

$(TRG).elf: $(OBJ)
	$(LD) -o $@ $(OBJ) $(LDFLAGS)

$(TRG).trb: $(TRG).elf
	avr-objdump $(TURBO_TAG) --turbo $(TRG).elf

dis: $(TRG).elf
	avr-objdump -D --architecture=avr:5  $(TRG).elf >$(TRG).dis

links:
	ln -sf ../output/main/main.c muvuku.c

clean:
	$(RM) *.o *~
	$(RM) $(TRG).dis
	$(RM) $(TRG).elf
	$(RM) $(TRG).trb
	$(RM) *.stackdump
	$(RM) -r .cyg*

