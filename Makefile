CC=arm-none-eabi-gcc
CFLAGS=-fPIE -std=c99 -mcpu=cortex-a9 -mthumb -D DEBUG
LD=arm-none-eabi-gcc
LDFLAGS=-T linker.x -nodefaultlibs -nostdlib -pie
OBJCOPY=arm-none-eabi-objcopy
OBJCOPYFLAGS=

ODIR=obj

OBJ=uvloader.o cleanup.o load.o resolve.o utils.o scefuncs.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

uvloader: $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)
	$(OBJCOPY) -O binary $@ uvloader.bin

.PHONY: clean

clean:
	rm -rf *~ *.o *.elf *.bin *.s uvloader
