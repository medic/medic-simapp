# Makefile

ifndef TURBO_DIR
TURBO_DIR := ../../../turbo-devel-1.5.24
endif

TURBO_TAG = --turbo-manifest "Vendor: Medic Mobile"
INCDIR  = -I$(TURBO_DIR)/include -I../../src -I.
LIBDIR = $(TURBO_DIR)/lib

CC = avr-gcc
LD = avr-ld
RM = rm -f

TRG = charset
SRC = charset.c

LIB = 

DEFINES = -D_MUVUKU_TINY_STRINGS
CFLAGS = $(DEFINES) -Os -Wall -fno-strict-aliasing \
    -fomit-frame-pointer -std=gnu99 -mmcu=atmega128 -mno-tablejump \
    -Wimplicit-function-declaration -fno-builtin

LDFLAGS = -L$(LIBDIR) -T $(TURBO_DIR)/lib/turbo.lds -d -r \
    --emit-relocs -R $(TURBO_DIR)/lib/public_calls $(LIB) -lc \
    `avr-gcc -print-libgcc-file-name`

OBJ = $(SRC:.c=.o)
  
all: $(TRG).trb

%.o : %.c 
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@

$(TRG).elf: $(OBJ)
	$(LD) -o $@ $(OBJ) $(LDFLAGS)

$(TRG).trb: $(TRG).elf
	avr-objdump $(TURBO_TAG) --turbo $(TRG).elf

dis: $(TRG).elf
	avr-objdump -D --architecture=avr:5  $(TRG).elf >$(TRG).dis

clean:
	$(RM) *.o
	$(RM) *~
	$(RM) $(TRG).dis
	$(RM) $(TRG).elf
	$(RM) $(TRG).trb

