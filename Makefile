CC=arm-none-eabi-gcc
CFLAGS=-fPIE -std=c99 -mcpu=cortex-a9
LD=arm-none-eabi-gcc
LDFLAGS=-nodefaultlibs -nostdlib -pie

ODIR=obj

OBJ=uvloader.o cleanup.o load.o resolve.o utils.o scefuncs.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

uvloader: $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *~ *.o *.elf *.bin *.s
