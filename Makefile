PREFIX=arm-vita-eabi
CC=$(PREFIX)-gcc
CFLAGS=-DUVLOADER -fPIE -fno-zero-initialized-in-bss -std=gnu99 -mcpu=cortex-a9 -D DEBUG -D FW_3XX -mthumb-interwork
CFLAGS_THUMB=-mthumb
LD=$(PREFIX)-gcc
LDFLAGS=-T linker.x -nodefaultlibs -nostdlib -pie
OBJCOPY=$(PREFIX)-objcopy
OBJCOPYFLAGS=
TARGET_LIB=libUVLoader_stub.a

OBJ=uvloader.o cleanup.o load.o relocate.o resolve.o utils.o scefuncs.o debugnet.o

all: uvloader

scefuncs.o: scefuncs.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CFLAGS_THUMB)

uvloader: $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)
	$(OBJCOPY) -O binary $@ $@.bin

$(TARGET_LIB): uvloader.json
	mkdir build_lib
	vita-libs-gen uvloader.json build_lib
	cd build_lib && make
	mv build_lib/$(TARGET_LIB) $(TARGET_LIB)
	rm -rf build_lib

check-env:
ifndef VITASDK
	$(error VITASDK is not set)
endif

install_lib: check-env $(TARGET_LIB) uvloader.json uvloader.h
	cp $(TARGET_LIB) $(VITASDK)/arm-vita-eabi/lib
	cp uvloader.h $(VITASDK)/arm-vita-eabi/include/psp2/uvl.h
	cp uvloader.json $(VITASDK)/share/uvloader.json
	@echo "Installed!"

.PHONY: clean

clean:
	rm -rf *~ *.o *.elf *.bin *.s build_lib $(TARGET_LIB) uvloader
