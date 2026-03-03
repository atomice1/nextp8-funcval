# Makefile for Test Suite: SD Card SPI Block Device
# Links with sdblockdevice.c and spi.c from nextp8-bsp

NEXTP8_BSP = ../../../nextp8-bsp

# SD card image for model testing
SDCARD_IMG = sdcard.img
SDCARD_SIZE_KB = 8192
EXTRA_OBJS = sdblockdevice.o spi.o
EXTRA_MODEL_DEPS = $(SDCARD_IMG)
EXTRA_SIM_DEPS = $(SDCARD_IMG)
EXTRA_CLEAN = $(SDCARD_IMG)
export SDCARD_IMAGE := $(abspath $(SDCARD_IMG))

TIMEOUT=3000
export TIMEOUT

include ../Makefile.common

# Append after Makefile.common sets base values
CPPFLAGS += -I$(NEXTP8_BSP)/include -I$(NEXTP8_BSP)/src
CFLAGS += -DROM
LIBS += -L$(NEXTP8_BSP)/lib -lnextp8-rom

sdblockdevice.o: $(NEXTP8_BSP)/src/sdblockdevice.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

spi.o: $(NEXTP8_BSP)/src/spi.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(SDCARD_IMG):
	dd if=/dev/zero of=$@ bs=1024 count=$(SDCARD_SIZE_KB)
	mkfs.vfat $@
