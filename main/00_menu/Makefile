# Makefile for nextp8 Functional Validation Test Menu
# Builds menu.c and links with all test suites from other main/ directories
# Allows interactive selection and running of individual test suites

# Tools
CPP = m68k-elf-cpp
CC = m68k-elf-gcc
AS = m68k-elf-as
OBJCOPY = m68k-elf-objcopy
OBJDUMP = m68k-elf-objdump
PYTHON = python3

# Directories
COMMON_DIR = ../../common
TOOLS_DIR = ../../tools
NEXTP8_BSP = ../../../nextp8-bsp

# Files
TARGET = 00_menu
ELF = $(TARGET).elf
BIN = $(TARGET).bin
MEM = $(TARGET).mem
LST = $(TARGET).lst
MENU_OBJ = $(TARGET).o
CRT0_OBJ = crt0.o
CRT1_OBJ = crt1.o

# Find all test.o files from numbered test suite directories (01-14 only, exclude 00)
# Note: test object files are now named after their directory (e.g., 01_version_info.o)
# Explicitly list in numerical order: 01, 03, 04, 05a, 05b, 06, 07, 08, 09, 10, 12, 13, 14
# (02, 11 are intentionally omitted - they don't exist)
TEST_SUITE_DIRS = ../01_version_info ../03_joystick ../04_mouse ../05a_built_in_keyboard ../05b_external_keyboard ../06_sdspi ../07_i2c ../08_advanced_screen_output ../09_uart_esp ../10_digital_audio ../12_palette ../13_p8audio ../14_overlay
TEST_OBJS = $(foreach dir, $(TEST_SUITE_DIRS), $(dir)/$(notdir $(dir)).o)
# Add extra objects for sdspi (sdblockdevice and spi)
SDSPI_EXTRA_OBJS = ../06_sdspi/sdblockdevice.o ../06_sdspi/spi.o

# Flags
CPPFLAGS = -I$(COMMON_DIR)/include -I$(NEXTP8_BSP)/include
CFLAGS = -m68000 -O2 -Wall -Wextra -fomit-frame-pointer -nostartfiles
ASFLAGS = -m68000 --register-prefix-optional
LDFLAGS = -nostartfiles -T $(COMMON_DIR)/nextp8_funcval.ld
LIBS = -L$(COMMON_DIR) -lfuncval

# Default target
all: $(MEM) $(LST)
	@echo "Menu test binary built: $(BIN)"

# Compile crt0.S from common directory
$(CRT0_OBJ): $(COMMON_DIR)/crt0.S
	$(CPP) $(CPPFLAGS) $< | $(AS) $(ASFLAGS) -o $@

# Compile crt1.c from common directory
$(CRT1_OBJ): $(COMMON_DIR)/crt1.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compile 00_menu.c to 00_menu.o
$(MENU_OBJ): 00_menu.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# SD card block device objects (for 06_sdspi)
../06_sdspi/sdblockdevice.o: $(NEXTP8_BSP)/src/sdblockdevice.c
	$(CC) -m68000 -O2 -Wall -Wextra -fomit-frame-pointer -nostartfiles \
		-I$(COMMON_DIR)/include -I$(NEXTP8_BSP)/include \
		-c $< -o $@

../06_sdspi/spi.o: $(NEXTP8_BSP)/src/spi.c
	$(CC) -m68000 -O2 -Wall -Wextra -fomit-frame-pointer -nostartfiles \
		-I$(COMMON_DIR)/include -I$(NEXTP8_BSP)/include \
		-c $< -o $@

# Link menu with all test suite objects (no MAIN_OBJ - menu.c provides main())
# The linker will collect all test_suite structures from .test_suite_entry section
EXTRA_OBJS = $(SDSPI_EXTRA_OBJS)
$(ELF): $(CRT0_OBJ) $(CRT1_OBJ) $(MENU_OBJ) $(TEST_OBJS) $(EXTRA_OBJS) $(COMMON_DIR)/libfuncval.a
	$(CC) $(LDFLAGS) -o $@ $(CRT0_OBJ) $(CRT1_OBJ) $(MENU_OBJ) $(TEST_OBJS) $(EXTRA_OBJS) $(LIBS)

# Convert to binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Generate listing
$(LST): $(ELF)
	$(OBJDUMP) -d -S $< > $@

# Convert to .mem format
$(MEM): $(BIN)
	$(PYTHON) $(TOOLS_DIR)/bin2mem.py $< $@

# SD card image (needed for 06_sdspi)
SDCARD_IMG = ../06_sdspi/sdcard.img
SDCARD_SIZE_KB = 8192
$(SDCARD_IMG):
	dd if=/dev/zero of=$@ bs=1024 count=$(SDCARD_SIZE_KB)
	mkfs.vfat $@

sim: $(MEM) $(SDCARD_IMG)
	@$(TOOLS_DIR)/test_wrapper.sh sim

# Run with model
model: $(BIN) $(SDCARD_IMG)
	@$(TOOLS_DIR)/test_wrapper.sh model

# Alias
.PHONY: test
test: model

EXTRA_CLEAN = $(SDCARD_IMG)
clean:
	rm -f $(MENU_OBJ) $(CRT0_OBJ) $(CRT1_OBJ) $(ELF) $(BIN) $(MEM) $(LST) $(EXTRA_CLEAN)

.PHONY: all sim model test clean
