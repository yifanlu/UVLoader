CC=arm-none-eabi-gcc
CFLAGS=-fPIE -fno-zero-initialized-in-bss -std=c99 -mcpu=cortex-a9 -D DEBUG -D FW_169 -mthumb-interwork
CFLAGS_THUMB=-mthumb
LD=arm-none-eabi-gcc
LDFLAGS=-T linker.x -nodefaultlibs -nostdlib -pie
OBJCOPY=arm-none-eabi-objcopy
OBJCOPYFLAGS=

OBJ=uvloader.o cleanup.o load.o relocate.o resolve.o utils.o scefuncs.o

all: uvloader

scefuncs.o: scefuncs.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CFLAGS_THUMB)

uvloader: $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)
	$(OBJCOPY) -O binary $@ $@.bin

.PHONY: clean

clean:
	rm -rf *~ *.o *.elf *.bin *.s uvloader
